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
 * Author: Salvatore Pitrulli <salvopitru@users.sourceforge.net>
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include "mac.h"

#include <string.h>
#include <stdio.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",lladdr->addr[0], lladdr->addr[1], lladdr->addr[2], lladdr->addr[3],lladdr->addr[4], lladdr->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#endif

#define PING6_NB 5
#define PING6_DATALEN 16

#define UIP_IP_BUF                ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_ICMP_BUF            ((struct uip_icmp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_ND6_RA_BUF            ((struct uip_nd6_ra *)&uip_buf[uip_l2_l3_icmp_hdr_len])
#define UIP_ND6_OPT_HDR_BUF  ((struct uip_nd6_opt_hdr *)&uip_buf[uip_l2_l3_icmp_hdr_len + nd6_opt_offset])

#define ADV_ON_LINK             0x80
#define ADV_AUTO_CONF           0x40


#define ADV_CUR_HOP_LIMIT       0
#define ADV_DEFAULT_LIFETIME    300
#define ADV_REACHABLE_TIME      0
#define ADV_RETRANS_TIME        0
#define ADV_PREFIX_LENGTH       64
#define ADV_L_A_FLAGS           ADV_AUTO_CONF       // On-link and Autonoumus autoconfiguration.
#define ADV_VALID_LIFETIME      1000
#define ADV_PREFERRED_LIFETIME  300

#define ADV_PREFIX_1            0x2001
#define ADV_PREFIX_2            0xdb8
#define ADV_PREFIX_3            0xbbbb
#define ADV_PREFIX_4            0xaa00
#define ADV_PREFIX_5            0
#define ADV_PREFIX_6            0
#define ADV_PREFIX_7            0
#define ADV_PREFIX_8            0

#define MAX_RTR_ADV_INTERVAL    20
#define MIN_RTR_ADV_INTERVAL    10

static u8_t nd6_opt_offset;
static struct uip_nd6_opt_llao *nd6_opt_llao;
static struct uip_nd6_opt_prefix_info *nd6_opt_prefix_info;

static struct etimer radvd_timer;
uip_ipaddr_t dest_addr;

PROCESS(radvd_process, "RADVD process");
//AUTOSTART_PROCESSES(&radvd_process);

/*---------------------------------------------------------------------------*/
static void
radvdhandler(process_event_t ev, process_data_t data)
{
  uip_create_linklocal_allnodes_mcast(&dest_addr);  
  
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 1;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = UIP_ND6_HOP_LIMIT;
  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &dest_addr);
  uip_netif_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);
  
  UIP_ICMP_BUF->type = ICMP6_RA;
  UIP_ICMP_BUF->icode = 0;
  
  UIP_ND6_RA_BUF->cur_ttl = ADV_CUR_HOP_LIMIT;
  UIP_ND6_RA_BUF->flags_reserved = 0;
  UIP_ND6_RA_BUF->router_lifetime = HTONS(ADV_DEFAULT_LIFETIME);
  UIP_ND6_RA_BUF->reachable_time = HTONL(ADV_REACHABLE_TIME);
  UIP_ND6_RA_BUF->retrans_timer = HTONL(ADV_RETRANS_TIME);
  
  nd6_opt_offset = UIP_ND6_RA_LEN;
  nd6_opt_llao = (struct uip_nd6_opt_llao *)UIP_ND6_OPT_HDR_BUF;
  nd6_opt_llao->type = UIP_ND6_OPT_SLLAO;
  nd6_opt_llao->len = UIP_ND6_OPT_LLAO_LEN>>3;
  memcpy(&nd6_opt_llao->addr, &uip_lladdr.addr, sizeof(uip_lladdr_t));
  
  nd6_opt_offset += UIP_ND6_OPT_LLAO_LEN;
  nd6_opt_prefix_info = (struct uip_nd6_opt_prefix_info *)UIP_ND6_OPT_HDR_BUF;
  nd6_opt_prefix_info->type = UIP_ND6_OPT_PREFIX_INFO;
  nd6_opt_prefix_info->len = UIP_ND6_OPT_PREFIX_INFO_LEN>>3;
  nd6_opt_prefix_info->preflen = ADV_PREFIX_LENGTH;
  nd6_opt_prefix_info->flagsreserved1 = ADV_L_A_FLAGS;
  nd6_opt_prefix_info->validlt = HTONL(ADV_VALID_LIFETIME);
  nd6_opt_prefix_info->preferredlt = HTONL(ADV_PREFERRED_LIFETIME);
  nd6_opt_prefix_info->reserved2 = 0;
  
  uip_ip6addr(&nd6_opt_prefix_info->prefix,  ADV_PREFIX_1, ADV_PREFIX_2, ADV_PREFIX_3, ADV_PREFIX_4, ADV_PREFIX_5, ADV_PREFIX_6, ADV_PREFIX_7, ADV_PREFIX_8);
  
  uip_len = UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_RA_LEN + UIP_ND6_OPT_LLAO_LEN + UIP_ND6_OPT_PREFIX_INFO_LEN;
  
  
  
  UIP_IP_BUF->len[0] = (u8_t)((uip_len - 40) >> 8);
  UIP_IP_BUF->len[1] = (u8_t)((uip_len - 40) & 0x00FF);
  
  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();
  
  
  PRINTF("Sending Router Advertisemement to");
  PRINT6ADDR(&UIP_IP_BUF->destipaddr);
  PRINTF("from");
  PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
  PRINTF("\n");
  UIP_STAT(++uip_stat.icmp.sent);
  
  tcpip_ipv6_output();
  
  etimer_set(&radvd_timer, (MIN_RTR_ADV_INTERVAL + (random_rand() % (MAX_RTR_ADV_INTERVAL - MIN_RTR_ADV_INTERVAL)))*CLOCK_SECOND);
  
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(radvd_process, ev, data)
{
  
  PROCESS_BEGIN();
  PRINTF("In Process RADVD\n");
  PRINTF("Wait for DAD\n");

  etimer_set(&radvd_timer, 15*CLOCK_SECOND);
  
  PROCESS_YIELD();
  
  //icmp6_new(NULL);  
  
  etimer_set(&radvd_timer, (MIN_RTR_ADV_INTERVAL + (random_rand() % (MAX_RTR_ADV_INTERVAL - MIN_RTR_ADV_INTERVAL)))*CLOCK_SECOND);
 
  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_TIMER){
      radvdhandler(ev, data);
    }
//    else if(ev == tcpip_event){
//      icmp6handler(
//    }
  }
  
  PRINTF("END RADVD\n");
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
