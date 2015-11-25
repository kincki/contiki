/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *         ETX Module (.c)
 *
 * \author Mathilde Durvy <mdurvy@cisco.com> 
 * \author Marco Valente <marcvale@cisco.com>
 *
 * This module measures the expected number of transmissions necessary
 * to successfully send a packet to a neighbor. It uses echo request /
 * reply packets to do so (and so is independent of the Layer-2
 * technology used).
 */
/*
 * If you have questions about this software, please contact: 
 * contiki-developers@lists.sourceforge.net
 * Copyright (c) 2010 Cisco Systems, Inc.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include <string.h>
#include "etx.h"

#if (UIP_CONF_ETX & UIP_CONF_ICMP6)

#define DEBUG 1

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#endif

/* Pointer inside packet */
#define UIP_IP_BUF        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_ICMP_BUF    ((struct uip_icmp_hdr *)&uip_buf[uip_l2_l3_hdr_len])

/** ETX periodic timer */
static struct etimer etx_timer_periodic; 
/** The processes listening to the ETX module */
etx_listener  etx_conns[ETX_CONNS];
/** The neighboring nodes for which we are computing ETX */
etx_nbr etx_nbrs[ETX_NBRS];         

etx_listener *locetx_conn;
etx_nbr *locetx_nbr;

PROCESS(etx_process, "The ETX process");

PROCESS_THREAD(etx_process, ev, data)
{
  PROCESS_BEGIN();
  
  if(icmp6_new(NULL) == NULL) {
    PRINTF("[ETX] Critical: cannot register ICMPv6 listener. Closing\n");
  } else {
    etimer_set(&etx_timer_periodic, ETX_PERIOD*CLOCK_SECOND);
    while(1) {
      PROCESS_YIELD();
      etx_event_handler(ev, data);
    }
  }
  PROCESS_END();
}

void
etx_event_handler(process_event_t ev, process_data_t data)
{
  //PRINTF("[ETX_EVENT] %p %u\n", PROCESS_CURRENT(), ev); 

  switch(ev) {
    case PROCESS_EVENT_TIMER:
      {
        if (data == &etx_timer_periodic &&
            etimer_expired(&etx_timer_periodic)) {
    	  etx_periodic();
	  tcpip_ipv6_output();	
        }
	
        break;
      }
    case UIP_ICMP6_EVENT_PACKET_IN:
      {
        if(UIP_ICMP_BUF->type == ICMP6_ECHO_REPLY) {
          for(locetx_nbr = etx_nbrs; locetx_nbr < etx_nbrs + ETX_NBRS;
              locetx_nbr++) {
            if(locetx_nbr->isused > 0 &&
               uip_ipaddr_cmp(&UIP_IP_BUF->srcipaddr, &locetx_nbr->ipaddr) &&
               locetx_nbr->count > locetx_nbr->success){
              PRINTF("[ETX] Received packet echo reply from\n");
              PRINT6ADDR(&locetx_nbr->ipaddr);
              locetx_nbr->success++;
              PRINTF("success %d\n",locetx_nbr->success);
              break;
            }
          }
        }  
      }
      break;
    default:
      break;
  }
}

etx_listener *
etx_listener_new() {
  for(locetx_conn = etx_conns; locetx_conn < etx_conns + ETX_CONNS;
      locetx_conn++) {
    if(locetx_conn->p == PROCESS_NONE) {
      locetx_conn->p = PROCESS_CURRENT();
      PRINTF("[ETX] Registering process %p\n", locetx_conn->p); 
      return locetx_conn;
    }
  }
  return NULL;
}

void
etx_listener_rm() {
  for(locetx_conn = etx_conns; locetx_conn < etx_conns + ETX_CONNS;
      locetx_conn++) {
    if(locetx_conn->p == PROCESS_CURRENT()) {
      locetx_conn->p = PROCESS_NONE;
      break;
    }
  }
}

void etx_listeners_call(u8_t event, void *data) {
  for(locetx_conn = etx_conns; locetx_conn < etx_conns + ETX_CONNS;
      locetx_conn++) {
    if(locetx_conn->p != PROCESS_NONE) {
      process_post_synch(locetx_conn->p, event, data);
    }
  }
}

uint8_t etx_register(uip_ipaddr_t * ipaddr){
  /* Register a neighbor for ETX computation */
  etx_nbr *r = NULL;
  
  for(locetx_nbr = etx_nbrs; locetx_nbr < etx_nbrs + ETX_NBRS;
      locetx_nbr++) {
    if(locetx_nbr->isused == 0) {
      r = locetx_nbr;
    } else if (uip_ipaddr_cmp(ipaddr, &locetx_nbr->ipaddr)) {
      locetx_nbr->isused = 1;
      PRINTF("[ETX] Registering already existing IP address\n");
      PRINT6ADDR(ipaddr);
      PRINTF("\n");
      return 1;
    }
  }
  if( r == NULL ){
    return 0;
  }
  PRINTF("[ETX] Registering IP address\n");
  PRINT6ADDR(ipaddr);
  PRINTF("\n");
  r->isused = 1;
  uip_ipaddr_copy(&r->ipaddr, ipaddr);
  r->count = 0;
  r->success = 0;
  stimer_set(&r->timer, 0);
  return 1;
}

void etx_unregister(uip_ipaddr_t * ipaddr){
  /* Remove neighbor registration for ETX computation */
  for(locetx_nbr = etx_nbrs; locetx_nbr < etx_nbrs + ETX_NBRS;
      locetx_nbr++) {
    if(locetx_nbr->isused > 0 && uip_ipaddr_cmp(ipaddr, &locetx_nbr->ipaddr)){
      /* scheduling remove (avoid fluctuation)*/
      PRINTF("[ETX] Scheduling unregistering of IP address\n");
      PRINT6ADDR(&locetx_nbr->ipaddr);
      PRINTF("\n");
      locetx_nbr->isused = 2;
      break;
    }
  }
}

void etx_periodic(){
  /* Go through the registered neighbor */
  /* Remove stale neighbors */
  for(locetx_nbr = etx_nbrs; locetx_nbr < etx_nbrs + ETX_NBRS;
      locetx_nbr++) {
    if(locetx_nbr->isused == 2){
      locetx_nbr->isused = 0;
      PRINTF("[ETX] UNregistering IP address\n");
      PRINT6ADDR(&locetx_nbr->ipaddr);
      PRINTF("\n");
    }
  }
  for(locetx_nbr = etx_nbrs; locetx_nbr < etx_nbrs + ETX_NBRS;
      locetx_nbr++) {
    if(locetx_nbr->isused > 0 && stimer_expired(&locetx_nbr->timer)){
      if(locetx_nbr->count < ETX_PROBE_NB){
        PRINTF("[ETX] Periodic, Sending echo request %d\n", locetx_nbr->count);
        uip_icmp6_echo_request_output(&locetx_nbr->ipaddr, NULL, 0);
        locetx_nbr->count++;
      } else {
        PRINTF("[ETX] Periodic, new value\n");
        /* Tell the processes listening the new etx values for the
           neighbor */
        etx_listeners_call(ETX_EVENT_NEW_VALUE, (void *)locetx_nbr);
        locetx_nbr->count = 0;
        locetx_nbr->success = 0;
        stimer_set(&locetx_nbr->timer, ETX_PROBE_INTERVAL);
      }
      break;
    }
  }
  etimer_restart(&etx_timer_periodic);
}
#endif
/** @} */
