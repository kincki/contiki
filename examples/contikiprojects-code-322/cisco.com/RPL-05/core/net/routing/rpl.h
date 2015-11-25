/**
 * \addtogroup rpl
 * @{
 */

/**
 * \file
 *         IPv6 Routing Protocol for Low power and Lossy Networks (.h)
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

#ifndef RPL_H
#define RPL_H

#include "contiki-net.h"

#if (UIP_CONF_ROUTER & UIP_CONF_ROUTER_RPL & UIP_CONF_ICMP6 & UIP_CONF_ND6_API) 

/*------------------------------------------------------------------*/
/** @{ \name RPL Configuration Options */
/*------------------------------------------------------------------*/

/** Do I form my own DAG when no better DAG is available */
#ifndef RPL_CONF_ROOT_ENABLE
#define RPL_ROOT_ENABLE 0
#else
#define RPL_ROOT_ENABLE RPL_CONF_ROOT_ENABLE
#endif

/** Do I want to act only as a leaf */
#ifndef RPL_CONF_LEAF_ONLY
#define RPL_LEAF_ONLY 0
#else
#define RPL_LEAF_ONLY RPL_CONF_LEAF_ONLY
#endif

/** Do I send DIS when without parents */
#ifndef RPL_CONF_SEND_DIS
#define RPL_SEND_DIS 0
#else
#define RPL_SEND_DIS RPL_CONF_SEND_DIS
#endif

/** Maximum number of DAO routes that can be stored*/
#ifndef RPL_CONF_DAOROUTE_NB 
#define RPL_DAOROUTE_NB  4
#else
#define RPL_DAOROUTE_NB  RPL_CONF_DAOROUTE_NB
#endif

/** Maximum number of RPL parents */
#ifndef RPL_CONF_PARENT_NBU
#define RPL_PARENT_NB 2
#else
#define RPL_PARENT_NB RPL_CONF_PARENT_NBU
#endif

/** Instance ID of the application */
#ifndef RPL_CONF_INSTANCE_ID
#define RPL_INSTANCE_ID 0 /*This is RPL_DEFAULT_INSTANCE */
#else
#define RPL_INSTANCE_ID RPL_CONF_INSTANCE_ID
#endif

/** DAG control flag when root */
#ifndef RPL_CONF_DAG_CONTROL
#define RPL_DAG_CONTROL 0
#else
#define RPL_DAG_CONTROL RPL_CONF_DAG_CONTROL
#endif

/** The (only) OCP that we support */
#ifndef RPL_CONF_DAG_OCP
#define RPL_DAG_OCP 0
#else
#define RPL_DAG_OCP RPL_CONF_DAG_OCP
#endif
/** @} */

/*------------------------------------------------------------------*/
/** @{ \name RPL Defines */
/*------------------------------------------------------------------*/
#define RPL_ROOT_RANK               1
#define RPL_DAO_LATENCY             1   /* s, TBD */
/* Note that 2^(Imin+Idoublings) * CLOCK_CONF_SECOND/1000 is limited
   by the size of clock_time_t */ 
#define RPL_DIO_INTERVAL_MIN        10  /* TBD */
#define RPL_DIO_INTERVAL_DOUBLINGS  8   /* TBD */
#define RPL_DIO_REDUNDANCY_CONSTANT 5   /* TBD, for trickle timer */
/** Interval to wait for DIOs after DIS sending */
#define RPL_DIS_INTERVAL            10
/** Safety interval for local repair*/
#define RPL_REPAIR_INTERVAL         10 
/** The max number of DIO with D bit set sent to solicit a DAO
 * response from a child */
#define RPL_MAX_ROUTE_SOLICIT       3

#define RPL_DAO_ZERO_LIFETIME       0x00000000
#define RPL_DAO_INFINITE_LIFETIME   0xFFFFFFFF

#define RPL_HOP_LIMIT               255
#define RPL_ORDER_UNKNOWN           0xFF

