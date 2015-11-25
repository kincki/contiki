/*
 * Copyright (c) 2015, Ozyegin University,Computer Science.
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
 * This file is part of the CEP implementation on Contiki Operating System
 *
 */

/**
 * \file
 *         Each mote trying to become the leader of the herd
 * \author
 *         Koray INCKI <koray.incki@ozu.edu.tr>
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
PROCESS(leader_mote, "Leader Candidate");
AUTOSTART_PROCESSES(&leader_mote);
/*---------------------------------------------------------------------------*/

// keep track of current leader status
static bool isLeaderAssigned = false; // initially no leader is assigned
static bool amILeader = false; // initially no body is leader
static uint16_t leader = 0; // initially nobody is leader
static uint8_t conflictingLeaders = 0; // counts the number of conflicting leadership messages
static struct etimer et_leader; // timer for keeping track of leader beacon status messages

// define a new structure for xferring leadership beacons
typedef struct {
	uint8_t leader;	// the id of leader mote
	uint8_t lSource; // the source of leadership beacon
	clock_time_t refreshTime;
} strLeader;

static strLeader s_leader;


static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	uint16_t *inputNodeID;
	uint16_t senderID;
	
	senderID = (uint16_t) from->u8[0];

	strLeader *sLeadMe;
	sLeadMe = (strLeader *) packetbuf_dataptr();
	
	*inputNodeID = sLeadMe->leader;

	//inputNodeID = (uint16_t *) packetbuf_dataptr();


	printf("Received a Leadership Beacon for Mote-%d from Mote-%d.%d\n", *inputNodeID, from->u8[0], from->u8[1]);

	if (false == isLeaderAssigned)
	{ // if leader is not assigned, we should acknowledge the sender of this message as leader
		// check if this node-id is smallar than my id; if not, i'm the leader
		if (node_id < *inputNodeID)
		{
			//leader = node_id;
			s_leader.leader = node_id;
			s_leader.lSource = node_id;
			s_leader.refreshTime = clock_time();
			
			amILeader = true;
			isLeaderAssigned = true;
			// ignore the incoming leadership beacon
			
			// in fact i should immediately send out a broadcast messages stating that i'm the real leader
		}
		else if (node_id == *inputNodeID)
		{ // This means that I'm the leader, and this is a relay of my leadership message; ignore it.
			isLeaderAssigned = true;
			amILeader = true;
			leader = node_id;
			
			s_leader.leader = node_id;
			s_leader.lSource = node_id;
			s_leader.refreshTime = clock_time();
		}
		else if ((uint16_t) from->u8[0] != *inputNodeID)
		{ // this is a relay leadership message;
			if (node_id < *inputNodeID)
			{ // i must be the leader, because my id is smallar
				//leader = node_id;
				s_leader.leader = node_id;
				s_leader.lSource = node_id;
				s_leader.refreshTime = clock_time();
				
				isLeaderAssigned = true;
				amILeader = true;
				
				// in fact i should immediately send out a broadcast messages stating that i'm the real leader
			}
			else if (s_leader.leader < *inputNodeID)
			{ // this case should never occur, because leader has not been assigned yet
				// because it means that I assigned myself as the leader and my id is smaller than the incoming message

				// in fact i should immediately send out a broadcast messages stating that i'm the real leader

				printf("This case should not have happened!\n");					
			}
		}
		else
		{
			isLeaderAssigned = true;
			amILeader = false;
			// leader = (uint16_t) from->u8[0]; // leader Mote-ID is assigned for local reference
			
			s_leader.leader = (uint16_t) from->u8[0];
			s_leader.lSource = (uint16_t) from->u8[0];
			s_leader.refreshTime = clock_time();

			printf("I'm Mote-%d. Leader Elected as %d.%d\n.", node_id, from->u8[0], from->u8[1]);
		}
	}
	else if (*inputNodeID != s_leader.leader)
	{
		printf("Duplicate Leadership broadcast message received from %d.%d. Leader is %d. \n",
					from->u8[0], from->u8[1], s_leader.leader);
		
		// in fact i should immediately send out a broadcast messages stating the real value of leader

		if ( s_leader.leader < *inputNodeID )
		{ // my Leader's id is smaller than leader candidate message id, i must be the leader
			
			isLeaderAssigned = true;
			
			if (s_leader.leader == node_id)
				amILeader = true;
			else
				amILeader = false;
			
			// I need to restart the beacon timeout timer
			etimer_restart(&et_leader);
			printf("Restart the Leader Timer - %d.1\n", node_id);
			
			// in fact i should immediately send out a broadcast messages stating the real value of leader


		}
		else if ( *inputNodeID < s_leader.leader )
		{ // leader id is bigger than the incoming leadership candidate
			
			printf("The incoming leader has smaller id (%d), My leader has been revoked and (%d) is not leader anymore\n", *inputNodeID, s_leader.leader);

			//leader = *inputNodeID;
			s_leader.leader = *inputNodeID;
			s_leader.lSource = senderID;
			s_leader.refreshTime = clock_time();
			
			
			isLeaderAssigned = true; 
			amILeader = false;			
						
			// I need to restart the beacon timeout timer
			etimer_restart(&et_leader);
			printf("Restart the Leader Timer - %d.2\n", node_id);
			
			// in fact i should immediately send out a broadcast messages stating the real value of leader
		}
		else
		{ // the current leader seams to be a legitimate leader, carry on buddy!
		}
	}
	else // means that  (*inputNodeID == leader)
	{ 
		// i should find a way to relay the livelihood of leader to my neighboring motes. ??
		// otherwise, the motes that doesn't have direct link to leader mote will immediately
		// flag a Leader is Unreachable message to the network.!!
		
		
		// this is a beacon message from my beloved current leader, reset the leader timer
		if (senderID == s_leader.leader)
		{
			printf("Leadership Beacon is Received from: %d.%d. \n", from->u8[0], from->u8[1]);
			etimer_restart(&et_leader);
			printf("Restart the Leader Timer - %d.3\n", node_id);
		}
	}


}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(leader_mote, ev, data)
{
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);

	// initialize leader beaconing structure, because if you don't explicitly
	// initialize to something, computer will initialize its members to 0, since it is 
	// defined with 'static' storage class.
	
	s_leader.leader = node_id;
	s_leader.lSource = node_id;
	s_leader.refreshTime = clock_time();
	
	
  /* Set up a timer for waiting Leadership Beacon messages for 10 seconds */
  etimer_set(&et_leader, CLOCK_SECOND * 10 + random_rand() % (CLOCK_SECOND * 10));
    
  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2));
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

	
    if (false == isLeaderAssigned)
    { // this statement should not be entered, if a prior leadership message received!

		// so, if it enters this statement, I'm the first mote to start broadcasting leadership beacons
		s_leader.leader = node_id;
		s_leader.lSource = node_id;
		s_leader.refreshTime = clock_time();
		
		isLeaderAssigned = true;
		amILeader = true;
		
    	//send my nodeID as the leadership beacon
    	packetbuf_copyfrom(&s_leader, sizeof(s_leader));
		broadcast_send(&broadcast);
		printf("I declare that I'm a Candidate Leader-%d\n", s_leader.leader);
		
		// restart the timer
		etimer_restart(&et_leader);

		// amILeader = true; // assumes that if no leader is assigned I'm the leader.
    }
    else if (true == amILeader)
    { // if I am the leader, I should keep sending periodic Leadership Beacon Messages.

    	//send my nodeID as the leadership beacon
    	packetbuf_copyfrom(&s_leader, sizeof(s_leader));
		broadcast_send(&broadcast);
		printf("I survive as the Leader Mote-%d\n", node_id);
		
		// restart the timer
		etimer_restart(&et_leader);
    }
    else
    { // if I'm not the leader, I should keep relaying the Leadership Beacons to outer regions of the network;
    	// although, this feature is not yet used.

    	//send the nodeID of the leader as the beacon
    	packetbuf_copyfrom(&s_leader, sizeof(s_leader));
		broadcast_send(&broadcast);
		printf("Leader is not me (%d), but is Mote-%d\n", node_id, s_leader.leader);
    }
    
	if (etimer_expired(&et_leader))
	{ // Leader Beacon Timer Has Expired!
		printf("I (%d) Sensed that Leader (%d) is Unreachable!\n", node_id, s_leader.leader);
	}
  } // while(1)

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
