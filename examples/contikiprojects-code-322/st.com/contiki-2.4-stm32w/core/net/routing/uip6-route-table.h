/*
 * A ipv6 route table.
 * In the future, maybe it will be better if it will be integrated with the
 * uip_nd6_prefix_list, because they are quite similar.
 */

#ifndef __UIP6_ROUTE_TABLE_H__
#define __UIP6_ROUTE_TABLE_H__

#include "net/uip.h"

struct uip6_route_entry {
  struct uip6_route_entry *next;
  uip_ipaddr_t destipaddr;
  uint8_t prefixlength;          /* The net prefix length. */
  uip_ipaddr_t nexthop;
  uint32_t seqno;
  uint8_t cost;
  //uint8_t lifetime;

  //uint8_t decay;
  //uint8_t time_last_decay;
};

void uip6_route_table_init(void);
struct uip6_route_entry *uip6_route_table_lookup(const uip_ipaddr_t *destipaddr);
struct uip6_route_entry *uip6_route_table_add(const uip_ipaddr_t *destipaddr, uint8_t prefixlength,
                         const uip_ipaddr_t *nexthop, uint8_t cost, uint32_t seqno);


void uip_ip6addr_mask(uip_ipaddr_t *dest,const uip_ipaddr_t *src, uint8_t prefixlen);


#endif /* __UIP6_ROUTE_TABLE_H__ */