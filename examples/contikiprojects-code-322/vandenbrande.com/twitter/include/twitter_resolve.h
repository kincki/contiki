#ifndef __TWITTER_RESOLVE_H__
#define __TWITTER_RESOLVE_H__

enum twitter_resolve_phase
{
    twitter_resolve_lookup = 0,
    twitter_resolve_success,
    twitter_resolve_failure,
    twitter_resolve_finally
};

typedef void (*cb_resolve_status_t)(enum twitter_resolve_phase status);

int twitter_resolve(cb_resolve_status_t cb_resolve_status, struct process *parent);

#endif /* __TWITTER_RESOLVE_H__ */
