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

#define PACKETS_PER_SECOND 32
#define PACKETS_TIME (CLOCK_SECOND/PACKETS_PER_SECOND)
#define UPDATES_PER_SECOND 2
#define UPDATE_TIME (CLOCK_SECOND/UPDATES_PER_SECOND)

struct message{
	long seqno;	
};

struct message_base{
	long seqno;
	int noise;
	int rssi;
	int lqi;
	int snr;
	int prr;
};

// Timer
static struct ctimer timer1;
static void ctimer1_callback(void *ptr);

// Unicast connections
static struct unicast_conn uc;
static struct unicast_conn ucb;

// Arrays to store the info
int noise[PACKETS_PER_SECOND/UPDATES_PER_SECOND] = {0};
int rssi[PACKETS_PER_SECOND/UPDATES_PER_SECOND] = {0};
int lqi[PACKETS_PER_SECOND/UPDATES_PER_SECOND] = {0};
int snr[PACKETS_PER_SECOND/UPDATES_PER_SECOND] = {0};

int received = 0;
long count = 0;

PROCESS(sensys_rx, "SenSys receiver (RX)...");
AUTOSTART_PROCESSES(&sensys_rx);

// Handlers for unicast sender
static void recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{ 
 if((from->u8[0] == SENDER) && (from->u8[1] == 0)){	 
	 // Receive message
	 leds_on(LEDS_GREEN);
	 struct message mesg;
	 memcpy(&mesg, packetbuf_dataptr(), sizeof(mesg));
	 rssi[mesg.seqno] = (cc2420_last_rssi - 45);	 
	 noise[mesg.seqno] = (cc2420_rssi() - 45);
	 lqi[mesg.seqno] = cc2420_last_correlation;
	 snr[mesg.seqno] = rssi[mesg.seqno] - noise[mesg.seqno];	 
	 if(snr[mesg.seqno]<0) snr[mesg.seqno]=0;
	 received++;
	 printf("R: %d, N: %d, L: %d, S: %d, seqno:%ld, received=%d\n",rssi[mesg.seqno],noise[mesg.seqno],lqi[mesg.seqno],snr[mesg.seqno],mesg.seqno,received);
	 leds_off(LEDS_GREEN); 	 
 }
 else{
     printf("ERROR: received an unexpected unicast from host (%d:%d)!\n",from->u8[0],from->u8[1]);  
 }
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};

// Empty the arrays
void empty_arrays()
{
 int i=0;
 for(i=0;i<(PACKETS_PER_SECOND/UPDATES_PER_SECOND);i++){
	noise[i] = 0;
	rssi[i] = 0;
	lqi[i] = 0;
	snr[i] = 0;
 }
}

// Handlers for unicast base
static void recv_uc_base(struct unicast_conn *c, const rimeaddr_t *from)
{ 
 printf("ERROR: received an unexpected unicast from host (%d:%d)!\n",from->u8[0],from->u8[1]);  
}
static const struct unicast_callbacks unicast_base_callbacks = {recv_uc_base};

// Timer timeout callback
static void ctimer1_callback(void *ptr)
{ 	
 // Computing the average from the arrays
 long n_sum=0,r_sum=0,l_sum=0,s_sum=0;
 int i=0,n_avg=0,r_avg=0,l_avg=0,s_avg=0;
 for(i=0;i<(PACKETS_PER_SECOND/UPDATES_PER_SECOND);i++){
	n_sum += noise[i];
	r_sum += rssi[i];
	l_sum += lqi[i];
	s_sum += snr[i];		
 }
 printf("SUM: n:%ld, r:%ld, l:%ld, s:%ld, received=%d\n", n_sum,r_sum,l_sum,s_sum,received);
 n_avg= (int) (n_sum/received);
 r_avg= (int) (r_sum/received);
 l_avg= (int) (l_sum/received);
 s_avg= (int) (s_sum/received);
 //printf("AVG: n:%d, r:%d, l:%d, s:%d\n", n_avg,r_avg,l_avg,s_avg);
 // Send message to the base
 leds_on(LEDS_RED);
 struct message_base msg;
 memcpy(&msg, packetbuf_dataptr(), sizeof(msg));	
 msg.seqno = count++;
 msg.noise = n_avg;
 msg.rssi = r_avg;
 msg.lqi = l_avg;
 msg.snr = s_avg;	
 int prr = (received*100)/(PACKETS_PER_SECOND/UPDATES_PER_SECOND); 
 //if(prr>90) prr = 100; // Artefact
 msg.prr = prr;
 packetbuf_clear();
 packetbuf_copyfrom(&msg, sizeof(struct message_base));
 rimeaddr_t addr;
 addr.u8[0] = BASE;
 addr.u8[1] = 0;
 cc2420_set_channel(CHANNEL_BASE);
 unicast_send(&ucb, &addr);	
 cc2420_set_channel(CHANNEL_SENDER);
 printf("Sending message %ld to %d.0 with S=%d,P=%d [recv=%d]\n",count-1,BASE,msg.snr,msg.prr,received);
 // Refreshing variables and arrays
 received = 0;
 empty_arrays();
 // Wait some time
 //clock_delay(353*10);
 // Send message to the sender to trigger the new sending
 struct message mesg;
 mesg.seqno = PACKETS_PER_SECOND/UPDATES_PER_SECOND;
 packetbuf_clear();
 packetbuf_copyfrom(&mesg, sizeof(struct message));
 rimeaddr_t addr2;
 addr2.u8[0] = SENDER;
 addr2.u8[1] = 0;
 unicast_send(&uc, &addr2);	
 //printf("Sending message to the sender with seqno %ld\n",mesg->seqno);
 ctimer_set(&timer1, UPDATE_TIME+4*PACKETS_TIME, ctimer1_callback, NULL);
}


PROCESS_THREAD(sensys_rx, ev, data)
{
 PROCESS_EXITHANDLER()
 PROCESS_BEGIN();
 
 // Initial configurations  on CC2420 and resetting the timer
 leds_off(LEDS_ALL);
 cc2420_set_txpower(POWER);
 cc2420_set_channel(CHANNEL_SENDER);
 unicast_open(&uc, RIME_SENDER, &unicast_callbacks); 
 unicast_open(&ucb, RIME_BASE, &unicast_base_callbacks); 
 ctimer_stop(&timer1);
  
 // Emptying the arrays
 empty_arrays();
 
 // Ready to receive...
 leds_on(LEDS_BLUE); 
 ctimer_set(&timer1, UPDATE_TIME, ctimer1_callback, NULL);
 
 while(1) {
	PROCESS_WAIT_EVENT();		
 }        
 PROCESS_END();
}
