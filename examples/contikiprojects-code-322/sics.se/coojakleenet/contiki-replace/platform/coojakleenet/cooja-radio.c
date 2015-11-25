/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * $Id: cooja-radio.c,v 1.14 2010/03/23 15:19:55 adamdunkels Exp $
 */

#include <stdio.h>
#include <string.h>

#include "contiki.h"

#include "lib/simEnvChange.h"

#include "net/rime/rimestats.h"
#include "net/netstack.h"
#include "net/packetbuf.h"

#include "dev/radio.h"
#include "dev/cooja-radio.h"

#define COOJA_RADIO_BUFSIZE PACKETBUF_SIZE

#define CCA_SS_THRESHOLD -95

const struct simInterface radio_interface;

/* COOJA */
char simReceiving = 0;
char simInDataBuffer[COOJA_RADIO_BUFSIZE];
int simInSize = 0;
char simOutDataBuffer[COOJA_RADIO_BUFSIZE];
int simOutSize = 0;
char simRadioHWOn = 1;
int simSignalStrength = -100;
int simLastSignalStrength = -100;
char simPower = 100;
int simRadioChannel = 26;

static const void *pending_data;

/* prefix-symbols won't rename the following symbols */
typedef struct {
  char *simReceiving;
  char *simInSize;
  char *simInDataBuffer[];
} in_buffer;
extern in_buffer * const in_buffers[];
extern int dgrm_nr_links;
extern int dgrm_from[];
extern int dgrm_to[];

extern int simMoteID;
extern clock_time_t simCurrentTime;
extern clock_time_t simTimeDrift;

clock_time_t transmissionDuration = -1;

extern int failure_drop_packet(void);
extern int failure_duplicate_packet(void);
extern int failure_delay(void);

#define RADIO_TRANSMISSION_RATE_kbps 250
#define MILLISECOND 1000UL

#define MAX(a, b)   ( (a)>(b) ? (a) : (b) )
#define PRINTF(...) /*printf(__VA_ARGS__)*/
#define NODE_TIME() (kleenet_get_virtual_time()/MILLISECOND - simTimeDrift)
#define SIM_TIME() (kleenet_get_virtual_time()/MILLISECOND)

/*---------------------------------------------------*/

PROCESS(cooja_radio_process, "cooja radio process");

