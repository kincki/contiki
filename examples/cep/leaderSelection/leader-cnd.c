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
static struct etimer et_leader; // timer for keeping track of leader beacon status messages

// these variables are going to be used for conflicting leaders cases
static uint8_t conflictingLeaders = 0; // counts the number of conflicting leadership messages

// define a new structure for xferring leadership beacons
typedef struct {
	uint8_t leader;	// the id of leader mote
	uint8_t lSource; // the source of leadership beacon
	clock_time_t refreshTime;
	bool lAlive; // determines if the current leader is alive
} strLeader;

static strLeader s_leader;

// This method is used for sending out leadership beacons that contains
// leadership structure contents.
void send_leadership_beacon(void);

// This method sets the Leader structure with new values and current clock_time
void set_leader(uint8_t, uint8_t, bool);

// This method retrieves the leader id from the leader struct
uint8_t get_leader_id();

// This method retrieves the source of leadership structure
uint8_t get_source_id();

// This method retrieves the refresh time from the leadership structure
clock_time_t get_refreshTime();

// This method checks whether the leadership beacon is fresh or not
bool is_leader_fresh(clock_time_t);

// This is the Callback function that is called by Contiki-OS whenever a new 
// broadcast message is received
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	uint16_t promotedLeaderID; // this is the leader ID that is conveyed in this message
	uint16_t senderID; // this is the ID of message originator
	
	senderID = (uint16_t) from->u8[0];

	strLeader *sLeadMe;
	sLeadMe = (strLeader *) packetbuf_dataptr();
	
	// delete this line, it's just for example
	is_leader_fresh( (clock_time_t) sLeadMe->refreshTime);
	
	promotedLeaderID = (uint16_t) sLeadMe->leader;

	printf("Received ->  LBeacon(%d) from Mote-%d\n", promotedLeaderID, senderID);

	if (sLeadMe->lSource == node_id)
	{
		// this message originates from myself, i can omit it
		printf("Received -> My LBeacon(%d) Bounced Back from Mote-%d\n", node_id, senderID);
		
		// hmm. but it might be the case that I acquired a new leader
		// i should check that too
		if (sLeadMe->leader == get_leader_id())
		{
			printf("Received -> Ok, I know I'm the Leader!\n");
			return; // don't do this again!!!
		}
		else
		{
			if ( sLeadMe->refreshTime < get_refreshTime() )
				printf("Receieved -> Dear Lord! I happened to acquire a better leader (%d) since the last leader (%d)\n", get_leader_id(), sLeadMe->leader);
			else
				printf("Receieved -> Dear Lordy! I happened to acquire a better leader (%d) since the last leader (%d)\n", get_leader_id(), sLeadMe->leader);
		}
	}
	
	if (sLeadMe->lAlive == true)
	{
		if (false == isLeaderAssigned)
		{ 
			// check if this node-id is smallar than my id; if not, i'm the leader
			if (node_id < promotedLeaderID)
			{
				set_leader(node_id, node_id, true);
				
				amILeader = true;
				isLeaderAssigned = true;
				// ignore the incoming leadership beacon
				
				// in fact i should immediately send out a broadcast messages stating that i'm the real leader
			}
			else if (node_id == promotedLeaderID)
			{ 
				// This means that I'm the leader, and this is a relay of my leadership message; ignore it.
				// I don't need to do anything
			}
			else if ( senderID != promotedLeaderID)
			{ // this is a relay leadership message;
				if (node_id < promotedLeaderID)
				{ // i must be the leader, because my id is smallar
					
					set_leader(node_id, node_id, true);
					
					isLeaderAssigned = true;
					amILeader = true;
					
					printf("Received -> LBeacon(%d) is Greater Than my Leader (%d)\n", promotedLeaderID, get_leader_id());
					
					// in fact i should immediately send out a broadcast messages stating that i'm the real leader
				}
				else if (get_leader_id() < promotedLeaderID)
				{ // this case should never occur, because leader has not been assigned yet
					// because it means that I assigned myself as the leader and my id is smaller than the incoming message

					// in fact i should immediately send out a broadcast messages stating that i'm the real leader

					printf("Received -> This case should not have happened!\n");					
				}
			}
			else
			{
				isLeaderAssigned = true;
				amILeader = false;
				
				/* ******
				s_leader.leader = (uint16_t) from->u8[0];
				s_leader.lSource = (uint16_t) from->u8[0];
				s_leader.refreshTime = clock_time();
				s_leader.lAlive = true;
				* ******/
				
				set_leader(promotedLeaderID, senderID, true);

				printf("Received -> Leader Elected as %d\n", get_leader_id());
			}
		} // isLeaderAssigned == true
		else if (promotedLeaderID != s_leader.leader)
		{
			printf("Received -> Yet Another LBeacon(%d) from Mote-%d while My Leader is %d \n",	promotedLeaderID, senderID, get_leader_id());
			
			// verify that this message is fresher than my records!
			if (sLeadMe->refreshTime > get_refreshTime())
			{
				printf("Received -> a Fresh LBeacon(%d) from Mote-%d\n", promotedLeaderID, senderID);
			}
			else
			{
				printf("Received -> an Obsolete LBeacon(%d) from Mote-%d\n", promotedLeaderID, senderID);
			}

			if ( get_leader_id() < promotedLeaderID )
			{ // my Leader's id is smaller than leader candidate message id, i must be the leader
				
				isLeaderAssigned = true;
				
				// I need to restart the beacon timeout timer
				etimer_restart(&et_leader);
				//printf("Received -> Restart the Leader Timer - %d: My Leader Rules!\n", node_id);
				
				// in fact i should immediately send out a broadcast messages stating the real value of leader
				send_leadership_beacon();
			}
			else if ( promotedLeaderID < s_leader.leader )
			{ // leader id is bigger than the incoming leadership candidate
				
				printf("Received -> LBeacon(%d) is Smaller Than My Leader (%d), I revoke its leadership\n", promotedLeaderID, get_leader_id());

				set_leader(promotedLeaderID, senderID, true);
								
				isLeaderAssigned = true; 
				amILeader = false;			
							
				// I need to restart the beacon timeout timer
				etimer_restart(&et_leader);
				//printf("Received -> Restart the Leader Timer - %d: Revoke My Leader! \n", node_id);
				
				// in fact i should immediately send out a broadcast messages stating the real value of leader
				send_leadership_beacon();
			}
			else
			{ // the current leader seams to be a legitimate leader, carry on buddy!
			}
		}
		else // means that  (promotedLeaderID == leader)
		{ 
			// i should find a way to relay the livelihood of leader to my neighboring motes. ??
			// otherwise, the motes that doesn't have direct link to leader mote will immediately
			// flag a Leader is Unreachable message to the network.!!
			
			
			// this is a beacon message from my beloved current leader, reset the leader timer
			if ( senderID == get_leader_id() )
			{
				printf("Received -> LBeacon(%d) from our Beloved Leader! \n", get_leader_id());
				etimer_restart(&et_leader);
				//printf("Received -> Restart the Leader Timer - %d: LBeacon Received from The Leader! \n", node_id);
			}
		}
	} // if (sLeadMe->lAlive == true)
	else 
	{ // sLeadMe->lAlive == false
		// i see that our beloved (!) leader has went awry
		// this is my chance to reclaim the throne and announce myself as the eternal leader (!!)
		set_leader(node_id, node_id, true);
		
		printf("Received -> I (%d) might be Leader Again!\n", node_id);
		
		// notify all network that I am in business!!
		send_leadership_beacon();
	}
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
//This method sends out current content of leadership structure as a beacon
void
send_leadership_beacon()
{
	packetbuf_copyfrom(&s_leader, sizeof(s_leader));
	broadcast_send(&broadcast);
}

