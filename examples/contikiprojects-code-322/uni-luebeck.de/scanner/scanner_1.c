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
 * Fast scan with 1-bit precision
 *
 */

#include "contiki.h"
#include <stdio.h>
#include <signal.h>
#include "dev/leds.h"
#include "dev/cc2420.h"
#include "dev/watchdog.h"
#include "interferer_settings.h"

/*---------------------------------------------------------------------------*/
#define CHANNEL 24				// Channel to be used

// Buffers
#define BUFFER_SIZE 7168		// Size of the buffer 
static uint8_t buffer0[BUFFER_SIZE+1] = {0};		// First I store here all the elements and then the number of occurrencies in each bit of buffer1
static uint8_t buffer1[(BUFFER_SIZE/8)+1] = {0};	// Used for the compression: stores in each bit whether it is an interfered instant or not

// RLE (Run Length Encoding) parameters
#define THRESHOLD_RSSI -32		// Threshold for marking an instant as interfered or not
#define THRESHOLD_TO_STORE 0	// If I get <= THRESHOLD_TO_STORE values, I won't save them in the array

// Periodicity of the scan
#define PERIOD_TIME (CLOCK_SECOND*2)

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

PROCESS(scanning, "Scanning @ 1 bit");
AUTOSTART_PROCESSES(&scanning);
PROCESS_THREAD(scanning, ev, data)
{
 PROCESS_BEGIN();
 
 // Initial operations
 leds_off(LEDS_ALL);
 watchdog_stop(); 
 
 // Avoiding wrong RSSI readings
 unsigned temp;
 CC2420_READ_REG(CC2420_AGCTST1, temp);
 CC2420_WRITE_REG(CC2420_AGCTST1, (temp + (1 << 8) + (1 << 13))); 
 
 // Selecting the channel	
 SPI_SETCHANNEL_SUPERFAST(357+((CHANNEL-11)*5));
 
 // Avoiding the initial wrong readings by discarding the wrong readings
 CC2420_SPI_ENABLE();
 int k=0;
 for (k=0; k<=15; k++) {MY_FASTSPI_GETRSSI(temp);}
 CC2420_SPI_DISABLE(); 
 
 static struct etimer et;
 while(1){
 
 	 printf("#START\n");
	 
	 // Resetting everything
	 for(k=0;k<(BUFFER_SIZE/8);k++){	
		buffer1[k] = 0;
	 }
	 for(k=0;k<BUFFER_SIZE;k++){	
		buffer0[k] = 0;
	 }	 
	 
	 dint();				// Disable interrupts
	 boost_cpu(); 			// Temporarily boost CPU speed
	 CC2420_SPI_ENABLE(); 	// Enable SPI
	
	 // Actual scanning 
	 for(k=0; k<BUFFER_SIZE; k++){		
		MY_FASTSPI_GETRSSI(buffer0[k]); //temp = cc2420_rssi();	
	 } 

	 // Compressing the bufferized data in small arrays
	 int count = 0, previous = 0, cnt = 1, w = 0, current = 0;
	 signed char rssi = 0;
	 for(k=0; (k<(BUFFER_SIZE/8) && count < BUFFER_SIZE);k++){	
		for(w=0; (w<8 && (count < BUFFER_SIZE));){
			// Get one RSSI value and mark it as interfered instant or not
			rssi = buffer0[count++];
			if(rssi < THRESHOLD_RSSI){
				current = 0;
			}
			else {
				current = 1;
			}	
			// Check if the instant before was the same as the current one (or we have more than 1 byte)
			if((current == previous)&&(cnt<255)){
				cnt++;
			}
			else {
				if(cnt > THRESHOLD_TO_STORE){
					buffer1[k] += (previous << w);
					buffer0[k*8+w] = cnt;				
					w++;
				}
				cnt = 1;
				previous = current;
			}
		}
	 }
 
	 CC2420_SPI_DISABLE();	// Disable SPI
	 restore_cpu();			 // Restore CPU speed
	 eint(); 				// Re-enable interrupts
 
	 // Printing the stored values in compressed form 
	 uint8_t a = 0, b = 0;
	 count = 0; 
	 for(temp=0; temp<(BUFFER_SIZE/8); temp++) {
		for(k=0; (k<8 && (count < BUFFER_SIZE));k++) {			
			a = ((buffer1[temp] >> k) & 1);
			b = buffer0[temp*8+k];	
			count += b;		
			printf("%u: %d\n",a, b);
			clock_delay(30000);
		}
	 }
	 
	 printf("#END\n");
	 
	 // Waiting for timer
	 etimer_set(&et, PERIOD_TIME);
	 PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));	
 
 }
 
 PROCESS_WAIT_EVENT();
 
 PROCESS_END();
}
/*---------------------------------------------------------------------------*/
