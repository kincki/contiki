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
 *         A simple routing module for uip6 that uses the uip6 route table only,
 *         with no discovery.
 *
 * \author Salvatore Pitrulli <salvopitru@users.sourceforge.net>
 */

#include "net/uip.h"
#include "uip6-route-table.h"

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

static int activate(void);
static int deactivate(void);
static uip_ipaddr_t *lookup(uip_ipaddr_t *, uip_ipaddr_t *);

const struct uip_router uip6_router = { activate, deactivate, lookup };


/************************************************************************/

static int
activate(void)
{
  PRINTF("uip6-route started\n");
  return 0;
}

static int
deactivate(void)
{
  PRINTF("uip6-route stopped\n");
  return 0;
}

static uip_ipaddr_t *
lookup(uip_ipaddr_t *destipaddr, uip_ipaddr_t *nexthop)
{
  struct uip6_route_entry *route;
  
  PRINTF("uip6-route: looking up ");
  PRINT6ADDR(destipaddr);
  PRINTF("\n");

  route = uip6_route_table_lookup(destipaddr);
  if(route == NULL) {
    return NULL;
  }
  
  uip_ipaddr_copy(nexthop,&route->nexthop);
  
  PRINTF("uip6-route: ");
  PRINT6ADDR(destipaddr);
  PRINTF(" can be reached via ");
  PRINT6ADDR(nexthop);
  PRINTF("\n");  
  
  return nexthop;  
  
}
