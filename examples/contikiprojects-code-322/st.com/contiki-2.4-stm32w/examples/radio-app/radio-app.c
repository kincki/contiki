/*
 * Copyright (c) 2010, STMicroelectronics.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         A simple Contiki application showing how to use the STM32W radio
 *         driver.
 *         Another process is also present, but it is not required to use the
 *         radio.
 * 
 * \author
 *         Salvatore Pitrulli <salvopitru@users.sourceforge.net>
 */

#include "contiki.h"

#include <stdio.h> /* For printf() */

#include "dev/stm32w-radio.h"
#include "dev/serial-line.h"
#include "net/mac/frame802154.h"


//generic data packet

int8u __attribute__ (( aligned(2) )) txPacket[] = {
  0x61, // fcf - intra pan, ack request, data
  0x08, // fcf - src, dst mode
  0x00, // seq
  (char)IEEE802154_PANID, // dst pan l
  (char)(IEEE802154_PANID>>8), // dst pan h
  (char)STM32W_NODE_ID, // dst addr l
  (char)(STM32W_NODE_ID>>8), // dst addr h
  0x12, // data...
  0x34,
  0x56,
  0x78,
  0x9A,
  0xBC
};

int8u rxPacket[128];

PROCESS(ticktack_process, "Tick Tack process");
PROCESS(rxtx_process, "RX TX process");

/* Uncomment this line and comment the following one if you don't want ticktack
  process to start. */
// AUTOSTART_PROCESSES(&rxtx_process);

AUTOSTART_PROCESSES(&ticktack_process,&rxtx_process);

PROCESS_THREAD(ticktack_process, ev, data)
{
  static struct etimer etimer;
  
  PROCESS_BEGIN();
  
  printf("ticktack_process starting\r\n");
  
  while(1) {
    etimer_set(&etimer, CLOCK_SECOND);
    PROCESS_WAIT_UNTIL(etimer_expired(&etimer));
    printf("Tick ");
    etimer_set(&etimer, CLOCK_SECOND);
    PROCESS_WAIT_UNTIL(etimer_expired(&etimer));
    printf("Tack ");
  }
  
  PROCESS_END();
}

static void input_packet(const struct radio_driver *d)
{
  int8u len, i;
  
  len = d->read(rxPacket,128);
  printf("\r\nRX");
  for(i=0; i<len; i++) {
    printf(" %x", rxPacket[i]);
  }
  printf("\r\n");
}

static void init_mac()
{
  stm32w_radio_driver.set_receive_function(input_packet);
  stm32w_radio_driver.on();
}

PROCESS_THREAD(rxtx_process, ev, data)
{
  
  PROCESS_BEGIN();
  
  printf("rxtx_process starting\r\n");
  
  init_mac();
  
  while(1) {
    
    // transmit a packet on every line feed.
    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message && data != NULL);
      
    printf("T\r\n");
    txPacket[2]++; //increment sequence number
    stm32w_radio_driver.send(txPacket,13);
  }
  
  PROCESS_END();
}



