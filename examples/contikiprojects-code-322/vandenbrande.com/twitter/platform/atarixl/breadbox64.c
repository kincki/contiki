#include "contiki-net.h"
#include <string.h>
#include <stdio.h>
#include "breadbox64.h"
#include "twitter_resolve.h"
#include "twitter_post.h"
#include "twitter_timeline.h"
#include "base64.h"
#include "input.h"

static char *resolve_status[] =
{
    "d:lookup  ",
    "d:success ",
    "d:failure ",
    "          "
};

static char *receive_status[] =
{
    "r:connect ",
    "r:init    ",
    "r:get     ",
    "r:auth    ",
    "r:agent   ",
    "r:boundary",
    "r:body    ",
    "r:end     ",
    "          "
};

static char *send_status[] =
{
    "s:connect ",
    "s:post    ",
    "s:auth    ",
    "s:agent   ",
    "s:length  ",
    "s:type    ",
    "s:boundary",
    "s:status  ",
    "s:end     ",
    "          "
};

enum connection_status
{
    inactive = 0,
    resolving,
    updating,
    posting
};

PROCESS(twitter_process, "Twitter Client");
AUTOSTART_PROCESSES(&twitter_process);

void
update_status_bar(struct prompt *p)
{
    gotoxy(SCREEN_WIDTH-8, SCREEN_HEIGHT-5);
    cprintf("%03d/%03d", prompt_remaining(p), prompt_size(p));
}

int
show_status(char *friend, char *status)
{
    unsigned char x;
    static unsigned char y;
    size_t friend_len;
    size_t status_len;

    static char index = 0;

    if (NULL == friend) { // reset of screen
        y = 2;
        index = 0;
        return 1;
    }

    friend_len = strlen(friend);
    status_len = strlen(status);
    if (y + (friend_len + status_len + 2)/SCREEN_WIDTH > SCREEN_HEIGHT-6) return 0;

    gotoxy(0, y);
    revers(index%2);
    cprintf("%s: ", friend);

    x = friend_len + 2;
    while (status_len > SCREEN_WIDTH - x) {
        char save = status[SCREEN_WIDTH - x];

        status[SCREEN_WIDTH - x] = '\0';
        cputsxy(x, y, status);
        status[SCREEN_WIDTH - x] = save;
        status += SCREEN_WIDTH - x;
        status_len -= SCREEN_WIDTH - x;
        x = 0;
        ++y;
    }
    cputsxy(x, y, status);
    ++y;

    revers(0);
    ++index;

    return 1;
}

void
show_connection_status(unsigned char index, char *strings[])
{
    cputsxy(SCREEN_WIDTH-10, 0, strings[index]);
}

void
show_resolve_phase(enum twitter_resolve_phase status)
{
    show_connection_status((unsigned char)status, resolve_status);
}

void
show_status_phase(enum twitter_timeline_phase status)
{
    show_connection_status((unsigned char)status, receive_status);
}

void
show_post_phase(enum twitter_post_phase status)
{
    show_connection_status((unsigned char)status, send_status);
}

void
spacer(unsigned char y)
{
    cputcxy(0,              y, CH_CROSS); chlinexy(1,  y, SCREEN_WIDTH-2);
    cputcxy(SCREEN_WIDTH-1, y, CH_URCORNER);
}

void
clear_status_pane(void)
{
    unsigned char i;
    for (i=2; i<SCREEN_HEIGHT-5; i++) {
        cclearxy(0, i, SCREEN_WIDTH);
    }
}

PROCESS_THREAD(twitter_process, ev, data)
{
    static struct timer status_timer;
    static struct prompt p;
    static char buffer[141];
    static char credentials[32];
    static int status_counter;
    static char* credentials_ok;
    static enum connection_status connection_status;


    PROCESS_BEGIN();

    // init screen
    clrscr();
    bgcolor(COLOR_BLACK);
    bordercolor(COLOR_BLACK);

    // show logo
    revers(1);
    cputsxy(1, 0, "BREADBOX64");
    revers(0);

    spacer(1);
    cputsxy(0, 5, "  Welcome to the Atari twitter client!");
    cputsxy(0, 7, "   (c) 2009-2010 Johan Van den Brande");
    cputsxy(0, 8, "                 and Oliver Schmidt");
    cputsxy(0, 16,"Note: You need a supertweet.net account!");

    spacer(SCREEN_HEIGHT-5);
    cputsxy(2, SCREEN_HEIGHT-5, "Enter your supertweet credentials.");
    cputsxy(0, SCREEN_HEIGHT-4, "(username>:<password)");

    // allow other processes to initialize
    PROCESS_PAUSE();

    connection_status = twitter_resolve(show_resolve_phase, PROCESS_CURRENT()) ? resolving : inactive;

    prompt_init(0, SCREEN_HEIGHT-3, credentials, sizeof(credentials), 0, 0, NULL, &p);

    credentials_ok = 0;
    while (!credentials_ok || connection_status != inactive) {
        while (!prompt_ask(&p)) {
            process_poll(&twitter_process);
            PROCESS_WAIT_EVENT();
            if (ev == PROCESS_EVENT_CONTINUE) {
                connection_status = inactive;
            }
        }
        credentials_ok = strchr(credentials, ':');
    }

    *credentials_ok = '\0';
    cputsxy(12, 0, credentials);
    *credentials_ok = ':';

    spacer(SCREEN_HEIGHT-5);
    cputsxy(2, SCREEN_HEIGHT-5, "What are you doing?");
    cclearxy(0, SCREEN_HEIGHT-4, SCREEN_WIDTH);
    cclearxy(0, SCREEN_HEIGHT-3, SCREEN_WIDTH);

    prompt_init(0, SCREEN_HEIGHT-4, buffer, sizeof(buffer), 0, 0, update_status_bar, &p);

    timer_set(&status_timer, CLOCK_SECOND);
    status_counter = 0;
    while (1) {
        process_poll(&twitter_process);
        PROCESS_WAIT_EVENT();

        if (prompt_ask(&p) && connection_status == inactive) {
            twitter_post((uint8_t *)credentials, buffer, show_post_phase, PROCESS_CURRENT());
            connection_status = posting;
            prompt_reset(&p);
        }

        if (timer_expired(&status_timer) && connection_status == inactive) {
            gotoxy(SCREEN_WIDTH-3, 0);
            cprintf("%03d", status_counter);
            if (status_counter == 0) {
                status_counter = 120;
                clear_status_pane();
                twitter_timeline((uint8_t *)credentials, SCREEN_HEIGHT-7, show_status, show_status_phase, PROCESS_CURRENT());
                connection_status = updating;
            } else {
                --status_counter;
                timer_reset(&status_timer);
            }
        }

        if (ev == PROCESS_EVENT_CONTINUE) {
            switch (connection_status) {
            case posting:
                show_post_phase(twitter_post_finally);
                status_counter = 0; // force update in 1 second
                break;
            case updating:
                show_status_phase(twitter_timeline_finally);
                timer_reset(&status_timer);
                break;
            }
            connection_status = inactive;
        }
   }

    PROCESS_END();
}
