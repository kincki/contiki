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
 *         Moritz Str�be <Moritz.Struebe@informatik.uni-erlangen.de>
 */


#include "migmem.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <sys/process.h>


#ifndef MAX
#define MAX(a, b) ((a) > (b)? (a) : (b))
#endif /* MAX */

#ifndef MIN
#define MIN(a, b) ((a) < (b)? (a) : (b))
#endif /* MIN */


#define DEBUG_LED 0
#if DEBUG_LED
#include <dev/leds.h>
#define LEDRON  leds_red(LEDS_ON)
#define LEDROFF leds_red(LEDS_OFF)
#define LEDGON  leds_green(LEDS_ON)
#define LEDGOFF leds_green(LEDS_OFF)
#define LEDBON  leds_blue(LEDS_ON)
#define LEDBOFF leds_blue(LEDS_OFF)

#else
#define LEDRON
#define LEDROFF
#define LEDGON
#define LEDGOFF
#define LEDBON
#define LEDBOFF
#endif


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define SDPRINTF(...) printf(__VA_ARGS__)
#define SDPUTS(x)     puts(x)
#else
#define SDPRINTF(...)
#define SDPUTS(x)
#endif

#if DEBUG
static void printdata(uint8_t *data, uint16_t len, char *outstr){
  uint16_t i;
  printf("\n%s: Len: %i", outstr, len);
  for(i = 0; i < len; i++){
    if(i % 16 == 0){
      printf("\n%s :", outstr);
    }
    if(i%2 == 0) putchar(' ');
    printf("%.2x", data[i]);

  }
  putchar('\n');
}
#else
#define printdata(...)
#endif

struct process_list {
	struct process * pcr;
	struct process_list * next;
	struct migmem_ptr * ptrs;
	struct migmem_hdr * mem;
};



#define CPY16(dest, src) do {                    \
    *((uint8_t *)&(dest)) = *((uint8_t *)&(src));              \
    *(((uint8_t *)&(dest)) + 1) = *(((uint8_t *)&(src)) + 1);      \
} while(0)


struct migmem_ptr * migmem_ptr_root;
struct process_list * procs;
struct process * proc_override;

#define PROC_OVR ((proc_override != NULL) ? proc_override : PROCESS_CURRENT())

static struct process_list * getproc_l(void){
  struct process_list * tmp;
  for(tmp = procs; tmp != NULL; tmp =  tmp->next){
    if(tmp->pcr == PROC_OVR) return tmp;
  }
  return NULL;
}

static struct process_list * proc_new(void){
  struct process_list * tmp;
  tmp  = malloc(sizeof(struct process_list));
  if(tmp == NULL) return NULL;
  memset(tmp, 0, sizeof(struct process_list));
  tmp->pcr = PROC_OVR;
  tmp->next = procs;
  procs = tmp;
  return tmp;
}

static struct process_list * curproc_l(void){
  struct process_list * tmp;
	if((tmp = getproc_l()) != NULL) return tmp;
	return proc_new();

}





int_fast8_t migmem_register(void ** ptrptr){
	struct migmem_ptr * pMS;
	struct migmem_ptr ** ppMS;

	ppMS = &(curproc_l()->ptrs);


	while(1){
		uint8_t ctr;
		if(*ppMS == NULL){
			*ppMS = malloc(sizeof(struct migmem_ptr));
			if(*ppMS == NULL) return 0;
			memset(*ppMS, 0, sizeof(struct migmem_ptr));
		}

		pMS = *ppMS;

		for(ctr = 0; ctr < PTRBLOCK; ctr++){
			if(pMS->ptr[ctr] == NULL){
				pMS->ptr[ctr] = ptrptr;
				return 1;
			}
		}

		ppMS = &(pMS->next);
	}
}


int_fast8_t migmem_unregister(void ** ptrptr){
	struct migmem_ptr * pMS;
	struct migmem_ptr ** ppMS;

	ppMS = &(curproc_l()->ptrs);


	while(1){
		uint8_t ctr;
		if(*ppMS == NULL) return 0;


		pMS = *ppMS;

		for(ctr = 0; ctr < PTRBLOCK; ctr++){
			if(pMS->ptr[ctr] == ptrptr){
				pMS->ptr[ctr] = NULL;
				return 1;
			}
		}

		ppMS = &(pMS->next);
	}
	return 0;
}

