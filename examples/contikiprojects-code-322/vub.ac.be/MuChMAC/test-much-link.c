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
 * \author Joris Borms <joris.borms@vub.ac.be>
 *
 */
#include "contiki.h"

#include "net/mac/muchmac.h"
#include "net/rime/much-link.h"
#include "net/rime/neighbor-attr.h"
#include "net/rime/packetbuf.h"
#include "sys/clock.h"
#include "sys/energest.h"

#define mtypeof(struct, member) typeof(((struct*)0)->member)
#define UCLOCK_SECOND ((clock_time_t) CLOCK_SECOND)
#define ACTIVITY_SCALE 1000 /* warning: high scale will cause overflows */

#include <stdio.h>

#define PAYLOAD_SIZE 60

/* total time between generating messages
 * = timeout[0] + random[0 -> timeout[1]]  */
static const clock_time_t timeout[2] = {2 , 4};
#define TIMEOUT_SCALE (UCLOCK_SECOND)

PROCESS(test_much_link_process, "much link test process");
AUTOSTART_PROCESSES(&test_much_link_process);

/* for shorter printf */
#define U8(addr) (addr)->u8[0], (addr)->u8[1]
#define PRINTF(...)
#define VAR_POWERLEVEL 1 /* test powerlevel variation */
/* --------------------------------------------------------------------- */
/* static routing setup */
static rimeaddr_t* parent;
static const uint8_t network[12][2] =
{{127,40}, {71,219}, {139,29}, {202,31}, {184,35}, {30,41}, {155,44},
		{235,29}, {186,64}, {235,53}, {66,33}, {31,109}};
static uint8_t node_index;
static const uint8_t parent_index[12] = {0,0,0,1,2,1,2,3,4,5,6};
/* --------------------------------------------------------------------- */
struct much_link_conn conn;
static uint16_t sent, received, lost, damaged, dropped;

struct info {
	uint16_t   counter;
	uint16_t   dropped;
	uint16_t   powerlevel;
	uint16_t   activity;
};

struct message {
	rimeaddr_t   origin;
	struct info  info;
	/* payload to give message a more realistic size */
	char         padding[PAYLOAD_SIZE];
	 /* additional xor check just to be sure -> one corrupted message could
	  * seriously perturb experimental results */
	char         xor;
};
/* --------------------------------------------------------------------- */
/* let sink keep the counter of nodes in the network, so we can keep
 * track of the number of messages generated */
NEIGHBOR_ATTRIBUTE(struct info, node_info, NULL);
NEIGHBOR_ATTRIBUTE(uint16_t, msg_recv, NULL);
/* --------------------------------------------------------------------- */
static char
get_xor(struct message *msg)
{
	char* p;
	char xor = 0;
	for (p = (char*) msg; p < &(msg->xor); p++){
		xor ^= (*p);
	}
	return xor;
}
/* --------------------------------------------------------------------- */
#if VAR_POWERLEVEL
/* estimate message inter-arrival time */
static clock_time_t last_recv = 0, delta = 0;
#define CLOCK_TIME_T_RANGE ((uint16_t) 0xFFFF / UCLOCK_SECOND)
static unsigned long last_recv_long = 0; // correctly handle clock_time_t wraparound
#endif
extern uint8_t node_powerlevel;
/* --------------------------------------------------------------------- */
static uint16_t
set_powerlevel(clock_time_t delta)
{
#if VAR_POWERLEVEL
	uint16_t i = 1;
	while(i < 31 && ((UCLOCK_SECOND << 2) / i > delta)){
		i++;
	}
	return i;
#else
	return 1;
#endif
}
/* --------------------------------------------------------------------- */
static void
much_receive(struct much_link_conn* c, rimeaddr_t* from, uint8_t seqno)
{
	received++;

	if (rimeaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),&rimeaddr_null)){
		/* manually add a string terminator so we can show the message content
		 * with %s in printf -> this way, we don't have to copy the data first */
		((char*)packetbuf_dataptr())[packetbuf_datalen()] = 0;
		printf("got message \"%s\" from %u.%u with seqno %u\n",
				packetbuf_dataptr(), U8(from), seqno);
		return;
	}

	if (!rimeaddr_cmp(parent, &rimeaddr_node_addr)){
		much_link_send(c, parent, 4);
	}	else {
		struct message *msg = packetbuf_dataptr();
		if (msg->xor == get_xor(msg) && (msg->info.powerlevel > 0)){
			neighbor_attr_set_data(&node_info, &(msg->origin), &(msg->info));
			uint16_t* recv = (uint16_t*) neighbor_attr_get_data(&msg_recv, &(msg->origin));
			(*recv)++ ;
		} else {
			damaged++;
		}
	}
