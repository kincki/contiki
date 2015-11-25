/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 * $Id: uip-over-mesh.c,v 1.15 2009/11/08 19:40:16 adamdunkels Exp $
 */

/**
 * \file
 *         Code for tunnelling uIP packets over the Rime mesh routing module
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include <stdio.h>

#include "net/hc.h"
#include "net/uip-fw.h"
#include "net/uip-over-mesh.h"
#include "net/rime/route-discovery.h"
#include "net/rime/route.h"
#include "net/rime/trickle.h"

#define ROUTE_TIMEOUT CLOCK_SECOND * 4

static struct queuebuf *queued_packet;
static rimeaddr_t queued_receiver;

 /* Connection for route discovery: */
static struct route_discovery_conn route_discovery;

/* Connection for sending data packets to the next hop node: */
static struct unicast_conn dataconn;

/* Connection for sending gateway announcement message to the entire
   network: */
static struct trickle_conn gateway_announce_conn;

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define BUF ((struct uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])

//DY : uncomment for static routing
#define TCPIP_HDR(p) ((struct uip_tcpip_hdr *)p)

static struct uip_fw_netif *gw_netif;
static rimeaddr_t gateway;
static uip_ipaddr_t netaddr, netmask;

static uint8_t is_gateway;

//DY
/* Structures and definitions. */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20
#define TCP_CTL 0x3f

#define ROUTE_LENGTH 6

#define RIMEADDR(a, b) (((a) << 8) + (b))

static uint16_t route_list_initalizer[ROUTE_LENGTH] =
  {
    /* this is the address of the sentilla gateway */
    //RIMEADDR(4, 136),

    RIMEADDR(0, 78),
    RIMEADDR(0, 81),
    RIMEADDR(192, 246),
    RIMEADDR(0, 53),
    RIMEADDR(0, 47),
    /*        RIMEADDR(192, 246), this is the address of the one without a number on it */
  };
static rimeaddr_t route_list[ROUTE_LENGTH];

static void
init_lookup(void)
{
  int i;
  for(i = 0; i < ROUTE_LENGTH; ++i) {
    memcpy(&route_list[i], &route_list_initalizer[i], sizeof(rimeaddr_t));
  }
}

static rimeaddr_t *
lookup(rimeaddr_t const *dest)
{
  int i;
  int node_addr_index, dest_index;

  node_addr_index = dest_index = 0;

  /* Find ourself in the list */
  for(i = 0; i < ROUTE_LENGTH; ++i) {
//    printf("Compare myself %d.%d with %d.%d\n", route_list[i].u8[0], route_list[i].u8[1],
//        rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
    if(rimeaddr_cmp(&route_list[i], &rimeaddr_node_addr)) {
//      printf("My index is %d\n", i);
      node_addr_index = i;
      break;
    }
  }
  if(i == ROUTE_LENGTH) {
//    printf("didn't find myself in the list\n");
    return NULL;
  }

  /* Find destination in the list */
  for(i = 0; i < ROUTE_LENGTH; ++i) {
    if(rimeaddr_cmp(&route_list[i], dest)) {
//      printf("The index for %d.%d is %d\n", dest->u8[0], dest->u8[1], i);
      dest_index = i;
      break;
    }
  }
  if(i == ROUTE_LENGTH) {
    return &route_list[node_addr_index - 1];
  }
  if(dest_index < node_addr_index) {
    return &route_list[node_addr_index - 1];
  } else {
    return &route_list[node_addr_index + 1];
  }

}
/////////////////