void * migmem_alloc(size_t size){
	struct migmem_hdr * tmp;
	struct process_list * cp;
	tmp = malloc(size + sizeof(struct migmem_hdr));
	if(tmp == NULL) return NULL;
	tmp->size = size;
	cp = curproc_l();
	if(PROC_OVR){
	  struct migmem_hdr ** walk = &(cp->mem);
	  while(*walk != NULL) walk= &((*walk)->next);
	  *walk = tmp;
	  tmp->next = NULL;

	} else {
	  tmp->next = cp->mem;
	  cp->mem = tmp;
	}
	return (void *)(tmp + 1);
}


void migmem_free(void * ptr){
	struct migmem_hdr * tmp;
	struct migmem_hdr * walk;
	tmp = (struct migmem_hdr *) ptr;
	tmp -= 1;
	for(walk = curproc_l()->mem; walk; walk = walk->next){
		if(walk->next == tmp){
			walk->next = tmp->next;
			free(tmp);
			return;
		}
	}
}

/**
 * Initialises the walker to serialise the migmem
 * @param state The State
 * @param proc The process to serialise
 * @return 1 Success
 * @return 0 Failed
 *
 */
int_fast8_t migmem_init_state_ser(struct migmem_state * state, struct process * proc){
  LEDRON;
  memset(state, 0, sizeof(struct migmem_state));
  //Get List entry
  state->proc = proc;
  proc_override = proc;
  state->proc_list =  getproc_l();
  proc_override = NULL;
  state->pih = minilink_programm_ih(proc);
  LEDROFF;
  if(state->pih == NULL) return 0;
  return 1;
  //All other values are set to 0.
}




/**
 *
 * @param walk
 * @param dst
 * @param len On first call len must be > 10
 * @return
 */
uintptr_t migmem_serialise(struct migmem_state * state, char * dst, uintptr_t len){
  char * end = dst + len;
  //char * start = dst;
  uint16_t lcopy;
  uint8_t ctr;
  // Push memory data
  LEDBON;
  if(!state->init){ //Init

    struct migmem_hdr * lmem;

    uint16_t nummem;
    uint16_t * nummemp;



    state->init = 1;

    nummemp = (uint16_t *)dst;
    (nummem) = 0;
    dst += 2;

    //Init data

    //Copy static addreses
    for(ctr = 0; ctr < MINILINK_SEC; ctr++){
      if(state->pih->mem[ctr].size){
        CPY16(*dst, state->pih->mem[ctr].ptr);
        dst += 2;  //.txt
        (nummem) ++;
      }
    }

    //copy dynamic addresses
    if(state->proc_list != NULL){
      lmem = state->proc_list->mem;
      state->cur.mem = lmem;

      //Count number of memory sections

      while(lmem != NULL){
        uintptr_t tmp;
        tmp = (uintptr_t) (lmem + 1);

        CPY16(*dst, tmp);
        dst += 2;
        (nummem) ++;
        lmem = lmem->next;

      }
    }




    CPY16(*nummemp, nummem);
    SDPRINTF("MIGM: Nmems: %i\n", nummem);
    /*
    if(DEBUG){
      uint8_t i;
      uint16_t * tmp;
      tmp =(uint16_t * )( dst - 8);
      for(i = 0; i< 4; i++){
        printf("MIGM: %i: %x\n", i, *tmp++);
      }
    }*/

  }
  while(!state->migptr){ //Migmem & ptr
    uint8_t memidx;
    //which one of the memories to handle
    if(!state->migmem){
      memidx = MINILINK_MIG;
    } else {
      memidx = MINILINK_MIGPTR;
    }
    if(end-dst >= 2){ // at least two chars.
      if(state->pos == 0){ //Init
        SDPRINTF("MIG %i: size: %i\n",memidx, state->pih->mem[memidx].size);
        //Copy size (security)
        CPY16(*dst, state->pih->mem[memidx].size);      dst += 2;  //.mig, size
      }
      lcopy = MIN(end - dst, state->pih->mem[memidx].size - state->pos);
      if(lcopy > 0){
        memcpy(dst,(char *)(state->pih->mem[memidx].ptr) + state->pos, lcopy);
        state->pos += lcopy;
        dst += lcopy;
      }
    }
    if(state->pos != state->pih->mem[memidx].size){
      goto done; //Do not attach any more data!
    }

    if(!state->migmem){
      state->migmem = 1;
    } else {
      state->migptr = 1;
    }

    state->pos = 0;

  }

  if(!state->mem){ //Memory is not finished
    while(end - dst >= 4){ // We need 4 bytes for init.
      if(state->cur.mem == NULL){
        state->mem = 1;
        SDPRINTF("Done with mem\n");

        //No need to set cur.ptr to NULL, as it has not been set
        //to a value before
        if(state->proc_list != NULL){
          state->cur.ptr = state->proc_list->ptrs;
        }

        //Terminate with 0
        *dst++ = 0;
        *dst++ = 0;
        break;
      }
      if(state->pos == 0){ //write header
        void * tmp;
        tmp = &(state->cur.mem[1]);
        //CPY16(*dst, tmp);                    dst += 2;// migmem, addr
        CPY16(*dst, state->cur.mem->size);    dst += 2;// migmem, size
      }
      lcopy = MIN(end-dst, state->cur.mem->size - state->pos);
      memcpy(dst,(char *)( &(state->cur.mem[1])) + state->pos, lcopy);
      state->pos += lcopy;
      dst += lcopy;
      if(state->cur.mem->size == state->pos){
        state->pos = 0;
        state->cur.mem = state->cur.mem->next;
      } else {
        goto done; //Do not attach any more data
      }
    }
  }
  //Push pointers
  if(!state->ptr){
    while(end-dst > 1){ //we need at least two bytes
      if(state->cur.ptr == NULL){
        state->ptr = 1;
        //Terminate with 0
        *dst++ = 0;
        *dst++ = 0;
        SDPRINTF("Done with ptr\n");
        break;
      }
      //If entry is used (!= NULL) copy
      if(state->cur.ptr->ptr[state->pos] != NULL){
        CPY16(*dst, state->cur.ptr->ptr[state->pos]);
        dst += 2;
      }
      state->pos++;
      if(state->pos == PTRBLOCK){
        state->pos = 0;
        state->cur.ptr = state->cur.ptr->next;
      }
    }
  }

done:

  if(DEBUG){
    printdata(start, len - (end-dst), "SER");
  }
  LEDBOFF;
  return len - (end-dst);
}