/** Periodic processing in seconds */
#define RPL_PERIOD            1.1   /*(avoid integers)*/ 
/** When root, increment sequence number periodically (s) */
#define RPL_SEQUENCE_INTERVAL 100

/** \name ETX/OCP1 related defines */
/** @{ */
#define RPL_ETX_VAR           0.2 /* Max variation for current value that
                                     triggers no change */
#define RPL_ETX_MIN           10  /* This correspond to ETX=1 */
#define RPL_ETX_MAX           30  /* This correspond to the max ETX
                                     allowed for a link */
/** @} */

/** \name Possible states for a parent */
/** @{ */
#define RPL_PREFERRED   1
#define RPL_CANDIDATE   2
/** @} */

/** \name Possible states for a DAG */
/** @{ */
#define RPL_INIT_DAG    0
#define RPL_NO_DAG      1
#define RPL_OWN_DAG     2
#define RPL_DAG         3
/** @} */

/** \name RPL control flag mask */
/** @{ */
#define RPL_CONTROL_G    0x80
#define RPL_CONTROL_D    0x40
#define RPL_CONTROL_A    0x20
#define RPL_CONTROL_PRF  0x07
/** @} */

/** \name RPL Metric flags */
/** @{ */
#define RPL_METRIC_FLAGS_G_ADD 0x10  /* Global, aggregated, additive metric */
#define RPL_METRIC_FLAGS_C     0x01  /* Local constraint */

#define RPL_NE_FLAGS_BAT  0x02       /* Constraint is to exclude battery
                                        operated nodes */
#define RPL_NE_FLAGS_POW  0x00       /* Constraint is to include main 
                                        powered nodes */
/** @} */

/** \name Header/option length and types */
/** @{ */
#define RPL_DIO_LEN              20  /* DIO base (in bytes) */
#define RPL_DAO_LEN              14  /* Without prefix and RRstack */
#define RPL_DIOOPT_HDR_LEN        3
#define RPL_DIOOPT_OCP_LEN        2
#define RPL_DIOOPT_METRIC_HDR_LEN 4
#define RPL_DIOOPT_METRIC_NE_LEN  2
#define RPL_DIOOPT_METRIC_ETX_LEN 1

#define RPL_DIOOPT_PADN              1
#define RPL_DIOOPT_METRIC            2
#define RPL_DIOOPT_PREFIX            3
#define RPL_DIOOPT_TIMER             4
#define RPL_DIOOPT_OCP               5
#define RPL_DIOOPT_METRIC_NE         2
#define RPL_DIOOPT_METRIC_ETX        7
#define RPL_DAOOPT_NONE              0
#define RPL_DAOOPT_RANK              5
#define RPL_DAOOPT_LIFETIME          6
#define RPL_DAOOPT_ROUTETAG          7
#define RPL_DAOOPT_PREFIX            8
/** @} */
/** @} */

/**
 * \name Trickle
 * @{
 */
/** \brief Trickle Timer */
typedef struct trickle_timer{
  uint8_t c;          /**< the redundancy counter */
  clock_time_t i_min; /**< the minimum communication interval */
  clock_time_t i_max; /**< the maximum communication interval */
  struct timer i;     /**< the length of the communication interval */
  struct timer t;     /**< the timer */
} trickle_timer;
/** @} */

/**
 * \name RPL Structure Definitions
 * TBD link to RFC
 * @{
 */
/** Base DAG Information Option (DIO)*/
typedef struct rpl_dio {
  uint8_t control;       /**< DAG grounded (G), DAO trigger (D) &
                          * supported (A), DAG preference (Prf 3 bit)
                          * -> GDA00Prf*/
  uint8_t sequence;      /**< the DAG sequence number */
  uint8_t instanceID;    /**< topology instance associated with the DAG */
  uint8_t rank;          /**< the DAG rank of the node */
  uip_ipaddr_t dagID;    /**< the DAG ID (ipv6 address of the root) */
} rpl_dio;