/*---------------------------------------------------------------------------*/
static void
recv_data(struct unicast_conn *c, const rimeaddr_t *from)
{
  //DY
  //struct route_entry *e;
  rimeaddr_t source;

  uip_len = packetbuf_copyto(&uip_buf[UIP_LLH_LEN]);

  source.u8[0] = BUF->srcipaddr.u8[2];
  source.u8[1] = BUF->srcipaddr.u8[3];

  //DY
  if((BUF->flags & TCP_CTL) == TCP_SYN) {
    rime_mac->off(1);
  } else if((BUF->flags & TCP_CTL) == (TCP_FIN + TCP_ACK)) {
    if(!is_gateway) {
      rime_mac->on();
    }
  }

  //DY - comment here for static routing
//  e = route_lookup(from);
//  if(e == NULL) {
//    route_add(&source, from, 10, 0);
//  } else {
//    route_refresh(e);
//  }

  /*  uip_len = hc_inflate(&uip_buf[UIP_LLH_LEN], uip_len);*/

  PRINTF("uip-over-mesh: %d.%d: recv_data with len %d\n",
	 rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], uip_len);
  tcpip_input();
}
/*---------------------------------------------------------------------------*/
static void
send_data(rimeaddr_t *next)
{
  PRINTF("uip-over-mesh: %d.%d: send_data with len %d\n",
	 rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
	 packetbuf_totlen());
  unicast_send(&dataconn, next);
}
/*---------------------------------------------------------------------------*/
static void
new_route(struct route_discovery_conn *c, const rimeaddr_t *to)
{
  //DY
  //struct route_entry *rt;

  if(queued_packet) {
    PRINTF("uip-over-mesh: new route, sending queued packet\n");

    queuebuf_to_packetbuf(queued_packet);
    queuebuf_free(queued_packet);
    queued_packet = NULL;

    //DY - comment here (including if) for static routing
//    rt = route_lookup(&queued_receiver);
//    if(rt) {
//      route_decay(rt);
//      send_data(&queued_receiver);
//    }
  }
}
/*---------------------------------------------------------------------------*/
static void
timedout(struct route_discovery_conn *c)
{
  PRINTF("uip-over-mesh: packet timed out\n");
  if(queued_packet) {
    PRINTF("uip-over-mesh: freeing queued packet\n");
    queuebuf_free(queued_packet);
    queued_packet = NULL;
  }
}
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks data_callbacks = { recv_data };
static const struct route_discovery_callbacks rdc = { new_route, timedout };
/*---------------------------------------------------------------------------*/
struct gateway_msg {
  rimeaddr_t gateway;
};

