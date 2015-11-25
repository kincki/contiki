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
 * $Id: rimeroute.h,v 1.1 2009/04/06 13:11:25 nvt-se Exp $
 */

/**
 * \defgroup rimeroute Rimeroute
 * 
 * The rimeroute implements a uip_router module using Rime. It uses the
 * route discovery protocol offered by Rime.
 *
 * To be able to reach a network outside the yours, you have to tell rimeroute
 * which is the address of your network and who is the gateway that connects
 * your net to the other networks (e.g., Internet).
 * So if the destination of a packet is inside the network, rimeroute will look
 * for a way to reach it, else it will look for a way towards the gateway.
 *
 * A custom route can also be specified to reach a particular network.
 * To do this, you can add the route to the uip6-route-table. These routes
 * will be used only if they are more specific than the given network, else 
 * they will be used only for packets that are addressed to outside the network.
 * That is, you can think as if in the uip6 route table there is a record
 * with your network as the network address. When there is a match with this
 * record, the Rime discovery protocol will be used, else the next hop will be
 * determined by the route spcified in the uip6 route table.
 *
 * @{
 */

/**
 * \file
 *         Declarations for rimeroute.
 *
 * \author Nicolas Tsiftes <nvt@sics.se>
 */

#ifndef RIMEROUTE_H
#define RIMEROUTE_H

#include "net/uip.h"

extern const struct uip_router rimeroute;

void rimeroute_set_net(uip_ipaddr_t *addr, u8_t prefixlen);

void rimeroute_set_gateway(rimeaddr_t *gw);

#endif /*!RIMEROUTE_H*/
