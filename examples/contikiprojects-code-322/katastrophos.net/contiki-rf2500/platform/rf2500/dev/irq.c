/**
 * \file
 *         IRQ subsystem, re-implemented for ez430-RF2500
 * \author
 *         Wincent Balin <wincent.balin@gmail.com>
 */

#include <signal.h>

#include "lib/sensors.h"
#include "dev/irq.h"
#include "dev/lpm.h"

/*---------------------------------------------------------------------------*/
#if IRQ_PORT1_VECTOR
interrupt(PORT1_VECTOR)
isr_p1(void)
{
  if(sensors_handle_irq(IRQ_PORT1)) {
    LPM4_EXIT;
  }
  P1IFG = 0x00;
}
#endif
/*---------------------------------------------------------------------------*/
#if IRQ_PORT2_VECTOR
interrupt(PORT2_VECTOR)
isr_p2(void)
{
  if(sensors_handle_irq(IRQ_PORT2)) {
    LPM4_EXIT;
  }
  P2IFG = 0x00;
}
#endif
/*---------------------------------------------------------------------------*/
void
irq_init(void)
{
}
