#ifndef __PROMPT_H_
#define __PROMPT_H_

struct prompt
{
    char x;
    char y;
    char *buffer;
    size_t buffer_size;
    size_t buffer_p;
    int cursor_time;
    char cursor_on;
    char cx;
    char cy;
    char last_cx;
    char last_cy;
    void (*cb_char)(struct prompt *p);
    unsigned char cursor_color;
    unsigned char input_color;
};

void prompt_init(unsigned char x, unsigned char y, char *buffer, size_t buffer_size, unsigned char input_color, unsigned char cursor_color, void (*cb_char)(struct prompt *p), struct prompt *p);
size_t prompt_remaining(struct prompt *p);
size_t prompt_size(struct prompt *p);
void prompt_reset(struct prompt *p);
int prompt_ask(struct prompt *p);

#endif
