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

#define NUM_MOTES 5 // NUMBER OF NETWORKS PREDEFINED BEFORE THE SIMULATION

#define RVK_TIMEOUT 4 // WAIT FOR THIS PERIOD BEFORE PROMOTING YOURSELF AS LEADER

#define CONF_TIMEOUT 8 // WAIT FOR THIS PERIOD BEFORE SENDING OUT CONFLICT ALARM

#define ERROR_ON_MOTE3 1 // ENABLE/DISABLE A PREJUDICE ON MOTE3 AGAINST MOTE2 LEADERSHIP

/*-------------------------MESSAGE TYPES--------------------------------------*/
typedef enum {
  LDR_BEACON = 10, 	// VERY FIRST BEACON MESSAGE FOR THIS LEADER
  LDR_REFRESH = 20, 	// FOLLOW-UP MESSAGES FOR SAME LEADER	
  LDR_REVOKE = 30,	// REVOKE UNREACHABLE LEADER	
  LDR_CONFLICT = 40    // RELAY MESSAGE FOR LEADER BY SOME OTHER MOTE
} eLdrMsgType;

// keep track of current leader status
static bool isLeaderAssigned = false; // initially no leader is assigned
static bool amILeader = false; // initially no body is leader
static struct etimer et_leader; // timer for keeping track of leader beacon status messages
static struct etimer et_revoke; // timer for promoting oneself as leader
static bool revokeStatus = false; // keep track of the status of revoke timer

// define a new structure for xferring leadership beacons
typedef struct {
  uint8_t leader;	// the id of leader mote
  uint8_t lSource; // the source of leadership beacon
  clock_time_t refreshTime;
  eLdrMsgType lMsgType; // determines the type of message 	
} strLeader;

static strLeader s_leader;

// these variables are going to be used for conflicting leaders cases
typedef struct {
  uint8_t confLeader; // conflicting leader ID
  uint8_t sourceID; // who reported this conflict
  struct etimer confTimer;
  bool bSet; // determines if timer is set
} confStr;

static confStr confTmrArr[NUM_MOTES];

/*-----------------------Function Prototypes--------------------------*/

// This method is used for sending out leadership beacons that contains
// leadership structure contents.
void send_leadership_beacon(strLeader*);

// This method sets the Leader structure with new values and current clock_time
void set_leader(uint8_t, uint8_t, eLdrMsgType, clock_time_t);

// This method retrieves the leader id from the leader struct
uint8_t get_leader_id();

// This method retrieves the source of leadership structure
uint8_t get_source_id();

// This method retrieves the refresh time from the leadership structure
clock_time_t get_refreshTime();

// This method checks whether the incoming message is fresh
bool is_msg_fresh(clock_time_t);

// This method retrieves the leadership message type
uint8_t get_leader_status();


/*--------------------------------------------------------------------*/

