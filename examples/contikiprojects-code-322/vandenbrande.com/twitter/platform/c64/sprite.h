#ifndef __SPRITE_H__
#define __SPRITE_H__

void sprite_init(unsigned char sprite, const unsigned char *data);
void sprite_pos(unsigned char sprite, int x, unsigned char y);
void sprite_enable(unsigned char sprite, unsigned char enable);
void sprite_color(unsigned char sprite, unsigned char color);
void sprite_double(unsigned char sprite, unsigned char double_x, unsigned char double_y);

#endif /* __SPRITE_H__ */
