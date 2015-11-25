#include "contiki.h"
#include "contiki-net.h"
#include "strings.h"
#include "base64.h"
#include "parse.h"
#include <stdio.h>
#include <string.h>

#include "twitter_timeline.h"

#include "twitter_private.h"

#define MAX_USERNAME_PASSWORD  44
#define MAX_MESSAGE           160
#define MAX_LENGTH             10


extern uip_ipaddr_t twitter_address;
extern int twitter_port;

static struct twitter_timeline_state {
  struct psock sock;
  char base64_username_password[MAX_USERNAME_PASSWORD];
  uint8_t inputbuf[2][256];
  cb_on_status_t cb_on_status;
  unsigned char count;
  cb_connection_status_t cb_connection_status;
  struct process *parent;
} state;


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

PROCESS(twitter_timeline_process, "Twitter status");

// Add master process
// Post event if process ready
int
twitter_timeline(const uint8_t *username_password, unsigned char count, cb_on_status_t cb_on_status, cb_connection_status_t cb_connection_status, struct process *parent)
{
  process_exit(&twitter_timeline_process);

  state.cb_on_status = cb_on_status;
  if (count <= 99) {
        state.count = count;
  } else {
        state.count = 99;
  }
  state.cb_connection_status = cb_connection_status;
  state.parent = parent;

  base64_encode((char*)username_password,  strlen(username_password), state.base64_username_password, sizeof(state.base64_username_password));

  state.cb_connection_status(twitter_timeline_init);

  process_start(&twitter_timeline_process, NULL);
  return 1;
}

static int
handle_connection(void)
{
  static unsigned char on_status_needed;
  static uint16_t last_datalen;
  static char twitter_get[64];

  PSOCK_BEGIN(&state.sock);

  /* Send GET verb */
  state.cb_connection_status(twitter_timeline_get);
  memcpy(twitter_get, TWTR_GET_STATUS, sizeof(TWTR_GET_STATUS));
  twitter_get[43] = 0x30 + (state.count / 10); // 'Calculate' number instead of just printf. Reason is PETSCII on C64
  twitter_get[44] = 0x30 + (state.count % 10);
  PSOCK_SEND_STR(&state.sock, twitter_get);

  /* Send Authorization header */
  state.cb_connection_status(twitter_timeline_auth);
  PSOCK_SEND_STR(&state.sock, TWTR_AUTH);
  PSOCK_SEND_STR(&state.sock, state.base64_username_password);
  PSOCK_SEND_STR(&state.sock, TWTR_NEWLINE);

  /* Send Agent header */
  state.cb_connection_status(twitter_timeline_agent);
  PSOCK_SEND_STR(&state.sock, TWTR_USER_AGENT);
  PSOCK_SEND_STR(&state.sock, TWTR_HOST);

  /* Send header-body boundary */
  state.cb_connection_status(twitter_timeline_boundary);
  PSOCK_SEND_STR(&state.sock, TWTR_NEWLINE);

  /* Read body and pass on to parser */
  state.cb_connection_status(twitter_timeline_body);
  on_status_needed = state.cb_on_status(NULL, NULL);
  state.inputbuf[1][0] = 0;
  last_datalen = 0;
  while (on_status_needed) {
    // Prefill inputbuf[0] with blanks which will be skipped during parsing
    memset(state.inputbuf[0], 0x20, sizeof(state.inputbuf[0])); // ' '
    // Copy what we read last time from inputbuf[1] to the end of inputbuf[0]
    // on order to parse it again together with what we'll read this time so
    // that strings crossing read borders can be found
    strcpy(state.inputbuf[1] - last_datalen, state.inputbuf[1]);
    // Prefill inputbuf[1] with zeros to make sure there's a zero terminated
    // string after the read below
    memset(state.inputbuf[1], '\0', sizeof(state.inputbuf[1]));
    PSOCK_READTO(&state.sock, 0x7d); // '}'
    last_datalen = PSOCK_DATALEN(&state.sock);
    PRINTF("%s", &state.inputbuf[1]);
    on_status_needed = findStatus((const char*)&state.inputbuf[0], state.cb_on_status, NULL);
  }

  state.cb_connection_status(twitter_timeline_end);

  PSOCK_CLOSE(&state.sock);
  PSOCK_EXIT(&state.sock);

  PSOCK_END(&state.sock);
}

PROCESS_THREAD(twitter_timeline_process, ev, data)
{
    struct uip_conn *conn;

    PROCESS_BEGIN();
    PRINTF("CONNECT\r\n");
    state.cb_connection_status(twitter_timeline_connect);

    conn = tcp_connect(&twitter_address, uip_htons(twitter_port), NULL);
    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);

    if (uip_connected()) {
        PRINTF("CONNECTED\r\n");
        PSOCK_INIT(&state.sock, (uint8_t *)state.inputbuf[1], sizeof(state.inputbuf[1]) - 1);
        while(!(uip_closed() || uip_aborted() || uip_timedout())) {
            PRINTF("LOOP\r\n");
            PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event);
            handle_connection();
        }
    }
    PRINTF("DONE");
    if (state.parent) {
        process_post(state.parent, PROCESS_EVENT_CONTINUE, NULL);
    }

    PROCESS_EXIT();
    PROCESS_END();
}
