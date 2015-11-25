/**
 * \addtogroup rpl
 * @{
 */

/**
 * \file
 *         IPv6 Routing Protocol for Low power and Lossy Networks (.c)
 *
 * \author Mathilde Durvy <mdurvy@cisco.com> 
 * \author Julien Abeille <jabeille@cisco.com> 
 * \author Marco Valente <marcvale@cisco.com>
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

#include <math.h>
#include <string.h>
#include "rpl.h"

#if (UIP_CONF_ROUTER & UIP_CONF_ROUTER_RPL & UIP_CONF_ICMP6 & UIP_CONF_ND6_API) 


/* Pointer inside packet */
#define UIP_IP_BUF                  ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_ICMP_BUF              ((struct uip_icmp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define RPL_DIO_BUF                           ((rpl_dio *)&uip_buf[uip_l2_l3_icmp_hdr_len])
#define RPL_DAO_BUF                           ((rpl_dao *)&uip_buf[uip_l2_l3_icmp_hdr_len])
#define RPL_DIOOPT_HDR_BUF              ((rpl_dioopt_hdr*)&uip_buf[uip_l2_l3_icmp_hdr_len + rpl_opt_offset])
#define RPL_DIOOPT_OCP_BUF               ((rpl_dioopt_ocp *)&uip_buf[uip_l2_l3_icmp_hdr_len + RPL_DIOOPT_HDR_LEN + rpl_opt_offset])
#define RPL_DIOOPT_METRIC_HDR_BUF     ((rpl_dioopt_metric_hdr *)&uip_buf[uip_l2_l3_icmp_hdr_len + RPL_DIOOPT_HDR_LEN + rpl_opt_offset + rpl_metric_offset])
#define RPL_DIOOPT_METRIC_NE_BUF   ((rpl_dioopt_metric_ne *)&uip_buf[uip_l2_l3_icmp_hdr_len + RPL_DIOOPT_HDR_LEN+ RPL_DIOOPT_METRIC_HDR_LEN + rpl_opt_offset + rpl_metric_offset])
#define RPL_DIOOPT_METRIC_ETX_BUF ((rpl_dioopt_metric_etx *)&uip_buf[uip_l2_l3_icmp_hdr_len + RPL_DIOOPT_HDR_LEN+ RPL_DIOOPT_METRIC_HDR_LEN + rpl_opt_offset + rpl_metric_offset])

#define DEBUG 1

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",lladdr->addr[0], lladdr->addr[1], lladdr->addr[2], lladdr->addr[3],lladdr->addr[4], lladdr->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

/** The RPL DAG structure */
static rpl_dag mydag;
/** The RPL specific stuff for routes learned via DAO*/
static rpl_daoroute rpl_daoroute_table[RPL_DAOROUTE_NB];
/** RPL periodic timer */
static struct etimer  rpl_timer_periodic;
/** The ocp function in use */
uint8_t (* rpl_ocp)(rpl_dio *, uint8_t, uint8_t);
#if RPL_SEND_DIS
 /** Sent DIS, wait for DIOs during this time window */
struct etimer dis_window_timer;
#endif
/** Wait for poison to propagate during local repair */
struct etimer local_repair_timer;         

/* Option treatment */
uint16_t rpl_opt_offset;
uint16_t rpl_metric_offset;
uint16_t rpl_daoprefixblock_offset;

/* Other local pointers and variables used in this file */
static rpl_parent* locparent;
static rpl_daoroute* locdaoroute;
static uip_ds6_route* locroute;
static uip_ds6_prefix* locprefix;
static uip_ds6_addr* locaddr;
static uip_ipaddr_t locipaddr;
#if UIP_CONF_ETX
static etx_nbr * locetx;
#endif

PROCESS(rpl_process, "RPL routing algorithm");

PROCESS_THREAD(rpl_process, ev, data)
{
  PROCESS_BEGIN();
  
  if(icmp6_new(NULL) == NULL) {
    PRINTF("[RPL_ERROR] Cannot register ICMPv6 listener\n");
  }
#if UIP_CONF_ND6_API  
  else if (nd6_listener_new(NULL) == NULL ) {
    PRINTF("[RPL_ERROR] Cannot register ND6 listener\n");
  }
#endif
#if UIP_CONF_ETX
  else if(etx_listener_new() == NULL) {
    PRINTF("[RPL_ERROR] Cannot register ETX listener\n");
  }
#endif
  else {
    etimer_set(&rpl_timer_periodic, RPL_PERIOD*CLOCK_SECOND);
    PRINTF("[RPL_DAG] RPL starting. Initializing DAG.\n");
    rpl_dag_init();
#if UIP_CONF_ETX
    process_start(&etx_process, NULL);
#endif
    while(1) {
      PROCESS_YIELD();
      rpl_event_handler(ev, data);
    }
  }
  PROCESS_END();
}

