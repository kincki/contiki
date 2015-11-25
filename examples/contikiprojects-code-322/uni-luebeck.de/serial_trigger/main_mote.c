/*
 * Copyright (c) 2010, University of Luebeck, Germany.
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
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ''AS IS'' AND
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
 * Author: Carlo Alberto Boano <cboano@iti.uni-luebeck.de>
 * 
 * Description: A simple program of a sensor mote that triggers via serial port
 * another mote to send a specific amount of packets on a specific channel. 
 * The mote then computes the amount of received packets over the expected amount.
 * The trigger is sent not over the air, but through the serial/USB interface (for
 * reliability purposes). 
 *
 */

#include <stdio.h>
#include <signal.h>
#include "contiki.h"
#include "node-id.h"
#include "net/rime.h"
#include "dev/leds.h"
#include "dev/cc2420.h"

#define UC_CONNECTION 151				// Unicast connection of the two nodes
#define PACKETS_TO_BE_SENT 1000			// Amount of packets to be sent by the second node
#define WAITING_TIME (CLOCK_SECOND*4)	// Waiting time after the trigger was sent

// Global variables definition
int count_triggers = 0;
int channel = 0;

// Format of the message to be exchanged
struct message{
	long seqno;	
};

// Definition of the unicast callbacks
static struct unicast_conn uc;
static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{ 
 leds_on(LEDS_GREEN);
 struct message mesg;
 memcpy(&mesg, packetbuf_dataptr(), sizeof(mesg));
 count_triggers++; 
 leds_off(LEDS_GREEN);
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};

/*---------------------------------------------------------------------------*/

PROCESS(triggering_mote, "Triggering mote...");
AUTOSTART_PROCESSES(&triggering_mote);
PROCESS_THREAD(triggering_mote, ev, data)
{
 PROCESS_EXITHANDLER(unicast_close(&uc);)
 PROCESS_BEGIN();
 
 // Initial operations 
 cc2420_set_txpower(CC2420_TXPOWER_MAX); 
 unicast_open(&uc, UC_CONNECTION, &unicast_callbacks);
 leds_on(LEDS_BLUE); 
  
 while(1){ 
	// Repeating this for all channels 11-26 forever 
	for(channel = 11;channel<=26;channel++){ 
		cc2420_set_channel(channel);		 
		// Sending the trigger over the serial interface		
		printf("#%d#%d#%d#\n",channel, node_id, PACKETS_TO_BE_SENT); 
		// Waiting some time so to receive the packets
		static struct etimer et;
		etimer_set(&et, WAITING_TIME);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		// Printing statistics
		printf("Received %d/%d packets in channel %d\n", count_triggers, PACKETS_TO_BE_SENT, channel);
		count_triggers = 0;		 
	 }
 }
 
 PROCESS_END();
}
/*---------------------------------------------------------------------------*/
