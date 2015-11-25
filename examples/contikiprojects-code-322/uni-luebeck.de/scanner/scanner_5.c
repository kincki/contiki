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
 * Histogram of noise in all channels
 *
 */
 
#include "contiki.h"
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include "dev/leds.h"
#include "dev/cc2420.h"
#include "dev/watchdog.h"
#include "interferer_settings.h"

#define MIN_CHANNEL 11				// Min channel
#define MAX_CHANNEL 26				// Max channel
#define THRESHOLD_RSSI -32		 	// Threshold for defining an instant as interfered or not (-15 = -60 dBm, just subtract -45)
#define SAMPLE_SIZE 16384			// Amount of scans

// Global variables
static unsigned long sum_state[MAX_CHANNEL-MIN_CHANNEL+1][2];  // Sum of the channel states
static float CV[MAX_CHANNEL-MIN_CHANNEL+1] = {0};	  // Channel vacancy

/*---------------------------------------------------------------------------*/
// CPU Boosting 
uint16_t cpu1, cpu2;
void boost_cpu() // Temporarily boost CPU speed
{
 cpu1 = DCOCTL;
 cpu2 = BCSCTL1;
 DCOCTL = 0xff;
 BCSCTL1 |= 0x07;
}	
void restore_cpu() // Restore CPU speed
{
 DCOCTL = cpu1;
 BCSCTL1 = cpu2;
}
/*---------------------------------------------------------------------------*/


// Check and scan all the channels
// Fills in the array CV with the channel vacancy
void check_channels(){
 int channel = 0;	
 for(channel=MIN_CHANNEL; channel<=MAX_CHANNEL; channel++){ 	
	// Reset arrays
	sum_state[channel-MIN_CHANNEL][0] = 0;
	sum_state[channel-MIN_CHANNEL][1] = 0; 
	
	 // Selecting the channel	
	 SPI_SETCHANNEL_SUPERFAST(357+((channel-MIN_CHANNEL)*5));
		 
	 // Avoiding the initial wrong readings by discarding the wrong readings
	 CC2420_SPI_ENABLE();
	 unsigned temp;
	 unsigned long k=0; for (k=0; k<=16; k++) {MY_FASTSPI_GETRSSI(temp);}
	 CC2420_SPI_DISABLE();
		 
	 dint();				// Disable interrupt
	 boost_cpu(); 			// Temporarily boost CPU speed 
	 CC2420_SPI_ENABLE(); 	// Enable SPI 
		  
	 // Sampling (main logic of the program)
	 int previous = 0, current = 0;
	 long cnt = 0;
	 signed char rssival = 0;		 
		 
	 for(k=0;k<SAMPLE_SIZE;k++){
	 
		// Sampling RSSI
		MY_FASTSPI_GETRSSI(rssival);			
		
		// Checking if the medium is busy or not (RSSI threshold)
		if(rssival < THRESHOLD_RSSI) current = 0; 
		else current = 1;
		
		// Check if there was a transition or if the state is not changed
		if(current == previous) { // The state is not changed
			cnt++;
		}
		else {	
			// The state is changed, there was a transition
			if(cnt != 0){ 
				sum_state[channel-MIN_CHANNEL][previous] += cnt;
			}
			cnt = 1;
			previous = current;
		}		
 	 }
	 sum_state[channel-MIN_CHANNEL][previous] += cnt;
	 
	 CV[channel-MIN_CHANNEL] = ((float) sum_state[channel-MIN_CHANNEL][0]*1.000) / (((float)(sum_state[channel-MIN_CHANNEL][0]+sum_state[channel-MIN_CHANNEL][1])*1.000));
	
	 CC2420_SPI_DISABLE();	// Disable SPI   
	 restore_cpu(); 		// Restore CPU speed
	 eint(); 				// Re-enable interrupt
 }
 
 // Results --- The maximum CV is the most free channel 
 printf("----\n");
 for(channel=MIN_CHANNEL; channel<=MAX_CHANNEL; channel++){
	printf("Channel %d: 0 = %lu, 1 = %lu, CV = %ld.%03u\n", channel, sum_state[channel-MIN_CHANNEL][0], sum_state[channel-MIN_CHANNEL][1], (long)CV[channel-MIN_CHANNEL], (unsigned)((CV[channel-MIN_CHANNEL]-floor(CV[channel-MIN_CHANNEL]))*1000));
 }
 printf("----\n");
 
 return;
}

PROCESS(scanning, "Scanning @ histogram");
AUTOSTART_PROCESSES(&scanning);
PROCESS_THREAD(scanning, ev, data)
{
 PROCESS_BEGIN(); 
 
 // Stop the watchdog
 watchdog_stop(); 
 //printf("#START\n");

 // Set peak detectors for the AGC loop (so that the amplifier chain is linear in arbitrary noise)
 unsigned temp;
 CC2420_READ_REG(CC2420_AGCTST1, temp);
 CC2420_WRITE_REG(CC2420_AGCTST1, (temp + (1 << 8) + (1 << 13)));  

 // Scan the channels: it performs CCA and counts how much each channel is busy (1) or idle (0) and puts this information in the array sum_state. 
 //	The channel is marked as busy or idle depending on the THRESHOLD_RSSI (currently the CCA threshold)
 // Each channel is scanned for SAMPLE_SIZE time units (one time unit is roughly 20 us). CV is the % of how much the channel was idle (0).
 // The higher CV the better the channel (it means that it was not used).
 check_channels();
 
 //printf("#END\n");
 PROCESS_WAIT_EVENT();  
 PROCESS_END();
}
