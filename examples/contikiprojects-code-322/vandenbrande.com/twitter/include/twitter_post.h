#ifndef __TWITTER_CLIENT_H__
#define __TWITTER_CLIENT_H__

enum twitter_post_phase
{
    twitter_post_connect = 0,
    twitter_post_post,
    twitter_post_auth,
    twitter_post_agent,
    twitter_post_length,
    twitter_post_type,
    twitter_post_boundary,
    twitter_post_status,
    twitter_post_end,
    twitter_post_finally
};

typedef void (*cb_connection_post_status_t)(enum twitter_post_phase status);

int twitter_post(const uint8_t *username_password, const char *message, cb_connection_post_status_t cb_connection_status, struct process *parent);

#endif /* __TWITTER_CLIENT_H__ */