static void *
oldadd_to_new(void * addr, struct migmem_state * state)
{
  uint16_t ctr;
  uint16_t diff;
  uint16_t ign = 0;
  struct migmem_hdr * mhdr = NULL;

  if(state->proc_list) {
    mhdr = state->proc_list->mem;
  }
  SDPRINTF("Looking up %.4x\n", (uintptr_t)addr);
  for (ctr = 0; (mhdr != NULL) || ctr < MINILINK_SEC; ctr++) {
    SDPRINTF("Checking  %.4x ", (uintptr_t)state->oldmem[ctr - ign]);
    {
      void * addr_l;
      uint16_t size;
      if(ctr < MINILINK_SEC) {
        addr_l = state->pih->mem[ctr].ptr;
        size = state->pih->mem[ctr].size;
        if(size == 0) {
          ign++;
          continue;
        }
      }
      else {
        addr_l = mhdr + 1;
        size = mhdr->size;
        mhdr = mhdr->next; //And continue to next element
      }
      SDPRINTF(" -> %.4x\n", (uintptr_t)addr_l );

      if(addr < state->oldmem[ctr - ign])
        continue;

      diff = addr - state->oldmem[ctr - ign];
      if(diff <= size) { //Found it!
        SDPRINTF("New ADD %.4x, Size %.4x\n", (uintptr_t)addr_l, size);
        return addr_l + diff;
      }

    }
  }
  SDPUTS("FAILED!");
  return NULL;
}


int_fast8_t migmem_init_state_deser(struct migmem_state * state, struct process * proc){
  LEDRON;
  memset(state, 0, sizeof(struct migmem_state));
  //Get List entry
  state->proc = proc;
  proc_override = proc;
  state->pih = minilink_programm_ih(proc);
  if(state->pih == NULL) goto err;
  proc_override = NULL;
  LEDROFF;
  return 1;
err:
  proc_override = NULL;
  return 0;

  //All other values are set to 0.
}


/**
 * Deserialises migrated memory
 * @param state The state structure
 * @param src Where the serialised data is copied from
 * @param len The length of the data
 * @return 0 Succcess
 * @return 1 Done - no more data do be expected
 * @return < 0 Error
 */