void
rpl_event_handler(process_event_t ev, process_data_t data)
{
  rpl_parent *parent;
  
  switch(ev) {
#if UIP_CONF_ETX
    case ETX_EVENT_NEW_VALUE:
      {
        /* We just received a new ETX value for one of our parent */
        locetx = (etx_nbr *)data;
        parent = rpl_parent_lookup_ip(&locetx->ipaddr);
        if (parent != NULL ){
          uint8_t newlinketx;
          uint8_t newrootetx;
          if(locetx->success == 0){
            /* Avoid division by 0, put to max */
            newlinketx = 0xff;
          } else {
            /* We use multiples of 10 for ETX to be able to use uint */
            if(parent->linketx != 0)
              newlinketx = 0.8 * parent->linketx +
                0.2 * ((RPL_ETX_MIN * locetx->count) / locetx->success);
            else
              newlinketx = (RPL_ETX_MIN * locetx->count) / locetx->success;
          }
          PRINTF("[RPL_EVENT] New ETX value for parent "); PRINT6ADDR((&parent->ipaddr)); PRINTF(" %u\n", newlinketx);
          if(newlinketx > RPL_ETX_MAX){
            rpl_parent_rm(parent);
            /* If parent was preferred change */
            if(parent->state == RPL_PREFERRED){
              parent = rpl_parent_lookup_order(1);           
              if (parent != NULL){
                PRINTF("[RPL_DAG] Lost PREFERRED parent: "); PRINT6ADDR(((uip_ipaddr_t *)data)); PRINTF(". Change it.\n");
                rpl_new_preferred(parent);
              } else {
                /* We have no parent left */
                PRINTF("[RPL_DAG] No parents left. Perform LOCAL REPAIR.\n");
                rpl_local_repair();
              }
            }
          }
#if RPL_DAG_OCP == 1
          else if (newlinketx > (1 + RPL_ETX_VAR)*parent->linketx ||
                     newlinketx < (1 - RPL_ETX_VAR)*parent->linketx ||
                     parent->order == RPL_ORDER_UNKNOWN){
            /* We have a siginficant change, update etx value */
            PRINTF("[RPL_EVENT] ETX significant change\n");
            newrootetx = newlinketx - parent->linketx + parent->rootetx;
            /* reorder parent set */
            rpl_parent_rm(parent);
            uint8_t order = rpl_ocp(&parent->last_dio,
                                    newrootetx, newlinketx);
#if RPL_SEND_DIS
            if (!etimer_expired(&local_repair_timer) ||
                !etimer_expired(&dis_window_timer)){
#else
            if (!etimer_expired(&local_repair_timer)){
#endif
              rpl_parent_add(&parent->last_dio, &parent->ipaddr,
                             RPL_CANDIDATE, order, newlinketx,
                             newrootetx, 0);
            } else {
              rpl_parent * parent_pref =
                rpl_parent_lookup_state(RPL_PREFERRED); 
              if (order == 1) {
                if(parent_pref != NULL){
                  parent_pref->state = RPL_CANDIDATE;
                  uip_ds6_route_rm(parent_pref->route);
                  parent_pref->route = NULL;
                }
                parent_pref = rpl_parent_add(&parent->last_dio,
                                             &parent->ipaddr,
                                             RPL_PREFERRED, order,
                                             newlinketx,
                                             newrootetx, 0);
                rpl_new_preferred(parent_pref);
              } else if (order > 0) {
                rpl_parent_add(&parent->last_dio, &parent->ipaddr,
                               RPL_CANDIDATE, order, newlinketx,
                               newrootetx, 0);
                if (parent_pref == NULL){
                  parent_pref = rpl_parent_lookup_order(1);
                  rpl_new_preferred(parent_pref);
                }
              }          
            }
          }
#else  /* RPL_DAG_OCP == 1 */
          else {
            PRINTF("[RPL_EVENT] Updating ETX value for parent\n"); 
            parent->linketx = newlinketx;
          }
#endif /* RPL_DAG_OCP == 1 */
        }
      }
      break;
#endif /* UIP_CONF_ETX */
#if UIP_CONF_ND6_API  
    case UIP_ND6_EVENT_NBR_RM:
      {
        /* The ND neighbor in data is in the process of being deleted */
        PRINTF("[RPL_EVENT] Lost neighbor (ND6): "); PRINT6ADDR(((uip_ipaddr_t *)data)); PRINTF("\n");
	//If the IP corresponds to a child, routes thru it will be removed
	rpl_child_rm((uip_ipaddr_t *)data);    
        parent = rpl_parent_lookup_ip((uip_ipaddr_t *)data);
	if (parent != NULL ) {
          /* We remove the parent and its default route */
          /* It does not make sense to keep a parent without a
             corresponding neighbor as we wouldn't be able to fwd
             traffic through it */
	  PRINTF("[RPL_DAG] Lost parent: "); PRINT6ADDR(((uip_ipaddr_t *)data)); PRINTF(". Removing it.\n");
          rpl_parent_rm(parent);
          /* If parent was preferred change */
          if(parent->state == RPL_PREFERRED){
            parent = rpl_parent_lookup_order(1);           
            if (parent != NULL){
	      PRINTF("[RPL_DAG] Lost PREFERRED parent: "); PRINT6ADDR(((uip_ipaddr_t *)data)); PRINTF(". Change it.\n");
              rpl_new_preferred(parent);
            } else {
              /* We have no parent left */
	      PRINTF("[RPL_DAG] No parents left. Perform LOCAL REPAIR.\n");
              rpl_local_repair();
            }
          }
        }
      }
      break;
#endif  /* UIP_CONF_ND6_API */
        case PROCESS_EVENT_TIMER:
      {
        if (data == &mydag.dao_delay_timer &&
            etimer_expired(&mydag.dao_delay_timer)) {
	  PRINTF("[RPL_DAO] DAO delay timer expired. Sending DAO batch.\n");
          rpl_dao_output_all(1, 1, NULL);  
	}
	/* DIS timer */
#if RPL_SEND_DIS
	else if (data == &dis_window_timer &&
  		 etimer_expired(&dis_window_timer)){
          PRINTF("[RPL_DIS] End of DIS window\n");
          parent = rpl_parent_lookup_order(1);
          if(parent != NULL){
            rpl_new_preferred(parent);
	    PRINTF("[RPL_DAG] New PREFERRED after DIS window: "); PRINT6ADDR(&(parent->ipaddr)); PRINTF("\n");
	  }
        }	          
#endif
        else if (data == &local_repair_timer &&
  		 etimer_expired(&local_repair_timer)){
          PRINTF("[RPL_DIO] End of Local Repair\n");
          parent = rpl_parent_lookup_order(1);
          if(parent != NULL){
            rpl_new_preferred(parent);
	    PRINTF("[RPL_DAG] End of LOCAL REPAIR. New PREFERRED: "); PRINT6ADDR(&(parent->ipaddr)); PRINTF("\n");
          }
        }
        else if(data == &rpl_timer_periodic &&
                etimer_expired(&rpl_timer_periodic)){
          rpl_periodic();
        }
      }
      break;
    case UIP_ICMP6_EVENT_PACKET_IN:
      {
        if(UIP_ICMP_BUF->type == ICMP6_RPL) {        
          switch(UIP_ICMP_BUF->icode) {
#if (RPL_LEAF_ONLY == 0)
           case ICMP6_RPL_DAO:
              rpl_dao_input();
              break;
#endif
            case ICMP6_RPL_DIO:
              rpl_dio_input();
              break;
#if (RPL_LEAF_ONLY == 0)             
            case ICMP6_RPL_DIS:
              rpl_dis_input();
              break;
#endif              
            default:
              break;
          }
        }
      }
      break;
    default:
      break;
  }
}

void
rpl_dag_init()
{
  /* Reinitialize
   * - it RPL_ROOT_ENABLE = 1, the node is the root of it's own DAG
   * or 
   * - we are not part of any DAG, in this situation we put the rank
   *   to infinity
   */
  
  
  mydag.instanceID = RPL_INSTANCE_ID;
  /* Check if IP stack is ready */
  uip_create_linklocal_prefix(&locipaddr);
  if(!uip_ds6_select_src(&locipaddr, &locipaddr)){ 
    /* We do not have a preferred address (DAD for link local is not
     * done) */
    mydag.state = RPL_INIT_DAG;
    //PRINTF("[RPL] DAG init, no pref address, INIT\n");
    return;
  }
  
#if RPL_ROOT_ENABLE
  /* select one of my global preferred IPv6 addresses for the dagID */
  uip_create_unspecified(&locipaddr);
  if(!uip_ds6_select_src(&locipaddr, &locipaddr)){
    /* We have no global preferred address */
    mydag.state = RPL_INIT_DAG;
    //PRINTF("[RPL] DAG init, no global address, INIT\n");
    return;
  } else {
    uip_ipaddr_copy(&(mydag.dagID), &locipaddr);
    mydag.state = RPL_OWN_DAG;
    mydag.sequence = 255;     
    mydag.isconsistent = 1;
    mydag.control = RPL_DAG_CONTROL;
    mydag.rank = RPL_ROOT_RANK;
    PRINTF("[RPL_DAG] DAG init, DAGID "); PRINT6ADDR(&(mydag.dagID)); PRINTF("\n");
  }
#else  /*RPL_ROOT_ENABLE*/
  mydag.state = RPL_NO_DAG;
  mydag.rank = 0xff;
  mydag.DAOsequence = 0;
  PRINTF("[RPL_DAG] DAG init, NODAG\n");
#endif /*RPL_ROOT_ENABLE*/

  for(locparent = mydag.parent_set;
      locparent < mydag.parent_set + RPL_PARENT_NB;
      locparent++) {
    locparent->isused = 0;
  }

  for(locdaoroute = rpl_daoroute_table;
      locdaoroute < rpl_daoroute_table + RPL_DAOROUTE_NB;
      locdaoroute++) {
    rpl_daoroute_rm(locdaoroute);
  }

  mydag.ocp = RPL_DAG_OCP;
#if RPL_DAG_OCP == 0
  rpl_ocp = rpl_ocp0;
#else
#if RPL_DAG_OCP == 1
  rpl_ocp = rpl_ocp1;
#endif
#endif
#if RPL_SEND_DIS
  /* Send DIS only if we are not in local repair */
  if(etimer_expired(&local_repair_timer)){
    PRINTF("[RPL_DIS] Start of DIS window. Sending DIS.\n");
    etimer_set(&dis_window_timer, RPL_DIS_INTERVAL*CLOCK_SECOND);
    rpl_dis_output();
  }
#endif /* RPL_SEND_DIS */
#if RPL_ROOT_ENABLE
  timer_set(&(mydag.sequence_timer), RPL_SEQUENCE_INTERVAL * CLOCK_SECOND);
  mydag.dio_send_timer.i_min =  (1 << RPL_DIO_INTERVAL_MIN) * (CLOCK_SECOND / 1000.0);
  mydag.dio_send_timer.i_max =  mydag.dio_send_timer.i_min * (1 << RPL_DIO_INTERVAL_DOUBLINGS);
  rpl_dio_send_timer_init();  
#endif /* RPL_ROOT_ENABLE */
  return;
}

void
rpl_dag_new(rpl_dio *dio)
{
  
  /* Initialize new DAG based on DIO */
  /* The instanceID and the associated OCP never change */
  /* The rank is computed when adding the first parent */
  uip_ipaddr_copy(&(mydag.dagID), &(dio->dagID));
  PRINTF("[RPL_DAG] DAG new, DAGID "); PRINT6ADDR(&(mydag.dagID)); PRINTF("\n");
  mydag.sequence = dio->sequence;    
  mydag.control = dio->control;
  mydag.state = RPL_DAG;
  mydag.isconsistent = 1;
  
  for(locdaoroute = rpl_daoroute_table;
      locdaoroute < rpl_daoroute_table + RPL_DAOROUTE_NB;
      locdaoroute++) {
    rpl_daoroute_rm(locdaoroute);
  }
  /* parent set is cleaned when adding first, preferred, parent */
    
  /* New DAG id, reset the trickle timer */
  /* TBD check how we handle the timer option in DIO */
  mydag.dio_send_timer.i_min = (1 << RPL_DIO_INTERVAL_MIN) * (CLOCK_SECOND / 1000.0);
  mydag.dio_send_timer.i_max = mydag.dio_send_timer.i_min * (1 << RPL_DIO_INTERVAL_DOUBLINGS);
  rpl_dio_send_timer_init();  
}

#if (RPL_LEAF_ONLY == 0)
void
rpl_dis_input() 
{
  /* Reset trickle instead (we do not consider unicast DIS)*/
  PRINTF("[RPL_DIS] DIS input from "); PRINT6ADDR(&(UIP_IP_BUF->srcipaddr)); PRINTF("\n");
  if((mydag.state > RPL_NO_DAG) &&
     uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)){
    PRINTF("[RPL_DIS] DIS input, scheduling DIO\n");
    rpl_dio_send_timer_init();
  }
  return;
}
#endif /* RPL_LEAF_ONLY == 0 */

