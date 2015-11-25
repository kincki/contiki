/**
 * \addtogroup rudolph1mh
 * @{
 */

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
 * $Id: rudolph1mh.c,v 1.13 2009/11/08 19:40:18 adamdunkels Exp $
 */

/**
 * \file
 *         rudolph1mh: a simple block data flooding protocol
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Moritz "Morty" Str�be <morty@cs.fau.de>
 */

#include <stdio.h>
#include <stddef.h> /* for offsetof */

#include "net/rime.h"
#include "rudolph1mh.h"
#include "cfs/cfs.h"

#define DEFAULT_SEND_INTERVAL CLOCK_SECOND * 2
#define TRICKLE_INTERVAL CLOCK_SECOND / 2
#define NACK_TIMEOUT CLOCK_SECOND / 4
#define REPAIR_TIMEOUT CLOCK_SECOND / 4

struct rudolph1mh_hdr
{
  uint8_t type;
  uint8_t s_id;
  uint16_t chunk;
};

#define RUDOLPH1MH_DATASIZE 64

struct rudolph1mh_datapacket
{
  struct rudolph1mh_hdr h;
  uint8_t datalen;
  uint8_t data[RUDOLPH1MH_DATASIZE];
};

enum
{
  TYPE_DATA, TYPE_NACK, TYPE_BUSY, TYPE_FIN, TYPE_ACK
};

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define LT(a, b) ((signed char)((a) - (b)) < 0)


/*---------------------------------------------------------------------------*/
static int
format_data(struct rudolph1mh_conn *c, int chunk)
{
  struct rudolph1mh_datapacket *p;
  
  packetbuf_clear();
  p = packetbuf_dataptr();
  p->h.type = TYPE_DATA;
  p->h.s_id = c->s_id;
  p->h.chunk = chunk;
  p->datalen = 0; //read_data(c, p->data, chunk);
  if(c->cb->read_chunk) {
    p->datalen = c->cb->read_chunk(c, chunk * RUDOLPH1MH_DATASIZE, p->data,
        RUDOLPH1MH_DATASIZE);
  }

  if(p->datalen != RUDOLPH1MH_DATASIZE){
    p->h.type = TYPE_FIN;
  }

  packetbuf_set_datalen(sizeof(struct rudolph1mh_datapacket) - (RUDOLPH1MH_DATASIZE
      - p->datalen));

  return p->datalen;
}
/*---------------------------------------------------------------------------*/
static void
write_data(struct rudolph1mh_conn *c, int chunk, uint8_t *data, int datalen)
{

  //Not called, when c->cb->write_chunk == NULL
  if(datalen < RUDOLPH1MH_DATASIZE) {
    PRINTF("%d.%d: get %d bytes, file complete\n",
        rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
        datalen);
    c->cb->write_chunk(c, chunk * RUDOLPH1MH_DATASIZE, RUDOLPH1MH_FLAG_LASTCHUNK,
        data, datalen);
  }
  else {
    c->cb->write_chunk(c, chunk * RUDOLPH1MH_DATASIZE, RUDOLPH1MH_FLAG_NONE, data,
        datalen);
  }
}
/*---------------------------------------------------------------------------*/
static void
send_nack(struct rudolph1mh_conn *c)
{
  struct rudolph1mh_hdr *hdr;
  packetbuf_clear();
  packetbuf_hdralloc(sizeof(struct rudolph1mh_hdr));
  hdr = packetbuf_hdrptr();

  hdr->type = TYPE_NACK;
  hdr->s_id = c->s_id;
  hdr->chunk = c->chunk;

  PRINTF("%d.%d: Sending nack for %d:%d\n",
      rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
      hdr->s_id, hdr->chunk);
  mesh_send(&c->mesh, &c->partner);
}

/*---------------------------------------------------------------------------*/
static void
send_ack(struct rudolph1mh_conn *c)
{
  struct rudolph1mh_hdr *hdr;
  packetbuf_clear();
  packetbuf_hdralloc(sizeof(struct rudolph1mh_hdr));
  hdr = packetbuf_hdrptr();

  hdr->type = TYPE_ACK;
  hdr->s_id = c->s_id;
  hdr->chunk = c->highest_chunk; //The highest chunk received

  PRINTF("%d.%d: Sending ack for %d:%d to %d.%d\n",
      rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
      hdr->s_id, hdr->chunk, c->partner.u8[0], c->partner.u8[1]);
  mesh_send(&c->mesh, &c->partner);
}

