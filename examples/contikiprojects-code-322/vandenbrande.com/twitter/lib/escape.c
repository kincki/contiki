#include <stdlib.h>
#include <string.h>

void
decode_escape(const char* source, char* destination)
{
    while (*source) {
        if (*source != 0x5c) { // '\'
            *destination++ = *source++;
        } else {
            ++source;
            if (*source == 0x22 || // '"'
                *source == 0x27 || // '''
                *source == 0x2f || // '/'
                *source == 0x3f || // '?'
                *source == 0x5c) { // '\'
                *destination++ = *source++;
            } else {
                *destination++ = 0x20; // ' '
            }
        }
    }
    *destination = '\0';
}
