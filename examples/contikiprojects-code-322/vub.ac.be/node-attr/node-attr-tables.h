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

#ifndef __NODE_ATTR_TABLES_H__
#define __NODE_ATTR_TABLES_H__

#include "node-attr.h"

#ifdef NEIGHBOR_TABLE_CONF_SIZE
#define NEIGHBOR_TABLE_SIZE NEIGHBOR_TABLE_CONF_SIZE
#else
#define NEIGHBOR_TABLE_SIZE  8
#endif

#ifdef NETWORK_TABLE_CONF_SIZE
#define NETWORK_TABLE_SIZE NETWORK_TABLE_CONF_SIZE
#else
#define NETWORK_TABLE_SIZE  16
#endif

#if NEIGHBOR_TABLE_SIZE > 0
	NODE_ATTR_TABLE(neighbor_table, NEIGHBOR_TABLE_SIZE);
#endif

#if NETWORK_TABLE_SIZE > 0
	NODE_ATTR_TABLE(network_table, NETWORK_TABLE_SIZE);
#endif




#endif /* __NODE_ATTR_TABLES_H__ */


