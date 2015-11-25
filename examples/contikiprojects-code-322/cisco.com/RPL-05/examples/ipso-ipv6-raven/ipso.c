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
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#define set_lladdr(a, addr0,addr1,addr2,addr3,addr4,addr5,addr6,addr7) do { \
    ((a)->addr[0]) = addr0;                                      \
    ((a)->addr[1]) = addr1;                                      \
    ((a)->addr[2]) = addr2;                                      \
    ((a)->addr[3]) = addr3;                                      \
    ((a)->addr[4]) = addr4;                                      \
    ((a)->addr[5]) = addr5;                                      \
    ((a)->addr[6]) = addr6;                                      \
    ((a)->addr[7]) = addr7;                                      \
  } while(0)

/*---------------------------------------------------------------------------*/
PROCESS(init_stack_process, "Init stack process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(init_stack_process, ev, data)
{
  uip_ipaddr_t ipaddr1;
  uip_ipaddr_t ipaddr2;
  uip_lladdr_t lladdr;

  PROCESS_BEGIN();
  
#if UIP_CONF_ROUTER

  /* set the global address (corresponding to MAC address set in
     contiki-raven-main.c) */
  uip_ip6addr(&ipaddr1, 0xaaaa,0,0,0,0x0011,0x11ff,0xfe11,0x1111);
  //uip_ip6addr(&ipaddr1, 0xaaaa,0,0,0,0x00aa,0xaaff,0xfeaa,0xaaaa);
  uip_ds6_addr_add(&ipaddr1, 0,  ADDR_MANUAL);

  /* add static route to PC with 802.15.4 USB dongle (for
     DAG root only) */
  /*uip_ip6addr(&ipaddr1, 0xaaaa,0,0,0,0,0,0,0x0001);
    uip_ip6addr(&ipaddr2, 0xfe80,0,0,0,0x0012,0x13ff,0xfe14,0x1516);
    uip_ds6_route_add(&ipaddr1, 128,  &ipaddr2, 0, 0, 0);*/
#endif
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
AUTOSTART_PROCESSES(&init_stack_process);
/*---------------------------------------------------------------------------*/
