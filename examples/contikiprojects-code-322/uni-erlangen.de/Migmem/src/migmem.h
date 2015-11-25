/*
 * Copyright (c) 2010, Friedrich-Alexander University Erlangen, Germany
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
 * \addtogroup minilink
 * @{
 * \file
 *         (De-)Serialization of programs linked using the minilink Linker
 * \author
 *         Moritz Strübe <Moritz.Struebe@informatik.uni-erlangen.de>
 */

#ifndef MIGMEM_H_
#define MIGMEM_H_
#include <stddef.h>
#include <stdint.h>
#include <minilink.h>

#define PTRBLOCK 10



struct migmem_ptr {
  void ** ptr[PTRBLOCK];
  struct migmem_ptr * next;
};


struct migmem_hdr {
  struct migmem_hdr * next;
  uint16_t size;
};


struct migmem_state{
  struct process_list * proc_list;
  Minilink_ProgramInfoHeader * pih;
  void ** oldmem;
  union{
    struct migmem_hdr * mem;
    struct migmem_ptr * ptr;
  } cur;
  uint16_t pos;
  uint16_t memblockpos;
  struct process * proc;

  unsigned init:1;
  unsigned mem:1;
  unsigned ptr:1;
  unsigned adr:1;
  unsigned migmem :1;
  unsigned migptr :1;
};

int_fast8_t migmem_register(void ** ptrptr);
int_fast8_t migmem_unregister(void ** ptrptr);
int_fast8_t migmem_init_state_ser(struct migmem_state * state, struct process * proc);
int_fast8_t migmem_init_state_deser(struct migmem_state * state, struct process * proc);
size_t migmem_serialise(struct migmem_state * state, char * dst, size_t len);
int_fast8_t migmem_deserialize(struct migmem_state * state, char * src, size_t len);
void * migmem_alloc(size_t size);
void migmem_free(void * ptr);



#endif /* MIGMEM_H_ */

