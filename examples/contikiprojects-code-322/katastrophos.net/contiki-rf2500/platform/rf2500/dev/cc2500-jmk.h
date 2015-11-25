/**
 * \file
 *         CC2500 low-level driver header file
 * \author
 *         Jean-Marc Koller
 *         Wincent Balin <wincent.balin@gmail.com>
 */

#ifndef CC2500_JMK_H
#define CC2500_JMK_H

#include "contiki.h"

#include "dev/cc2500_regs.h"
#include "dev/smartrf_CC2500.h"

/* Function prototypes. */
void cc2500_init(void);
void cc2500_set_receiver(void (* recv)(void));
uint8_t cc2500_read_register(uint8_t address);
void cc2500_write_register(uint8_t address, uint8_t value);
void cc2500_write_tx_fifo(const uint8_t* data, uint8_t length);
void cc2500_read_rx_fifo(uint8_t* data, uint8_t length);
void cc2500_write_patable01(uint8_t pa0, uint8_t pa1);
int cc2500_receive_frame(uint8_t* data, uint8_t* length, uint8_t maxlength);
int cc2500_send_frame(uint8_t* data, uint8_t length);


#endif
