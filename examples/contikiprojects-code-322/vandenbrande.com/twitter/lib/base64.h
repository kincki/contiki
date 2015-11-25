#ifndef __BASE64_H
#define __BASE64_H

extern int base64_encode( unsigned char *src, int dlen, unsigned char *target, int targsize );
extern int base64_encode_size( int len );

#endif