void
rpl_dis_output() 
{
  uip_ext_len = 0;
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = RPL_HOP_LIMIT;

  uip_create_ll_allrouters_maddr(&locipaddr);
  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &locipaddr);
  uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);

  UIP_ICMP_BUF->type = ICMP6_RPL; 
  UIP_ICMP_BUF->icode = ICMP6_RPL_DIS;
  
  memset((void *)UIP_ICMP_BUF+UIP_ICMPH_LEN, 0, 4);
  /*Note: adding 4 bytes of random data to respect 8 byte alignment */
  uip_len = UIP_IPH_LEN + UIP_ICMPH_LEN + 4;
  
  UIP_IP_BUF->len[0] = ((uip_len - UIP_IPH_LEN) >> 8);
  UIP_IP_BUF->len[1] = ((uip_len - UIP_IPH_LEN) & 0xff);
  
  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();
  
  UIP_STAT(++uip_stat.icmp.sent);
  
  //PRINTF("[RPL_DIS] Sending DIS from "); PRINT6ADDR(&UIP_IP_BUF->srcipaddr); PRINTF(" to "); PRINT6ADDR(&UIP_IP_BUF->destipaddr); PRINTF("\n");
  PRINTF("[RPL_DIS] Sending DIS\n");
  
  tcpip_ipv6_output();

  return;
}

#if (RPL_LEAF_ONLY == 0)
void
rpl_dao_input() 
{
  uint16_t DAOsequence = 0;
  uint8_t  rank = 0;
  uint32_t lifetime = 0;
  uint32_t routetag = 0;
   
  PRINTF("[RPL_DAO] DAO input from "); PRINT6ADDR(&UIP_IP_BUF->srcipaddr); PRINTF("\n");
  
  /* We do not support multicast DAO and DAO with RR stack for now */
  /* Check that the instanceID matches and the a neighbor exist */
  if((uip_ds6_nbr_lookup(&UIP_IP_BUF->srcipaddr) == NULL) ||
     (uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) ||
     (RPL_DAO_BUF->instanceID != mydag.instanceID)) {
    //PRINTF("[RPL_DAO] No neighbor cache entry OR multicast dest OR non NULL reverse route stack OR other instance\n");
    goto discard;
  }
  
  DAOsequence = ntohs(RPL_DAO_BUF->DAOsequence);
  rank = RPL_DAO_BUF->rank;
  lifetime = ntohl(RPL_DAO_BUF->lifetime);
  routetag = ntohl(RPL_DAO_BUF->routetag);
  memcpy(&locipaddr, &RPL_DAO_BUF->prefix, RPL_DAO_BUF->preflen >> 3);
  locdaoroute = rpl_daoroute_lookup(&locipaddr, RPL_DAO_BUF->preflen);

  PRINTF("[RPL_DAO] DAO: DAOsequence = %u, rank = %u, lifetime = %lu, routetag = %lu, ", DAOsequence, rank, lifetime, routetag);
  PRINTF("prefix = "); PRINT6ADDR(&RPL_DAO_BUF->prefix); PRINTF("\n");

  // If no route exists and DAO -> add route (except is there is already
  // a non RPL route)
  // If a route exists through this neighbor and DAO -> update all OK
  // If a route exists through another neighbor and DAO as better rank -> remove
  // route, add new route 

  // If a route exists through this neighbor and noDAO -> remove route OK 
  // If a route exists through another neighbor and noDAO -> do nothing
  
  if(locdaoroute != NULL) {
    /*if(DAOsequence <= locdaoroute->DAOsequence) {
      PRINTF("[RPL_DAO] DAO sequence too old.\n");
      goto discard;
      }*/
    if(uip_ipaddr_cmp(&locdaoroute->route->nexthop, &UIP_IP_BUF->srcipaddr)) {
      if(lifetime != 0) {
        PRINTF("[RPL_DAO] Processing DAO for existing route. Updating record.\n");
        stimer_set(&locdaoroute->route->lifetime, lifetime);
	locdaoroute->DAOsequence = DAOsequence;
	locdaoroute->rank = rank;
	locdaoroute->routetag = routetag;
	locdaoroute->retry_count = 0;
      } else { //This is a NODAO
        PRINTF("[RPL_DAO] Processing a NODAO for existing route\n");
	locdaoroute->retry_count = (RPL_MAX_ROUTE_SOLICIT + 1);
      }
    } else {
      PRINTF("[RPL_DAO] Processing DAO for existing route, but nexthop has changed.\n");
      if((locdaoroute->rank > rank) && (lifetime != 0)) {
        PRINTF("[RPL_DAO] Removing previous route and adding another one.\n"); 
        rpl_daoroute_rm(locdaoroute);
        locdaoroute = rpl_daoroute_add(&locipaddr, RPL_DAO_BUF->preflen, &UIP_IP_BUF->srcipaddr, 0, lifetime, rank, routetag, DAOsequence);
      }
    }
  } else {
    locroute = uip_ds6_route_lookup(&locipaddr, RPL_DAO_BUF->preflen);
    if((lifetime != 0) && (locroute == NULL)) {
      PRINTF("[RPL_DAO] Processing DAO. Adding new route for "); PRINT6ADDR(&locipaddr); PRINTF(" via "); PRINT6ADDR(&UIP_IP_BUF->srcipaddr); PRINTF("\n");
      locdaoroute = rpl_daoroute_add(&locipaddr, RPL_DAO_BUF->preflen, &UIP_IP_BUF->srcipaddr, 0, lifetime, rank, routetag, DAOsequence);
    }
  }  
  
 discard:
  //PRINTF("[RPL_DAO] End DAO processing\n");
  uip_len = 0;
  return;
}
#endif /* RPL_LEAF_ONLY == 0 */


