
#include "contiki.h"
#include "contiki-net.h"

#include <stdio.h>
#include <string.h>

#include "data.h"

static struct uip_udp_conn *udpconn;
u16_t serverport, localport;

/*---------------------------------------------------------------------------*/
PROCESS(server_process, "UDP server process");
/*---------------------------------------------------------------------------*/
AUTOSTART_PROCESSES(&server_process);
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static void
newdata(char *data, uint16_t len)
{
  printf("Node C received data.\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(server_process, ev, data)
{
  PROCESS_BEGIN();

  uip_ipaddr_t serveraddr;
  uip_ipaddr(&serveraddr, 172,16,2,0);
  serverport = 2222;
  localport = 3333;

  udpconn = udp_new(&serveraddr, uip_htons(serverport), NULL);
  udp_bind(udpconn, uip_htons(localport));

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    if (uip_newdata()) {
      newdata(uip_appdata, uip_datalen());
    }
  }

  PROCESS_END();
}