static void
gateway_announce_recv(struct trickle_conn *c)
{
  struct gateway_msg *msg;
  msg = packetbuf_dataptr();
  PRINTF("%d.%d: gateway message: %d.%d\n",
	 rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
	 msg->gateway.u8[0], msg->gateway.u8[1]);

  if(!is_gateway) {
    uip_over_mesh_set_gateway(&msg->gateway);
  }

}
/*---------------------------------------------------------------------------*/
void
uip_over_mesh_make_announced_gateway(void)
{
  struct gateway_msg msg;
  /* Make this node the gateway node, unless it already is the
     gateway. */
  if(!is_gateway) {
    PRINTF("%d.%d: making myself the gateway\n",
	   rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
    uip_over_mesh_set_gateway(&rimeaddr_node_addr);
    rimeaddr_copy(&(msg.gateway), &rimeaddr_node_addr);
    packetbuf_copyfrom(&msg, sizeof(struct gateway_msg));
    trickle_send(&gateway_announce_conn);
    is_gateway = 1;
  }
}
const static struct trickle_callbacks trickle_call = {gateway_announce_recv};
/*---------------------------------------------------------------------------*/
void
uip_over_mesh_init(u16_t channels)
{

  PRINTF("Our address is %d.%d (%d.%d.%d.%d)\n",
	 rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
	 uip_hostaddr.u8[0], uip_hostaddr.u8[1],
	 uip_hostaddr.u8[2], uip_hostaddr.u8[3]);

  //DY - inits static routing, uncomment this and comment route_discovery_open for dynamic routing
  init_lookup();

  unicast_open(&dataconn, channels, &data_callbacks);

//DY
//  route_discovery_open(&route_discovery, CLOCK_SECOND / 4,
//		       channels + 1, &rdc);
  trickle_open(&gateway_announce_conn, CLOCK_SECOND * 4, channels + 3,
	       &trickle_call);

  /* Set lifetime to 10 seconds for non-refreshed routes. */
  route_set_lifetime(30);
}
/*---------------------------------------------------------------------------*/
u8_t
uip_over_mesh_send(void)
{
  rimeaddr_t receiver;
  struct route_entry *rt;

  /* This function is called by the uip-fw module to send out an IP
     packet. We try to send the IP packet to the next hop route, or we
     queue the packet and send out a route request for the final
     receiver of the packet. */

  /* Packets destined to this network is sent using mesh, whereas
     packets destined to a network outside this network is sent towards
     the gateway node. */

  if(uip_ipaddr_maskcmp(&BUF->destipaddr, &netaddr, &netmask)) {
    receiver.u8[0] = BUF->destipaddr.u8[2];
    receiver.u8[1] = BUF->destipaddr.u8[3];
  } else {
    if(rimeaddr_cmp(&gateway, &rimeaddr_node_addr)) {
      PRINTF("uip_over_mesh_send: I am gateway, packet to %d.%d.%d.%d to local interface\n",
	     uip_ipaddr_to_quad(&BUF->destipaddr));
      if(gw_netif != NULL) {
	return gw_netif->output();
      }
      return UIP_FW_DROPPED;
    } else if(rimeaddr_cmp(&gateway, &rimeaddr_null)) {
      PRINTF("uip_over_mesh_send: No gateway setup, dropping packet\n");
      return UIP_FW_OK;
    } else {
      PRINTF("uip_over_mesh_send: forwarding packet to %d.%d.%d.%d towards gateway %d.%d\n",
	     uip_ipaddr_to_quad(&BUF->destipaddr),
	     gateway.u8[0], gateway.u8[1]);
      rimeaddr_copy(&receiver, &gateway);
    }
  }

  PRINTF("uIP over mesh send to %d.%d with len %d\n",
	 receiver.u8[0], receiver.u8[1],
	 uip_len);

  /*  uip_len = hc_compress(&uip_buf[UIP_LLH_LEN], uip_len);*/

  packetbuf_copyfrom(&uip_buf[UIP_LLH_LEN], uip_len);

  /* Send TCP data with the PACKETBUF_ATTR_ERELIABLE set so that
     an underlying power-saving MAC layer knows that it should be
     waiting for an ACK. */
  if(BUF->proto == UIP_PROTO_TCP) {
    packetbuf_set_attr(PACKETBUF_ATTR_ERELIABLE, 1);
  }

  //DY : comment this and open lookup for static routing
//    rt = route_lookup(&receiver);
  rt = lookup(&receiver);


  if(rt == NULL) {
    PRINTF("uIP over mesh no route to %d.%d\n", receiver.u8[0], receiver.u8[1]);
    //DY : comment this for static routing
    /*
    if(queued_packet == NULL) {
      queued_packet = queuebuf_new_from_packetbuf();
      rimeaddr_copy(&queued_receiver, &receiver);
      route_discovery_discover(&route_discovery, &receiver, ROUTE_TIMEOUT);
    } else if(!rimeaddr_cmp(&queued_receiver, &receiver)) {
      route_discovery_discover(&route_discovery, &receiver, ROUTE_TIMEOUT);
    }*/
  } else {
    //DY : comment these 2 lines and open send_data(rt) for static routing
    //route_decay(rt);
    //send_data(&rt->nexthop);
    send_data(rt);
  }


  return UIP_FW_OK;
}
/*---------------------------------------------------------------------------*/
void
uip_over_mesh_set_gateway_netif(struct uip_fw_netif *n)
{
  gw_netif = n;
}
/*---------------------------------------------------------------------------*/
void
uip_over_mesh_set_gateway(rimeaddr_t *gw)
{
  rimeaddr_copy(&gateway, gw);
}
/*---------------------------------------------------------------------------*/
void
uip_over_mesh_set_net(uip_ipaddr_t *addr, uip_ipaddr_t *mask)
{
  uip_ipaddr_copy(&netaddr, addr);
  uip_ipaddr_copy(&netmask, mask);
}
/*---------------------------------------------------------------------------*/
