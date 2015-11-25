/**
 * \addtogroup uip6
 * @{
 */
/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 *         IPv6 routing table.
 * \author
 *         Salvatore Pitrulli <salvopitru@users.sourceforge.net>
 */

#include "lib/list.h"
#include "lib/memb.h"
#include "net/routing/uip6-route-table.h"
#include "net/rime/ctimer.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",lladdr->addr[0], lladdr->addr[1], lladdr->addr[2], lladdr->addr[3],lladdr->addr[4], lladdr->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#endif


#ifdef UIP6_ROUTE_CONF_ENTRIES
#define UIP6_ROUTE_ENTRIES UIP6_ROUTE_CONF_ENTRIES
#else /* ROUTE_CONF_ENTRIES */
#define UIP6_ROUTE_ENTRIES 5
#endif /* ROUTE_CONF_ENTRIES */

#define INFINITE_LIFETIME 0xFF
#define TIME_EXP          2

/*
 * List of route entries.
 */
LIST(uip6_route_table);
MEMB(uip6_route_mem, struct uip6_route_entry, UIP6_ROUTE_ENTRIES);

//static struct ctimer t;

/*---------------------------------------------------------------------------*/
/*static void
periodic(void *ptr)
{
  struct uip6_route_entry *e;

  for(e = list_head(uip6_route_table); e != NULL; e = e->next) {
    if(e->lifetime!=INFINITE_LIFETIME){
      e->lifetime--;
    }
    if(e->lifetime == 0) {
      PRINTF("uip6route periodic: removing entry to ");
      PRINT6ADDR(&e->dest);
      PRINTF("/%u with nexthop ",e->prefixlength);
      PRINT6ADDR(&e->nexthop);
      PRINTF(" cost %d and seqno %ul\n", e->cost, e->seqno);        
      list_remove(uip6_route_table, e);
      memb_free(&uip6_route_mem, e);
    }
  }

  ctimer_set(&t, CLOCK_SECOND*(1<<TIME_EXP), periodic, NULL);
}*/
/*---------------------------------------------------------------------------*/
void
uip6_route_table_init(void)
{
  list_init(uip6_route_table);
  memb_init(&uip6_route_mem);

  //ctimer_set(&t, CLOCK_SECOND*(1<<TIME_EXP), periodic, NULL);
}
/*---------------------------------------------------------------------------*/
void uip_ip6addr_mask(uip_ipaddr_t *dest,const uip_ipaddr_t *src, uint8_t prefixlen)
{
  uint8_t i = 128 - prefixlen;
  uint8_t j = 15;
  
  if(prefixlen > 128){
    prefixlen = 128;
  }
  
  memcpy(dest,src,sizeof(uip_ipaddr_t));  
  
  while(i>0){
    if(i>7){
      dest->u8[j--] = 0;
      i-=8;
    }
    else {
      dest->u8[j] &= ((uint8_t)0xFF)<<i;
      break;
    }
  }  
}
/*---------------------------------------------------------------------------*/
struct uip6_route_entry *
uip6_route_table_add(const uip_ipaddr_t *destipaddr, uint8_t prefixlength, const uip_ipaddr_t *nexthop, uint8_t cost, uint32_t seqno/*, uint8_t lifetime*/)
{
  struct uip6_route_entry *e;
  uip_ipaddr_t ipaddr;
  
  uip_ip6addr_mask(&ipaddr,destipaddr,prefixlength);
  
  
  /* Avoid inserting duplicate entries. */  
  /* Find the route with the same prefix and the same nexthop. */
  for(e = list_head(uip6_route_table); e != NULL; e = e->next) {
    
    if(e->prefixlength == prefixlength && uip_ipaddr_cmp(&e->destipaddr,&ipaddr) == 0
       && uip_ipaddr_cmp(&e->nexthop, nexthop)){
         
         e->cost = cost;
         e->seqno = seqno;
         //e->lifetime = lifetime;
         
         PRINTF("uip6route_add: updating entry to ");
         PRINT6ADDR(&e->dest);
         PRINTF("/%u with nexthop ",e->prefixlength);
         PRINT6ADDR(&e->nexthop);
         PRINTF(" cost %d and seqno %ul\n", e->cost, e->seqno);        
         
         return e;
       }    
  }
  
  /* Allocate a new entry or reuse the oldest entry with highest cost. */
  e = memb_alloc(&uip6_route_mem);
  if(e == NULL) {
    /* Remove oldest entry.  XXX */
    e = list_chop(uip6_route_table);
    PRINTF("uip6route_add: removing entry to ");
    PRINT6ADDR(&e->dest);
    PRINTF("/%u with nexthop ",e->prefixlength);
    PRINT6ADDR(&e->nexthop);
    PRINTF(" and cost %d\n", e->cost);
  }   
  
  uip_ipaddr_copy(&e->destipaddr, &ipaddr);
  e->prefixlength = prefixlength;
  uip_ipaddr_copy(&e->nexthop, nexthop);
  e->cost = cost;
  e->seqno = seqno;
  //e->lifetime = lifetime;
  
  /* New entry goes first. */
  list_push(uip6_route_table, e);
  
  PRINTF("uip6route_add: new entry to ");
  PRINT6ADDR(&e->dest);
  PRINTF("/%u with nexthop ",e->prefixlength);
  PRINT6ADDR(&e->nexthop);
  PRINTF(" and cost %d\n", e->cost);
  
  return e;
}
/*---------------------------------------------------------------------------*/
struct uip6_route_entry *
uip6_route_table_lookup(const uip_ipaddr_t *destipaddr)
{   
  
  struct uip6_route_entry *e;
  uint8_t lowest_cost;
  uint8_t longest_prefixlength;
  struct uip6_route_entry *best_entry;

  lowest_cost = -1;
  longest_prefixlength = 0;
  best_entry = NULL;
  
  /* Find the route with the longest prefix matching and the lowest cost. */
  for(e = list_head(uip6_route_table); e != NULL; e = e->next) {
    
    if(uip_ipaddr_prefixcmp(destipaddr,&e->destipaddr,e->prefixlength)){
      
      if(e->prefixlength > longest_prefixlength){ 
        
        best_entry = e;
        lowest_cost = e->cost;
        longest_prefixlength = e->prefixlength;        
      }
      else if(e->prefixlength == longest_prefixlength && e->cost < lowest_cost){
        
        best_entry = e;
        lowest_cost = e->cost;
      }
    }   

  }
  
  return best_entry;
  
}
/*---------------------------------------------------------------------------*/
void
uip6_route_table_remove(struct uip6_route_entry *e)
{
  list_remove(uip6_route_table, e);
  memb_free(&uip6_route_mem, e);
}
/*---------------------------------------------------------------------------*/