void
rpl_dao_output_all(uint8_t isdao, uint8_t connected, uip_ipaddr_t *ipaddr){
  uint32_t lifetime = 0;

  if((locparent = rpl_parent_lookup_state(RPL_PREFERRED)) == NULL) {
    PRINTF("[RPL_DAO] DAO output: no preffered parent to send to\n");
    return;
  }
  if(connected){

    /*Increment our DAO sequence for our own addresses and connected
      prefixes */  
    mydag.DAOsequence++;
  
    for(locprefix = uip_ds6_prefix_list;
        locprefix < uip_ds6_prefix_list + UIP_DS6_PREFIX_NB; 
        locprefix++) {
      if((locprefix->isused) && 
         (locprefix->l_a_reserved & UIP_ND6_RA_FLAG_ONLINK) &&
         (locprefix->advertise)) {
        if(isdao){
          lifetime = locprefix->vlifetime;
	} else {
          lifetime = 0;
        }
        rpl_dao_output(mydag.DAOsequence, mydag.rank,
                       lifetime, locprefix->len, &locprefix->ipaddr);
      }
    }
    for(locaddr = uip_ds6_if.addr_list;
        locaddr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; 
        locaddr++) {
      if((locaddr->isused) &&
         (locaddr->state == ADDR_PREFERRED) && 
         (!uip_is_addr_link_local(&locaddr->ipaddr))) {
        if(isdao){
          if(locaddr->isinfinite == 0){
            lifetime = stimer_remaining(&locaddr->vlifetime);
          } else {
            lifetime = RPL_DAO_INFINITE_LIFETIME;
          }
        } else {
          lifetime = 0;
        }
        rpl_dao_output(mydag.DAOsequence, mydag.rank,
                       lifetime, 128, &locaddr->ipaddr);
      }
    }
  }
  for(locdaoroute = rpl_daoroute_table; 
      locdaoroute < rpl_daoroute_table + RPL_DAOROUTE_NB; 
      locdaoroute++) {
    if(locdaoroute->isused &&
       (ipaddr == NULL ||
        uip_ipaddr_cmp(&locdaoroute->route->nexthop, ipaddr))){
      if(isdao &&
         (locdaoroute->retry_count <= RPL_MAX_ROUTE_SOLICIT)) {
        if(locdaoroute->route->isinfinite == 1){
          lifetime = RPL_DAO_INFINITE_LIFETIME;
        } else {
          lifetime = stimer_remaining(&locdaoroute->route->lifetime);
        }
      } else {
        lifetime = 0;
      }
      rpl_dao_output(locdaoroute->DAOsequence, locdaoroute->rank,
                     lifetime, locdaoroute->route->len,
                     &locdaoroute->route->ipaddr);
      if(locdaoroute->retry_count > RPL_MAX_ROUTE_SOLICIT) {
        PRINTF("[RPL_DAO] Removing DAOroute to"); PRINT6ADDR(&locdaoroute->route->ipaddr); PRINTF("\n");
        rpl_daoroute_rm(locdaoroute);
      }
    }
  }
}


void
rpl_dao_output(uint16_t DAOsequence, uint8_t rank, uint32_t lifetime,
               uint8_t preflen, uip_ipaddr_t *prefix){
 
  PRINTF("[RPL_DAO] Sending DAO to"); PRINT6ADDR(&locparent->ipaddr); PRINTF("\n");
  
  uip_ext_len = 0;
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = RPL_HOP_LIMIT;
  UIP_ICMP_BUF->type = ICMP6_RPL; 
  UIP_ICMP_BUF->icode = ICMP6_RPL_DAO; 
  
  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &locparent->ipaddr);
  uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);
  
  uip_len = (uint16_t)(UIP_IPH_LEN + UIP_ICMPH_LEN + RPL_DAO_LEN + (preflen >> 3));

  RPL_DAO_BUF->DAOsequence = htons(DAOsequence);
  RPL_DAO_BUF->instanceID = mydag.instanceID;
  RPL_DAO_BUF->rank = rank;
  RPL_DAO_BUF->lifetime = htonl(lifetime); 
  RPL_DAO_BUF->routetag = 0;  
  RPL_DAO_BUF->preflen = preflen; /* in bits */
  RPL_DAO_BUF->rrcount = 0;     
  memcpy(&RPL_DAO_BUF->prefix, prefix, preflen >> 3);
  uip_ipaddr_copy(&RPL_DAO_BUF->prefix, prefix);
  
  UIP_IP_BUF->len[0] = ((uip_len - UIP_IPH_LEN) >> 8);
  UIP_IP_BUF->len[1] = ((uip_len - UIP_IPH_LEN) & 0xff);
  
  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();
  
  PRINTF("[RPL_DAO] DAO: DAOsequence = %u, rank = %u, lifetime = %lu, prefix len = %u ", DAOsequence, rank, lifetime, preflen);
  PRINTF("prefix = "); PRINT6ADDR(&RPL_DAO_BUF->prefix); PRINTF("\n");
  
  tcpip_ipv6_output();
  
  return; 
}

