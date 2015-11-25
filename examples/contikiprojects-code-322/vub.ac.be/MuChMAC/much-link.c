/*
 * Copyright (c) 2009, Vrije Universiteit Brussel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
 /*
 * \author
 * 					Ward Van Heddegem
 *					Joris Borms <joris.borms@vub.ac.be>
 *
 */



#include "net/rime/much-link.h"
#include "net/mac/muchmac.h"

#include "net/rime.h"
#include "net/rime/packetbuf.h"
#include "net/rime/unicast.h"
#include "net/rime/broadcast.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "sys/clock.h"

#define UCLOCK_SECOND ((clock_time_t) CLOCK_SECOND)

#define MUCH_LINK_PACKET_ID_BITS 4

static const struct packetbuf_attrlist attributes[] =
{
		MUCH_LINK_ATTRIBUTES
		PACKETBUF_ATTR_LAST
};

#define ADDR(a) (a)->u8[0],(a)->u8[1]

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf("DEBUG:  " __VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* WARNING: There's a bug somewhere which cause nodes to crash under
 * elevated traffic loads (several messages per second). It's not entirely certain
 * that bug resides here, though. Proceed with caution...
 * I will probably refactor this module in the future to make its structure a
 * bit simpler, hopefully this will make bugs will be easier to track...
 */

struct much_link_queue_item {
	struct much_link_queue_item* next;
	struct queuebuf* q;
	rimeaddr_t recv;
	struct much_link_conn* conn;
};


MEMB(much_link_queue_mem, struct much_link_queue_item, MUCH_LINK_QUEUE_SIZE);
LIST(much_link_queue);

/* memory for duplicate packet checks */
static struct packet_recv {
	rimeaddr_t  sender;
	uint8_t     seqno;
} packet_recv_cache[MUCH_LINK_DUPLICATE_CHECK];

/* seq counter for our packets */
static uint8_t packet_sndnxt, packet_sndlast;
static volatile uint8_t much_link_is_tx = 255;

/* keep currently transmitting packet copy */
static volatile struct much_link_queue_item* send_buffer;
/* global retransmit timer */
static struct ctimer ct;
/*---------------------------------------------------------------------------*/
/* a few forward pointers for convenience */
static void send_next_from_buffer(const rimeaddr_t* last_recv);
static void evaluate(void *ptr);
/*---------------------------------------------------------------------------*/
static volatile uint8_t packet_train = 0;
static void
set_packet_train(void)
{
	packetbuf_set_attr(PACKETBUF_ATTR_PACKET_TRAIN, 0);
	packet_train = 0;

	if (packetbuf_attr(PACKETBUF_ATTR_PACKET_TYPE) != PACKETBUF_ATTR_PACKET_TYPE_ACK){
		struct much_link_queue_item* item = list_head(much_link_queue);
		while (item != NULL){
			if (item != send_buffer && rimeaddr_cmp(&item->recv, &send_buffer->recv)){
				packetbuf_set_attr(PACKETBUF_ATTR_PACKET_TRAIN, 1);
				packet_train = 1;
				//printf("packet train!\n");
				return;
			}
			item = item->next;
		}
		if (list_length(much_link_queue) == 1 && packet_train){
			printf("impossible!!!! \n");
		}
	}
}
/*---------------------------------------------------------------------------*/
/* Increment the send next counter */
inline static void
inc_sndnxt(struct much_link_conn *c)
{
	packet_sndnxt = (packet_sndnxt + 1) % (1 << MUCH_LINK_PACKET_ID_BITS);
}
/*---------------------------------------------------------------------------*/
static uint8_t packet_recv_index = 0;
static void
save_packet_params(const rimeaddr_t* from, uint16_t seqno)
{
	packet_recv_cache[packet_recv_index].seqno = seqno;
	rimeaddr_copy(&(packet_recv_cache[packet_recv_index].sender), from);
	packet_recv_index = (packet_recv_index + 1) % MUCH_LINK_DUPLICATE_CHECK;
}
/*---------------------------------------------------------------------------*/
static int
duplicate_packet_check(const rimeaddr_t* from, uint16_t seqno)
{
	uint8_t i = 0;
	for (; i < MUCH_LINK_DUPLICATE_CHECK; i++){
		if (packet_recv_cache[i].seqno == seqno
				&& rimeaddr_cmp(from, &(packet_recv_cache[i].sender))){
			return 0;
		}
	}
	return 1;
}
/*---------------------------------------------------------------------------*/
static void
clear_send_buffer(void)
{
	//printf("queue size: %u ", list_length(much_link_queue));
	if (send_buffer != NULL){
		queuebuf_free(send_buffer->q);
		list_remove(much_link_queue, send_buffer);
		memb_free(&much_link_queue_mem, send_buffer);
		send_buffer = NULL;
	}
	//printf("queue size: %u \n", list_length(much_link_queue));
}
/*---------------------------------------------------------------------------*/
/* Called by unicast each time when a packet is received.
 * If it is an ACK packet (and correct seq no)
 * 		--> cancel sending and notify higher up primitive
 * If it is an ACK packet (and wrong seq no)
 * 		--> do nothing
 * If it is  a data packet
 * 		--> extract seq no, send (once!) an ACK packet back, and notify higher up
 */
static void
recv_from_broadcast(struct broadcast_conn *uc, const rimeaddr_t *from)
{
	struct much_link_conn *c = (struct much_link_conn *) uc;

	PRINTF("%d.%d: much-link: recv_from_broadcast from %d.%d type %d seqno %.2d\n",
			rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],
			from->u8[0], from->u8[1],
			packetbuf_attr(PACKETBUF_ATTR_PACKET_TYPE),
			packetbuf_attr(PACKETBUF_ATTR_PACKET_ID));

	/* Check packet type */
	if (packetbuf_attr(PACKETBUF_ATTR_PACKET_TYPE) == PACKETBUF_ATTR_PACKET_TYPE_ACK) {
		/* (OPTION 1) ACK packet */

		if (packetbuf_attr(PACKETBUF_ATTR_PACKET_ID) == packet_sndlast) {
			/* correct seq no -> stop sending, and notify higher up */
			RIMESTATS_ADD(ackrx);
			PRINTF("%d.%d: much-link: ACKed %.2d\n",
					rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
					packetbuf_attr(PACKETBUF_ATTR_PACKET_ID));
			//inc_sndnxt(c);
			ctimer_stop(&ct); /* stop resending */
			clear_send_buffer();
			/* send next before notifying upper layer of a receive, since
			 * the upper layer may schedule a new packet in respose */
			send_next_from_buffer(from);
			if (c->u->sent != NULL) {
				c->u->sent(c, from, c->rxmit);
			}
		} else {
			/* wrong seq no -> ignore packet */
			PRINTF("%d.%d: much-link: received bad ACK %.2d for expected %.2d\n",
					rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],
					packetbuf_attr(PACKETBUF_ATTR_PACKET_ID),
					packet_sndnxt);
			RIMESTATS_ADD(badackrx);
		}

	} else if (packetbuf_attr(PACKETBUF_ATTR_PACKET_TYPE)
			== PACKETBUF_ATTR_PACKET_TYPE_DATA) {
		/* (OPTION 2) DATA packet
		 -> get seq no, and send once an ACK packet with same seq no */

		uint16_t packet_seqno;
		struct queuebuf *temp_q;

		RIMESTATS_ADD(reliablerx);

		PRINTF("%d.%d: much-link: got packet %.2d\n",
				rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],
				packetbuf_attr(PACKETBUF_ATTR_PACKET_ID));

		packet_seqno = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);

		if (!rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),&rimeaddr_null)){
			/* Send ACK back */
			/* First, put packetbuffer data temporarily in a queue
			 * buffer, so we can user packetbuffer for sending ACK */
			temp_q = queuebuf_new_from_packetbuf();
			if (temp_q != NULL) {
				PRINTF("%d.%d: much-link: Sending ACK to %d.%d for %.2d\n",
						rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],
						from->u8[0], from->u8[1],
						packet_seqno);
				packetbuf_clear();
				packetbuf_set_attr(PACKETBUF_ATTR_PACKET_TYPE, PACKETBUF_ATTR_PACKET_TYPE_ACK);
				packetbuf_set_attr(PACKETBUF_ATTR_PACKET_ID, packet_seqno);
				packetbuf_set_attr(PACKETBUF_ATTR_PACKET_TRAIN, 0);
				packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, from);
				broadcast_send(&c->c);
				RIMESTATS_ADD(acktx);

				/* Restore packetbuffer */
				queuebuf_to_packetbuf(temp_q);
				queuebuf_free(temp_q);
			}
		}

		/* notify higher up about data packet, if packet was not a duplicate
		 * (we check for duplication here since an ACK may have been lost,
		 *  so we need to send an ACK even if packet was a duplicate) */
		if (c->u->recv != NULL) {
			if (duplicate_packet_check(from, packet_seqno)) {
				save_packet_params(from, packet_seqno);
				c->u->recv(c, from, packet_seqno);
			} else {
				PRINTF("%d.%d: much-link: Supressing duplicate receive "
						"callback from %d.%d for %.2d\n",
						rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],
						from->u8[0], from->u8[1],
						packet_seqno);
			}

		}

	}
}
/*---------------------------------------------------------------------------*/
/* Send the data in the queuebuffer using unicast */
static int
send_from_buffer(void) {
	if (send_buffer != NULL){
		much_link_is_tx = 1;
		queuebuf_to_packetbuf(send_buffer->q); /* get data from queuebuffer */
		set_packet_train();
		packet_sndlast = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
		PRINTF("%d.%d: much-link send a packet to %d.%d using unicast\n",
				rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],
				c->receiver.u8[0],c->receiver.u8[1]);
		packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &(send_buffer->recv));
		return broadcast_send(&(send_buffer->conn->c));
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
/* Select next message to send and start sending */
static void
send_next_from_buffer(const rimeaddr_t* last_recv) {
	struct much_link_queue_item* item = NULL;
	//printf("send next \n");
	if (last_recv != NULL){
		//printf("x queue size: %u \n", list_length(much_link_queue));
		/* prefer packet to same receiver to accomodate packet trains */
		item = list_head(much_link_queue);
		while (item != NULL){
			//printf("item->recv %u.%u, last_recv %u.%u \n",ADDR(&item->recv),ADDR(last_recv));
			if (rimeaddr_cmp(last_recv, &item->recv)){
				break;
			}
			item = item->next;
		}
	}
	if (item == NULL){
		/* if no item found with same receiver, just take oldest item in queue */
		item = list_tail(much_link_queue);
	}
	if (item != NULL){
		send_buffer = item;
		send_from_buffer();
		ctimer_set(&ct, 1 + ((unsigned int) rand()) %
				(3 * (UCLOCK_SECOND) / level_lookup(&(send_buffer->recv))),
				evaluate, send_buffer->conn);
	}	else {
		/* empty queue, pause sending */
		much_link_is_tx = 0;
	}
}
/*---------------------------------------------------------------------------*/
/* Called each time the resend interval has elapsed.
 * If this gets called, it means we didn't receive an ACK yet
 * (because an ACK cancels the timer that calls this function).
 * So, either retransmit the packet or time-out:
 * - If the maximum number of resends has been reached, time out and notify
 *    higher up.
 * -  Otherwise, resend the packet and double evaluation timer (with a maximum
 *    of 16x the original interval). */
