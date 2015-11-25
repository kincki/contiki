#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "breadbox64.h"
#include "input.h"

#define CURSOR_BLINK_INTERVAL 50

static void
prompt_clear(struct prompt *p)
{
    unsigned char x = p->x;
    unsigned char y = p->y;
    size_t len = strlen(p->buffer);

    while (len > SCREEN_WIDTH - x) {
        cclearxy(x, y, SCREEN_WIDTH - x);
        len -= SCREEN_WIDTH - x;
        x = 0;
        ++y;
    }
    cclearxy(x, y, len);
}

static void
prompt_puts(struct prompt *p)
{
    unsigned char x = p->x;
    unsigned char y = p->y;
    char *buffer = p->buffer;
    size_t len = strlen(buffer);

    // Intentionally '>=' here to have p->cx and p->cy
    // set correctly at the right edge of the screen!
    while (len >= SCREEN_WIDTH - x) {
        char save = buffer[SCREEN_WIDTH - x];

        buffer[SCREEN_WIDTH - x] = '\0';
        cputsxy(x, y, buffer);
        buffer[SCREEN_WIDTH - x] = save;
        buffer += SCREEN_WIDTH - x;
        len -= SCREEN_WIDTH - x;
        x = 0;
        ++y;
    }
    cputsxy(x, y, buffer);

    p->cx = x + len;
    p->cy = y;
}

void
prompt_init(unsigned char x, unsigned char y, char *buffer, size_t buffer_size, unsigned char input_color, unsigned char cursor_color, void (*cb_char)(struct prompt *p), struct prompt *p)
{
    p->x = x;
    p->y = y;
    p->buffer = buffer;
    p->buffer_size = buffer_size;
    p->cb_char = cb_char;
    p->buffer_p = 0;
    p->input_color = input_color;
    p->cursor_color = cursor_color;

    p->buffer[p->buffer_p] = '\0';

    p->cursor_time = CURSOR_BLINK_INTERVAL;
    p->cursor_on = 1;
    p->cx = x;
    p->cy = y;
    p->last_cx = x;
    p->last_cy = y;

    if (p->cb_char) p->cb_char(p);
}

size_t prompt_size(struct prompt *p)
{
    return p->buffer_size - 1;
}

size_t prompt_remaining(struct prompt *p)
{
    return prompt_size(p) - p->buffer_p;
}

void
prompt_reset(struct prompt *p)
{
    prompt_clear(p);
    cputcxy(p->cx, p->cy, ' ');
    p->buffer_p = 0;
    p->buffer[p->buffer_p] = '\0';
    p->cx = p->x;
    p->cy = p->y;
    p->last_cx = p->cx;
    p->last_cy = p->cy;

    if (p->cb_char) p->cb_char(p);
}

static void
handle_cursor(struct prompt *p)
{
    p->cursor_time--;
    if (p->cursor_time == 0) {
        p->cursor_time = CURSOR_BLINK_INTERVAL;
        if (p->cursor_on) {
            p->cursor_on = 0;
            if (p->cy < SCREEN_HEIGHT) {
                (void)textcolor(p->cursor_color);
                cputcxy(p->cx, p->cy, '_');
            }
            p->last_cx = p->cx;
            p->last_cy = p->cy;
        } else {
            p->cursor_on = 1;
            if (p->cx == p->last_cx && p->cy == p->last_cy) {
                cputcxy(p->last_cx, p->last_cy, ' ');
            }
        }
    }
}


static int
handle_input(struct prompt *p)
{
    char ch;

    if (ctk_arch_keyavail()) {
        ch = ctk_arch_getkey();
        switch (ch) {
            case 3:
                clrscr();
                exit(EXIT_SUCCESS);
            case CH_ENTER:
                return 1;
                break;
            case CH_DEL:
                prompt_clear(p);
                if (p->buffer_p > 0) p->buffer[--p->buffer_p] = '\0';
                (void)textcolor(p->input_color);
                prompt_puts(p);
                cputcxy(p->cx, p->cy, ' ');
                cputcxy(p->last_cx, p->last_cy, ' ');
                break;
            default:
                if (isalnum(ch) || isdigit(ch) || isspace(ch) || ispunct(ch)) {
                    p->buffer[p->buffer_p] = ch;
                    p->buffer[p->buffer_p+1] = '\0';
                    if (p->buffer_p < p->buffer_size-2) p->buffer_p++;
                    (void)textcolor(p->input_color);
                    prompt_puts(p);
                }
        }
        if (p->cb_char) p->cb_char(p);
    }

    return 0;
}


int
prompt_ask(struct prompt *p)
{
    handle_cursor(p);
    return handle_input(p);
}