// This is the Callback function that is called by Contiki-OS whenever a new 
// broadcast message is received
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  uint16_t promotedLeaderID; // this is the leader ID that is conveyed in this message
  uint16_t senderID; // this is the ID of message originator
  strLeader confLeader; // this variable defines conflictin leaders
  strLeader *sLeadMe; // this variable represents incoming leader structure
  uint8_t index; // looop counter

  senderID = (uint16_t) from->u8[0];
  sLeadMe  = (strLeader *) packetbuf_dataptr();
  
  promotedLeaderID = (uint16_t) sLeadMe->leader;
  printf("Received ->  LBeacon(%d) from Mote-%d\n", promotedLeaderID, senderID);

  if ( true == is_msg_fresh(sLeadMe->refreshTime) )
    { // message is last updated in less than 10 seconds
		
      switch (sLeadMe->lMsgType)
	{ 

	case LDR_REFRESH:// A Refresh or Beacon message, carry on regularly

	  if ( promotedLeaderID == get_leader_id() )
	    { // check if I received a Revoke Message recently
	      if ( LDR_REVOKE == get_leader_status() )
		{ // I should notify these immediate neighbors about the 
		  // REVOKE status of our leader
		  send_leadership_beacon(&s_leader);
		  printf("Received -> Already Sent a REVOKE for this Leader (%d)\n", promotedLeaderID);
		  break;
		}
	    }

	case LDR_BEACON:

	  if (promotedLeaderID < get_leader_id())
	    { // I have to update my leadership structure
#if ERROR_ON_MOTE3 // if the system contains an error on mote3
	      if ((3 == node_id) && (2 == promotedLeaderID) ) {
		printf("Received -> I (3) don't trust 2's leadership!\n");
		break;
	      }
#endif
	      set_leader(promotedLeaderID, senderID, LDR_BEACON, sLeadMe->refreshTime);
	      send_leadership_beacon(&s_leader);
	      etimer_restart(&et_leader);
	      printf("Received -> The Leader is set to %d\n", get_leader_id());
	      
	      if (get_leader_id() != node_id)
		amILeader = false;
	    }
	  else if ( get_leader_id() == promotedLeaderID )
	    {
	      if ( get_leader_id() != node_id )
		{ // I'm not leader
 		  		      
		  // if there is a conflict timer for Mote-Source, stop it
		  if ( confTmrArr[senderID - 1].bSet == true )
		    {
		      etimer_stop( &(confTmrArr[senderID - 1].confTimer) );
		      printf("Received -> Stopped Conflicting Timer for Conf Leader %d on Mote-%d\n", 
			     confTmrArr[senderID - 1].confLeader, confTmrArr[senderID - 1].sourceID);
		      confTmrArr[senderID - 1].bSet = false;
		    }
		  
		  if ( get_refreshTime() < sLeadMe->refreshTime )
		    { // This is a fresher leadership beacon
		      set_leader(promotedLeaderID, senderID, LDR_REFRESH, sLeadMe->refreshTime);
		      send_leadership_beacon(&s_leader);
		      etimer_restart(&et_leader);
		      printf("Received -> Refreshed the leader %d\n", get_leader_id());
		      amILeader = false;
		  
		      
		    }
		  else if ( get_refreshTime() > sLeadMe->refreshTime )
		    { // i  need to update this sender's lbeacon with my refresh time
		      send_leadership_beacon(&s_leader);
		      printf("Received -> My Leader Beacon is fresher. Take This!\n");
		    }
		} // I'm not the leader
	      else
		{
		  printf("Received -> (%d) Acknowledges that I'm the Leader\n", senderID);
		  // I should stop any conflict timers set
		  for ( index = 0; index < NUM_MOTES; index++ )
		    {
		      if ( true == confTmrArr[index].bSet )
			{
			  etimer_stop( &(confTmrArr[index].confTimer) );
			  confTmrArr[index].bSet = false;
			  printf("Received -> Stopped Conflicting Timer for Conf Leader %d on Mote-%d\n", 
				 confTmrArr[index].confLeader, confTmrArr[index].sourceID);
			}
		    }
		}
	    }
	  else
	    { // promotedLeaderID > myLeaderID - There might occur conflicts in the future. 
	      if ( true == confTmrArr[senderID - 1].bSet )
		{ // if there is already a conflict timer for Mote-source
		  if ( confTmrArr[senderID - 1].confLeader == get_leader_id() )
		    { // if it's on same LBeacon(ID)
		      if ( etimer_expired( &(confTmrArr[senderID - 1].confTimer) ) )
			{//  if Conflict Timer Expired
			  //	Trigger a Conflicting Leaders Event
			  printf("Received -> Trigger a Conflicting Leader Event w/ %d from Mote-%d\n", promotedLeaderID, senderID);
			  confLeader.leader = get_leader_id();
			  confLeader.lSource = promotedLeaderID;
			  confLeader.refreshTime = clock_time();
			  confLeader.lMsgType = LDR_CONFLICT;
			  
			  send_leadership_beacon(&confLeader);	

			  etimer_stop(&(confTmrArr[senderID - 1].confTimer));
			}
		      else
			;
		    }
		  else
		    { // this is a new conflict
		      // restart the conflict timer for Mote-Source on LBeacon(ID)
		      confTmrArr[senderID - 1].confLeader = promotedLeaderID;
		      confTmrArr[senderID - 1].bSet = true;
		      confTmrArr[senderID - 1].sourceID = senderID;
		      etimer_restart( &(confTmrArr[senderID-1].confTimer) );
		      printf("Received -> Refresh a Conflicting Leader Event w/ %d from Mote-%d\n", promotedLeaderID, senderID);

		      // let the conflicting leader know that there is another leader in range
		      send_leadership_beacon(&s_leader);
		    }
		}
	      else
		{ // start a new conflict timer for Mote-Source on LBeacon(ID)
		  confTmrArr[senderID - 1].confLeader = promotedLeaderID;
		  confTmrArr[senderID - 1].bSet = true;
		  confTmrArr[senderID - 1].sourceID = senderID;
		  etimer_set( &(confTmrArr[senderID - 1].confTimer), CLOCK_SECOND * CONF_TIMEOUT + random_rand() % (CLOCK_SECOND * CONF_TIMEOUT) );
		  
		  printf("Received -> Start a Conflicting Leader Timer w/ %d from Mote-%d\n", promotedLeaderID, senderID);

		  // let the conflicting leader know that there is another leader in range
		  send_leadership_beacon(&s_leader);
		}
	    } // else // promotedLeaderID > myLeaderID
	  break;

	case LDR_REVOKE:

	  printf("Received -> REVOKE received for Ldr(%d) from (%d)\n", promotedLeaderID, senderID);
	  if ( promotedLeaderID  == get_leader_id() )
	    { // REVOKE message comes from my leader source, i have to act upon it
	      if ( false == revokeStatus )
		{
		  printf("Received -> I will also REVOKE the leader\n");
		  set_leader(promotedLeaderID, senderID, LDR_REVOKE, sLeadMe->refreshTime);
		  send_leadership_beacon(&s_leader);

		  // and also start the revoke timer
		  etimer_set(&et_revoke, CLOCK_SECOND * RVK_TIMEOUT + random_rand() % (CLOCK_SECOND * RVK_TIMEOUT));
		  printf("Received -> REVOKE Timer Started\n");
		  revokeStatus = true;
		}
	      else
		printf("Received -> I already REVOKE this leader (%d) Dude\n", promotedLeaderID);
	    }
	  else
	    printf("Received -> REVOKE (%d) is Ignored. My leader is (%d)\n", promotedLeaderID, get_leader_id() );

	  break;

	case LDR_CONFLICT:

	  printf("Received -> CONFLICT message on Leader (%d) with ConfLeader (%d)\n, FROM %d", sLeadMe->leader, sLeadMe->lSource, senderID);
	  break;

	default:
	  printf("Received -> Why Am I Here? !\n");
	  break;
	}	// switch (sLeadMe->lMsgType)	
    } // if (is_msg_fresh())
  else 
    { // sLeadMe->lAlive == false
      // i see that our beloved (!) leader has went awry
      // this is my chance to reclaim the throne and announce myself as the eternal leader (!!)
      set_leader(node_id, node_id, LDR_BEACON, clock_time());
      
      printf("Received -> I (%d) might be Leader Again!\n", node_id);
      
      // notify all network that I am in business!!
      send_leadership_beacon(&s_leader);
      
      amILeader = true;
    }
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
//This method sends out current content of leadership structure as a beacon
void
send_leadership_beacon(strLeader *sLeader)
{
  packetbuf_copyfrom(sLeader, sizeof(*sLeader));
  broadcast_send(&broadcast);
}

