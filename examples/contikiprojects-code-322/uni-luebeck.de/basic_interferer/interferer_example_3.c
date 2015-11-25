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
 * Continuous interference with predefined power value
 *
 */
 
#include <stdio.h>
#include "contiki.h"
#include "settings_cc2420_interferer.h"
#include "dev/cc2420.h"
#include "dev/watchdog.h"


// Variables definition
#define INTERFERED_CHANNEL 24	// Interfered channel
#define CARRIER_TYPE 1 		 	// Carrier type: 0 = unmodulated, !0 = modulated
#define POWER_LEVEL 31		  // CC2420_TXPOWER_MAX


// Main Process
PROCESS(interferer_example, "Simple jammer");
AUTOSTART_PROCESSES(&interferer_example);
PROCESS_THREAD(interferer_example, ev, data)
{
 PROCESS_EXITHANDLER()  
 PROCESS_BEGIN();

 // Initial configurations on CC2420: channel and tx power
 cc2420_set_txpower(POWER_LEVEL);
 cc2420_set_channel(INTERFERED_CHANNEL);
 
 // Stop the watchdog
 watchdog_stop();
 
 printf("HandyMote: interfering on channel %d at transmission power %d with a carrier type %d\n", INTERFERED_CHANNEL, POWER_LEVEL, CARRIER_TYPE);
 
 // Interfering continuously with fixed power
 CC2420_SPI_ENABLE();
 set_jammer(CARRIER_TYPE);
 power_jammer(POWER_LEVEL);
 CC2420_SPI_DISABLE();

 PROCESS_WAIT_EVENT();  
 PROCESS_END();
}

