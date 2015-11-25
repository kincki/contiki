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
 * Author: Carlo Alberto Boano <cboano@iti.uni-luebeck.de>
 *
 */
 
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "contiki.h"
#include "net/rime.h"
#include "dev/leds.h"
#include "dev/cc2420.h"	

#define POWER 31
#define CHANNEL_BASE 22
#define CHANNEL_SENDER 24
#define RIME_SENDER 122
#define RIME_BASE 123

#define SENDER 104
#define RECEIVER 102
#define BASE 110

struct message_base{
	long seqno;
	int noise;
	int rssi;
	int lqi;
	int snr;
	int prr;
};

PROCESS(sensys_base, "SenSys base...");
AUTOSTART_PROCESSES(&sensys_base);

// Handlers for unicast
static struct unicast_conn ucb;
static void recv_uc_base(struct unicast_conn *c, const rimeaddr_t *from)
{ 
 if((from->u8[0] == RECEIVER) && (from->u8[1] == 0)){	 	 
	 leds_on(LEDS_GREEN);
	 struct message_base mesg;
	 memcpy(&mesg, packetbuf_dataptr(), sizeof(mesg)); 	 
	 int r = (cc2420_last_rssi - 45);	 
	 int n = (cc2420_rssi() - 45);
	 int l = (cc2420_last_correlation);
	 int s = r - n;	 
	 if(s<0) s=0;
	 printf("BASE:%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",mesg.seqno,mesg.noise,mesg.rssi,mesg.lqi,mesg.snr,mesg.prr,r,n,l,s);
	 leds_off(LEDS_GREEN); 			
 }
 else{
     printf("ERROR: received an unexpected unicast from host (%d:%d)!\n",from->u8[0],from->u8[1]);  
 }
}
static const struct unicast_callbacks unicast_base_callbacks = {recv_uc_base};


PROCESS_THREAD(sensys_base, ev, data)
{
 PROCESS_EXITHANDLER()
 PROCESS_BEGIN();
 
 // Initial configurations  on CC2420 and resetting the timer
 leds_off(LEDS_ALL);
 cc2420_set_txpower(POWER);
 cc2420_set_channel(CHANNEL_BASE);
 unicast_open(&ucb, RIME_BASE, &unicast_base_callbacks); 
  
 // Ready to receive...
 leds_on(LEDS_BLUE); 
 
 while(1) {
	PROCESS_WAIT_EVENT();		
 }        
 PROCESS_END();
}