/*---------------------------------------------------------------------------*/
static void
send_busy(struct rudolph1mh_conn *c, rimeaddr_t * to)
{
  struct rudolph1mh_hdr *hdr;
  packetbuf_clear();
  packetbuf_hdralloc(sizeof(struct rudolph1mh_hdr));
  hdr = packetbuf_hdrptr();

  hdr->type = TYPE_BUSY;
  hdr->s_id = 0;
  hdr->chunk = 0;

  PRINTF("%d.%d: Sending nack for %d:%d\n",
      rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
      hdr->s_id, hdr->chunk);
  mesh_send(&c->mesh, to);
}



/*---------------------------------------------------------------------------*/
static void
handle_data(struct rudolph1mh_conn *c, struct rudolph1mh_datapacket *p)
{
  //Only called if partner is correct of NULL
  PRINTF("Handle P: %d:%d - %d\n",
         c->partner.u8[0], c->partner.u8[1], p->h.chunk);
  if(p->h.chunk == 0) { //New stream
    PRINTF("%d.%d: rudolph1mh new v_id %d, chunk %d\n",
        rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
        p->h.s_id, p->h.chunk);
    c->s_id = p->h.s_id;
    c->highest_chunk = c->chunk = 0;
    if(p->h.chunk != 0) {
      send_nack(c);
    }
    else {
       if(!c->cb->write_chunk){
         printf("No write-callback\n");
         return;
       }
      c->cb->write_chunk(c, 0, RUDOLPH1MH_FLAG_NEWFILE, p->data, 0);
      write_data(c, 0, p->data, p->datalen);
      c->chunk = 1; /* Next chunk is 1. */
      //ctimer_set(&c->t, send_interval * 30, send_next_packet, c);
    }
    /*    }*/
  }
  else if(p->h.s_id == c->s_id) {

    PRINTF("%d.%d: got chunk %d (%d) highest heard %d\n",
        rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
        p->h.chunk, c->chunk, c->highest_chunk);

    if(p->h.chunk == c->chunk) {
      PRINTF("%d.%d: received chunk %d\n",
          rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
          p->h.chunk);
      write_data(c, p->h.chunk, p->data, p->datalen);
      if(c->highest_chunk < c->chunk) {
        c->highest_chunk = c->chunk;
      }
      c->chunk++;
    }
    else if(p->h.chunk > c->chunk) {
      PRINTF("%d.%d: received chunk %d > %d, sending NACK\n",
          rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
          p->h.chunk, c->chunk);
      send_nack(c);
      c->highest_chunk = p->h.chunk;
    }
    else if(p->h.chunk < c->chunk) {
      /* Ignore packets with a lower chunk number */
    }

    /* If we have heard a higher chunk number, we send a NACK so that
     we get a repair for the next packet. */

    if(c->highest_chunk > p->h.chunk) {
      send_nack(c);
    }
  }
  else { /* p->h.s_id < c->current.h.s_id */
    PRINTF("%d.%d: Recieved s_id %i expected %i\n",
              rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
              p->h.s_id, c->s_id);
    /* Ignore packets with old s_id */
  }

}

