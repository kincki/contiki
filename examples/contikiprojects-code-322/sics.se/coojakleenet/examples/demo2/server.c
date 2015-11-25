
#include "contiki.h"
#include "contiki-net.h"

#include <stdio.h>

#include "concrete_queue.h"

int connected = 0;

/*---------------------------------------------------------------------------*/
PROCESS(server_process, "TCP server process");
/*---------------------------------------------------------------------------*/
AUTOSTART_PROCESSES(&server_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(server_process, ev, data)
{
  PROCESS_BEGIN();

  u16_t hostport = 7777;

  printf("TCP listen on %d.%d.%d.%d:%u\n", 
    uip_ipaddr_to_quad(&uip_hostaddr), hostport);

  tcp_listen(uip_htons(hostport));

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    if (uip_connected()) {
      printf("server: client arrived!\n");
      connected = 1;
      while (!(uip_aborted() || uip_closed() || uip_timedout())) {
        PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
      }
      if (uip_aborted() || uip_closed()) {
        connected = 0;
      }
    } else {
      printf("server: client not accepted!\n");
      KLEENET_ASSERT(0 && "server: client not accepted!\n");
    }
  }

  PROCESS_END();
}

