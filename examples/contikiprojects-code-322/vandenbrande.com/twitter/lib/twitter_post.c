#include "contiki.h"
#include "contiki-net.h"
#include "strings.h"
#include "petascii.h"
#include "base64.h"
#include <stdio.h>
#include <string.h>

#include "twitter_post.h"

#include "twitter_private.h"

#define MAX_USERNAME_PASSWORD  44
#define MAX_MESSAGE           160
#define MAX_LENGTH             10

extern uip_ipaddr_t twitter_address;
extern int twitter_port;

static struct twitter_state {
    struct psock sock;
    char lengthstr[MAX_LENGTH];
    char base64_username_password[MAX_USERNAME_PASSWORD];
    char message[MAX_MESSAGE];
    uint8_t inputbuf[UIP_TCP_MSS];
    cb_connection_post_status_t cb_connection_status;
    struct process *parent;
} state;


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

PROCESS(twitter_post_process, "Twitter client");

    int
twitter_post(const uint8_t *username_password, const char *msg, cb_connection_post_status_t cb_connection_status, struct process *parent)
{
    process_exit(&twitter_post_process);

    PSOCK_INIT(&state.sock, (uint8_t *)state.inputbuf, sizeof(state.inputbuf) - 1);
    state.cb_connection_status = cb_connection_status;
    state.parent = parent;

    base64_encode((char*)username_password,  strlen(username_password), state.base64_username_password, sizeof(state.base64_username_password));

    /* Copy the status message, and avoid the leading whitespace. */
    strcpy(state.message, TWTR_STATUS);
    strcpy(&state.message[7], msg);

    PRINTF("username_password '%s'\n", state.base64_username_password);
    PRINTF("message '%s'\n", state.message);

    /* Spawn process to deal with TCP connection */
    process_start(&twitter_post_process, NULL);
    return 1;
}
/*---------------------------------------------------------------------------*/
    static int
handle_connection(void)
{
    PSOCK_BEGIN(&state.sock);
    /* Send POST header */

    PRINTF("START SENDING\n");
    state.cb_connection_status(twitter_post_post);
    PSOCK_SEND_STR(&state.sock, TWTR_POST_HEADER);

    /* Send Authorization header */
    state.cb_connection_status(twitter_post_auth);
    PSOCK_SEND_STR(&state.sock, TWTR_AUTH);
    PSOCK_SEND_STR(&state.sock, state.base64_username_password);
    PSOCK_SEND_STR(&state.sock, TWTR_NEWLINE);

    /* Send Agent header */
    state.cb_connection_status(twitter_post_agent);
    PSOCK_SEND_STR(&state.sock, TWTR_USER_AGENT);
    PSOCK_SEND_STR(&state.sock, TWTR_HOST);
    PSOCK_SEND_STR(&state.sock, TWTR_ACCEPT);

    /* Send Content length header */
    state.cb_connection_status(twitter_post_length);
    PSOCK_SEND_STR(&state.sock, TWTR_CONTENT_LENGTH);
    snprintf(state.lengthstr, sizeof(state.lengthstr), "%d", strlen(state.message));
    PSOCK_SEND_STR(&state.sock, state.lengthstr);
    PSOCK_SEND_STR(&state.sock, TWTR_NEWLINE);

    /* Send Content type header */
    state.cb_connection_status(twitter_post_type);
    PSOCK_SEND_STR(&state.sock, TWTR_CONTENT_TYPE);

    /* Send header-body boundary */
    state.cb_connection_status(twitter_post_boundary);
    PSOCK_SEND_STR(&state.sock, TWTR_NEWLINE);

    /* Send status message */
    state.cb_connection_status(twitter_post_status);
    PSOCK_SEND_STR(&state.sock, state.message);

    /* Close connection */
    PSOCK_CLOSE(&state.sock);

    PRINTF("END SENDING\n");

    PSOCK_EXIT(&state.sock);
    PSOCK_END(&state.sock);

}

PROCESS_THREAD(twitter_post_process, ev, data)
{
    struct uip_conn *conn;

    PROCESS_BEGIN();

    state.cb_connection_status(twitter_post_connect);
    conn = tcp_connect(&twitter_address, uip_htons(twitter_port), NULL);

    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

    if (uip_connected()) {
        PSOCK_INIT(&state.sock, (uint8_t *)state.inputbuf, sizeof(state.inputbuf) - 1);
        while(!(uip_closed() || uip_aborted() || uip_timedout())) {
            PRINTF("LOOP\r\n");
            PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
            handle_connection();
        }
    }

    state.cb_connection_status(twitter_post_end);
    if (state.parent) process_post(state.parent, PROCESS_EVENT_CONTINUE, NULL);
    PROCESS_END();
}
