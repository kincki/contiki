
/* this file is only used to generate TestWilabContiki.java with nescc-mig */

enum {
  AM_WILAB_CONTIKI_PRINTF = 65,
};

#define PRINTF_MSG_LENGTH    100

typedef nx_struct wilab_Contiki_printf {
  nx_uint8_t buffer[PRINTF_MSG_LENGTH];
};

