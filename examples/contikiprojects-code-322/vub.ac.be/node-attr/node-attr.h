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

#ifndef NODEATTR_H_
#define NODEATTR_H_

#include "rime.h"
#include "lib/list.h"
#include "lib/memb.h"

#define mtypeof(struct,member) typeof(((struct*)0)->member)
#define msizeof(struct,member) sizeof(((struct*)0)->member)


/**
 * \brief      properties of a node table
*/
typedef struct {
	struct memb* nodes_mem;
	list_t nodes;
	list_t attrs;
	uint16_t timeout;
	struct ctimer* timer;
} node_attr_table;

/**
 * \brief      properties of a single node
*/
struct node_addr {
  struct node_addr* next;
  rimeaddr_t  addr;
  uint16_t    time;
  uint16_t    index;
};

/**
 * \brief      properties that define a node attribute
 */
struct node_attr {
  struct node_attr* next;
  uint16_t  size;
  void*     default_value;
  void*     data;
};


/* \brief			Defines a table of nodes that can hold at most 'size' node addresses
 */
#define NODE_ATTR_TABLE(name, size) \
	typedef struct { char bytes[size]; } name##_size_t; \
	MEMB(_##name##_memb, struct node_addr, size); \
	static void* list_##name##_nodes = NULL; \
	static void* list_##name##_attrs = NULL; \
	static struct ctimer name##_ctimer; \
	node_attr_table name = \
	{&_##name##_memb, &list_##name##_nodes, &list_##name##_attrs, 0, &name##_ctimer}; \


/**
 * \brief        Define space a parameter in a node table.
 * \param table  The name (not a reference) of the table this attribute belongs to.
 * \param type   The type of the attribute.
 * \param attr   The name of the attribute.
 * \param def    A ptr to the default value for this attribute. If NULL, attribute will
 *               be filled with zeroes by default.
 *
 *               The attribute 'attr_name' should be registered with 'node_attr_register'
 *               during initialization.
 */
#define NODE_ATTRIBUTE(table_name, type, attr_name, default_value_ptr) \
  static type _##attr_name##_mem[msizeof(table_name##_size_t,bytes)]; \
  static struct node_attr attr_name = \
    {NULL, sizeof(type), default_value_ptr, (void*)_##attr_name##_mem} ; \


/**
 * \brief      register a node attribute
 * \retval     non-zero if successful, zero if not
 */
int node_attr_register(node_attr_table*, struct node_attr*);

/**
 * \retval     head of node list, useful for iterating over all nodes
 */
struct node_addr* node_attr_list_nodes(node_attr_table*);

/**
 * \brief      Check if a node is already added to the node table
 * \retval     non-zero if present, zero if not
 */
int node_attr_has_node(node_attr_table*, const rimeaddr_t* addr);

/**
 * \brief      Add a node entry to node table
 * \retval     zero if unsuccessful, non-zero if successful or if node was already in the table
 */
int node_attr_add_node(node_attr_table*, const rimeaddr_t* addr);

/**
 * \brief      Get pointer to node table data specified by id
 * \param      table to query
 * \param      requested attribute
 * \param addr requested node
 * \retval     pointer to data, NULL if node was not found
 *
 *             Searches node table for addr and returns pointer to data section
 *             specified by attribute type and addr.
 *             This pointer should not be saved, as it may point to data from another
 *             node in the future if nodes get removed/added over time.
 */
void* node_attr_get_data(node_attr_table*, struct node_attr*, const rimeaddr_t* addr);

/**
 * \brief      Copy data to node table
 * \retval     non-zero if successful, zero if not
 *
 *             Copies data to specific part of the node table, specified by
 *             node and attribute type, and resets timeout for that node.
 *             If node was not found, this will add a new node to the table.
 */
int node_attr_set_data(node_attr_table*, struct node_attr*, const rimeaddr_t* addr, void* data);

/**
 * \brief      Set lifetime of node entries in a table
 * \param      table to query
 * \param      Lifetime in seconds. If 0, entries will not time out
 */
void node_attr_set_timeout(node_attr_table*, uint16_t);

/**
 * \brief      get lifetime of node entries in a table. If 0, entries will not time out
 */
uint16_t node_attr_get_timeout(node_attr_table*);

/**
 * \brief      reset timeout of a node to prevent it from being removed
 */
void node_attr_tick(node_attr_table*, const rimeaddr_t*);




#endif /* NODEATTR_H_ */