void
rpl_dio_input()
{
  uint8_t      order            = 0;
  uint8_t      battery          = 0;
  uint8_t      linketx          = 0;
  uint8_t      rootetx          = 0;
  int8_t       stability        = 0;
  uint16_t     ocp              = 0;
  uip_ds6_nbr* nbr              = NULL;
  rpl_parent * parent           = NULL;
  rpl_parent * parent_pref_old  = NULL;
  rpl_parent * parent_pref_new  = NULL;

  PRINTF("[RPL_DIO] DIO input from "); PRINT6ADDR(&UIP_IP_BUF->srcipaddr); PRINTF("\n");
  
  /* We are not ready to accept packet, drop*/
  if(mydag.state == RPL_INIT_DAG){
    goto discard;
  }
  
  /* TBD Check format of DIO, no specific checks mentioned in the draft*/
  /* TBD Check whether there is a risk of collision (not supported for now) */
 
  /* nbr_add will not add but return the entry if it already exists
   * if nbr_add returns NULL it means there is no more space in the cache */
#if !UIP_ND6_MANUAL
  if((nbr = uip_ds6_nbr_add(&(UIP_IP_BUF->srcipaddr),
                            NULL, 1, NBR_INCOMPLETE)) == NULL) {
    PRINTF("[RPL_DIO] DIO input, no neighbor\n");
    goto discard;
  }
#else
  if((nbr = uip_ds6_nbr_lookup(&UIP_IP_BUF->srcipaddr)) == NULL) {
    PRINTF("[RPL_DIO] DIO input, no neighbor\n");
    goto discard;
  }
#endif
  
  /* TBD Additional check to see if nbr it a viable neighbor? */

  if (RPL_DIO_BUF->instanceID != mydag.instanceID){
    PRINTF("[RPL_DIO] DIO input, wrong instance ID\n");
    goto discard;
  }
  
  /* Check if a parent with the same IP address already exists */
  parent = rpl_parent_lookup_ip(&(UIP_IP_BUF->srcipaddr));
  if(parent != NULL){
    stability = parent->stability;
#if UIP_CONF_ETX
    linketx = parent->linketx;
#if (RPL_DAG_OCP == 1)
    rootetx = parent->rootetx;
#endif
#endif 
    PRINTF("[RPL_DIO] DIO input, parent exist %p (linketx %u, rootetx %u)\n", parent, linketx, rootetx);
  }
 
  /* Check for a options, only metric for now*/
  rpl_opt_offset = RPL_DIO_LEN;
  while(uip_l3_icmp_hdr_len + rpl_opt_offset < uip_len){
    switch(RPL_DIOOPT_HDR_BUF->type) {
#if (RPL_DAG_OCP == 1) && (UIP_CONF_ETX == 1)
      case RPL_DIOOPT_METRIC:
        rpl_metric_offset = 0;
        while(rpl_metric_offset <
              ((RPL_DIOOPT_HDR_BUF->len[0]<<8) | RPL_DIOOPT_HDR_BUF->len[1])){
          switch(RPL_DIOOPT_METRIC_HDR_BUF->type) {
            case RPL_DIOOPT_METRIC_NE:
              /* Receiving node energy object, we only process if used
                 as a local constraint for now */
              PRINTF("[RPL_DIO] DIO input, OCP1, ne object\n");
              if(RPL_DIOOPT_METRIC_HDR_BUF->flags == RPL_METRIC_FLAGS_C &&
                 (RPL_DIOOPT_METRIC_NE_BUF->flags & RPL_NE_FLAGS_BAT)){
                PRINTF("[RPL_DIO] DIO input, OCP1, battery operated\n");
                battery = 1;
              }
              break;
            case RPL_DIOOPT_METRIC_ETX:
              /* Receiving new etx value, store in tmp var */
              PRINTF("[RPL_DIO] DIO input, OCP1, etx %u\n", RPL_DIOOPT_METRIC_ETX_BUF->etx);
              rootetx = RPL_DIOOPT_METRIC_ETX_BUF->etx + linketx;
              break;
            default:
              PRINTF("[RPL_DIO] DIO metric/constraint not supported\n");
              break; 
          }
          rpl_metric_offset += RPL_DIOOPT_METRIC_HDR_LEN +
            ntohs(RPL_DIOOPT_METRIC_HDR_BUF->len);
        }
        break;
#endif
      case RPL_DIOOPT_OCP:
        ocp = ntohs(RPL_DIOOPT_OCP_BUF->ocp);
        if (ocp != RPL_DAG_OCP){
          /* This is a configuration mistake, it should not happen */
          PRINTF("[RPL_DIO] DIO input, wrong OCP\n");
          goto discard;
        }
        break;
      default:
        PRINTF("[RPL_DIO] DIO option not supported %u\n", RPL_DIOOPT_HDR_BUF->type);
        break;
    }
    rpl_opt_offset += RPL_DIOOPT_HDR_LEN +
      ((RPL_DIOOPT_HDR_BUF->len[0]<<8) | RPL_DIOOPT_HDR_BUF->len[1]);
  }

#if (RPL_DAG_OCP == 1) && (UIP_CONF_ETX == 1)
  if(rootetx == 0 && rpl_metric_offset == 0){
    /* We have no rootetx value, we cannot proceed further */
    PRINTF("[RPL_DIO] DIO input, OCP1, no rootetx\n");
    goto discard;
  }
#endif

  /* Note: In the following, if the src of the DIO is already our parent
   * we first remove the parent from the parent set before calling the
   * ocp fuction (OF). The ocp then compares the DIO just received with
   * the DIO of the remaining parents. This artifact avoids to compare
   * two versions (stored and just received) of the same parent.   
   */
  
#if RPL_SEND_DIS
  if (!etimer_expired(&local_repair_timer) ||
      !etimer_expired(&dis_window_timer)){
#else
  if (!etimer_expired(&local_repair_timer)){
#endif
    rpl_parent_rm(parent);
    if(RPL_DIO_BUF->rank != 0xff && battery == 0){
      PRINTF("[RPL_DIS] DIO input, still in DIS or repair. Consider sender as CANDIDATE parent.\n");
      order = rpl_ocp(RPL_DIO_BUF, rootetx, linketx);
      locparent = rpl_parent_add(RPL_DIO_BUF, &(nbr->ipaddr), RPL_CANDIDATE,
                                 order, linketx, rootetx, stability);
    }
    goto discard;
  }
  
  if(RPL_DIO_BUF->rank == 0xff || battery == 1){
    /* We received a noDIO or DIO from battery operated node*/
    PRINTF("[RPL_DIO] Received NO-DIO or Battery-DIO\n");
    rpl_parent_rm(parent);
  } else if(mydag.state == RPL_NO_DAG){
#if (RPL_DAG_OCP == 0)
    order = 1;
#else
    order = RPL_ORDER_UNKNOWN;
#endif
    PRINTF("[RPL_DIO] DIO input, NO_DAG\n");
  } else {
    //PRINTF("[RPL_DIO] DIO input, DAG\n");
    if (uip_ipaddr_cmp(&(RPL_DIO_BUF->dagID), &(mydag.dagID)) == 1) {
      if ((RPL_DIO_BUF->sequence - mydag.sequence) < 128 ) { 
        /* The DIO sender belongs to a current or subsequent DAG iteration */
        if( parent == NULL ){
          /* new parent same DAG iteration */
          PRINTF("[RPL_DIO] DIO input, new parent with rank %u. Current rank = %u\n", RPL_DIO_BUF->rank, mydag.rank);
          if (RPL_DIO_BUF->rank < mydag.rank ){ 
            order = rpl_ocp(RPL_DIO_BUF, rootetx, linketx);
          } else {
            PRINTF("[RPL_DIO] Discard message as rank is too high.\n");
            goto discard;
          }
        } else {
          /* Existing parent same DAGID */
          /* Possible changes in the parent include rank and metric */
#if (RPL_DAG_OCP == 0)
          if ((RPL_DIO_BUF->rank != (parent->last_dio).rank)
              || (RPL_DIO_BUF->sequence != mydag.sequence)){
#else
          if ((RPL_DIO_BUF->rank != (parent->last_dio).rank)
              || (RPL_DIO_BUF->sequence != mydag.sequence)
              || (rootetx != parent->rootetx)){
#endif
            PRINTF("[RPL_DIO] DIO input, known parent with changes in metrics or rank.\n");
            rpl_parent_rm(parent);
            if ((RPL_DIO_BUF->rank < mydag.rank) ||
                (RPL_DIO_BUF->sequence != mydag.sequence)){
              order = rpl_ocp(RPL_DIO_BUF, rootetx, linketx);
            }
          } else {
            mydag.dio_send_timer.c++;
          }
        }
      } else {
        /* Previous DAG iteration */
        goto discard;
      }
    } else {
      /* The DIO sender belongs to a different DODAG */
      rpl_parent_rm(parent);
      order = rpl_ocp(RPL_DIO_BUF, rootetx, linketx);
    }
  }
    
  parent_pref_old = rpl_parent_lookup_state(RPL_PREFERRED);
  
  if (order == 1) {
    if(parent_pref_old != NULL){
      parent_pref_old->state = RPL_CANDIDATE;
      /* Remove default route*/
      uip_ds6_route_rm(parent_pref_old->route);
      parent_pref_old->route = NULL;
    }
    PRINTF("[RPL_DAG] DIO input, adding PREFERRED: "); PRINT6ADDR(&UIP_IP_BUF->srcipaddr); PRINTF("\n");
    parent_pref_new = rpl_parent_add(RPL_DIO_BUF, &(nbr->ipaddr),
                                     RPL_PREFERRED, order, linketx,
                                     rootetx, stability);
  } else if (order > 0) {
    PRINTF("[RPL_DAG] DIO input, adding CANDIDATE: "); PRINT6ADDR(&UIP_IP_BUF->srcipaddr); PRINTF("\n");
    rpl_parent_add(RPL_DIO_BUF, &(nbr->ipaddr), RPL_CANDIDATE,
                   order, linketx, rootetx, stability);
  }

  if(rpl_parent_count() == 0){
    /* We deleted the last parent */
    PRINTF("[RPL_DAG] DIO input, no parent left. Perform LOCAL REPAIR.\n");
    rpl_local_repair();
    goto discard;
  }

  /* Promote new preferred, in case the node removed was the preferred */
  if(parent_pref_old == NULL && parent_pref_new == NULL) {
    PRINTF("[RPL_DAG] DIO input, removed PREFERRED. Electing another one.\n");
    parent_pref_new = rpl_parent_lookup_order(1);
  }
  
  if (parent_pref_new != NULL){
    /* We have a new preferred */
    PRINTF("[RPL_DAG] DIO input, new PREFERRED\n");
    rpl_new_preferred(parent_pref_new);
  } else {
    parent_pref_new = parent_pref_old;
  }

  /* If DIO from preferred parent, and D flag set, send DAO to it */
  if(uip_ipaddr_cmp(&(parent_pref_new->ipaddr),
                    &(UIP_IP_BUF->srcipaddr))){
    mydag.control = RPL_DIO_BUF->control;
    if((mydag.control & RPL_CONTROL_D) &&
       etimer_expired(&mydag.dao_delay_timer)){
      PRINTF("[RPL_DIO] DIO input, D flag set. Scheduling DAO\n");
      etimer_set(&mydag.dao_delay_timer, random_rand()%(RPL_DAO_LATENCY*CLOCK_SECOND));
      /* TBD DEF_DAO_LATENCY should be divided by a multiple of the DAG rank
         (not defined yet) */
      /* Note: we do not find any usage for the reported flag, hence
       * we do not set entries in daoroutes and connected list to not
       * reported, see RFC... */
    }
  }
     
 discard:       
  uip_len = 0;
  return;
}

#if (RPL_LEAF_ONLY == 0)
void
rpl_dio_output()
{
  //Increment retry count for each existing route
  if(mydag.control & RPL_CONTROL_D){
    for(locdaoroute = rpl_daoroute_table; 
        locdaoroute < rpl_daoroute_table + RPL_DAOROUTE_NB; 
        locdaoroute++) {
      if(locdaoroute->isused) {
        if((locdaoroute->retry_count > RPL_MAX_ROUTE_SOLICIT) &&
           (mydag.rank == RPL_ROOT_RANK)) {
	  PRINTF("[RPL_DAO] Reached retry count limit for route to"); PRINT6ADDR(&locdaoroute->route->ipaddr); PRINTF("\n");
	  rpl_daoroute_rm(locdaoroute);
	} else {
          locdaoroute->retry_count++;
        }
      }
    }
  }
    
  /* periodic sending (via trickle timer)*/
  uip_ext_len = 0;
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = RPL_HOP_LIMIT;

  uip_create_ll_allrouters_maddr(&locipaddr);
  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &locipaddr);
  uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);

  UIP_ICMP_BUF->type = ICMP6_RPL; 
  UIP_ICMP_BUF->icode = ICMP6_RPL_DIO; 
  
  // TBD: D bit shoud depend on mydag.isconsistent
  RPL_DIO_BUF->control = mydag.control;
  RPL_DIO_BUF->sequence = mydag.sequence;
  RPL_DIO_BUF->instanceID = mydag.instanceID;
  RPL_DIO_BUF->rank = mydag.rank;
  uip_ipaddr_copy(&(RPL_DIO_BUF->dagID), &mydag.dagID);

  uip_len = UIP_IPH_LEN + UIP_ICMPH_LEN + RPL_DIO_LEN;

  /* For now we include OCP and possible metric container in every DIO */
  rpl_opt_offset = RPL_DIO_LEN;
  RPL_DIOOPT_HDR_BUF->type = RPL_DIOOPT_OCP;
  RPL_DIOOPT_HDR_BUF->len[0] = 0;
  RPL_DIOOPT_HDR_BUF->len[1] = RPL_DIOOPT_OCP_LEN;  
  RPL_DIOOPT_OCP_BUF->ocp = htons(mydag.ocp);
  uip_len += RPL_DIOOPT_HDR_LEN + RPL_DIOOPT_OCP_LEN;

#if (RPL_DAG_OCP == 1) && (UIP_CONF_ETX == 1)
  locparent = rpl_parent_lookup_state(RPL_PREFERRED);
  rpl_opt_offset += RPL_DIOOPT_HDR_LEN + RPL_DIOOPT_OCP_LEN;
  rpl_metric_offset = 0;
  RPL_DIOOPT_HDR_BUF->type = RPL_DIOOPT_METRIC;
  RPL_DIOOPT_HDR_BUF->len[0] = 0;
  RPL_DIOOPT_HDR_BUF->len[1] = 2*RPL_DIOOPT_METRIC_HDR_LEN +
    RPL_DIOOPT_METRIC_ETX_LEN + RPL_DIOOPT_METRIC_NE_LEN; 
  RPL_DIOOPT_METRIC_HDR_BUF->type  = RPL_DIOOPT_METRIC_ETX;
  RPL_DIOOPT_METRIC_HDR_BUF->flags = RPL_METRIC_FLAGS_G_ADD;
  RPL_DIOOPT_METRIC_HDR_BUF->len   = htons(RPL_DIOOPT_METRIC_ETX_LEN);
  if(locparent != NULL) {
    RPL_DIOOPT_METRIC_ETX_BUF->etx = locparent->rootetx;
  } else {
    RPL_DIOOPT_METRIC_ETX_BUF->etx = 0;
  }
  rpl_metric_offset += RPL_DIOOPT_METRIC_HDR_LEN + RPL_DIOOPT_METRIC_ETX_LEN;
  RPL_DIOOPT_METRIC_HDR_BUF->type  = RPL_DIOOPT_METRIC_NE;
  RPL_DIOOPT_METRIC_HDR_BUF->flags = RPL_METRIC_FLAGS_C;
  RPL_DIOOPT_METRIC_HDR_BUF->len   = htons(RPL_DIOOPT_METRIC_NE_LEN);
  RPL_DIOOPT_METRIC_NE_BUF->flags  = RPL_NE_FLAGS_POW;
  RPL_DIOOPT_METRIC_NE_BUF->energy = 0;

  uip_len += RPL_DIOOPT_HDR_LEN + 2*RPL_DIOOPT_METRIC_HDR_LEN +
    RPL_DIOOPT_METRIC_ETX_LEN + RPL_DIOOPT_METRIC_NE_LEN;
#endif
    
  UIP_IP_BUF->len[0] = ((uip_len - UIP_IPH_LEN) >> 8);
  UIP_IP_BUF->len[1] = ((uip_len - UIP_IPH_LEN) & 0xff);
  
  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();
  
  UIP_STAT(++uip_stat.icmp.sent);
  
  //PRINTF("[RPL_DIO] Sending DIO from "); PRINT6ADDR(&UIP_IP_BUF->srcipaddr); PRINTF(" to "); PRINT6ADDR(&UIP_IP_BUF->destipaddr); PRINTF("\n");
  PRINTF("[RPL_DIO] Sending DIO\n");
  tcpip_ipv6_output();
  
  return;
}
#endif /* RPL_LEAF_ONLY == 0 */

