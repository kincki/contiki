/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Testing the broadcast layer in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "lib/random.h"
#include "net/rime/rimestats.h"
#include "dev/button-sensor.h"
#include "sys/node-id.h"
#include "dev/leds.h"

#include <stdio.h>
#include <stdbool.h>

/*---------------------------------------------------------------------------*/
PROCESS(leader_mote_1, "Leader Selection-1");
AUTOSTART_PROCESSES(&leader_mote_1);
/*---------------------------------------------------------------------------*/

// keep track of current leader status
static bool isLeaderAssigned = false; // initially no leader is assigned
static bool amILeader = false; // initially no body is leader
static uint16_t leader = 0; // initially nobody is leader

	
uint8_t nodeID = 1;


static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	uint16_t *inputNodeID;

	inputNodeID = (uint16_t *) packetbuf_dataptr();


	printf("Received Candidate Leader NodeID is %d from %d.%d\n", *inputNodeID, from->u8[0], from->u8[1]);

	if (false == isLeaderAssigned)
	{ // if leader is not assigned, we should acknowledge this sender of this message as leader
		if (nodeID == *inputNodeID)
		{ // This means that I'm the leader, and this is a relay of my leadership message; ignore it.
			isLeaderAssigned = true;
			amILeader = true;
			leader = nodeID;
		}
		else if ((uint16_t) from->u8[0] != *inputNodeID)
		{ // this is a relay leadership message;
			isLeaderAssigned = true;
			amILeader = false;
			leader = *inputNodeID;

			printf("I'm Mote-%d. Leader selected as %d\n.", nodeID, leader);
		}
		else
		{
			isLeaderAssigned = true;
			amILeader = false;
			leader = (uint16_t) from->u8[0]; // leader Mote-ID is assigned for local reference

			printf("I'm Mote-%d. Leader selected as %d.%d\n.", nodeID, from->u8[0], from->u8[1]);
		}
	}
	else if (*inputNodeID != leader)
	{
		printf("Duplicate Leadership broadcast message received from %d.%d. Leader is %d. \n",
					from->u8[0], from->u8[1], leader);
	}
	else
	{
		printf("Leadership Beacon is Received from: %d.%d. \n", from->u8[0], from->u8[1]);
	}


}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(leader_mote_1, ev, data)
{
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if (false == isLeaderAssigned)
    { // this statement should not be entered, if a prior leadership message received!

    	//send my nodeID as the leadership beacon
    	packetbuf_copyfrom(&nodeID, 1);
		broadcast_send(&broadcast);
		printf("Broadcast Message Sent From Mote-%d\n", nodeID);

//		amILeader = true; // assumes that if no leader is assigned I'm the leader.
    }
    else if (true == amILeader)
    { // if I am the leader, I should keep sending periodic Leadership Beacon Messages.

    	//send my nodeID as the leadership beacon
    	packetbuf_copyfrom(&nodeID, 1);
		broadcast_send(&broadcast);
		printf("Broadcast Message Sent From Mote-%d\n", nodeID);
    }
    else
    { // if I'm not the leader, I should keep relaying the Leadership Beacons to outer regions of the network;
    	// although, this feature is not yet used.

    	//send the nodeID of the leader as the beacon
    	packetbuf_copyfrom(&leader, 1);
		broadcast_send(&broadcast);
		printf("Broadcast Message Sent From Mote-%d\n", nodeID);

    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
