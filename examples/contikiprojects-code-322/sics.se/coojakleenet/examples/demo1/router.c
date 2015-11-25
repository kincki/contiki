
#include "contiki.h"
#include "contiki-net.h"

#include <stdio.h>
#include <string.h>

#include "data.h"

static struct uip_udp_conn *udpconn;
static struct uip_udp_conn *udpconn2;
u16_t serverport, localport;

static struct etimer timer;
static struct protocol_data *pd;

/*---------------------------------------------------------------------------*/
PROCESS(router_process, "UDP router process");
/*---------------------------------------------------------------------------*/
AUTOSTART_PROCESSES(&router_process);
/*---------------------------------------------------------------------------*/
static void
newdata(char *data, uint16_t len)
{
  pd = (struct protocol_data*)data;
  if (pd->dst == 'B') {
    printf("Node B received data from node %c.\n", pd->src);
    struct protocol_data reply;
    reply.src = 'B';
    reply.dst = 'A';
    if (pd->ack) {
      printf("Node B sends ACK back.\n");
      reply.ack = 1;
      reply.nack = 0;
      if (pd->constrained <= 20) {
        //
        printf("constraining with <= 20\n");
      } else {
        //
        printf("constraining with > 20\n");
      }
      reply.constrained = pd->constrained;
      uip_udp_packet_send(udpconn2, (void*)&reply, sizeof(reply));
    } else if (pd->nack) {
      printf("Node B sends NACK back.\n");
      reply.ack = 0;
      reply.nack = 1;
      reply.constrained = pd->constrained;
      uip_udp_packet_send(udpconn2, (void*)&reply, sizeof(reply));
    } else {
      printf("Node B discards the packet.\n");
    }
  } else {
    //printf("Node B received data from node %c to forward to node %c.\n", pd->src, pd->dst);
    printf("Node B forwards the packet to C.\n");
    uip_udp_packet_send(udpconn, data, len);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(router_process, ev, data)
{
  PROCESS_BEGIN();

  uip_ipaddr_t serveraddr;
  uip_ipaddr(&serveraddr, 172,16,3,0);
  serverport = 3333;
  localport = 2222;
  udpconn = udp_new(&serveraddr, uip_htons(serverport), NULL);
  udp_bind(udpconn, uip_htons(localport));

  uip_ipaddr(&serveraddr, 172,16,1,0);
  serverport = 1111;
  udpconn2 = udp_new(&serveraddr, uip_htons(serverport), NULL);
  udp_bind(udpconn2, uip_htons(localport));

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    if (uip_newdata()) {
      newdata(uip_appdata, uip_datalen());
    }
  }

  PROCESS_END();
}

