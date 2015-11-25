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
 * Description: a simple program that checks the ongoing noise in a given channel
 *
 */


#include <stdio.h>
#include "contiki.h"
#include "interferer_settings.h"
#include "dev/cc2420.h"
#include "dev/watchdog.h"

// VARIABLES DEFINITION
#define INTERFERED_CHANNEL 24			 // Channel to monitor
#define CHECK_TIME (CLOCK_SECOND / 32)  // Time in seconds

// Main Process
PROCESS(noise_monitor, "Simple monitor");
AUTOSTART_PROCESSES(&noise_monitor);
PROCESS_THREAD(noise_monitor, ev, data)
{
 PROCESS_EXITHANDLER()  
 PROCESS_BEGIN();
 
 // Stop the watchdog
 watchdog_stop();

 // Initial configurations on CC2420: channel and tx power
 cc2420_set_channel(INTERFERED_CHANNEL);
  
 static struct etimer et;  
 while(1) {		
	 etimer_set(&et, CHECK_TIME);
	 PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	 int cn = cc2420_rssi() - 45;
	 printf("%d\n",cn);
 }     
 
 PROCESS_WAIT_EVENT(); 
 PROCESS_END();
}