#if VAR_POWERLEVEL
	/* powerlevel control */
	if (!packetbuf_attr(PACKETBUF_ATTR_PACKET_TRAIN)){
		// only count last packet of a packet train
		unsigned long seconds = clock_seconds();
		if (last_recv == 0 || seconds - last_recv_long > CLOCK_TIME_T_RANGE){
			delta = 0;
			node_powerlevel = 1;
		} else {
			if (delta == 0){
				delta = clock_time() - last_recv;
			} else {
				// EWMA: 8/9 old value + 1/9 new value
				delta = ((delta << 3) + (clock_time() - last_recv)) / 9;
				node_powerlevel = set_powerlevel(delta);
			}
		}
		last_recv = clock_time();
		last_recv_long = seconds;
	}
#endif
}

static void
much_sent(struct much_link_conn* c, rimeaddr_t* from, uint8_t rexmits)
{
	sent++;
}

static void
much_lost(struct much_link_conn* c, rimeaddr_t* from, uint8_t err)
{
	lost++;
	if (err & (MUCH_LINK_QUEUE_FULL | MUCH_LINK_QUEUEBUF_FULL)){
		dropped++;
		PRINTF("packet droppped, queue full \n");
		if (err & MUCH_LINK_QUEUEBUF_FULL){
			printf("system error: all queuebufs used! \n");
		}
	} else { /* MUCH_LINK_TIMEOUT */
		// reduce powerlevel
		PRINTF("packet dropped after rexmits \n");
#if VAR_POWERLEVEL
		uint8_t* p = neighbor_attr_get_data(&powerlevel, from);
		if (p != NULL && (*p) > 1){
			(*p)--;
		}
#endif
	}
}
/* --------------------------------------------------------------------- */
static uint16_t activity(void) // truncate to 16-bit for simplicity
{
#if ENERGEST_CONF_ON
	mtypeof(energest_t,current) activity =
			(energest_total_time[ENERGEST_TYPE_TRANSMIT].current +
						energest_total_time[ENERGEST_TYPE_LISTEN].current) * ACTIVITY_SCALE /
						(energest_total_time[ENERGEST_TYPE_CPU].current +
								energest_total_time[ENERGEST_TYPE_LPM].current);
	return activity & 0xFFFF;
#else
	return 0;
#endif
}