/*---------------------------------------------------------------------------*/
void
radio_set_channel(int channel)
{
  simRadioChannel = channel;
}
/*---------------------------------------------------------------------------*/
void
radio_set_txpower(unsigned char power)
{
  /* 1 - 100: Number indicating output power */
  simPower = power;
}
/*---------------------------------------------------------------------------*/
int
radio_signal_strength_last(void)
{
  return simLastSignalStrength;
}
/*---------------------------------------------------------------------------*/
int
radio_signal_strength_current(void)
{
  return simSignalStrength;
}
/*---------------------------------------------------------------------------*/
static int
radio_on(void)
{
  simRadioHWOn = 1;
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
radio_off(void)
{
  simRadioHWOn = 0;
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
doInterfaceActionsBeforeTick(void)
{
  if (!simRadioHWOn) {
    simInSize = 0;
    return;
  }
  if (simReceiving) {
    simLastSignalStrength = simSignalStrength;
    return;
  }

  if (simInSize > 0) {
    /* drop packets here */
    if (clock_seconds() >= failure_delay()) {
      if (failure_drop_packet()) {
        /* XXX */
        PRINTF("(%d) %lu COOJAKLEENET: dropping packet once @ %lu\n",
          simMoteID, SIM_TIME(), NODE_TIME());
        simInSize = 0;
        return;
      }
    }
    process_poll(&cooja_radio_process);
  }
}
/*---------------------------------------------------------------------------*/
static void
doInterfaceActionsAfterTick(void)
{
}
/*---------------------------------------------------------------------------*/
static int
radio_read(void *buf, unsigned short bufsize)
{
  int tmp = simInSize;

  if (simInSize == 0) {
    return 0;
  }

  if(bufsize < simInSize) {
    simInSize = 0; /* rx flush */
    RIMESTATS_ADD(toolong);
    return 0;
  }

  memcpy(buf, simInDataBuffer, simInSize);
  simInSize = 0;
  return tmp;
}
/*---------------------------------------------------------------------------*/
static int
radio_send(const void *payload, unsigned short payload_len)
{
  int radiostate = simRadioHWOn;

  if(!simRadioHWOn) {
    /* Turn on radio temporarily */
    simRadioHWOn = 1;
  }
  if(payload_len > COOJA_RADIO_BUFSIZE) {
    return RADIO_TX_ERR;
  }
  if(payload_len == 0) {
    return RADIO_TX_ERR;
  }
  if(simOutSize > 0) {
    return RADIO_TX_ERR;
  }

  /* Copy packet data to temporary storage */
  memcpy(simOutDataBuffer, payload, payload_len);
  simOutSize = payload_len;

  if (simOutSize > 0) {
    /* calculate transmission duration in us */
    transmissionDuration = (MILLISECOND*(8 * simOutSize)) / RADIO_TRANSMISSION_RATE_kbps;

    int i;
    /* setting simReceiving=1 of receiving nodes */
    for(i=0; i < dgrm_nr_links; i++) {
      if(dgrm_from[i] == simMoteID) {
        int dest = dgrm_to[i];
        kleenet_memset(in_buffers[dest]->simReceiving, 1, 1, dest);
        kleenet_wakeup_dest_states(dest);
      }
    }

    /* blocking TX */
    PRINTF("(%d) %lu COOJAKLEENET: sending %d bytes @ %lu\n",
      simMoteID, SIM_TIME(), simOutSize, NODE_TIME());
    kleenet_schedule_state(MAX(1, transmissionDuration));
    kleenet_yield_state();
    simCurrentTime = NODE_TIME();

    /* data delivery and immediate wakeup of receiving nodes */
    for(i=0; i < dgrm_nr_links; i++) {
      if(dgrm_from[i] == simMoteID) {
        int dest = dgrm_to[i];
        kleenet_memcpy(*in_buffers[dest]->simInDataBuffer, &simOutDataBuffer,
            simOutSize, dest);
        kleenet_memset(in_buffers[dest]->simInSize, simOutSize, 1, dest);
        kleenet_memset(in_buffers[dest]->simReceiving, 0, 1, dest);
        kleenet_wakeup_dest_states(dest);
      }
    }

    /* TX done */
    simOutSize = 0;
  }

  simRadioHWOn = radiostate;

  /* duplicate packets here */
  if (clock_seconds() >= failure_delay()) {
    if (failure_duplicate_packet()) {
      kleenet_schedule_state(MAX(1, transmissionDuration));
      kleenet_yield_state();
      PRINTF("(%d) %lu COOJAKLEENET: sending %d bytes @ %lu (twice)\n",
        simMoteID, SIM_TIME(), payload_len, NODE_TIME());
      radio_send(payload, payload_len);
    }
  }

  return RADIO_TX_OK;
}
/*---------------------------------------------------------------------------*/
static int
prepare_packet(const void *data, unsigned short len)
{
  pending_data = data;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit_packet(unsigned short len)
{
  int ret = RADIO_TX_ERR;
  if(pending_data != NULL) {
    ret = radio_send(pending_data, len);
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  return simReceiving;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  return !simReceiving && simInSize > 0;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  if(simSignalStrength > CCA_SS_THRESHOLD) {
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cooja_radio_process, ev, data)
{
  int len;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    packetbuf_clear();
    len = radio_read(packetbuf_dataptr(), PACKETBUF_SIZE);
    if(len > 0) {
      PRINTF("(%d) %lu COOJAKLEENET: receiving %d bytes @ %lu\n",
        simMoteID, SIM_TIME(), len, NODE_TIME());
        packetbuf_set_datalen(len);
        NETSTACK_RDC.input();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  process_start(&cooja_radio_process, NULL);
  return 1;
}
/*---------------------------------------------------------------------------*/
const struct radio_driver cooja_radio_driver =
{
    init,
    prepare_packet,
    transmit_packet,
    radio_send,
    radio_read,
    channel_clear,
    receiving_packet,
    pending_packet,
    radio_on,
    radio_off,
};
/*---------------------------------------------------------------------------*/
SIM_INTERFACE(radio_interface,
	      doInterfaceActionsBeforeTick,
	      doInterfaceActionsAfterTick);