/* output is in clock ticks */
clock_time_t
rpl_dio_send_timer_compute_t(void)
{
  return (clock_time_t)((mydag.dio_send_timer.i.interval / 2) + (uint16_t)random_rand() % (mydag.dio_send_timer.i.interval / 2));
}


/*  the caller has to be sure i_min is set properly in the DAG structure */
void 
rpl_dio_send_timer_init() 
{
  mydag.dio_send_timer.c = 0;
  timer_set(&mydag.dio_send_timer.i, mydag.dio_send_timer.i_min);
  timer_set(&mydag.dio_send_timer.t, rpl_dio_send_timer_compute_t());
  PRINTF("[RPL_DIO] Trickle Timer Imin: %u Imax: %u Init I:%u T:%u\n", mydag.dio_send_timer.i_min, mydag.dio_send_timer.i_max, mydag.dio_send_timer.i.interval, mydag.dio_send_timer.t.interval);
  return;
}

void rpl_new_preferred(rpl_parent * par){
  uint8_t rank_offset;
  rpl_parent *parent;
  
  /* We have a new preferred */
  par->state = RPL_PREFERRED;
#if RPL_DAG_OCP == 0
  rank_offset = 4;
#else
#if RPL_DAG_OCP == 1
  rank_offset = 1;
#endif
#endif
  mydag.rank = (par->last_dio).rank + rank_offset;
  
  uip_create_unspecified(&locipaddr); 
  par->route = uip_ds6_route_add(&locipaddr, 0, &(par->ipaddr), 0,
                                 UIP_DS6_ROUTE_RPLDIO, 0);
  /* Check for DAG change */
  if ((mydag.state == RPL_NO_DAG) ||
      (uip_ipaddr_cmp(&((par->last_dio).dagID), &(mydag.dagID)) == 0) || 
      ((par->last_dio).sequence != mydag.sequence)){
    rpl_dag_new(&(par->last_dio)); //constant ocp for now
  } else{   
    rpl_dio_send_timer_init();
  }

  /* Remove candidate parents that have become invalid because
     of a rank or sequence number change */
  for(parent = mydag.parent_set;
      parent < mydag.parent_set + RPL_PARENT_NB;
      parent++) {
    if ((parent->isused) &&
        (uip_ipaddr_cmp(&(mydag.dagID),
                        &((parent->last_dio).dagID))) &&
        ((((parent->last_dio).rank + rank_offset) > mydag.rank) ||
         (((parent->last_dio).sequence - (par->last_dio).sequence) > 128))) {
      //TBD: put the rank condition in relationship with max_rank_increase
      rpl_parent_rm(parent);
      PRINTF("[RPL_DAG] Removed parent (new preferred)\n");
    }
  }
  /* Schedule an immediate DAO to speed up things*/
  if((mydag.control & RPL_CONTROL_A) &&
     (etimer_expired(&mydag.dao_delay_timer))){
    PRINTF("[RPL_DIO] New preferred, A flag set. Scheduling async DAO\n");
    etimer_set(&mydag.dao_delay_timer, 0);
  }
}

void rpl_local_repair(){
  etimer_set(&local_repair_timer, RPL_REPAIR_INTERVAL*CLOCK_SECOND);
  /* TBD: We have a problem here as we probably already removed the parent */
  rpl_dao_output_all(0, 1, NULL); //NODAOs
  rpl_dag_init();
#if ((RPL_ROOT_ENABLE == 0) && (RPL_LEAF_ONLY == 0)) 
  PRINTF("[RPL_DIO] LOCAL REPAIR: sending NO-DIO (poisoning).\n");
  rpl_dio_output(); /* This is a DIO with rank infinity to poison*/
#endif
}

