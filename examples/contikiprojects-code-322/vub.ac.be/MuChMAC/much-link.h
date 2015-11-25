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

#ifndef __MUCH_LINK_H__
#define __MUCH_LINK_H__

#include "contiki.h"

#include "net/rime/unicast.h"
#include "net/rime/ctimer.h"
#include "net/rime/queuebuf.h"

/**
 * the maximum size of the much-link queue
 * if the queue is full, additional packets will be dropped (lost)
 */
#define MUCH_LINK_QUEUE_SIZE       8
/**
 * define how many packets the much-link layer will remember when checking for
 * duplicates
 */
#define MUCH_LINK_DUPLICATE_CHECK  4


struct much_link_conn;

/* TODO refactor to use STREAM packet type instead of custom PACKETBUF_ATTR_PACKET_TRAIN
 * attribute */
#define MUCH_LINK_ATTRIBUTES  { PACKETBUF_ATTR_PACKET_TYPE,   PACKETBUF_ATTR_BIT }, \
                              { PACKETBUF_ATTR_PACKET_ID,     PACKETBUF_ATTR_BIT * 4 }, \
                        			{ PACKETBUF_ATTR_PACKET_TRAIN,  PACKETBUF_ATTR_BIT } , \
                              UNICAST_ATTRIBUTES




struct much_link_callbacks {
	/* Called when we have received a data packet */
	void (* recv)(struct much_link_conn* ,
			rimeaddr_t *from, uint8_t seqno);
	/* Called when packet we tried to send has been ACK'd */
	void (* sent)(struct much_link_conn* ,
			rimeaddr_t *to,	uint8_t retransmissions);
	/* Called when packet was dropped */
	void (* lost)(struct much_link_conn* ,
			rimeaddr_t *to,	uint8_t errcondition);
};

/* possible error conditions that call "lost" callback function */
#define MUCH_LINK_QUEUE_FULL     1
#define MUCH_LINK_QUEUEBUF_FULL  2
#define MUCH_LINK_TIMEOUT        4 /* max rexmit counter reached */

struct much_link_conn {
	struct broadcast_conn c;
	const struct much_link_callbacks *u;
	/* Number of re-transmission so far (= transmission - 1) */
	uint8_t rxmit;
	/* Maximum number of times a packet can be re-transmitted
	 (transmissions = max_rxmit +1) */
	uint8_t max_rxmit;
};


/**
 * \brief      Set up a reliable single hop unicast connection
 * \param c    A pointer to a struct brunnicast_conn
 * \param channel The channel on which the connection will operate
 * \param u    A struct brunicast_callbacks with function pointers to functions
 * 						 that will be called when a packet has been received, ACKed or
 * 						 timed-out
 *
 *             This function sets up a reliable single hop unicast connection on
 *             the specified channel. The caller must have allocated the
 *             memory for the struct brunicast, usually by declaring it
 *             as a static variable.
 *
 *             The struct brunicast_callbacks pointer must point to a structure
 *             containing pointers to a function that will be called
 *             when a packet arrives on the channel, when a packet has been
 *             acknowledged, and when a packet has timed-out.
 *
 */
void much_link_open(struct much_link_conn *c, uint16_t channel,
		const struct much_link_callbacks *u);

/**
 * \brief      Close a brunicast connection
 * \param c    A pointer to a struct brunicast_conn
 *
 *             This function closes a brunicast connection that has
 *             previously been opened with brunicast_open().
 *
 *             This function typically is called as an exit handler.
 *
 */
void much_link_close(struct much_link_conn *c);

/**
 * \brief      Reliable send a packet to a single hop neighbor
 * \param c    The brunicast connection on which the packet should be sent
 * \param receiver	The address of intended receiver
 * \param max_retransmissions	The number of REtransmission allowed to deliver
 * 														the packet. If zero, the packet will only be
 * 														sent once.
 * \retval     Non-zero if the packet could be sent, zero otherwise
 *
 *             This function reliably sends an packet to a single-hop neighbor.
 *             The packet must be present in the rimebuf before this function is
 *             called. If no ack is received upon the first send, a number of
 *             retransmissions are performed (specified by the parameter
 *             max_retransmission).The sending application is notified of
 *             successful delivery or time-out via the callback functions set
 *             up in parameter c.
 *
 *             The parameter c must point to a brunicast connection that
 *             must have previously been set up with brunicast_open().
 *
 */
int much_link_send(struct much_link_conn *c, const rimeaddr_t *receiver,
		uint8_t max_retransmissions);

int much_link_broadcast(struct much_link_conn *c);

/**
 * \brief       Indicates if the brunicast layer is still transmitting
 * \param c     The brunicast connection which is queried
 * \retval			Non-zero if still busy sending, zero otherwise
 *
 * 				      Indicates if the brunicast layer is still busy (re)sending a
 * 							previous packet (i.e. waiting for an ACK or time-out).
 *
 */
uint8_t much_link_is_transmitting(struct much_link_conn *c);


#endif /* __BRUNICAST_H__ */

