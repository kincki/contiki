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
 * Description: A simple program of a sensor mote that receives a trigger via serial 
 * port from another mote to send a specific amount of packets on a specific channel.
 * The trigger comes from the serial/USB interface (for reliability purposes). 
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include "contiki.h"
#include "net/rime.h"
#include "dev/leds.h"
#include "dev/cc2420.h"
#include "dev/serial-line.h"
#include "dev/watchdog.h"

#define UC_CONNECTION 151	// Unicast connection of the two nodes

// Global variables definition
long count = 0;

// Format of the message to be exchanged
struct message{
	long seqno;	
};

// Definition of the unicast callbacks
static struct unicast_conn uc;
static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{ 
 printf("ERROR: received an unexpected unicast from host (%d:%d)!\n",from->u8[0],from->u8[1]);  
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};

// Handlers to get the commands from the serial port
static void handle_command(char* cmd)
{  
  printf("Received command: %s\n", cmd);	
  
  int len = strlen(cmd);
  if(len <= 0) {
	leds_on(LEDS_ALL);
    printf("Error. Wrong input (length < 0).\n");
	leds_off(LEDS_ALL);
	leds_on(LEDS_BLUE);
	return;
  }
    
  if((len >= 5) && (cmd[0] == '#') && (cmd[3] == '#')) { 		    	
	printf("Command recognized. Getting parameters...\n");
	// Getting channel
	int ch = atoi(&cmd[1]);	
	if((ch > 26) || (ch < 11)) {
		printf("Error. Wrong channel.\n");		
		return;
	}
	// Getting destination address
	int dest = atoi(&cmd[4]);
	if((dest < 1) || (dest > 255)){	
		printf("Error. Invalid destination address.\n");
		return;
	}	
	// Getting amount of packets
	int i=4;
	while(cmd[i++] != '#');
	int number_of_packets = atoi(&cmd[i]);
	if((number_of_packets < 1) || (number_of_packets > 32000)){	
		printf("Error. Invalid number of packets.\n");
		return;
	}	
	
	// Starting to send the data as requested
	leds_on(LEDS_RED);
	cc2420_set_channel(ch);		
	printf("Set channel to %d, destination = %d. Sending %d packets...\n", ch, dest, number_of_packets);				
	count = 0;
	rimeaddr_t addr;
	addr.u8[0] = dest;
	addr.u8[1] = 0;	
	struct message msg;
	// Sending the packets as a burst
	for(i=0;i<number_of_packets;i++){
		msg.seqno = count++;	
		packetbuf_clear();
		packetbuf_copyfrom(&msg, sizeof(struct message));
		unicast_send(&uc, &addr);	  			
	}		
	leds_off(LEDS_RED);
  }
  else {
	leds_on(LEDS_GREEN);
	printf("Error. Unknown command %s. Correct command is '#channel#destination#'\n", cmd);	
	clock_delay(35000);
	leds_off(LEDS_GREEN);
  }
}

PROCESS(triggered_mote, "Triggered mote...");
AUTOSTART_PROCESSES(&triggered_mote);
PROCESS_THREAD(triggered_mote, ev, data)
{
 PROCESS_EXITHANDLER(unicast_close(&uc);)
 PROCESS_BEGIN();
  
 // Initial configurations  on CC2420 and resetting the timer
 leds_off(LEDS_ALL);
 cc2420_set_txpower(CC2420_TXPOWER_MAX); 
 unicast_open(&uc, UC_CONNECTION, &unicast_callbacks); 
 watchdog_stop(); 
  
 // Ready...
 leds_on(LEDS_BLUE); 
 
 // Triggering some packets to be sent
 while(1) {	
	PROCESS_WAIT_EVENT();
    if(ev == serial_line_event_message) {        
        handle_command((char*)data);
    }
 } 
 
 PROCESS_END();
}