static void
print_energest(void)
{
#if ENERGEST_CONF_ON
	printf("cpu %lu lpm %lu irq %lu tx %lu listen %lu // activity = %u:%u \n",
			//rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
			//clock_time() / CLOCK_SECOND,
			energest_total_time[ENERGEST_TYPE_CPU].current,
			energest_total_time[ENERGEST_TYPE_LPM].current,
			energest_total_time[ENERGEST_TYPE_IRQ].current,
			energest_total_time[ENERGEST_TYPE_TRANSMIT].current,
			energest_total_time[ENERGEST_TYPE_LISTEN].current,
			activity(),
			ACTIVITY_SCALE
	);
#endif
}
/* --------------------------------------------------------------------- */
struct much_link_callbacks cb = {much_receive, much_sent, much_lost};
/* --------------------------------------------------------------------- */
PROCESS_THREAD(test_much_link_process, ev, data)
{
	static struct etimer et;
	static uint16_t counter;

	PROCESS_BEGIN();

	node_powerlevel = 1;

	/* set up routing */
#if 1
	/* test with cooja, rime addrs are (1.0),(2.0),(3.0),... */
	if (rimeaddr_node_addr.u8[0] > 1){
		static rimeaddr_t parent_addr;
		parent_addr.u8[0] = parent_index[rimeaddr_node_addr.u8[0]] + 1;
		parent_addr.u8[1] = rimeaddr_node_addr.u8[1];
		parent = &parent_addr;
	} else {
		parent = &rimeaddr_node_addr;
		timesynch_set_authority_level(0);
	}
#else
	/* test with real nodes */
	node_index = 0;
	while (!rimeaddr_cmp(&rimeaddr_node_addr, (rimeaddr_t*) network[node_index])){
		node_index++;
	}
	parent = (rimeaddr_t*) network[parent_index[node_index]];
	if (rimeaddr_cmp(parent, &rimeaddr_node_addr)){
		// this is a gateway
		timesynch_set_authority_level(0);
	}
#endif

	/* set up params and connection  */
	neighbor_attr_register(&node_info);
	neighbor_attr_register(&msg_recv);
	sent = received = lost = damaged = dropped = 0;
	counter = 0;

	much_link_open(&conn, 0x54c, &cb);

	/* and go! */
	etimer_set(&et, 30 * CLOCK_SECOND);

	energest_init();
	PROCESS_WAIT_UNTIL(etimer_expired(&et));
	while (1) {
		etimer_set(&et, timeout[0] * TIMEOUT_SCALE
				+ (((unsigned int) rand()) % (timeout[1] * TIMEOUT_SCALE)));
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		counter++;
		if (counter % 15 == 0){
			if (!rimeaddr_cmp(parent, &rimeaddr_node_addr)){
				printf("%u.%u->%u.%u: loop %u, recv %u, sent %u, lost %u, drop %u\n",
						U8(&rimeaddr_node_addr), U8(parent),
						counter, received, sent, lost, dropped);
			} else {
				uint16_t total = 0;
				struct neighbor_addr *n = neighbor_attr_list_neighbors();
				while (n != NULL){
					struct info* info = neighbor_attr_get_data(&node_info, &(n->addr));
					uint16_t* r = neighbor_attr_get_data(&msg_recv, &(n->addr));
					if (info != NULL && r != NULL && info->counter > 0){
						total += info->counter;
						printf("%u.%u;%u;%u;%u;%u\n", U8(&(n->addr)),
								info->activity, info->counter, info->dropped, *r);
					}
					n = n->next;
				}
				printf("%u.%u;%u;%u;%u;%u\n", U8(&rimeaddr_node_addr),
												activity(), 0, 0, received);
				printf("\n");
				//printf("%u.%u: loop %u, total sent %u, recv %u, dmg %u, power %u\n",
				//		U8(&rimeaddr_node_addr), counter, total, received, damaged, node_powerlevel);
				//print_energest();
			}
		}
		if (!rimeaddr_cmp(parent, &rimeaddr_node_addr)){
			packetbuf_clear();
			//			uint16_t len = sprintf(packetbuf_dataptr(),
			//					"hello-{%u.%u}-[%02u]", U8(&rimeaddr_node_addr), (counter % 100));
			//			packetbuf_set_datalen(len);

			struct message *msg = packetbuf_dataptr();
			rimeaddr_copy(&(msg->origin), &rimeaddr_node_addr);
			msg->info.counter = counter;
			msg->info.powerlevel = node_powerlevel;
			msg->info.dropped = dropped;
			msg->info.activity = activity();
			msg->xor = get_xor(msg);
			packetbuf_set_datalen(sizeof(struct message));

			much_link_send(&conn, parent, 4);
		}
	}




	PROCESS_END();
}
