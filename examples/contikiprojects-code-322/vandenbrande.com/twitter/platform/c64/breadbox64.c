#include "contiki-net.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "breadbox64.h"
#include "twitter_resolve.h"
#include "twitter_post.h"
#include "twitter_timeline.h"
#include "base64.h"
#include "petascii.h"
#include "input.h"
#include "sprite.h"

unsigned char twitter_sprite[] = {
    192,0,0,96,0,0,48,0,0,56,0,0,28,0,56,14,0,124,15,1,239,7,255,252,7,255,
    248,3,255,248,3,255,240,1,255,240,1,255,224,0,255,192,0,127,128,0,62,
    0,0,34,0,0,34,0,0,102,0,0,49,128,0,33,0,0
};

static char title[] =
{
    COLOR_RED,      'B',
    COLOR_ORANGE,   'R',
    COLOR_YELLOW,   'E',
    COLOR_GREEN,    'A',
    COLOR_BLUE,     'D',
    COLOR_RED,      'B',
    COLOR_ORANGE,   'O',
    COLOR_YELLOW,   'X',
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
    gotoxy(SCREEN_WIDTH-8, SCREEN_HEIGHT-5);
    cprintf("%03d/%03d", prompt_remaining(p), prompt_size(p));
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
        color2 = COLOR_LIGHTRED;
        color3 = COLOR_ORANGE;
    }

    if (NULL == friend) { // reset of screen
        y = 2;
        index = 0;
        return 1;
    }

    friend_len = strlen(friend);
    status_len = strlen(status);
    if (y + (friend_len + status_len + 2)/SCREEN_WIDTH > SCREEN_HEIGHT-6) return 0;

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
    textcolor(COLOR_YELLOW);
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
    for (i=2; i<SCREEN_HEIGHT-5; i++) {
        cclearxy(0, i, SCREEN_WIDTH);
    }
}

void
exit_handler(void)
{
    sprite_enable(0, 0);
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
    atexit(exit_handler);

    // init screen
    clrscr();
    bgcolor(COLOR_WHITE);
    bordercolor(COLOR_WHITE);

    // show logo
    sprite_init(0, twitter_sprite);
    sprite_pos(0, 32, 50);
    sprite_color(0, COLOR_BLUE);
    sprite_enable(0, 1);
    colorfull(5, 0, title);

    spacer(1);
    textcolor(COLOR_BLUE);
    cputsxy(0, 5, "   Welcome to the C64 twitter client!");
    cputsxy(0, 7, "   (c) 2009-2010 Johan Van den Brande");
    cputsxy(0, 8, "                 and Oliver Schmidt");
    textcolor(COLOR_LIGHTBLUE);
    cputsxy(0, 16,"Note: You need a supertweet.net account!");
    
    spacer(SCREEN_HEIGHT-5);
    textcolor(COLOR_BLACK); cputsxy(2, SCREEN_HEIGHT-5, "Enter ");
    textcolor(COLOR_GRAY1); cputs("your ");
    textcolor(COLOR_GRAY2); cputs("supertweet ");
    textcolor(COLOR_GRAY3); cputs("credentials");
    textcolor(COLOR_BLACK); cputs(".");
    textcolor(COLOR_YELLOW);cputsxy(0, SCREEN_HEIGHT-4, "(username>:<password)");

    // allow other processes to initialize
    PROCESS_PAUSE();

    connection_status = twitter_resolve(show_resolve_phase, PROCESS_CURRENT()) ? resolving : inactive;

    prompt_init(0, SCREEN_HEIGHT-3, credentials, sizeof(credentials), COLOR_ORANGE, COLOR_YELLOW, NULL, &p);

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
    cputsxy(16, 0, credentials);

    spacer(SCREEN_HEIGHT-5);
    textcolor(COLOR_BLACK);
    cputsxy(2, SCREEN_HEIGHT-5, "What ");
    textcolor(COLOR_GRAY1);
    cputs("are ");
    textcolor(COLOR_GRAY2);
    cputs("you ");
    textcolor(COLOR_GRAY3);
    cputs("doing");
    textcolor(COLOR_BLACK);
    cputs("?");
    cclearxy(0, SCREEN_HEIGHT-4, SCREEN_WIDTH);
    cclearxy(0, SCREEN_HEIGHT-3, SCREEN_WIDTH);

    prompt_init(0, SCREEN_HEIGHT-4, buffer, sizeof(buffer), COLOR_ORANGE, COLOR_YELLOW, update_status_bar, &p);

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
            textcolor(COLOR_YELLOW);
            gotoxy(SCREEN_WIDTH-3, 0);
            cprintf("%03d", status_counter);
            if (status_counter == 0) {
                status_counter = 120;
                clear_status_pane();
                twitter_timeline((uint8_t *)credentials_ascii, SCREEN_HEIGHT-7, show_status, show_status_phase, PROCESS_CURRENT());
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
