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
 * Histogram of noise in all channels [both 0 and 1]
 *
 */

#include "contiki.h"
#include <stdio.h>
#include <signal.h>
#include "interferer_settings.h"
#include "dev/leds.h"
#include "dev/cc2420.h"
#include "dev/watchdog.h"

/*---------------------------------------------------------------------------*/
#define BUFFER_SIZE 1024	 	// Size of the buffer 
#define THRESHOLD_RSSI -32		// Threshold for marking an instant as interfered or not
#define PRINT_ZEROS 1			// Define if printing zeros (1) or ones states (0)
#define WITH_CPU_BOOST 1		// CPU boosting
#define LIMITED_PERIOD 100000	// Maximum sampling period per channel

static int count_state[2] = {0}; 		// Counting the number of '0s' and '1s' states
static long sum_state[2] = {0};			// Counting the total amount of time units were spent in '0s' or '1s' states
static long array_time[2][BUFFER_SIZE];	// Array containing the amount of time slots spent in each '0' or '1' state

/*---------------------------------------------------------------------------*/
#if WITH_CPU_BOOST
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
#endif
/*---------------------------------------------------------------------------*/


PROCESS(scanning, "Scanning @ histogram");
AUTOSTART_PROCESSES(&scanning);
PROCESS_THREAD(scanning, ev, data)
{
 PROCESS_BEGIN();
 watchdog_stop(); 		
 
 // Avoiding wrong RSSI readings
 unsigned temp;
 CC2420_READ_REG(CC2420_AGCTST1, temp);
 CC2420_WRITE_REG(CC2420_AGCTST1, (temp + (1 << 8) + (1 << 13))); 

 // Repeating this for all channels 11-26 forever
 int channel = 0;
 while(1){ 
	 for(channel = 11;channel<=26;channel++){ 
		 leds_off(LEDS_ALL);
		 leds_on(LEDS_RED);
		 // Selecting the channel fastly	
		 SPI_SETCHANNEL_SUPERFAST(357+((channel-11)*5)); //11 = 357
		 // Avoiding the initial wrong readings by discarding the wrong readings
		 CC2420_SPI_ENABLE();
		 int k=0; 
		 for (k=0; k<=16; k++) {MY_FASTSPI_GETRSSI(temp);}
		 CC2420_SPI_DISABLE();
		 
		 dint();		// Disable interrupt
#if WITH_CPU_BOOST
		 boost_cpu(); 	// Temporarily boost CPU speed
		 CC2420_SPI_ENABLE(); 	// Enable SPI 
#endif

		 // Reset arrays
		 count_state[0] = 0;
		 count_state[1] = 0;
		 sum_state[0] = 0;
		 sum_state[1] = 0;
		 for(k=0;k<BUFFER_SIZE;k++){
			array_time[0][k] = 0;
			array_time[1][k] = 0;
		 }
		 
		 // Computing statistics
		 signed char rssival = 0;
		 int previous = 0, current = 0;
		 long cnt = 0;		 
		 long max_counter = 0;
		 for(max_counter=0; ( (max_counter < LIMITED_PERIOD) && (count_state[0] < (BUFFER_SIZE-1)) && (count_state[1] < (BUFFER_SIZE-1)) ); max_counter++) {	
			MY_FASTSPI_GETRSSI(rssival);
			if(rssival < THRESHOLD_RSSI){
				current = 0;
			}
			else {
				current = 1;
			}
			// Check if the instant before was the same as the current one
			if(current == previous){
				cnt++;
			}
			else {				
				if(cnt != 0){
					count_state[previous]++;
					sum_state[previous] += cnt;					
					array_time[previous][count_state[previous]-1] = cnt;					
				}
				// Reset values
				cnt = 1;
				previous = current;
			}
		 }
		 count_state[previous]++;
		 sum_state[previous] += cnt;
		 array_time[previous][count_state[previous]-1] = cnt;
		 
		 leds_on(LEDS_GREEN);
		 
#if PRINT_ZEROS	 
		 // Bubble sort on zeros
		 int bub = 0, ble = 0;
		 for(ble=0; ble<(count_state[0]-1); ble++){
			for(bub=0; bub<(count_state[0]-1); bub++){
				if(array_time[0][bub] > array_time[0][bub+1]){
					long tempor = array_time[0][bub];
					array_time[0][bub] = array_time[0][bub+1];
					array_time[0][bub+1] = tempor;
				}
			}
		 }
#else
		 // Bubble sort on ones
		 bub = 0;
		 ble = 0;
		 for(ble=0; ble<(count_state[1]-1); ble++){
			for(bub=0; bub<(count_state[1]-1); bub++){
				if(array_time[1][bub] > array_time[1][bub+1]){
					long tempor = array_time[1][bub];
					array_time[1][bub] = array_time[1][bub+1];
					array_time[1][bub+1] = tempor;
				}
			}
		 }
#endif

		 CC2420_SPI_DISABLE();	// Disable SPI
#if WITH_CPU_BOOST		 
		 restore_cpu(); // Restore CPU speed
		 eint(); 		// Re-enable interrupt
#endif
		
		 // Printing final results - general statistics of a channel: '0' - channel number - '0' slots - '1' slots - '0' states - '1' states
		 printf("%d Channel_%d %ld %ld %d %d\n", 0, channel, sum_state[0], sum_state[1], count_state[0], count_state[1] );
		 
#if PRINT_ZEROS
		 // Array of '0s'
		 long temporary = array_time[0][0];		 
		 long count = 0;
		 for(k=0;k<count_state[0];k++){	
			if(array_time[0][k] == temporary) {
				count++;
			}
			else { 
				printf("%ld %ld\n", temporary, count);
				temporary = array_time[0][k];
				count = 1;
			}						
			clock_delay(750);
		 }
		 printf("%ld %ld\n", temporary, count);
#else		 
		 // Array of '1s'
		 temporary = array_time[1][0];		 
		 count = 0;
		 for(k=0;k<count_state[1];k++){
			if(array_time[1][k] == temporary) {
				count++;
			}
			else { 
				printf("%ld %ld ", temporary, count);
				temporary = array_time[1][k];
				count = 1;
			}						
			clock_delay(750);
		 }
		 printf("%ld %ld\n", temporary, count); 
#endif		 		 
		 printf("\n\n");		
	 }
 }
 
 PROCESS_END();
}
/*---------------------------------------------------------------------------*/
