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
 * Tunable interferer
 * Initial status: the node is idle (Blue LED);
 * Single click: Node starts interfering with the MIN TX power (0) ;
 * Every additional single click: increases/decreases TX power by 1 (it loops when reaching MIN or MAX power) ;
 * Every double click (it has to be within 0,5 seconds): changes the direction of the transmission power (if it was increasing, it will now decrease every single click and viceversa). Increase = Red LED, decrease = Green LED;
 * Reset button: the node is rebooted and gets back to idle (Blue LED).
 * BEWARE: if you click like crazy, it will activate the double click, so if you want to make two consecutive single clicks, wait at least 0,5-1 second to avoid problems
*/

#include "contiki.h"
#include <stdio.h>
#include "net/rime.h"
#include "dev/leds.h"
#include "dev/cc2420.h"
#include "dev/cc2420.c"
#include "dev/cc2420_const.h"
#include "dev/button-sensor.h"		

#define MIN_POWER 0
#define MAX_POWER 31

#define CHANNEL_BASE 22
#define CHANNEL_SENDER 24

#define CLICK_TIME (CLOCK_SECOND/2) // Time within making a double click
#define INTERFERENCE_TYPE 1 // 0 = Unmodulated, 1/else = Modulated

static struct ctimer timer_click;
static void ctimer_click_callback(void *ptr);

int tx_pow = MIN_POWER;
int direction = 0; // 0 = positive, 1 = negative
int already_interfering = 0; // 0 = no, 1 = yes
int click_count = 0;


PROCESS(sensys_jammer, "SenSys jammer (Interferer) ...");
AUTOSTART_PROCESSES(&sensys_jammer);

// Normal mode
static void interference_reset()
{
 setreg(CC2420_MDMCTRL1, 0x0500); 
 setreg(CC2420_MANOR, 0x0000);
 setreg(CC2420_TOPTST, 0x0010);
 setreg(CC2420_DACTST, 0x0000);
 strobe(CC2420_STXON);
}

// Unmodulated carrier
static void interference_unmodulated()
{
 //An unmodulated carrier may be transmitted by setting MDMCTRL1.TX_MODE to 2, writing 0x1800 to the DACTST register and issue a STXON command strobe (PAG.54)
 interference_reset();
 // Turning on MANOR and TOPTST as said in Pag. 78 under DACTST "This feature will often require..."
 setreg(CC2420_MANOR, 0x0100);
 setreg(CC2420_TOPTST, 0x0004);
 setreg(CC2420_DACTST, 0x1800);
 setreg(CC2420_MDMCTRL1, 0x0508);
 strobe(CC2420_STXON);
 printf("Starting interfering with the unmodulated carrier!\n");
 printf("New direction = %d\n",direction);
}

//  Modulated carrier
static void interference_modulated()
{
 // The CC2420 has a built-in test pattern generator that can generate pseudo random sequence using the CRC generator. This is enabled by setting MDMCTRL1.TX_MODE to 3 and issue a STXON command strobe.  The modulated spectrum is then available on the RF pins. 
 // The low byte of the CRC word is transmitted and the CRC is updated with 0xFF for each new byte. The length of the transmitted data sequence is 65535 bits. The transmitted data-sequence is then: [synchronisation header] [0x00, 0x78, 0xb8, 0x4b, 0x99, 0xc3, 0xe9, …]
 interference_reset();
 setreg(CC2420_MDMCTRL1, 0x000C);
 strobe(CC2420_STXON);
 printf("Starting interfering with the modulated carrier!\n");
 printf("New direction = %d\n",direction);
}

// Manage click
void manage_click()
{
 if(!already_interfering){
	if(INTERFERENCE_TYPE == 0) 
		interference_unmodulated();
	else 
		interference_modulated();	
	if(direction == 0) 
		leds_on(LEDS_RED);
	else 
		leds_on(LEDS_GREEN);
	already_interfering = 1;
 }
 else{
    if(direction == 0){
		tx_pow++;
		if(tx_pow > MAX_POWER) tx_pow = MIN_POWER;
	}
	else{
		tx_pow--;
		if(tx_pow < MIN_POWER) tx_pow = MAX_POWER;
	}
	cc2420_set_txpower(tx_pow);
	printf("Transmission power set to %d\n",tx_pow);
	printf("New direction = %d\n",direction);
 } 
}
	
// Timer click management
static void ctimer_click_callback(void *ptr)
{  
  if(click_count > 1){ // Double (or more) click
	if(direction == 0){
		direction = 1;
		leds_off(LEDS_RED);
		leds_on(LEDS_GREEN);
	}
	else{
		direction = 0;
		leds_off(LEDS_GREEN);
		leds_on(LEDS_RED);
	}	
	printf("New direction = %d\n",direction);
  }
  else { // Single click	
	manage_click();
  }
  click_count = 0;  
}


PROCESS_THREAD(sensys_jammer, ev, data)
{
 PROCESS_EXITHANDLER()
 PROCESS_BEGIN();
 
 SENSORS_ACTIVATE(button_sensor);
 
 // Initial configurations  on CC2420 and resetting the timer
 cc2420_set_txpower(tx_pow);
 cc2420_set_channel(CHANNEL_SENDER);
 ctimer_stop(&timer_click);
 
 // Ready...
 leds_on(LEDS_BLUE);
 printf("Ready... channel %d\n",CHANNEL_SENDER);
 while(1) {
	PROCESS_WAIT_EVENT();
	if(ev == sensors_event && data == &button_sensor){
		if(click_count == 0){
			ctimer_set(&timer_click, CLICK_TIME, ctimer_click_callback, NULL);		   
		}
		click_count++;		 
	} 
 }      
 PROCESS_END();
}
