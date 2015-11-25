#include "contiki-net.h"
#include <string.h>
#include <stdio.h>
#include "breadbox64.h"
#include "twitter_resolve.h"
#include "twitter_post.h"
#include "twitter_timeline.h"
#include "base64.h"
#include "petascii.h"
#include "input.h"

static char title[] =
{
    COLOR_RED,      'B',
    COLOR_BROWN,    'R',
    COLOR_GRAY1,    'E',
    COLOR_GREEN,    'A',
    COLOR_BLUE,     'D',
    COLOR_RED,      'B',
    COLOR_BROWN,    'O',
    COLOR_GRAY1,    'X',
    COLOR_GREEN,    '6',
    COLOR_BLUE,     '4',
    255,            NULL
};

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
    textcolor(COLOR_BLUE);
    gotoxy(SCREEN_WIDTH-14, SCREEN_HEIGHT-3);
    cprintf(" %03d / %03d ", prompt_remaining(p), prompt_size(p));
}

int
show_status(char *friend, char *status)
{
    char friend_pet[32];
    char status_pet[141];
    unsigned char x;
    static unsigned char y;
    char *status_ptr = status_pet;
    size_t friend_len;
    size_t status_len;

    unsigned char color1, color2, color3;
    static char index = 0;

    if (index%2) {
        color1 = COLOR_GREEN;
        color2 = COLOR_LIGHTBLUE;
        color3 = COLOR_LIGHTBLUE;
    } else {
        color1 = COLOR_RED;
        color2 = COLOR_BROWN;
        color3 = COLOR_BROWN;
    }

    if (NULL == friend) { // reset of screen
        y = 2;
        index = 0;
        return 1;
    }

    friend_len = strlen(friend);
    status_len = strlen(status);
    if (y + (friend_len + status_len + 2)/SCREEN_WIDTH > SCREEN_HEIGHT-4) return 0;

    gotoxy(0, y);
    ascToPet(friend, friend_len+1, friend_pet);
    ascToPet(status, status_len+1, status_pet);
    textcolor(color1); cputs(friend_pet);
    textcolor(color2); cputs(": ");
    textcolor(color3);

    x = friend_len + 2;
    while (status_len > SCREEN_WIDTH - x) {
        char save = status_ptr[SCREEN_WIDTH - x];

        status_ptr[SCREEN_WIDTH - x] = '\0';
        cputsxy(x, y, status_ptr);
        status_ptr[SCREEN_WIDTH - x] = save;
        status_ptr += SCREEN_WIDTH - x;
        status_len -= SCREEN_WIDTH - x;
        x = 0;
        ++y;
    }
    cputsxy(x, y, status_ptr);
    ++y;

    ++index;

    return 1;
}

void
show_connection_status(unsigned char index, char *strings[])
{
    textcolor(COLOR_GRAY1);
    cputsxy(SCREEN_WIDTH-12, 0, strings[index]);
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
colorfull(unsigned char x, unsigned char y, char *data)
{
    char *p = data;
    revers(1);
    gotoxy(x, y);
    while (*p != 255) {
        textcolor(*p++);
        cputc(*p++);
    }
    revers(0);
}

void
spacer(unsigned char y)
{
    textcolor(COLOR_LIGHTBLUE);
    cputcxy(0,              y, CH_CROSS); chlinexy(1,  y, SCREEN_WIDTH-2);
    cputcxy(SCREEN_WIDTH-1, y, CH_URCORNER);
}

void
clear_status_pane(void)
{
    unsigned char i;
    for (i=2; i<SCREEN_HEIGHT-3; i++) {
        cclearxy(0, i, SCREEN_WIDTH);
    }
}

PROCESS_THREAD(twitter_process, ev, data)
{
    static struct timer status_timer;
    static struct prompt p;
    static char buffer[141];
    static char credentials[32];
    static char buffer_ascii[141];
    static char credentials_ascii[32];
    static int status_counter;
    static char* credentials_ok;
    static enum connection_status connection_status;


    PROCESS_BEGIN();

    // init screen
    clrscr();
    bgcolor(COLOR_WHITE);
    bordercolor(COLOR_WHITE);

    // show logo
    colorfull(5, 0, title);

    spacer(1);
    textcolor(COLOR_BLUE);
    cputsxy(20, 5, "  Welcome to the C128 twitter client!");
    cputsxy(20, 6, "  (c) 2009-2010 Johan Van den Brande");
    cputsxy(20, 7, "                and Oliver Schmidt");
    textcolor(COLOR_LIGHTBLUE);
    cputsxy(20, 16,"Note: You need a supertweet.net account!");

    spacer(SCREEN_HEIGHT-3);
    textcolor(COLOR_BLACK); cputsxy(3, SCREEN_HEIGHT-3, " Enter your supertweet credentials. ");
    textcolor(COLOR_GRAY1); cputsxy(0, SCREEN_HEIGHT-2, "(username>:<password)");

    // allow other processes to initialize
    PROCESS_PAUSE();

    connection_status = twitter_resolve(show_resolve_phase, PROCESS_CURRENT()) ? resolving : inactive;

    prompt_init(0, SCREEN_HEIGHT-1, credentials, sizeof(credentials), COLOR_BROWN, COLOR_GRAY3, NULL, &p);

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
    petToAsc(credentials, strlen(credentials)+1, credentials_ascii);

    *credentials_ok = '\0';
    if (strlen(credentials) > 13) {
        credentials[13] = '\0';
    }
    textcolor(COLOR_GREEN);
    cputsxy(SCREEN_WIDTH/2, 0, credentials);

    spacer(SCREEN_HEIGHT-3);
    textcolor(COLOR_BLACK);
    cputsxy(3, SCREEN_HEIGHT-3, " What are you doing? ");
    cclearxy(0, SCREEN_HEIGHT-2, SCREEN_WIDTH);
    cclearxy(0, SCREEN_HEIGHT-1, SCREEN_WIDTH);

    prompt_init(0, SCREEN_HEIGHT-2, buffer, sizeof(buffer), COLOR_BROWN, COLOR_GRAY3, update_status_bar, &p);

    timer_set(&status_timer, CLOCK_SECOND);
    status_counter = 0;
    while (1) {
        process_poll(&twitter_process);
        PROCESS_WAIT_EVENT();

        if (prompt_ask(&p) && connection_status == inactive) {
            petToAsc(buffer, strlen(buffer)+1, buffer_ascii);
            twitter_post((uint8_t *)credentials_ascii, buffer_ascii, show_post_phase, PROCESS_CURRENT());
            connection_status = posting;
            prompt_reset(&p);
        }

        if (timer_expired(&status_timer) && connection_status == inactive) {
            textcolor(COLOR_GRAY1);
            gotoxy(SCREEN_WIDTH-5, 0);
            cprintf("%03d", status_counter);
            if (status_counter == 0) {
                status_counter = 120;
                clear_status_pane();
                twitter_timeline((uint8_t *)credentials_ascii, SCREEN_HEIGHT-5, show_status, show_status_phase, PROCESS_CURRENT());
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
