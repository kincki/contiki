/**
 * \file
 *	Contiki OS configuration file for TI MSP430 RF2500.
 * \author
 * 	Wincent Balin <wincent.balin@gmail.com>
 */

#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

#define HAVE_STDINT_H
#include "msp430def.h"

/* #define ENERGEST_CONF_ON		1 */

/* Clock */
typedef unsigned clock_time_t;
#define CLOCK_CONF_SECOND		32
#define F_CPU				8000000uL /* CPU target speed in Hz. */

#define BAUD2UBR(baud)			(F_CPU/(baud))


#define CC_CONF_REGISTER_ARGS		1
#define CC_CONF_FUNCTION_POINTER_ARGS	1
#define CC_CONF_INLINE			inline
#define CC_CONF_VA_ARGS			1

#define CCIF
#define CLIF

/**
 * The statistics data type.
 *
 * This datatype determines how high the statistics counters are able
 * to count.
 */
typedef uint16_t uip_stats_t;

typedef int bool;
#define	TRUE				1
#define FALSE 				0


/* LEDs ports TI MSP430 RF2500. */
#define LEDS_PxDIR P1DIR
#define LEDS_PxOUT P1OUT
#define LEDS_CONF_RED			0x01
#define LEDS_CONF_GREEN			0x02
#define LEDS_CONF_YELLOW		0x00

/* Ports connected to CC2500 chip. */
#define P2GDO0 6
#define P2GDO2 7
	
#define P3CSN  0
#define P3MOSI 1
#define P3MISO 2
#define P3CLK  3

/* Button input port. */
#define IRQ_PORT1 PORT1_VECTOR
#define P1BUTTON 2

/* Make serial input buffer shorter. */
#define SERIAL_LINE_CONF_BUFSIZE 64

#endif /* !CONTIKI_CONF_H */