/** The DIO option header */
typedef struct rpl_dioopt_hdr {
  uint8_t type;
  uint8_t len[2];
} rpl_dioopt_hdr;

/** The DAG ocp option */
typedef struct rpl_dioopt_ocp{
  uint16_t ocp;
} rpl_dioopt_ocp;

/** The Metric/Constraints common header */
typedef struct rpl_dioopt_metric_hdr{
  uint8_t type;
  uint8_t flags;
  uint16_t len;
} rpl_dioopt_metric_hdr;

/** The Node Energy Metric/Constraints */
typedef struct rpl_dioopt_metric_ne{
  uint8_t flags;
  uint8_t energy;
} rpl_dioopt_metric_ne;

/** The ETX Metric */
typedef struct rpl_dioopt_metric_etx{
  uint8_t etx;
} rpl_dioopt_metric_etx;

/** Destination Advertisement Option (DAO) */
typedef struct rpl_dao{
  uint16_t DAOsequence;   /**< a sequence nb set by prefix owner */
  uint8_t instanceID;     /**< topology instance associated with the DAG */
  uint8_t rank;           /**< rank of originator */
  uint32_t lifetime;      /**< lifetime of DAO route */
  uint32_t routetag;      /**< routetag (unused) */
  uint8_t preflen;        /**< prefix length */
  uint8_t rrcount;        /**< Reverse Route stack count */
  uip_ipaddr_t prefix;    /**< prefix (full Ipv6 address).
                             TBD: support variable length prefixes */
} rpl_dao;

/** A candidate parent of the node */
typedef struct rpl_parent {
  uint8_t isused;
  uip_ipaddr_t ipaddr;/**< IPv6 address of the parent */
  rpl_dio last_dio;   /**< the last DIO sent by the parent */
  uint8_t state;      /**< preferred, candidate */
  uint8_t order;      /**< as defined by OCP */
  uint8_t stability;  /**< statistical info on link with parent */
#if UIP_CONF_ETX
  uint8_t linketx;    /**< ETX to parent (can be used as stability
                        measure)*/
#if RPL_DAG_OCP == 1
  uint8_t rootetx;    /**< ETX to root of the DODAG */
#endif
#endif
  /**< the default route using the parent as next-hop */
  uip_ds6_route* route;
} rpl_parent;

//TBD change dag ID type from IP address to specific type
/** A Directed Acyclic Graph (DAG) */
typedef struct rpl_dag {
  uint8_t instanceID;    /**< topology instance associated with the DAG */
  uip_ipaddr_t dagID;    /**< the DAG ID (ipv6 address of the root) */
  uint8_t sequence;      /**< the DAG sequence number */
  uint16_t ocp;          /**< the DAG Objective Code Point (for now) */
  uint8_t control;       /**< Grounded, preference, DAO trigger & supported */
  uint8_t rank;          /**< the DAG rank of the node */
  uint8_t state;         /**< the DAG state */
  uint16_t DAOsequence;  /**< the DAO sequence for our connected prefixes */
  uint8_t isconsistent;
  rpl_parent parent_set[RPL_PARENT_NB]; /**< parent/neighbor set */
  trickle_timer dio_send_timer;  /**< Timer to gouvern the sending of DIO*/
  struct etimer dao_delay_timer; /**< Timer to gouvern the sending of DAO*/
#if RPL_ROOT_ENABLE
  struct timer sequence_timer;   /**< Timer to increment the sequence nb */
#endif
} rpl_dag;

/** A route to a prefix learned via a DAO.
 * We do not store the full DAOs (space constraint): some part
 * of the DAO info is stored in the routing table (e.g. prefix), 
 * some part in the structure below (to keep the routing table
 * protocol agnostic). The two structures are linked by the "route"
 * pointer */
typedef struct rpl_daoroute {
  uint8_t isused;
  uip_ds6_route* route;
  uint8_t rank;
  uint8_t routetag;
  uint16_t DAOsequence;
  uint8_t retry_count;
} rpl_daoroute;

/** @} */


