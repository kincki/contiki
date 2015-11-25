/*
 * Copyright (c) 2010, STMicroelectronics.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
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
 * This file is part of the Contiki OS
 *
 */
/*---------------------------------------------------------------------------*/
/**
* \file
*					Functions for net initialization.
* \author
*					Salvatore Pitrulli <salvopitru@users.sourceforge.net>
* \version
*					1.1
*/
/*---------------------------------------------------------------------------*/

#include "contiki-net.h"
#include "net/routing/uip6-route.h"
#include "net/routing/uip6-route-table.h"
#include "net/routing/rimeroute.h"

#if UIP_CONF_IPV6

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x ",lladdr.u8[0], lladdr.u8[1], lladdr.u8[2], lladdr.u8[3],lladdr.u8[4], lladdr.u8[5], lladdr.u8[6], lladdr.u8[7])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

static void
print_local_addresses(void)
{
  int i;
  uip_netif_state state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_CONF_NETIF_MAX_ADDRESSES; i++) {
    state = uip_netif_physical_if.addresses[i].state;           
    if(state == TENTATIVE || state == PREFERRED) {
      PRINT6ADDR(&uip_netif_physical_if.addresses[i].ipaddr);
      PRINTF("\r\n");
    }
  }
}

#if FIXED_NET_ADDRESS
#include "contiki-net.h"

void set_net_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, NET_ADDR_A, NET_ADDR_B, NET_ADDR_C, NET_ADDR_D, 0, 0, 0, 0);
  uip_netif_addr_add(&ipaddr, 64, 0, AUTOCONF);  
  uip_nd6_prefix_add(&ipaddr, 64, 0); // For on-link determination.
  
  print_local_addresses();
}
#endif /* FIXED_GLOBAL_ADDRESS */
/*---------------------------------------------------------------------------*/

#if UIP_CONF_ROUTER

/*---------------------------------------------------------------------------*/
#if ROUTING == RIMEROUTE
void init_net(void)
{
  uip_ipaddr_t ipaddr, nexthop, net;
  
  //rimeaddr_t gateway = {0x00,0x80,0xe1,0x02,0,0,0x00,0x8a};
  rimeaddr_t gateway = {GW_LL_ADDR};
  
  uip_ip6addr(&net, NET_ADDR_A, NET_ADDR_B, NET_ADDR_C, NET_ADDR_D, 0, 0, 0, 0);
  
  rimeroute_set_net(&net,64);
  
  rimeroute_set_gateway(&gateway);
  
  if(rimeaddr_cmp(&gateway, &rimeaddr_node_addr)) {
    
    PRINTF("I am gateway\n");
    
    uip_ip6addr(&ipaddr, GW_NET_ADDR_A, GW_NET_ADDR_B, GW_NET_ADDR_C, GW_NET_ADDR_D, 0, 0, 0, 0);
    uip_netif_addr_add(&ipaddr, 64, 0, AUTOCONF);
    uip_nd6_prefix_add(&ipaddr, 64, 0); // All the devices on this subnet are on-link.
    
    
    uip_ip6addr(&ipaddr, 0x2000, 0, 0, 0, 0, 0, 0, 0);    
    uip_ip6addr(&nexthop, GW_NET_ADDR_A,  GW_NET_ADDR_B, GW_NET_ADDR_C, GW_NET_ADDR_D, PC_IID_A, PC_IID_B, PC_IID_C, PC_IID_D); // The address of the internet-router.    
    uip6_route_table_add(&ipaddr, 3, &nexthop, 1, 0);
    
    
  }
  
  print_local_addresses();
}
#endif
/*---------------------------------------------------------------------------*/

#endif /* UIP_CONF_ROUTER */

#endif /* UIP_CONF_IPV6 */