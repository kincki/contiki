/*
 * Copyright (c) 2012, University of Luebeck, Germany.
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
 * Description: a simple example on how to use the CC2420 radio special test 
 * modes to create an interferer in order to debug wireless sensor network 
 * protocols robustness.
 *
 * Periodical interference
 *
 */
 
#include <stdio.h>
#include <math.h>
#include <random.h>
#include "contiki.h"
#include "settings_cc2420_interferer.h"
#include "dev/cc2420.h"
#include "dev/watchdog.h"


// Variables definition
#define INTERFERED_CHANNEL 24	// Interfered channel
#define CARRIER_TYPE 1			// Carrier type: 0 = unmodulated, !0 = modulated
#define EMULATION_TYPE 0		// 1 = random, 0 = linear


// More randomness in the output power
int random_power(int min, int max)
{
 return min+abs(random_rand()%(max-min+1));
}

// Simple periodic pattern (like the one from microwave ovens)
void periodic_jammer(int period_on, int period_off)
{
 CC2420_SPI_ENABLE();
 set_jammer(CARRIER_TYPE);
 while(1){	
	// ON cycle
	#if EMULATION_TYPE	
		power_jammer(random_power(0,31));
	#else
		power_jammer(CC2420_TXPOWER_MAX);
	#endif
	clock_delay(period_on);
	// OFF cycle
	power_jammer(CC2420_TXPOWER_MIN);
	clock_delay(period_off);
 }
 CC2420_SPI_DISABLE();
}


// Main Process
PROCESS(interferer_example, "Simple jammer");
AUTOSTART_PROCESSES(&interferer_example);
PROCESS_THREAD(interferer_example, ev, data)
{
 PROCESS_EXITHANDLER()  
 PROCESS_BEGIN();

 // Initial configurations on CC2420: channel and tx power
 watchdog_stop();
 cc2420_set_txpower(CC2420_TXPOWER_MAX);
 cc2420_set_channel(INTERFERED_CHANNEL);
 printf("HandyMote: interfering with periodic interference\n"); 
 
 // Interfering with a continuous pattern 
 periodic_jammer(9943, 10246); 

 PROCESS_WAIT_EVENT();  
 PROCESS_END();
}
