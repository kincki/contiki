#include "contiki.h"
#include "contiki-net.h"
#include "strings.h"
#include "petascii.h"
#include "base64.h"
#include <stdio.h>
#include <string.h>

#include "twitter_resolve.h"

#include "twitter_private.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uip_ipaddr_t twitter_address;
int twitter_port = 80;

static struct resolve_state {
    cb_resolve_status_t cb_resolve_status;
    struct process *parent;
} state;

PROCESS(twitter_resolve_process, "Twitter DNS resolver");

    int
twitter_resolve(cb_resolve_status_t cb_resolve_status, struct process *parent)
{
    state.cb_resolve_status = cb_resolve_status;
    state.parent = parent;

#if UIP_UDP
    resolv_query(DEFAULT_HOST);
    process_start(&twitter_resolve_process, NULL);
    return 1;
#else /* UIP_UDP */
    uip_ipaddr(ip_address, DEFAULT_IP[0], DEFAULT_IP[1], DEFAULT_IP[2], DEFAULT_IP[3]);
    return 0;
#endif /* UIP_UDP */
}

PROCESS_THREAD(twitter_resolve_process, ev, data)
{
#if UIP_UDP
    uip_ipaddr_t *ipaddr;

    PROCESS_BEGIN();
    PRINTF("DNS LOOKUP\r\n");
    state.cb_resolve_status(twitter_resolve_lookup);

    PROCESS_WAIT_EVENT_UNTIL(ev == resolv_event_found);

    if (resolv_lookup(DEFAULT_HOST, &ipaddr) == RESOLV_STATUS_CACHED) {
        PRINTF("DNS SUCCESS\r\n");
        state.cb_resolve_status(twitter_resolve_success);
        twitter_address = *ipaddr;
    } else {
        PRINTF("DNS FAILURE\r\n");
        state.cb_resolve_status(twitter_resolve_failure);
        uip_ipaddr(&twitter_address, DEFAULT_IP[0], DEFAULT_IP[1], DEFAULT_IP[2], DEFAULT_IP[3]);
    }

    if (state.parent) process_post(state.parent, PROCESS_EVENT_CONTINUE, NULL);

    PROCESS_END();
#endif /* UIP_UDP */
}
