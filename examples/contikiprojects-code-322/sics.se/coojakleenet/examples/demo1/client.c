
#include "contiki.h"
#include "contiki-net.h"

#include <stdio.h>
#include <string.h>

#include "data.h"

#include "concrete_queue.h"

static struct uip_udp_conn *udpconn;
u16_t serverport, localport;

static struct etimer timer;
static struct protocol_data *pd;
static int constrained;

char symack, symnack, symdst;

/*---------------------------------------------------------------------------*/
PROCESS(client_process, "UDP client process");
/*---------------------------------------------------------------------------*/
AUTOSTART_PROCESSES(&client_process);
/*---------------------------------------------------------------------------*/
static void
newdata(char *data, uint16_t len)
{
  printf("Node A received ");
  pd = (struct protocol_data*)data;
  if (pd->ack) {
    printf("ACK.\n");
    if (pd->constrained <= 40) {
      printf("constrained <= 40\n");
    } else {
      KLEENET_ASSERT(0 && "constrained > 40!");
    }
  } else if (pd->nack) {
    printf("NACK.\n");
  } else {
    printf("unkown flag.\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(client_process, ev, data)
{
  PROCESS_BEGIN();

  etimer_set(&timer, CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  uip_ipaddr_t serveraddr;
  uip_ipaddr(&serveraddr, 172,16,2,0);
  serverport = 2222;
  localport = 1111;

  udpconn = udp_new(&serveraddr, uip_htons(serverport), NULL);
  udp_bind(udpconn, uip_htons(localport));

  struct protocol_data p;
  p.src = 'A';
  p.dst = 'B';
  p.ack = 1;
  p.nack = 0;

  KLEENET_SYMBOLIC_MEMORY(&symdst, sizeof(symdst), "symdst");
  KLEENET_SYMBOLIC_MEMORY(&symack, sizeof(symack), "symack");
  KLEENET_SYMBOLIC_MEMORY(&symnack, sizeof(symnack), "symnack");
  KLEENET_SYMBOLIC_MEMORY(&constrained, sizeof(constrained), "constrained");
  p.dst = symdst;
  p.ack = symack;
  p.nack = symnack;
  p.constrained = constrained;

  uip_udp_packet_send(udpconn, (void *)&p, sizeof(p));

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
    if (uip_newdata()) {
      newdata(uip_appdata, uip_datalen());
    }
  }

  PROCESS_END();
}