/*---------------------------------------------------------------------------*/
// This method is used to set new values for leadership structure
void
set_leader(uint8_t newLeader, uint8_t newSource, eLdrMsgType msgType, clock_time_t rTime)
{
  s_leader.leader = newLeader;
  s_leader.lSource = newSource;
  s_leader.lMsgType = msgType;
  s_leader.refreshTime = rTime;
}

/*---------------------------------------------------------------------------*/
// This method retrieves the leadership message type
uint8_t
get_leader_status()
{
  return s_leader.lMsgType;
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

// This method checks whether the incoming message is fresh (sent in 10 seconds)
bool 
is_msg_fresh(clock_time_t rTime)
{
  int seconds = 0;
  bool bFresh = true;
	
  /* seconds = (clock_time() - rTime) / CLOCK_SECOND;  */
  seconds = ( rTime - get_refreshTime() ) / CLOCK_SECOND; 
  
  if (seconds < 10)
    ;
  else
    bFresh = false;
  
  return bFresh;
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(leader_mote, ev, data)
{
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  int index = 0; // loop counter
  strLeader confLeader; // it is going to be used in case a conflict occurs

  broadcast_open(&broadcast, 129, &broadcast_call);

  // initialize leader beaconing structure, because if you don't explicitly
  // initialize to something, computer will initialize its members to 0, since it is 
  // defined with 'static' storage class.
	
  set_leader(node_id, node_id, LDR_BEACON, clock_time());
  isLeaderAssigned = true;
  amILeader = true;

  // INITIALIZE Conflict Timers

  for ( index = 0; index < NUM_MOTES; index++ )
      confTmrArr[index].bSet = false;

  /* Set up a timer for waiting Leadership Beacon messages for 10 seconds */
  etimer_set(&et_leader, CLOCK_SECOND * 10 + random_rand() % (CLOCK_SECOND * 10));
    
  while(1) 
    {

      /* Delay 2-4 seconds */
      etimer_set(&et, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2));
      
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      
      
      if (false == isLeaderAssigned)
	{ // this statement should not be entered, if a prior leadership message received!
	  
	  // so, if it enters this statement, I'm the first mote to start broadcasting leadership beacons
	  set_leader(node_id, node_id, LDR_BEACON, clock_time());
	  amILeader = true;
	  
	  //send my nodeID as the leadership beacon
	  send_leadership_beacon(&s_leader);
	  printf("Sent -> Candidate LBeacon(%d)\n", s_leader.leader);
	  
	  // restart the timer
	  etimer_restart(&et_leader);
	  // amILeader = true; // assumes that if no leader is assigned I'm the leader.
	}
      else if (true == amILeader)
	{ // if I am the leader, I should keep sending periodic Leadership Beacon Messages.
	  
	  //send my nodeID as the leadership beacon
	  s_leader.refreshTime = clock_time();
	  
	  send_leadership_beacon(&s_leader);
	  printf("Sent -> Still I'm LBeacon(%d)\n", node_id);
	  
	  // restart the timer
	  etimer_restart(&et_leader);
	}
      else
	{ // if I'm not the leader, I should keep relaying the Leadership Beacons to outer regions of the network;
	  // although, this feature is not yet used.
	  
	  //send the nodeID of the leader as the beacon
	  if ( s_leader.lMsgType != LDR_REVOKE )
	    {
	      send_leadership_beacon(&s_leader);
	      printf("Sent -> Relay LBeacon(%d)\n", get_leader_id() );
	    }
	}
      
      // Check-out all the timers for further decision making

      if ( (etimer_expired(&et_leader)) && (false == revokeStatus) )
	{ // Leader Beacon Timer Has Expired!
	  printf("I (%d) Sensed that Leader (%d) is Unreachable!\n", node_id, get_leader_id() );
	  
	  set_leader(s_leader.leader, node_id, LDR_REVOKE, get_refreshTime());
	  
	  // notify all network that I cannot reach leader
	  send_leadership_beacon(&s_leader);
	  printf("Sent -> LBeacon(%d, Unreachable)\n", get_leader_id());
	  etimer_set(&et_revoke, CLOCK_SECOND * RVK_TIMEOUT + random_rand() % (CLOCK_SECOND * RVK_TIMEOUT));
	  printf("Sent -> REVOKE Timer Started \n");
	  etimer_stop(&et_leader);
	  revokeStatus = true;
	}

      if ( true == revokeStatus )
	{
	  if (etimer_expired(&et_revoke))
	    { // it is time to promote myself as the new leader
	      printf("Sent -> It is time to promote myself as the new leader\n");
	      set_leader(node_id, node_id, LDR_BEACON, clock_time());
	      amILeader = true;
	      send_leadership_beacon(&s_leader);
	      printf("Sent -> I Declare Myself as the Next Leader\n");
	      printf("Sent -> REVOKE Timer Stopped\n");
	      etimer_stop(&et_revoke);
	      etimer_restart(&et_leader);
	      revokeStatus = false;
	    }
	}

      // there are more than one timer for several conflicting leaders
      for ( index = 0; index < NUM_MOTES; index++ )
	{
	  if ( true == confTmrArr[index].bSet )
	    {
	      if ( etimer_expired( &(confTmrArr[index].confTimer) ) )
		{//  if Conflict Timer Expired
		  //	Trigger a Conflicting Leaders Event
		  printf("Sent -> CONFLICT: Trigger a Conflicting Leader Event w/ %d from Mote-%d\n", confTmrArr[index].confLeader, confTmrArr[index].sourceID);
		  confLeader.leader = get_leader_id();
		  confLeader.lSource = confTmrArr[index].confLeader;
		  confLeader.refreshTime = clock_time();
		  confLeader.lMsgType = LDR_CONFLICT;
			  
		  send_leadership_beacon(&confLeader);			  
		  etimer_stop(&(confTmrArr[index].confTimer));		  
		}
	    }
	}
    } // while(1)
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/


