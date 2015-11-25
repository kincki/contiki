/**
 * \file
 *         LED architecture for TI MSP430 RF2500 port.
 * \author
 *         Wincent Balin <wincent.balin@gmail.com>
 * \brief
 *         Basically a workaround for cpu/msp430/leds-arch.c , to set LEDs on properly.
 */


#include "contiki-conf.h"

#include "dev/leds.h"

#include <io.h>

/*---------------------------------------------------------------------------*/
void
leds_arch_init(void)
{
  LEDS_PxDIR |= (LEDS_CONF_RED | LEDS_CONF_GREEN | LEDS_CONF_YELLOW);
  LEDS_PxOUT |= (LEDS_CONF_RED | LEDS_CONF_GREEN | LEDS_CONF_YELLOW);
}
/*---------------------------------------------------------------------------*/
unsigned char
leds_arch_get(void)
{
  return ((LEDS_PxOUT & LEDS_CONF_RED) ? LEDS_RED : 0)
    | ((LEDS_PxOUT & LEDS_CONF_GREEN) ? LEDS_GREEN : 0)
    | ((LEDS_PxOUT & LEDS_CONF_YELLOW) ? LEDS_YELLOW : 0);
}
/*---------------------------------------------------------------------------*/
void
leds_arch_set(unsigned char leds)
{
  LEDS_PxOUT = (LEDS_PxOUT & ~(LEDS_CONF_RED|LEDS_CONF_GREEN|LEDS_CONF_YELLOW))
    | ((leds & LEDS_RED) ? LEDS_CONF_RED : 0)
    | ((leds & LEDS_GREEN) ? LEDS_CONF_GREEN : 0)
    | ((leds & LEDS_YELLOW) ? LEDS_CONF_YELLOW : 0);
}
/*---------------------------------------------------------------------------*/
