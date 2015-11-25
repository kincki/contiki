#include <string.h>
#include <c64.h>

#define setbit(addr, bit)   (*(addr) |= (1<<(bit)))
#define clearbit(addr, bit) (*(addr) &= ~(1<<(bit)))

void sprite_init(unsigned char sprite, const unsigned char *data)
{
#	define SPRITE0_DATA    0x0340
#	define SPRITE0_PTR     0x07F8

	unsigned char *data_ptr = (unsigned char *) (SPRITE0_DATA + (sprite << 6));
	unsigned char *sprite_ptr = (unsigned char*) (SPRITE0_PTR + sprite);
	memcpy ((void*) data_ptr, data, 64);
	*sprite_ptr = (unsigned char) ((unsigned int)(data_ptr) >> 6);
}

void sprite_pos(unsigned char sprite, int x, unsigned char y)
{
	unsigned char sprite_offset =  sprite << 1;
	unsigned char *sprite_addr = &VIC.spr0_x;
	*(sprite_addr + sprite_offset + 0) = x & 0xff;
	VIC.spr_hi_x = ((x >> 8) & 1) << sprite;
	*(sprite_addr + sprite_offset + 1) = y;
}

void sprite_enable(unsigned char sprite, unsigned char enable)
{
    if (enable) {
        setbit(&VIC.spr_ena, sprite);
    } else {
        clearbit(&VIC.spr_ena, sprite);
    }
}

void sprite_color(unsigned char sprite, unsigned char color)
{
    *(&VIC.spr0_color + sprite)  = color;
}

void sprite_double(unsigned char sprite, unsigned char double_x, unsigned char double_y)
{
    if (double_x) {
        setbit(&VIC.spr_exp_x, sprite);
    } else {
        clearbit(&VIC.spr_exp_x, sprite);
    }

    if (double_y) {
        setbit(&VIC.spr_exp_y, sprite);
    } else {
        clearbit(&VIC.spr_exp_y, sprite);
    }

}
