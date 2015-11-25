#ifndef __TWITTER_STATUS_H__
#define __TWITTER_STATUS_H__

// All phases in the communication protocol to get the timeline. These are low level HTTP phases.
enum twitter_timeline_phase
{
    twitter_timeline_connect = 0,
    twitter_timeline_init,
    twitter_timeline_get,
    twitter_timeline_auth,
    twitter_timeline_agent,
    twitter_timeline_boundary,
    twitter_timeline_body,
    twitter_timeline_end,
    twitter_timeline_finally
};

typedef int (*cb_on_status_t)(char *friend, char *status);
typedef void (*cb_connection_status_t)(enum twitter_timeline_phase status);

int twitter_timeline(const uint8_t *username_password, unsigned char count, cb_on_status_t cb_on_status, cb_connection_status_t cb_connection_status, struct process *parent);

#endif /* __TWITTER_STATUS_H__ */
