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
#include "rest-engine.h"
#include <stdbool.h>

#include "lib/random.h"
#include "sys/clock.h"

#include "tokenring.h"

#if RANDOM_TOKEN_ERROR

#define RANDOM_ERROR 14
#endif //RANDOM_TOKEN_ERROR

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
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern resource_t res_event;

/*
 * The data structure for token event representation
 */
token_data_t *new_token;
/* initially there is no last token, so when mote-1 sends a message, 
 * it should behave normally
 */
uint8_t lastToken = NUM_MOTES;

static int numOfViolations = 0;
static int numOfEvents = 0;

//test is initially off
uint8_t test_started = 0; 

/*
 * IPv6 udp connection variable for broadcast connection.
 * All messages sent over this connection will be relayed to all neighbors in the RPL neighborhood.
 */
static struct simple_udp_connection broadcast_connection;


/*
 * Control execution of token ring test
 */
const void toggle_test()
{
  uip_ipaddr_t addr;

  //toggle test_started 
  test_started ^= 1;
 
  printf("LISTENER>> Toggle Test: %d\n", test_started);

  //broadcast test execution state (1: Started, 0: Stopped)
  uip_create_linklocal_allnodes_mcast(&addr);
  simple_udp_sendto(&broadcast_connection, (uint8_t *) &node_id, 4, &addr);
}

/*
 * Determine if this token  is out of  order
 */
uint8_t isInOrder(uint8_t token)
{
  uint8_t result = 0;

  if (token != 1) {
    if ( (token - lastToken) != 1 ) {
      printf("Out of order token! (Last: %d, New: %d)\n", lastToken, token);
      result = 0;
    } else {
      printf("In order token! (Last: %d, New: %d)\n", lastToken, token);
      result = 1;
    } 
  } else {
    if ( lastToken == NUM_MOTES ) {
      printf("In order token! (Last: %d, New: %d)\n", lastToken, token);
      result = 1;
    } else {
      printf("Out of order token! (Last: %d, New: %d)\n", lastToken, token);
      result = 0;
    }
  }
  return result;
}
/*--------------------------CONTIKI PROCESSES------------------------------------------*/
/*
 * CoAP Server process is responsible for implementing CoAP services.
 * It is initially designed to suppor a single resource, named "res_event".
 */ 
PROCESS(coap_server, "Erbium CoAP Server");

/*
 * Broadcast Example Process is responsible for implementing simple broadcast
 * over IPv6 connection. 
 */
PROCESS(broadcast_example_process, "UDP broadcast process");

/*
 * This MACRO initiates all the Contiki Processes defined in this file.
 * You can autostart >= 1 Contiki Process(es) in an AUTOSTART call
 */
AUTOSTART_PROCESSES(&broadcast_example_process, &coap_server);


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

  //prepare a new token
  new_token->source_mote_id = *data;
  new_token->timeStamp = clock_time();

  if (!isInOrder(new_token->source_mote_id))
    numOfViolations++;

  printf("Token Ring Protocol #Events: %d, #Violations: %d\n", ++numOfEvents, numOfViolations);
  
  //update last token
  lastToken = new_token->source_mote_id;

  res_event.trigger();  
}
/*------------------------------------------------------------------*/

const token_data_t *get_tokenring_data()
{
  return new_token;
}

/*--------------------PROCESS BROADCAST-----------------------------*/
PROCESS_THREAD(broadcast_example_process, ev, data)
{
  PROCESS_BEGIN();

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  PROCESS_END();
}

/*---------------------------PROCESS COAP SERVER--------------------------------------*/
PROCESS_THREAD(coap_server, ev, data)
{

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  PRINTF("Starting Erbium CoAP Server\n");

#ifdef RF_CHANNEL
  PRINTF("RF channel: %u\n", RF_CHANNEL);
#endif
#ifdef IEEE802154_PANID
  PRINTF("PAN ID: 0x%04X\n", IEEE802154_PANID);
#endif

  PRINTF("uIP buffer: %u\n", UIP_BUFSIZE);
  PRINTF("LL header: %u\n", UIP_LLH_LEN);
  PRINTF("IP+UDP header: %u\n", UIP_IPUDPH_LEN);
  PRINTF("REST max chunk: %u\n", REST_MAX_CHUNK_SIZE);

  /* Initialize the REST engine. */
  rest_init_engine();

  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */

  rest_activate_resource(&res_event, "test/event");
#if PLATFORM_HAS_BUTTON
  SENSORS_ACTIVATE(button_sensor);
#endif

  /* Define application-specific events here. */
  while(1) {
    PROCESS_WAIT_EVENT();
#if PLATFORM_HAS_BUTTON
    if(ev == sensors_event && data == &button_sensor) {
      PRINTF("*******BUTTON*******\n");

      /* Call the event_handler for this application-specific event. */
      //      res_event.trigger();
    }
#endif /* PLATFORM_HAS_BUTTON */

  } /* while (1) */

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
