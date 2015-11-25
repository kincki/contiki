/**
 * \addtogroup uip6
 * @{
 */
/*
 * Copyright (c) 2009, Swedish Institute of Computer Science.
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
 * $Id: rimeroute.c,v 1.5 2009/11/20 14:35:58 nvt-se Exp $
 */
/**
 * \file
 *         A routing module for uip6 that uses Rime's routing.
 *
 * \author Nicolas Tsiftes <nvt@sics.se>
 */

#include <string.h>

#include "net/tcpip.h"
#include "net/uip.h"
#include "net/uip-netif.h"
#include "net/rime.h"
#include "net/sicslowpan.h"
#include "net/rime/route.h"
#include "net/rime/rime-udp.h"
#include "net/routing/uip6-route-table.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x ",lladdr->addr[0], lladdr->addr[1], lladdr->addr[2], lladdr->addr[3],lladdr->addr[4], lladdr->addr[5],lladdr->addr[6], lladdr->addr[7])
#define PRINTRIMEADDR(addr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x ",addr->u8[0], addr->u8[1], addr->u8[2], addr->u8[3],addr->u8[4], addr->u8[5],addr->u8[6], addr->u8[7])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(lladdr)
#define PRINTRIMEADDR(addr)
#endif /* DEBUG == 1*/

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif /* UIP_LOGGING == 1 */

#ifndef CUSTOM_UIP6_ROUTE
#define CUSTOM_UIP6_ROUTE 0
#endif

#define ROUTE_DISCOVERY_CHANNEL	70

#ifndef RIMEROUTE_CONF_CACHE_TIMEOUT
#define CACHE_TIMEOUT		600
#else
#define CACHE_TIMEOUT		RIMEROUTE_CONF_CACHE_TIMEOUT
#endif /* !RIMEROUTE_CONF_CACHE_TIMEOUT */

#ifndef RIMEROUTE_CONF_DISCOVERY_TIMEOUT
#define PACKET_TIMEOUT		(CLOCK_SECOND * 10)
#else
#define PACKET_TIMEOUT		(CLOCK_SECOND * RIMEROUTE_CONF_DISCOVERY_TIMEOUT)
#endif /* RIMEROUTE_CONF_DISCOVERY_TIMEOUT */

static void found_route(struct route_discovery_conn *, const rimeaddr_t *);
static void route_timed_out(struct route_discovery_conn *);

static int activate(void);
static int deactivate(void);
static uip_ipaddr_t *lookup(uip_ipaddr_t *, uip_ipaddr_t *);

const struct uip_router rimeroute = { activate, deactivate, lookup };

static const struct route_discovery_callbacks route_discovery_callbacks =
  { found_route, route_timed_out };

static process_event_t rimeroute_event;

static uip_ipaddr_t netaddr;
static u8_t prefixlength = 0;
static rimeaddr_t gateway;

PROCESS(rimeroute_process, "UIP6 rime router");

PROCESS_THREAD(rimeroute_process, ev, data)
{
  static struct route_discovery_conn route_discovery_conn;
  rimeaddr_t *dest;

  PROCESS_BEGIN();
  
  uip_ip6addr(&netaddr, 0xfe80, 0, 0, 0, 0, 0, 0, 0);

  rimeroute_event = process_alloc_event();

  rime_init(rime_udp_init(NULL));
  route_init();
  /* Cache routes for 10 minutes (default) */
  route_set_lifetime(CACHE_TIMEOUT);

  route_discovery_open(&route_discovery_conn,
                       PACKET_TIMEOUT,
                       ROUTE_DISCOVERY_CHANNEL,
                       &route_discovery_callbacks);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == rimeroute_event);
    dest = data;
    PRINTF("discovering route to %d.%d\n", dest->u8[RIMEADDR_SIZE-2], dest->u8[RIMEADDR_SIZE-1]);
    route_discovery_discover(&route_discovery_conn, dest, PACKET_TIMEOUT);
  }

  route_discovery_close(&route_discovery_conn);

  PROCESS_END();
}

static void
found_route(struct route_discovery_conn *rdc, const rimeaddr_t *dest)
{
}

static void
route_timed_out(struct route_discovery_conn *rdc)
{
}