static void
evaluate(void *ptr) {
	struct much_link_conn *c = ptr;

	PRINTF("%d.%d: much-link: evaluating \n",
			rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);

	if(!much_link_is_tx){
		printf("we shouldn't be sending!!\n");
		return;
	}

	/* Check if timed out */
	if (c->rxmit >= c->max_rxmit) {
		/* timed out */
		RIMESTATS_ADD(timedout);
		PRINTF("%d.%d: much-link: packet %.2d timed out\n",
				rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],
				packet_sndnxt);

		if (c->u->lost) {
			c->u->lost(c, &(send_buffer->recv), MUCH_LINK_TIMEOUT);
		}
		clear_send_buffer();
		//printf("lost... check next \n");
		send_next_from_buffer(NULL);
	} else {
		/* rexmit time has elapsed (and not reach maximum resends
		 * -> resend (with increasing interval)*/
		c->rxmit++;
		RIMESTATS_ADD(rexmit);
		PRINTF("%d.%d: much-link: packet %.2d resent %u\n",
				rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
				packet_sndnxt, c->rxmit);
		send_from_buffer();
		//int shift;
		//shift = c->rxmit > 4 ? 4 : c->rxmit; //2 x interval (and maximum 16x)
		ctimer_set(&ct,
				1 + ((unsigned int) rand()) %
				(4 * (UCLOCK_SECOND) / level_lookup(&(send_buffer->recv))),
				evaluate, c);
		PRINTF("%d.%d: much-link: next send timer set to %d seconds.\n",
				rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
				((REXMIT_TIME)<<shift)/CLOCK_SECOND);
	}

}
/*---------------------------------------------------------------------------*/
static const struct broadcast_callbacks bc_cb = {recv_from_broadcast};
/*---------------------------------------------------------------------------*/
void
much_link_open(struct much_link_conn *c, uint16_t channel,
		const struct much_link_callbacks *u)
{
	if (much_link_is_tx == 255){
		/* much-link module not initialised yet */
		much_link_is_tx = 0;
		packet_sndnxt = 0;
		send_buffer = NULL;
		memb_init(&much_link_queue_mem);
		list_init(much_link_queue);
		int i = 0;
		for ( ; i < MUCH_LINK_DUPLICATE_CHECK; i++){
			rimeaddr_copy(&packet_recv_cache[i].sender, &rimeaddr_null);
			packet_recv_cache[i].seqno = 255;
		}
	}

	broadcast_open(&c->c, channel, &bc_cb);
	channel_set_attributes(channel, attributes);
	c->u = u;

}
/*---------------------------------------------------------------------------*/
void
much_link_close(struct much_link_conn *c)
{
	broadcast_close(&c->c);
}
/*---------------------------------------------------------------------------*/
int
much_link_send(struct much_link_conn *c, const rimeaddr_t *receiver,
		uint8_t max_retransmissions)
{

	inc_sndnxt(c);

	c->max_rxmit = max_retransmissions;
	c->rxmit = 0;

	struct much_link_queue_item* queue_item;

	queue_item = memb_alloc(&much_link_queue_mem);
	if (queue_item == NULL){
		/* queue full -> packet drop */
		if (c->u->lost != NULL){
			c->u->lost(c, receiver, MUCH_LINK_QUEUE_FULL);
		}
		return 0;
	}

	/* Configure packet attributes and connection parameters */
	packetbuf_set_attr(PACKETBUF_ATTR_RELIABLE, 1);
	packetbuf_set_attr(PACKETBUF_ATTR_PACKET_TYPE, PACKETBUF_ATTR_PACKET_TYPE_DATA);
	packetbuf_set_attr(PACKETBUF_ATTR_PACKET_ID, packet_sndnxt);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, receiver);

	queue_item->q = queuebuf_new_from_packetbuf();
	if (queue_item->q == NULL){
		/* all queuebufs used -> packet drop */
		if (c->u->lost != NULL){
			c->u->lost(c, receiver, MUCH_LINK_QUEUEBUF_FULL);
		}
		memb_free(&much_link_queue_mem, queue_item);
		return 0;
	}

	list_push(much_link_queue, queue_item);

	rimeaddr_copy(&queue_item->recv, receiver);
	queue_item->conn = c;

	if (much_link_is_tx == 0){
		/* point "current" buffer to newly created buffer and send */
		if (send_buffer != NULL){
			printf("severe error 2\n");
			clear_send_buffer();
		}
		send_buffer = queue_item;

		RIMESTATS_ADD(reliabletx);
		PRINTF("%d.%d: much-link: sending packet %.2d\n",
				rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],
				packet_sndnxt);

		send_from_buffer();
		ctimer_set(&ct, 1 + ((unsigned int) rand()) %
				(3 * (UCLOCK_SECOND) / level_lookup(receiver)),
				evaluate, c);
		return 1;
	} else {
		/* item is added to queue and will be sent later */
		return 1;
	}

}
/*---------------------------------------------------------------------------*/
int
much_link_broadcast(struct much_link_conn *c)
{
	return much_link_send(c, &rimeaddr_null, 1);
}
/*---------------------------------------------------------------------------*/
uint8_t
much_link_is_transmitting(struct much_link_conn *c)
{
	return much_link_is_tx;
}
/*---------------------------------------------------------------------------*/