/*---------------------------------------------------------------------------*/
// This method is used to set new values for leadership structure
void
set_leader(uint8_t newLeader, uint8_t newSource, bool isAlive)
{
	s_leader.leader = newLeader;
	s_leader.lSource = newSource;
	s_leader.lAlive = isAlive;
	s_leader.refreshTime = clock_time();
}

/*---------------------------------------------------------------------------*/
// This method retrieves the leader id from leader structure
uint8_t
get_leader_id()
{
	return s_leader.leader;
}

/*---------------------------------------------------------------------------*/
// This method retrieves the source of leadership message 
uint8_t
get_source_id()
{
	return s_leader.lSource;
}

/*---------------------------------------------------------------------------*/
// This method retrieves the refresh time from the leadership structure
clock_time_t 
get_refreshTime()
{
	return s_leader.refreshTime;
}


/*---------------------------------------------------------------------------*/
// This method checks whether the leadership beacon is fresh or not
bool 
is_leader_fresh(clock_time_t rTime)
{
	int seconds = 0;
	bool bFresh = false;
	
	seconds = rTime / CLOCK_SECOND;
	
	printf("%d Seconds Fresh This Message is\n", seconds);
	
	if ( seconds < 10 )
		bFresh = true;
		
	return bFresh;
}

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
	
	set_leader(node_id, node_id, true);
	
	
  /* Set up a timer for waiting Leadership Beacon messages for 10 seconds */
  etimer_set(&et_leader, CLOCK_SECOND * 10 + random_rand() % (CLOCK_SECOND * 10));
    
  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2));
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

	
    if (false == isLeaderAssigned)
    { // this statement should not be entered, if a prior leadership message received!

		// so, if it enters this statement, I'm the first mote to start broadcasting leadership beacons
		set_leader(node_id, node_id, true);
		
		isLeaderAssigned = true;
		amILeader = true;
		
    	//send my nodeID as the leadership beacon
    	send_leadership_beacon();
		printf("Sent -> Candidate LBeacon(%d)\n", s_leader.leader);
		
		// restart the timer
		etimer_restart(&et_leader);

		// amILeader = true; // assumes that if no leader is assigned I'm the leader.
    }
    else if (true == amILeader)
    { // if I am the leader, I should keep sending periodic Leadership Beacon Messages.

    	//send my nodeID as the leadership beacon
    	send_leadership_beacon();
		printf("Sent -> Still I'm LBeacon(%d)\n", node_id);
		
		// restart the timer
		etimer_restart(&et_leader);
    }
    else
    { // if I'm not the leader, I should keep relaying the Leadership Beacons to outer regions of the network;
    	// although, this feature is not yet used.

    	//send the nodeID of the leader as the beacon
    	send_leadership_beacon();
		printf("Sent -> LBeacon(%d)\n", s_leader.leader);
    }
    
	if (etimer_expired(&et_leader))
	{ // Leader Beacon Timer Has Expired!
		printf("I (%d) Sensed that Leader (%d) is Unreachable!\n", node_id, s_leader.leader);
		
		set_leader(s_leader.leader, node_id, false);
		
		// notify all network that I cannot reach leader
    	send_leadership_beacon();
    	printf("Sent -> LBeacon(%d, Unreachable)\n", get_leader_id());
	}
  } // while(1)

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/