/*---------------------------------------------------------------------------*/
static int
activate(void)
{
  PRINTF("Rimeroute started\n");

  process_start(&rimeroute_process, NULL);

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
deactivate(void)
{
  PRINTF("Rimeroute stopped\n");

  return 0;
}
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t *
lookup(uip_ipaddr_t *destipaddr, uip_ipaddr_t *nexthop)
{
  static rimeaddr_t receiver;
  struct route_entry *route;
 
#if CUSTOM_UIP6_ROUTE  
  struct uip6_route_entry *uip6_route;

  /* We first look for a custom route in the ipv6 route table. */
  uip6_route = uip6_route_table_lookup(destipaddr);
#endif /* CUSTOM_UIP6_ROUTE */ 
  
  
  /* Packets destined to this network is sent towards the destination, whereas
     packets destined to a network outside this network is sent towards
     the gateway node. */

  if(uip_ipaddr_prefixcmp(destipaddr, &netaddr, prefixlength)) {
    
    /* Destination is in our network. */
    PRINTF("rimeroute: destination is in our network\n");
    
#if CUSTOM_UIP6_ROUTE
    
    if(uip6_route!=NULL && uip6_route->prefixlength >= prefixlength){
      
      /* There is a more specific route towards the node in the custom uip6
      route table. */
      
      uip_ipaddr_copy(nexthop,&uip6_route->nexthop);
      
      PRINTF("rimeroute: custom route found, ");
      PRINT6ADDR(destipaddr);
      PRINTF(" can be reached via ");
      PRINT6ADDR(nexthop);
      PRINTF("\n");
      
      return nexthop; 
    }
    
#endif /* CUSTOM_UIP6_ROUTE */
    
    rimeaddr_copy(&receiver,(rimeaddr_t *)(&destipaddr->u8[8]));
    receiver.u8[0] = 0;
    
    
    PRINTF("rimeroute: looking up ");
    PRINT6ADDR(destipaddr);
    PRINTF(" with Rime address ");
    PRINTRIMEADDR((&receiver));
    PRINTF("\n");
    
  } else {
    
#if CUSTOM_UIP6_ROUTE
    
    if(uip6_route!=NULL){
      
      uip_ipaddr_copy(nexthop,&uip6_route->nexthop);
      
      PRINTF("rimeroute: custom route found, ");
      PRINT6ADDR(destipaddr);
      PRINTF(" can be reached via ");
      PRINT6ADDR(nexthop);
      PRINTF("\n");
      
      return nexthop; 
    }
#endif /* CUSTOM_UIP6_ROUTE */
    
    if(rimeaddr_cmp(&gateway, &rimeaddr_node_addr)) {
      PRINTF("rimeroute: I am gateway\n");
      return NULL;
    } else if(rimeaddr_cmp(&gateway, &rimeaddr_null)) {
      PRINTF("rimeroute: No gateway setup\n");
      return NULL;
      
    } else {
      PRINTF("rimeroute: looking for next hop to ");
      PRINT6ADDR(destipaddr);
      PRINTF(" towards gateway ");
      PRINTRIMEADDR((&gateway));
      PRINTF("\n");
      rimeaddr_copy(&receiver, &gateway);
    }
  }
  

  route = route_lookup(&receiver);
  if(route == NULL) {
    process_post(&rimeroute_process, rimeroute_event, &receiver);
    return NULL;
  }

  uip_ip6addr(nexthop, 0xfe80, 0, 0, 0, 0, 0, 0, 0);
  uip_netif_addr_autoconf_set(nexthop, (uip_lladdr_t *)&route->nexthop);
  PRINTF("rimeroute: ");
  PRINT6ADDR(destipaddr);
  PRINTF(" can be reached via ");
  PRINT6ADDR(nexthop);
  PRINTF("\n");

  return nexthop;
}
/*---------------------------------------------------------------------------*/
void
rimeroute_set_net(uip_ipaddr_t *addr, u8_t prefixlen)
{
  uip_ipaddr_copy(&netaddr, addr);
  prefixlength = prefixlen;
}
/*---------------------------------------------------------------------------*/
void
rimeroute_set_gateway(rimeaddr_t *gw)
{
  rimeaddr_copy(&gateway, gw);
}
/*---------------------------------------------------------------------------*/
