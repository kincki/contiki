/*
 * Copyright (c) 2010, Vrije Universiteit Brussel
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
 *
 * Author: Joris Borms <joris.borms@vub.ac.be>
 *
 */

#include "contiki.h"

#include "lib/memb.h"
#include "lib/list.h"

#include "node-attr.h"

#define DEBUG 1

#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


/*---------------------------------------------------------------------------*/
static struct node_addr*
node_addr_get(node_attr_table* table, const rimeaddr_t* addr)
{
  // check if addr is derived from table, inside memb
  if (memb_inmemb(table->nodes_mem, addr)){
    return (struct node_addr*)
        (((void*)addr) - offsetof(struct node_addr, addr));
  }
  struct node_addr* item = list_head(table->nodes);
  while (item != NULL){
    if (rimeaddr_cmp(addr,&item->addr)){
      return item;
    }
    item = item->next;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
struct node_addr*
node_attr_list_nodes(node_attr_table* table)
{
  return list_head(table->nodes);
}
/*---------------------------------------------------------------------------*/
int
node_attr_register(node_attr_table* table, struct node_attr* def)
{
  list_push(table->attrs, def);
  // set default values for already existing nodes
  struct node_addr* addr = list_head(table->nodes);
  while (addr != NULL) {
    if (def->default_value != NULL){
      memcpy(def->data + addr->index * def->size, def->default_value, def->size);
    } else { // fill with zeroes
      memset(def->data + addr->index * def->size, 0, def->size);
    }
    addr = addr->next;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
int
node_attr_has_node(node_attr_table* table, const rimeaddr_t* addr){
  return node_addr_get(table, addr) != NULL;
}
/*---------------------------------------------------------------------------*/
int
node_attr_add_node(node_attr_table* table, const rimeaddr_t* addr)
{
  if (node_attr_has_node(table, addr)){
    return 1;
  }

  struct node_addr* item = memb_alloc(table->nodes_mem);
  if (item == NULL){
    return 0;
  }

  list_push(table->nodes, item);

  item->time = 0;
  rimeaddr_copy(&item->addr,addr);

  // look up index and set default values
  uint16_t i;
  struct node_addr* ptr = MEMB_MEM(table->nodes_mem);
  for(i = 0; i < MEMB_BLOCK_COUNT(table->nodes_mem); ++i) {
    if(&ptr[i] == item) {
      break;
    }
  }

  item->index = i;

  struct node_attr* def = list_head(table->attrs);
  while(def != NULL){
    if (def->default_value != NULL){
      memcpy(def->data + i * def->size, def->default_value, def->size);
    } else { // fill with zeroes
      memset(def->data + i * def->size, 0, def->size);
    }
    def = def->next;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
void*
node_attr_get_data(node_attr_table* table, struct node_attr* def, const rimeaddr_t* addr)
{
  struct node_addr* attr = node_addr_get(table, addr);
  if (attr != NULL){
    return (void*)(def->data + attr->index * def->size);
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
int
node_attr_set_data(node_attr_table* table, struct node_attr* def,
		const rimeaddr_t* addr, void* data)
{
  struct node_addr* attr = node_addr_get(table, addr);
  if (attr == NULL){
    if (node_attr_add_node(table, addr)){
      attr = node_addr_get(table, addr);
    }
  }
  if (attr != NULL){
    attr->time = 0;
    memcpy((void*)(def->data + attr->index * def->size), data, def->size);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
node_attr_tick(node_attr_table* table, const rimeaddr_t* addr)
{
  struct node_addr* attr = node_addr_get(table, addr);
  if (attr != NULL){
    attr->time = 0;
  }
}
/*---------------------------------------------------------------------------*/
#define TIMEOUT_SECONDS 5
static void timeout_check(void* ptr)
{
	node_attr_table* table = (node_attr_table*) ptr;
  if(table->timeout > 0){
    struct node_addr* item = list_head(table->nodes);
    while (item != NULL){
      item->time += TIMEOUT_SECONDS;
      if (item->time >= table->timeout){
        struct node_addr* next_item = item->next;
        memb_free(table->nodes_mem, item);
        list_remove(table->nodes, item);
        item = next_item;
      } else {
        item = item->next;
      }
    }
    ctimer_set(table->timer, TIMEOUT_SECONDS * CLOCK_SECOND,
    		timeout_check, ptr);
  }
}
/*---------------------------------------------------------------------------*/
void
node_attr_set_timeout(node_attr_table* table, uint16_t time)
{
  if (table->timeout == 0 && time > 0){
    ctimer_set(table->timer, TIMEOUT_SECONDS * CLOCK_SECOND,
     		timeout_check, table);
  } else if (table->timeout > 0 && time == 0){
  	ctimer_stop(table->timer);
  }
  table->timeout = time;
}
/*---------------------------------------------------------------------------*/
uint16_t node_attr_get_timeout(node_attr_table* table)
{
  return table->timeout;
}

