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
 * $Id: cooja-radio.c,v 1.15 2010/06/14 19:19:17 adamdunkels Exp $
 */

#include <stdio.h>
#include <string.h>

#include "contiki.h"

#include "sys/cooja_mt.h"
#include "lib/simEnvChange.h"

#include "net/packetbuf.h"
#include "net/rime/rimestats.h"
#include "net/netstack.h"

#include "dev/radio.h"
#include "dev/cooja-radio.h"

#define COOJA_RADIO_BUFSIZE PACKETBUF_SIZE

#define CCA_SS_THRESHOLD -95

#define PRINTF(...) /*printf(__VA_ARGS__)*/

const struct simInterface radio_interface;

extern int simMoteID;

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

PROCESS(cooja_radio_process, "cooja radio process");

/* radio failure */
int packet_drop[1000];
int packet_duplicate[1000];
int packet_subsequent_drop[1000];
extern clock_time_t failure_delay[1000];

int dropped = 0;

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
    if (clock_seconds() >= failure_delay[simMoteID]) {
      if (!--packet_drop[simMoteID]) {
        dropped = 1;
        /* XXX */
        printf("radio_read: dropping the packet once @ %lu\n", clock_time());
        simInSize = 0;
        return;
      } else if (dropped) {
        if (packet_subsequent_drop[simMoteID]--) {
          /* XXX */
          printf("radio_read: subsequently dropping the packet @ %lu\n", clock_time());
          simInSize = 0;
          return;
        } else {
          dropped = 0;
        }
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

  /* Transmit */
  if (simOutSize > 0) {
    PRINTF("COOJA: sending %d bytes @ %lu\n", simOutSize, clock_time());
  }
  while(simOutSize > 0) {
    cooja_mt_yield();
  }

  simRadioHWOn = radiostate;
  /* FIXME: this doesn't work */
  cooja_mt_yield();
  /* duplicate packets here */
  if (clock_seconds() >= failure_delay[simMoteID]) {
    if (!--packet_duplicate[simMoteID]) {
      PRINTF("COOJA: sending %d bytes @ %lu (twice)\n", payload_len, clock_time());
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
  char dupbuf[PACKETBUF_SIZE];
  int duplen;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    packetbuf_clear();
    len = radio_read(packetbuf_dataptr(), PACKETBUF_SIZE);
    memcpy(dupbuf, packetbuf_dataptr(), len);
    duplen = len;
    if(len > 0) {
      PRINTF("COOJA: receiving %d bytes @ %lu\n", len, clock_time());
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
