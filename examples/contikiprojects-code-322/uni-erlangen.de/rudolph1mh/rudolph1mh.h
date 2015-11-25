/**
 * \addtogroup rime
 * @{
 */

/**
 * \defgroup rudolph1mh Multi-hop reliable bulk data transfer
 * @{
 *
 * The rudolph1mh module implements a multi-hop reliable bulk data
 * transfer mechanism.
 *
 * \section channels Channels
 *
 * The rudolph1mh module uses 3 channels, as it is based on mesh networking
 *
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
 * This file is partner of the Contiki operating system.
 *
 * $Id: rudolph1mh.h,v 1.8 2008/02/24 22:05:27 adamdunkels Exp $
 */

/**
 * \file
 *         Header file for the multi-hop reliable bulk data transfer mechanism
 *         Useses mesh networking
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Moritz "Morty" Strübe <morty@cs.fau.de>
 */

#ifndef __rudolph1mh_H__
#define __rudolph1mh_H__

#include "net/rime/mesh.h"
#include "stdint.h"

struct rudolph1mh_conn;

enum {
  RUDOLPH1MH_FLAG_NONE,
  RUDOLPH1MH_FLAG_NEWFILE,
  RUDOLPH1MH_FLAG_LASTCHUNK,
  RUDOLPH1MH_FLAG_FAILED,
};

struct rudolph1mh_callbacks {
  /**
   * Write chunk
   */
  void (* write_chunk)(struct rudolph1mh_conn *c, int offset, int flag,
           uint8_t *data, int len);
  /**
   * Read a chunk
   */
  int (* read_chunk)(struct rudolph1mh_conn *c, int offset, uint8_t *part,
         int maxsize);
  /**
   * Reading is done - ack from dest, recources can be freed.
   */
  void (* read_done)(struct rudolph1mh_conn *c);
 /* void (* done)(void);*/
};

struct rudolph1mh_conn {
  struct mesh_conn mesh;
  const struct rudolph1mh_callbacks *cb;
  struct ctimer t;
  clock_time_t send_interval;
  uint16_t chunk, highest_chunk;
  rimeaddr_t partner;
  uint8_t s_id; // Session ID
  /*  uint8_t trickle_interval;*/
  uint8_t nacks;

};

/**
 * Needs three chanels, because of underlaying mesh.
 * Must also be opened to forward data
 * \sa mesh_open
 */
void rudolph1mh_open(struct rudolph1mh_conn *c, uint16_t channel,
       const struct rudolph1mh_callbacks *cb);

/**
 * Close connection
 */
void rudolph1mh_close(struct rudolph1mh_conn *c);

/**
 * Start sending data. When all data is sent and acknowledged, read_done will be called
 */
int rudolph1mh_send(struct rudolph1mh_conn *c, clock_time_t send_interval, const rimeaddr_t * dst);

/**
 *  Stop sending data
 */
void rudolph1mh_pause(struct rudolph1mh_conn *c);

/**
 *  Continue sending data
 */
void rudolph1mh_cont(struct rudolph1mh_conn *c);

#endif /* __rudolph1mh_H__ */
/** @} */
/** @} */
