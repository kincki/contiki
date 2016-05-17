/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"

#include "simple-udp.h"

#include <stdio.h>

#include "sys/node-id.h"

// Headers for Erbium REST Engine
#include "contiki-net.h"
#include <stdbool.h>

#if RANDOM_TOKEN_ERROR
#include "lib/random.h"
#include "sys/clock.h"

#define RANDOM_ERROR 12
#endif //  RANDOM_TOKEN_ERROR

#define UDP_PORT 1234

#define SEND_INTERVAL		(20 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

#define NUM_MOTES 5

#if PLATFORM_HAS_BUTTON
#include "dev/button-sensor.h"
#endif

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

/*
 * Token Passing Communication Paradigm:
 * Any mote that has something to say must possess the communication token.
 * Otherwise, it cannot transmit any data.
 * Token is initialized from mote-1, and transferred to other motes.
 * If a mote sends a message out of token order, this is a system failure
 */
static uint8_t token;
//test is initiall off
static uint8_t test_started = 0;

/*
 * IPv6 udp connection variable for broadcast connection.
 * All messages sent over this connection will be relayed to all neighbors in the RPL neighborhood.
 */
static struct simple_udp_connection broadcast_connection;

/*--------------------------CONTIKI PROCESSES------------------------------------------*/

/*
 * Broadcast Example Process is responsible for implementing simple broadcast
 * over IPv6 connection. 
 */
PROCESS(broadcast_example_process, "UDP broadcast example process");

/*
 * This MACRO initiates all the Contiki Processes defined in this file.
 * You can autostart >= 1 Contiki Process(es) in an AUTOSTART call
 */
AUTOSTART_PROCESSES(&broadcast_example_process);


/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("Data (%d) received on port %d from port %d with length %d\n", 
	 *data, receiver_port, sender_port, datalen);

  uint8_t mote_id = *data; 

  if ( 6 == mote_id ) { // HARDCODED BROADCAST LISTENER MOTE-ID:12
    test_started ^= 1;
    printf("TEst_STarted State: %d\n", test_started);
  } else if (test_started) {
#if RANDOM_TOKEN_ERROR
    if (0 == random_rand() % RANDOM_ERROR) {
      // this is written just to introduce random error to the algorithm
      printf("RANDOM Error Occurred\n");
      token = 1;
    }
#endif

    if ( (node_id - ((*data) % NUM_MOTES)) == 1)
    token = 1;
  }
}

/*--------------------------------PROCESS BROADCAST----------------------------------*/
PROCESS_THREAD(broadcast_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t addr;

  static uint8_t initialize = 1; //this is used for mote-1 only for intialization
#if RANDOM_TOKEN_ERROR
  random_init(clock_time() % 100);
#endif

  PROCESS_BEGIN();

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
      etimer_set(&send_timer, SEND_TIME);

      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
      //printf("Sending broadcast\n");
      uip_create_linklocal_allnodes_mcast(&addr);

      // initialize transmission by mote-1
      if (1 == node_id && 1 == initialize) {
	token = 1;
	initialize = 0; // this statement will never execute again
      }

    if (test_started) {
      if (token) { 
	printf("I got the token, it's my turn\n");
	simple_udp_sendto(&broadcast_connection, (uint8_t *) &node_id, 4, &addr);
	token = 0;
      }
    } //if(test_started)
  } //while

  PROCESS_END();
}
