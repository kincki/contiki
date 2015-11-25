
#include "contiki.h"
#include "contiki-net.h"

#include <stdio.h>
#include <string.h>

#include "concrete_queue.h"

/*---------------------------------------------------------------------------*/
PROCESS(client_process, "TCP client process");
/*---------------------------------------------------------------------------*/
AUTOSTART_PROCESSES(&client_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(client_process, ev, data)
{
  PROCESS_BEGIN();

  static struct uip_conn *tcpconn;
  static u16_t serverport;
  static int connected;

  uip_ipaddr_t serveraddr;
  uip_ipaddr(&serveraddr, 172,16,2,0);
  serverport = 7777;

  printf("TCP connect to %d.%d.%d.%d:%d\n", uip_ipaddr_to_quad(&serveraddr), serverport);

  tcpconn = tcp_connect(&serveraddr, uip_htons(serverport), NULL);
  if (tcpconn == NULL) {
    uip_abort();
    printf("could not connect!\n");
    KLEENET_ASSERT(0 && "tcpconn == NULL");
  }

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
      while (!(uip_aborted() || uip_closed() || uip_timedout()) || uip_rexmit()) {
        PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
        if (uip_connected()) {
          KLEENET_SYMBOL(&connected, 2, "mote2_connected");
          KLEENET_ASSERT(connected);
        }
      }
      if (uip_aborted() || uip_closed()) {
        KLEENET_SYMBOL(&connected, 2, "mote2_connected");
        KLEENET_ASSERT(!connected);
        KLEENET_ASSERT(0 && "could not connect!");
      }
  }

  PROCESS_END();
}

