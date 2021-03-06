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
 */

/**
 * \file
 *         Example showing how to setup a TCP client.
 * \author
 *         Salvatore Pitrulli <salvopitru@users.sourceforge.net>
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x ",lladdr->addr[0], lladdr->addr[1], lladdr->addr[2], lladdr->addr[3],lladdr->addr[4], lladdr->addr[5],lladdr->addr[6], lladdr->addr[7])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define SEND_INTERVAL		10 * CLOCK_SECOND
#define MAX_PAYLOAD_LEN		300

static struct uip_conn *client_conn;

static int seq_id;
static struct etimer et;

/*---------------------------------------------------------------------------*/
PROCESS(tcp_client_process, "TCP client process");
AUTOSTART_PROCESSES(&tcp_client_process);
/*---------------------------------------------------------------------------*/

static void send_data(void);

static void
tcpip_handler(void)
{
  char *str;
  
  if(uip_connected()){
    
    PRINTF("Created a connection with the server ");
    PRINT6ADDR(&client_conn->ripaddr);
    PRINTF("local/remote port %u/%u\r\n",
           HTONS(client_conn->lport), HTONS(client_conn->rport));
    
    seq_id = 1;
    
    send_data();
    etimer_set(&et, SEND_INTERVAL);
    
    return;
  }
  
  if(uip_closed()){
    PRINTF("Connection with remote address ");
    PRINT6ADDR(&client_conn->ripaddr);
    PRINTF("closed.\r\n");
  }
  
  if(uip_aborted()){
    PRINTF("Connection with remote address ");
    PRINT6ADDR(&client_conn->ripaddr);
    PRINTF("aborted.\r\n");    
  }
  
  if(uip_timedout()) {
    PRINTF("Connection with remote address ");
    PRINT6ADDR(&client_conn->ripaddr);
    PRINTF("timed out.\r\n");
  }
  
  if(uip_acked()){
    seq_id++;
  }
  
  if(uip_rexmit()){
    send_data(); // It will retransmit the data.
  }

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    printf("Response from the server: %s\r\n", str);
  }
  
  if(etimer_expired(&et)){
    if(uip_acked()||uip_newdata()||uip_poll()){    
      send_data();
      etimer_restart(&et);
    }
  }
  
}
/*---------------------------------------------------------------------------*/
static void send_data(void)
{
  char buf[MAX_PAYLOAD_LEN];
  
  printf("Client sending to: ");
  PRINT6ADDR(&client_conn->ripaddr);
  sprintf(buf, "Hello %d from the client\r", seq_id);
  printf("\r\nmsg: %s\n", buf);
  uip_send(buf, strlen(buf));
}
/*---------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------*/
static void
set_connection_address(uip_ipaddr_t *ipaddr)
{
#ifdef ADDR_A
  uip_ip6addr(ipaddr,
      ADDR_A,ADDR_B,ADDR_C,ADDR_D,
      ADDR_E,ADDR_F,ADDR_G,ADDR_H);
#elif UIP_CONF_ROUTER
  //uip_ip6addr(ipaddr,0xaaaa,0,0,0,0x0212,0x7404,0x0004,0x0404);
  uip_ip6addr(ipaddr,0x2001,0,0xabcd,0x1234,0x0212,0x7404,0x0004,0x0404);
  //uip_ip6addr(ipaddr,0xaaaa,0,0,1,0x0280,0xE102,0x0000,0x008A);
#else
  //uip_ip6addr(ipaddr,0xfe80,0,0,0,0x6466,0x6666,0x6666,0x6666);
  uip_ip6addr(ipaddr,0xfe80,0,0,0,0x0280,0xE102,0x0000,0x008A);
#endif /* UDP_ADDR_A */
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tcp_client_process, ev, data)
{
  static uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();
  PRINTF("TCP client process started\r\n");

  print_local_addresses();

  set_connection_address(&ipaddr);
  
  etimer_set(&et, CLOCK_SECOND*10 );  
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));  /* Wait for DAD procedure to end.*/

  /* new connection with remote host */
  client_conn = tcp_connect(&ipaddr, HTONS(3000), NULL);

  PRINTF("Trying to connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF("local/remote port %u/%u\r\n",
	HTONS(client_conn->lport), HTONS(client_conn->rport));

  while(1) {
    PROCESS_YIELD();    
    if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