//Todo Effizenter mit len und src arbeiten
int_fast8_t migmem_deserialize(struct migmem_state * state, char * src, size_t len){
  LEDGON;
  proc_override = state->proc;
  char * end = src + len;
  int8_t rv = 0;
  uint16_t copylen;
  if(DEBUG){
    printdata(src, len, "DSER");
  }
  if(!state->init){
    uint16_t nummem;
    if(len < 8){
      rv = -1;
      goto cleanup;
    }

    CPY16(nummem, *src);                   src += 2;
    SDPRINTF("MIGM: NMEM: %i\n", nummem);
    state->oldmem = malloc(nummem * sizeof(void *));
    if(state->oldmem == NULL){
      rv = -2;
      goto cleanup;
    }
    memcpy(state->oldmem, src, nummem * sizeof(void *));   src += nummem * sizeof(void *); //This includes migmem

    if(DEBUG){
      uint8_t i;
      uint16_t * tmp;
      tmp =(uint16_t * )( src - nummem * sizeof(void *));
      for(i = 0; i< nummem; i++){
        printf("MEM: %i: %x\n", i, *tmp++);
      }
    }

    state->init = 1;

  }
  while(!state->migptr){
    uint8_t memidx;
    if(!state->migmem){
      memidx = MINILINK_MIG;
    } else {
      memidx = MINILINK_MIGPTR;
    }

    if(state->pos == 0){
      uint16_t miglen;
      CPY16(miglen, *src);                   src +=2;
      SDPRINTF("MIGM %i: migmsize: %i - %x\n",memidx, miglen, *(uint16_t *)src);
      if(miglen != state->pih->mem[memidx].size){
        SDPRINTF("MIGM %i: Value: %i Expected: %i\n",memidx, miglen, state->pih->mem[memidx].size);
        rv = -3;
        goto cleanup;
      }

    }

    copylen = MIN(end-src, state->pih->mem[memidx].size - state->pos);
    if(copylen > 0){
      memcpy(state->pih->mem[memidx].ptr + state->pos, src, copylen);
      src += copylen;
      state->pos += copylen;
    }

    if(state->pos != state->pih->mem[memidx].size){
      break; //Get out of while loop!
    }


    if(!state->migmem){
      state->migmem = 1;
    } else {
      state->migptr = 1;
    }
    state->pos = 0;



  }
  if(!(end - src)) goto cleanup;

  if(!state->mem){
    while(end - src){
      if(state->pos == 0){
        //Here come a new memory block
        uint16_t tmp;
        //Let's get the size
        CPY16(tmp, *src);                            src += 2;
        //If tmp == 0 then it is the last memory block
        if(tmp == 0){
          //It's now possible to rewrite pointers...
          state->mem = 1;
          break;
        }
        SDPRINTF("Allocating %i bytes: ", tmp);
        if(state->proc_list == NULL) state->proc_list = curproc_l();

        //Now let's alloc the memory
        state->cur.mem = migmem_alloc(tmp);
        SDPRINTF("%x\n", (uintptr_t)state->cur.mem);

        if(state->cur.mem == 0){
          rv = -1;
          goto cleanup;
        }
        //One step back to get the header
        state->cur.mem --;
      }
      //Let's see how much data we can copy
      copylen = MIN(end - src, state->cur.mem->size - state->pos);
      //Copy the data
      memcpy(((char *)(state->cur.mem + 1)) + state->pos, src, copylen);
      src += copylen;
      state->pos += copylen;

      if(state->pos == state->cur.mem->size){
        //The block is copied!
        state->pos = 0;
      }
    }
  }
  if(!(end - src)) goto cleanup;


  if(!state->ptr){
    //TODO: Run only once!



    while(end - src){

      SDPUTS("migm: Ptrs");
      if((end -src) == 1){
        rv = -1;
        goto cleanup;
      }
      void **tmp;
      CPY16(tmp, *src);
      src += 2;
      if(tmp == 0){
        rv = 1;
        goto cleanup;
      }
      //Todo Errorchecking. -> Sender side?
      tmp = oldadd_to_new(tmp, state);
      *tmp = oldadd_to_new(*tmp, state);
      migmem_register(tmp);
    }
  }

cleanup:
  if(rv == 1){ //This isn't nice!
    {

      void ** end;
      void ** ptr;
      SDPUTS("migm: MIGPTR");
      ptr = state->pih->mem[MINILINK_MIGPTR].ptr;
      end = (void ** )((uintptr_t)ptr + state->pih->mem[MINILINK_MIGPTR].size);

      for(;ptr < end; ptr++){
        *ptr = oldadd_to_new(*ptr, state);
      }

    }
  }


  proc_override = NULL;
  LEDGOFF;
  return rv;
}
