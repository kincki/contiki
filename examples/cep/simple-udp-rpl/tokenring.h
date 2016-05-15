#ifndef TOKENRING_H
#define TOKENRING_H

#include "sys/clock.h"

typedef struct {
  unsigned int source_mote_id;
  clock_time_t timeStamp; //compare consequitive tokens 
} token_data_t;

const token_data_t *get_tokenring_data();

const void toggle_test();

#endif //TOKENRING_H
