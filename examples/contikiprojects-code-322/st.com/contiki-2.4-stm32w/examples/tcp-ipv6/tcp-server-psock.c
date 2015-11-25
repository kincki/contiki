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
 *         Example showing how to setup a TCP server, using protosockets.
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


#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define MAX_PAYLOAD_LEN 120

static uint8_t buf[MAX_PAYLOAD_LEN];

static struct psock ps;


PROCESS(tcp_server_process, "TCP server process");
AUTOSTART_PROCESSES(&tcp_server_process);
/*---------------------------------------------------------------------------*/

static
PT_THREAD(handle_connection(struct psock *p))
{
  static int seq_id = 1;
  
  PSOCK_BEGIN(p);
  
  while(1){
  
    PSOCK_READTO(p, '\r');
    
    buf[PSOCK_DATALEN(p)-1] = '\0';
    PRINTF("Server received: %s", buf);
    PRINTF("\r\nFrom ");
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF("\r\n");    
    
    PRINTF("Responding with message: ");
    sprintf((char*)buf, "Hello from the server! (%d)\r", seq_id++);
    PRINTF("%s\r\n", buf);
    PSOCK_SEND_STR(p, (char*)buf);
  
  }  
  
  PSOCK_END(p);
    
}

/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{

  if(uip_connected()){
    
    PSOCK_INIT(&ps, buf, MAX_PAYLOAD_LEN);    
    
    PRINTF("Created a server connection with remote address ");
    PRINT6ADDR(&uip_conn->ripaddr);
    PRINTF("local/remote port %u/%u\r\n", HTONS(uip_conn->lport),
           HTONS(uip_conn->rport));
  }
  
  if(uip_closed()){
    PRINTF("Connection with remote address ");
    PRINT6ADDR(&uip_conn->ripaddr);
    PRINTF("closed.\r\n");
    return;
  }
  
  if(uip_aborted()){
    PRINTF("Connection with remote address ");
    PRINT6ADDR(&uip_conn->ripaddr);
    PRINTF("aborted.\r\n"); 
    return;
  }
  
  if(uip_timedout()) {
    PRINTF("Connection with remote address ");
    PRINT6ADDR(&uip_conn->ripaddr);
    PRINTF("timed out.\r\n");
    return;
  }
  
  handle_connection(&ps);
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uip_netif_state state;

  PRINTF("Server IPv6 addresses: ");
  for(i = 0; i < UIP_CONF_NETIF_MAX_ADDRESSES; i++) {
    state = uip_netif_physical_if.addresses[i].state;
    if(state == TENTATIVE || state == PREFERRED) {
      PRINT6ADDR(&uip_netif_physical_if.addresses[i].ipaddr);
      PRINTF("\r\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tcp_server_process, ev, data)
{
  static uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();
  PRINTF("TCP server started\r\n");

  print_local_addresses();

  tcp_listen(HTONS(3000));

  while(1) {
    PROCESS_YIELD();  // or PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