void
rpl_periodic()
{
  //PRINTF("[RPL_DIO] Periodic, c:%u, i_min:%lu, i_max:%lu, start:%lu, interv=%lu\n",mydag.dio_send_timer.c, mydag.dio_send_timer.i_min,mydag.dio_send_timer.i_max, mydag.dio_send_timer.i.start, mydag.dio_send_timer.i.interval);
  
  /* Initialize (after ip stack) */
  if(mydag.state == RPL_INIT_DAG){
    rpl_dag_init();
  }

  /* DIO send timer T timer */
  if((mydag.state > RPL_NO_DAG) && 
     (mydag.dio_send_timer.c < RPL_DIO_REDUNDANCY_CONSTANT) && 
     (timer_expired(&mydag.dio_send_timer.t))) {
#if RPL_SEND_DIS
    if (etimer_expired(&local_repair_timer) &&
        etimer_expired(&dis_window_timer)){
#else
    if (etimer_expired(&local_repair_timer)){
#endif
      /* Next line ensures we send it maximum once in the interval I */
      mydag.dio_send_timer.c = 0xff;
#if (RPL_LEAF_ONLY == 0)
      rpl_dio_output();
#endif
    }
  }
  
  /* DIO send timer I timer */
  if((mydag.state > RPL_NO_DAG) &&
     (timer_expired(&mydag.dio_send_timer.i))) {
    
    PRINTF("[RPL_DIO] Periodic, c:%u, ",mydag.dio_send_timer.c);
    
    if((mydag.isconsistent) && 
       (mydag.dio_send_timer.i.interval * 2 < mydag.dio_send_timer.i_max)) {
      timer_set(&mydag.dio_send_timer.i, mydag.dio_send_timer.i.interval * 2);
    }
    else {
      timer_restart(&mydag.dio_send_timer.i);
    }
    mydag.dio_send_timer.c = 0;
    timer_set(&mydag.dio_send_timer.t, rpl_dio_send_timer_compute_t());

    PRINTF("I=%u, T=%u\n", mydag.dio_send_timer.i.interval, mydag.dio_send_timer.t.interval);
  }

  /* Sequence Number Timer */
#if RPL_ROOT_ENABLE
  if((mydag.state == RPL_OWN_DAG) &&
     (timer_expired(&mydag.sequence_timer))) {
    mydag.sequence++;
    timer_reset(&mydag.sequence_timer);
  }
#endif
  
  etimer_restart(&rpl_timer_periodic);
  return;
}

#if RPL_DAG_OCP == 0
uint8_t
rpl_ocp0(rpl_dio *dio, uint8_t unused1, uint8_t unused2)
{
  /* Order DIO in argument with respect to the ones of the exisiting
     parents, the main criteria is the rank */
  uint8_t i;
  for(i = 1; i < RPL_PARENT_NB; i++) {
    locparent = rpl_parent_lookup_order(i);
    if(locparent == NULL) {
      return i;
    }
    /* Warning: We test both the grounded flag and the preference in
       one shot */
    if((dio->control & (RPL_CONTROL_G | RPL_CONTROL_PRF)) > 
      (locparent->last_dio.control & (RPL_CONTROL_G | RPL_CONTROL_PRF))) {
      return i;
    } else if((dio->control & (RPL_CONTROL_G | RPL_CONTROL_PRF)) < 
      (locparent->last_dio.control & (RPL_CONTROL_G | RPL_CONTROL_PRF))) {
      continue;
    } 

    if((dio->rank) < locparent->last_dio.rank) {
      return i;
    } else if((dio->rank) > locparent->last_dio.rank) {
      continue;
    }
  }
  return 0;
}
#else
#if (RPL_DAG_OCP == 1) && (UIP_CONF_ETX == 1)
uint8_t
rpl_ocp1(rpl_dio *dio, uint8_t rootetx, uint8_t linketx){
  /* Order DIO in argument with respect to the ones of the exisiting
     parents, the main criteria is the etx */
  uint8_t i;

  if(linketx == 0){
    return RPL_ORDER_UNKNOWN;
  }
  for(i = 1; i < RPL_PARENT_NB; i++) {
    locparent = rpl_parent_lookup_order(i);
    if(locparent == NULL) {
      return i;
    }
    /* Warning: We test both the grounded flag and the preference in
       one shot */
    if((dio->control & (RPL_CONTROL_G | RPL_CONTROL_PRF)) > 
      (locparent->last_dio.control & (RPL_CONTROL_G | RPL_CONTROL_PRF))) {
      return i;
    } else if((dio->control & (RPL_CONTROL_G | RPL_CONTROL_PRF)) < 
      (locparent->last_dio.control & (RPL_CONTROL_G | RPL_CONTROL_PRF))) {
      continue;
    } 

    if(rootetx < locparent->rootetx) {
      return i;
    } else if(rootetx > locparent->rootetx) {
      continue;
    }
  }
  return 0;
}
#endif
#endif

rpl_parent *
rpl_parent_add(rpl_dio *dio, uip_ipaddr_t *ipaddr, uint8_t state,
               uint8_t order, uint8_t linketx, uint8_t rootetx,
               int8_t stability)
{
  rpl_parent* parent_free = NULL;
  /* We look for a free spot, or the worst parent */
  for(locparent = mydag.parent_set; locparent < mydag.parent_set + RPL_PARENT_NB; locparent++) {
    if(locparent->isused == 0) {
      parent_free = locparent;
      break;
    } else {
      if(locparent->order >= order){
        if(parent_free == NULL) {
          parent_free = locparent;
        } else {
          if(locparent->order > parent_free->order)
            parent_free = locparent;
        }
      }
    }
  }
  
  if(parent_free != NULL){
#if UIP_CONF_ETX
    if(etx_register(ipaddr)){
      parent_free->linketx = linketx;
#if (RPL_DAG_OCP == 1)
      parent_free->rootetx = rootetx;
#endif
#endif 
      /* update order of other parents */
      for(locparent = mydag.parent_set;
          locparent < mydag.parent_set + RPL_PARENT_NB;
          locparent++) {
        if(locparent->isused == 1 && locparent->order >= order){
          locparent->order++;
        }
      }
      parent_free->isused = 1;
      uip_ipaddr_copy(&(parent_free->ipaddr), ipaddr);
      memcpy(&parent_free->last_dio, dio, RPL_DIO_LEN);
      parent_free->state = state;
      parent_free->order = order;     
      parent_free->stability = stability;
      parent_free->route = NULL;
#if (RPL_DAG_OCP == 0)
      PRINTF("[RPL_DAG] Added parent: "); PRINT6ADDR(&parent_free->ipaddr); PRINTF("state = %u order = %u stability = %d\n", parent_free->state, parent_free->order, parent_free->stability);
      PRINTF("[RPL_DAG]    Last DIO: control = %x sequence = %u instance ID = %u rank = %u DAG ID = ", parent_free->last_dio.control, parent_free->last_dio.sequence, parent_free->last_dio.instanceID, parent_free->last_dio.rank); PRINT6ADDR(&parent_free->last_dio.dagID); PRINTF("\n");
#endif
      
#if (RPL_DAG_OCP == 1) 
      PRINTF("[RPL_DAG] Added parent: "); PRINT6ADDR(&parent_free->ipaddr); PRINTF("state = %u order = %u stability = %d, etx = %d (%d)\n", parent_free->state, parent_free->order, parent_free->stability, parent_free->rootetx, parent_free->linketx);
      PRINTF("[RPL_DAG]    Last DIO: control = %x sequence = %u instance ID = %u rank = %u DAG ID = ",  parent_free->last_dio.control, parent_free->last_dio.sequence, parent_free->last_dio.instanceID, parent_free->last_dio.rank); PRINT6ADDR(&parent_free->last_dio.dagID); PRINTF("\n");
#endif
      
#if UIP_CONF_ETX     
    } else {
      parent_free = NULL;
      PRINTF("[RPL_DIO] Could not add parent, unable to register etx\n");
    }
#endif
  } else {
    PRINTF("[RPL_DAG] Could not add parent, list full\n");
  }
  return parent_free;
}

rpl_parent *
rpl_parent_lookup_ip(uip_ipaddr_t *ipaddr) 
{
  for(locparent = mydag.parent_set;
      locparent < mydag.parent_set + RPL_PARENT_NB;
      locparent++) {
    if(locparent->isused == 1 &&
       (uip_ipaddr_cmp(ipaddr, &(locparent->ipaddr)) == 1) ){
      return locparent;
    }
  }
  return NULL;
}

rpl_parent *
rpl_parent_lookup_state(uint8_t state) 
{
  for(locparent = mydag.parent_set;
      locparent < mydag.parent_set + RPL_PARENT_NB;
      locparent++) {
    if(locparent->isused == 1 && locparent->state == state){
      return locparent;
    }
  }
  return NULL;
}

rpl_parent *
rpl_parent_lookup_order(uint8_t order) 
{
  for(locparent = mydag.parent_set;
      locparent < mydag.parent_set + RPL_PARENT_NB;
      locparent++) {
    if(locparent->isused == 1 && locparent->order == order){
      return locparent;
    }
  }
  return NULL;
}
 
void
rpl_parent_rm(struct rpl_parent * parent) 
{
  if(parent != NULL) {
    uip_ds6_route_rm(parent->route);
    parent->route = NULL; 
#if UIP_CONF_ETX
    etx_unregister(&parent->ipaddr);
#endif
    parent->isused = 0;
    /* Reorder set */
    for(locparent = mydag.parent_set;
        locparent < mydag.parent_set + RPL_PARENT_NB;
        locparent++) {
      if(locparent->isused == 1 && locparent->order > parent->order){
        locparent->order--;
      }
    }
    PRINTF("[RPL_DAG] Removed parent: "); PRINT6ADDR(&parent->ipaddr); PRINTF("state = %u order = %u stability = %d\n", parent->state, parent->order, parent->stability);
    PRINTF("[RPL_DAG]    Last DIO: control = %x sequence = %u instance ID = %u rank = %u DAG ID = ", parent->last_dio.control, parent->last_dio.sequence, parent->last_dio.instanceID, parent->last_dio.rank); PRINT6ADDR(&parent->last_dio.dagID); PRINTF("\n");
  }
  return;
}

uint8_t
rpl_parent_count(struct rpl_parent * par) 
{
  uint8_t count = 0;
  for(locparent = mydag.parent_set;
      locparent < mydag.parent_set + RPL_PARENT_NB;
      locparent++) {
    if(locparent->isused == 1){
      count++;
    }
  }
  return count;
}

void rpl_child_rm(uip_ipaddr_t *child) 
{
  /* Remove the destination using the child as next hop*/
  for(locdaoroute = rpl_daoroute_table;
      locdaoroute < rpl_daoroute_table + RPL_DAOROUTE_NB;
      locdaoroute++) {
    if((locdaoroute->isused) &&
       (uip_ipaddr_cmp(child, &(locdaoroute->route->nexthop)))) {
      PRINTF("[RPL_DAG] Removing child:"); PRINT6ADDR(child); PRINTF("\n");
      if(mydag.rank == RPL_ROOT_RANK){
        rpl_daoroute_rm(locdaoroute);
      } else {
        locdaoroute->retry_count = (RPL_MAX_ROUTE_SOLICIT + 1);
      }
    }
  }
  return;
}

rpl_daoroute* rpl_daoroute_lookup(uip_ipaddr_t *ipaddr, uint8_t len) 
{
  for(locdaoroute = rpl_daoroute_table; 
      locdaoroute < rpl_daoroute_table + RPL_DAOROUTE_NB; 
      locdaoroute++) {
    if((locdaoroute->isused) &&
       (uip_ipaddr_prefixcmp(ipaddr, &locdaoroute->route->ipaddr, len)) &&
       (locdaoroute->route->len == len)) {
      return locdaoroute;
    }
  }
  return NULL;
}

rpl_daoroute* 
rpl_daoroute_add(uip_ipaddr_t *ipaddr, uint8_t len,  uip_ipaddr_t *nexthop, uint8_t metric, unsigned long lifetime, uint8_t rank, uint32_t routetag, uint16_t sequence) 
{
  if(lifetime == 0xffffffff)
    lifetime = 0; //Indicates infinity to uip_ds6_route add
  locroute = uip_ds6_route_add(ipaddr, len, nexthop, metric, UIP_DS6_ROUTE_RPLDAO, lifetime);
  if(locroute != NULL) {
    for(locdaoroute = rpl_daoroute_table; 
        locdaoroute < rpl_daoroute_table + RPL_DAOROUTE_NB; 
        locdaoroute++) {
      if(!locdaoroute->isused) {
        locdaoroute->isused = 1;
        locdaoroute->rank = rank;
        locdaoroute->routetag = routetag;
        locdaoroute->route = locroute;
	locdaoroute->DAOsequence = sequence;
	locdaoroute->retry_count = 0;
        PRINTF("[RPL_DAO] Adding DAO route, routetag = %u, rank = %u, DAOsequence = %u, retry count = %u, route pointer = %p\n", locdaoroute->routetag, locdaoroute->rank, locdaoroute->DAOsequence, locdaoroute->retry_count, locdaoroute->route);
        return locdaoroute;
      }
    }
  } else {
    PRINTF("[RPL_DAO] Could not add DAO route, list full or routing table full\n");
  }
  return NULL;
}

void
rpl_daoroute_rm(rpl_daoroute* daoroute) 
{
  if(daoroute->isused > 0){
    uip_ds6_route_rm(daoroute->route);
    daoroute->route = NULL;
    daoroute->isused = 0;
    PRINTF("[RPL_DAO] Removing DAO route for "); PRINT6ADDR(&daoroute->route->ipaddr); PRINTF("\n");
  }
  return;
}

#if DEBUG
void 
rpl_parent_print(void) 
{
  PRINTF("[RPL_PRINT] Parent List dump, max entries = %u\n", RPL_PARENT_NB);
  for(locparent = mydag.parent_set;
      locparent < mydag.parent_set + RPL_PARENT_NB;
      locparent++) {
    PRINTF("[RPL_PRINT] IP address: ");
    PRINT6ADDR(&locparent->ipaddr);
    PRINTF(" used = %u state = %u order = %u stability = %d\n[RPL_PRINT]    Last DIO: control = %x sequence = %u instance ID = %u rank = %u DAG ID = ", 
           locparent->isused, locparent->state, locparent->order, 
           locparent->stability, locparent->last_dio.control, locparent->last_dio.sequence, 
           locparent->last_dio.instanceID, locparent->last_dio.rank);
    PRINT6ADDR(&locparent->last_dio.dagID);
    PRINTF("\n");
  }
  PRINTF("\n");
  return;
}

void
rpl_daoroute_print(void) 
{
  PRINTF("[RPL_PRINT] DAO routes dump, max entries = %u\n", RPL_DAOROUTE_NB);
  for(locdaoroute = rpl_daoroute_table; 
      locdaoroute < rpl_daoroute_table + RPL_DAOROUTE_NB; 
      locdaoroute++) {
    PRINTF("[RPL_PRINT] used = %u, routetag = %u, rank = %u, route pointer = %p ",
           locdaoroute->isused, locdaoroute->routetag, locdaoroute->rank, locdaoroute->route);
    if(locdaoroute->route != NULL) {
      PRINTF("route = ");
      PRINT6ADDR(&locdaoroute->route->ipaddr);
      PRINTF("/ %u, via", locdaoroute->route->len);
      PRINT6ADDR(&locdaoroute->route->nexthop);
      PRINTF("metric = %u\n", locdaoroute->route->metric);
    } else {
      PRINTF("null route\n");
    }
  }
  return;
}

void
rpl_dag_print(void)
{
  PRINTF("[RPL_PRINT] DAG state %u instance ID %u dag ID", mydag.state, mydag.instanceID);
  PRINT6ADDR(&mydag.dagID);
  PRINTF("sequence %u ocp %u\n[RPL_PRINT]   Grounded %u DAO supported %u preference %u my rank %u consistency %u\n",
    mydag.sequence, mydag.ocp, (mydag.control & RPL_CONTROL_G) >> 7, (mydag.control & RPL_CONTROL_A) >> 6, 
    (mydag.control & RPL_CONTROL_PRF), mydag.rank , mydag.isconsistent);
  return;
}

#endif /* DEBUG */
#endif
/* UIP_CONF_ROUTER & UIP_CONF_ROUTER_RPL & UIP_CONF_ICMP6 & UIP_CONF_ND6_API */
/** @} */
