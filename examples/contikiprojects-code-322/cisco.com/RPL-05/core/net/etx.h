/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *         ETX Module (.h)
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

#ifndef ETX_H
#define ETX_H

#include "contiki.h"
#include "contiki-net.h"

#if (UIP_CONF_ETX & UIP_CONF_ICMP6)

/*------------------------------------------------------------------*/
/** @{ \name ETX Configuration Options */
/*------------------------------------------------------------------*/

/** The maximum number of processes that can listen to the ETX
    module */ 
#ifdef ETX_CONF_CONNS
#define ETX_CONNS ETX_CONF_CONNS
#else 
#define ETX_CONNS 1
#endif

/** The maximum number of neighbors that can be register for ETX
    computation */
#ifdef ETX_CONF_NBRS
#define ETX_NBRS ETX_CONF_NBRS
#else 
#define ETX_NBRS 10
#endif

/** The number of probe sent for a single ETX computation */
#ifndef ETX_CONF_PROBE_NB
#define ETX_PROBE_NB 5
#else
#define ETX_PROBE_NB ETX_CONF_PROBE_NB
#endif

/** The time between two ETX computations */
#ifndef ETX_CONF_PROBE_INTERVAL
#define ETX_PROBE_INTERVAL 120  /*in seconds */
#else
#define ETX_PROBE_INTERVAL ETX_CONF_PROBE_INTERVAL
#endif

#define ETX_EVENT_NEW_VALUE  0x71

#define ETX_PERIOD  1 /*in seconds, periodic processing */
/** @}*/

/** A process listening to the ETX module*/
typedef struct etx_listener {
  struct process *p;
} etx_listener;

/** A neigbor registered for ETX computation */
typedef struct etx_nbr {
  uint8_t isused;
  uip_ipaddr_t ipaddr; /**< The IP address of the neighbor */
  uint8_t count;       /**< The number of echo request sent */
  uint8_t success;     /**< The number of echo reply received */
  struct stimer timer; /**< The time before the next ETX compution */ 
} etx_nbr;

/** Register a process interested in using the ETX module */
etx_listener * etx_listener_new();
/** Unregister a listening process */
void etx_listener_rm();
/** Call process listening to ETX module */
void etx_listeners_call(u8_t event, void *data);
/** Process events
 * (mostly the reception of an echo reply used for ETX computation)*/
void etx_event_handler(process_event_t ev, process_data_t data);
/** Register a neighbor for ETX computation */
uint8_t etx_register(uip_ipaddr_t * ipaddr);
/** Schedule unregistering of neighbor for ETX computation */
void    etx_unregister(uip_ipaddr_t * ipaddr);
/** Periodic processing
 *  (send echo request to measure etx, callback neighbor, etc)
 */
void etx_periodic();

PROCESS_NAME(etx_process);

#endif /* UIP_CONF_ETX & UIP_CONF_ICMP6 */
#endif /* __ETX_H__ */
/** @} */