/*---------------------------------------------------------------------------*/
static void
sent_mesh(struct mesh_conn *mesh)
{
  PRINTF("%d.%d: Sent mesh\n",
      rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
}
/*---------------------------------------------------------------------------*/
static void
timeout_mesh(struct mesh_conn *mesh)
{
  PRINTF("%d.%d: dropped ipolite\n",
      rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
}
/*---------------------------------------------------------------------------*/
static void
recv_mesh(struct mesh_conn *mesh, const rimeaddr_t *from, uint8_t hops)
{
  struct rudolph1mh_conn *c = (struct rudolph1mh_conn *) ((char *) mesh
      - offsetof(struct rudolph1mh_conn, mesh));
  struct rudolph1mh_datapacket *p = packetbuf_dataptr();

  PRINTF("%d.%d: Got mesh type %d from %d.%d\n",
      rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
      p->h.type, from->u8[0], from->u8[1]);

  if(!rimeaddr_cmp(&c->partner, from)){
    if(!rimeaddr_cmp(&c->partner, &rimeaddr_null)){
      rimeaddr_t lfrom;
      PRINTF("%d.%d: Unexpected packet from %d.%d\n",
            rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
            from->u8[0], from->u8[1]);
      rimeaddr_copy(&lfrom, from);
      send_busy(c, &lfrom);
      return;
    } else {
      rimeaddr_copy(&c->partner, from);
    }
  }


  if(p->h.type == TYPE_ACK){
    if(p->h.s_id == c->s_id && p->h.chunk == c->highest_chunk){
      ctimer_stop(&c->t);
      rimeaddr_copy(&c->partner, &rimeaddr_null);
      if(c->cb->read_done){
        c->cb->read_done(c);
      }
      PRINTF("GOT ACK\n");
    } else {
      PRINTF("Unexpected ACK sid %i,%i C %i,%i\n", p->h.s_id, c->s_id, p->h.chunk, c->highest_chunk);
    }



  } else if(p->h.type == TYPE_NACK) {
    c->nacks++;
    PRINTF("%d.%d: Got NACK for %d:%d (%d:%d)\n",
        rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
        p->h.s_id, p->h.chunk,
        c->s_id, c->chunk);
    if(p->h.s_id == c->s_id) {
      if(p->h.chunk < c->chunk) {
        /* Format and send a repair packet */
        PRINTF("%d.%d: sending repair for chunk %d\n",
            rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
            p->h.chunk);
        format_data(c, p->h.chunk);
        mesh_send(&c->mesh, &c->partner);
      }
    }
    else if(LT(p->h.s_id, c->s_id)) {
      format_data(c, 0);
      mesh_send(&c->mesh, &c->partner);
    }
  }
  else if(p->h.type == TYPE_DATA ) {
    /* This is a repair packet from someone else. */
    PRINTF("%d.%d: got chunk %d\n",
        rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
        p->h.chunk);
    handle_data(c, p);
  } else if(p->h.type == TYPE_FIN ) {
    /* This is a repair packet from someone else. */
    PRINTF("%d.%d: got last chunk %d\n",
        rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
        p->h.chunk);
    if(!rimeaddr_cmp(&c->partner, &rimeaddr_null)){
      handle_data(c, p);
    }{
      rimeaddr_copy(&c->partner, from);
    }
    send_ack(c);
    rimeaddr_copy(&c->partner, &rimeaddr_null);

  } else if(p->h.type == TYPE_BUSY){
    PRINTF("%d.%d: %d.%d is busy\n",
        rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
        from->u8[0], from->u8[1]);
    //Wait
  }
}
/*---------------------------------------------------------------------------*/
static void
send_next_packet(void *ptr)
{
  struct rudolph1mh_conn *c = ptr;
  int len;
  if(c->nacks == 0) {
    len = format_data(c, c->chunk);
    mesh_send(&c->mesh, &c->partner);

    ctimer_set(&c->t, c->send_interval, send_next_packet, c);


    PRINTF("%d.%d: send_next_packet s_id %d, chunk %d\n",
        rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
        c->s_id, c->chunk);
    c->highest_chunk = c->chunk;
    if(len == RUDOLPH1MH_DATASIZE) { //Continue sending last packet
      c->chunk++;
    }
  }
  else {
    ctimer_set(&c->t, c->send_interval, send_next_packet, c);
  }
  c->nacks = 0;
}
/*---------------------------------------------------------------------------*/
static const struct mesh_callbacks mesh =
{ recv_mesh, sent_mesh, timeout_mesh };

/*---------------------------------------------------------------------------*/
void
rudolph1mh_open(struct rudolph1mh_conn *c, uint16_t channel,
    const struct rudolph1mh_callbacks *cb)
{
  mesh_open(&c->mesh, channel, &mesh);
  c->cb = cb;
  c->s_id = 0;
  c->send_interval = DEFAULT_SEND_INTERVAL;
  rimeaddr_copy(&c->partner, &rimeaddr_null);
}
/*---------------------------------------------------------------------------*/
void
rudolph1mh_close(struct rudolph1mh_conn *c)
{
  mesh_close(&c->mesh);
}
/*---------------------------------------------------------------------------*/
int
rudolph1mh_send(struct rudolph1mh_conn *c, clock_time_t send_interval, const rimeaddr_t * dst)
{
  if(!rimeaddr_cmp(&c->partner, &rimeaddr_null) ){
    PRINTF("Still got partner");
    return 1;
  }
  rimeaddr_copy(&c->partner, dst);

  c->s_id++;
  c->chunk =  c->highest_chunk = 0;
  //send_next_packet(c);
  ctimer_set(&c->t, c->send_interval, send_next_packet, c);
  return 0;
}
/*---------------------------------------------------------------------------*/

void rudolph1mh_pause(struct rudolph1mh_conn *c) {
  ctimer_stop(&c->t);
}

/*---------------------------------------------------------------------------*/
void rudolph1mh_cont(struct rudolph1mh_conn *c) {
  ctimer_set(&c->t, c->send_interval, send_next_packet, c);
}
/*---------------------------------------------------------------------------*/
/** @} */