/*------------------------------------------------------------------*/
/** @{ \name RPL Functions */
/*------------------------------------------------------------------*/

/** Initialisation of our own DAG */
void rpl_dag_init();

/** The event handler for RPL */
void rpl_event_handler(process_event_t ev, process_data_t data);

/** Periodic check of data structures and timers*/
void rpl_periodic(void);

/** Use DIO information to initialize a new DAG structure */
void rpl_dag_new(rpl_dio *dio);

/** Print DAG info */
void rpl_dag_print(void);

/** Establish a new preferred parent */
void rpl_new_preferred(rpl_parent * par);

/** Perform Local Repair */
void rpl_local_repair();

/** OCP0 function. Takes in input one candidate parent DIO. 
 * Returns the target "order" in the parent ordered list.
 * Main criteria is rank.
 */
uint8_t rpl_ocp0(rpl_dio *dio, uint8_t unused1, uint8_t unused2);

/** OCP1 function. Takes in input one candidate parent DIO. 
 * Returns the target "order" in the parent ordered list
 * Main criteria is ETX
 */
uint8_t rpl_ocp1(rpl_dio *dio, uint8_t rootetx, uint8_t linketx);

/** Compute random value for DIO trickle timer  */
clock_time_t rpl_dio_send_timer_compute_t(void);

/** Init DIO trickle timer */
void rpl_dio_send_timer_init();

/** \name Input output function */
/** @{*/
/** Process a received DIS */
void rpl_dis_input();
/** Send a DIS */
void rpl_dis_output();
/** Process a received DAO */
void rpl_dao_input();
/** Send our prefixes/addresses/dao routes to our preferred parent
 * - the 'isdao' flag indicates whether this is a DAO or a NODAO
 * - 'connected' is 0 if we don't send our addresses and connected prefixes 
 * - 'ipaddr' is NULL we send all dao routes, it is the address of a child
 * if we send a NODAO only for the routes learnt from this child
 */
void rpl_dao_output_all(uint8_t isdao, uint8_t connected, uip_ipaddr_t *ipaddr);
/** Send a DAO to the preferred parent with the given fields */
void rpl_dao_output(uint16_t DAOsequence, uint8_t rank, uint32_t
                    lifetime, uint8_t preflen, uip_ipaddr_t *prefix);
/** Process a received DIO */
void rpl_dio_input();
/** Send a DIO */
void rpl_dio_output();
/** @} */

/** \name Functions handling the parent and DAO route set */
/** @{ */
rpl_parent *
rpl_parent_add(rpl_dio *dio, uip_ipaddr_t *ipaddr, uint8_t state,
               uint8_t order, uint8_t linketx, uint8_t rootetx,
               int8_t stability);
rpl_parent * rpl_parent_lookup_ip(uip_ipaddr_t *ipaddr);
rpl_parent * rpl_parent_lookup_state(uint8_t state);
rpl_parent * rpl_parent_lookup_order(uint8_t order);
void rpl_parent_rm(rpl_parent * par);
uint8_t rpl_parent_count();
void rpl_parent_print(void);
void rpl_child_rm(uip_ipaddr_t *child);

rpl_daoroute* rpl_daoroute_add(uip_ipaddr_t *ipaddr, uint8_t len,  uip_ipaddr_t *nexthop, uint8_t metric, unsigned long lifetime, uint8_t rank, uint32_t routetag, uint16_t sequence);
void rpl_daoroute_rm(rpl_daoroute* daoroute);
rpl_daoroute* rpl_daoroute_lookup(uip_ipaddr_t *ipaddr, uint8_t len);
void rpl_daoroute_print(void);
/** @} */
/** @} */

#define rpl_dagid_cmp(a, b) (memcmp(a, b, 16) == 0)
PROCESS_NAME(rpl_process);
#endif
/* UIP_CONF_ROUTER & UIP_CONF_ROUTER_RPL & UIP_CONF_ICMP6 & UIP_CONF_ND6_API */
#endif /* __RPL_H__ */
/** @} */
