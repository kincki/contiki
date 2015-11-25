/**
 * \file
 *         RF2500-specific rtimer code
 * \author
 *         Wincent Balin <wincent.balin@gmail.com>
 */

#include <io.h>
#include <signal.h>

#include "sys/rtimer.h"

/*---------------------------------------------------------------------------*/
interrupt(TIMERA0_VECTOR)
isr_timera0 (void)
{
  rtimer_run_next();
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_init(void)
{
  TACCTL0 = CCIE;
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_schedule(rtimer_clock_t t)
{
  TACCR0 = t;
}
