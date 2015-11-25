/*
 * Copyright (c) 2009, Siemens AG.
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
 * 3. Neither the name of the company nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COMPANY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COMPANY OR CONTRIBUTORS BE LIABLE
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
 * \file
 *         <purpose of the file>
 * \author Gidon Ernst <gidon.ernst@googlemail.com>
 * \author Thomas Mair <mair.thomas@gmx.net>
 */

/* a routing module using the uaodv implementation */
#include "uaodvroute.h"
#include "uaodv.h"

#include "util.h"

static int activate(void);
static int deactivate(void);
static uip_ipaddr_t *lookup(uip_ipaddr_t *, uip_ipaddr_t *);
static void unreachable(uip_ipaddr_t *);

const struct uip_router uaodvroute = { activate, deactivate, lookup, unreachable };


#define DEBUG 0

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


static int activate(void) {
    process_start(&uaodv_process, NULL);
    return 0;
}

static int deactivate(void) {
    // process_stop(&uaodv_process);
    return 0;
}

static uip_ipaddr_t *lookup(uip_ipaddr_t *destipaddr, uip_ipaddr_t *nexthop) {
    PRINTF("uaodvroute: looking ip route to " IP_FORMAT "\n", IP_PARAMS(destipaddr));

    struct uaodv_rt_entry *e = uaodv_rt_lookup(destipaddr);

    if(e == NULL) {
        uaodv_request_route_to(destipaddr);
        return NULL;
    }

    PRINTF("uaodvroute: " IP_FORMAT " can be reached via " IP_FORMAT "\n", IP_PARAMS(destipaddr), IP_PARAMS(&e->nexthop));
    return &e->nexthop;
}

static void unreachable(uip_ipaddr_t *destipaddr) {
    struct uaodv_rt_entry *e = uaodv_rt_reverse_lookup_neighbor(destipaddr);
    PRINTF("reverse lookup for " IP_FORMAT "\n", IP_PARAMS(destipaddr));
    uaodv_rt_print_all();

    if(e != NULL) {
        PRINTF("bad dest: " IP_FORMAT "\n", IP_PARAMS(&e->dest));
        uaodv_bad_dest(&e->dest);
    }
}
