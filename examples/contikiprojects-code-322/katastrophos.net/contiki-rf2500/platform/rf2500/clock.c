/**
 * \file
 *         Clock implementation for TI MSP430 RF2500 port.
 * \author
 *         Wincent Balin <wincent.balin@gmail.com>
 * \brief
 *         Mostly a copy of cpu/msp430/clock.c , adapted for RF2500.
 */

#include <io.h>
#include <signal.h>
 
#include "contiki-conf.h"

#include "sys/clock.h"
#include "sys/etimer.h"


#define MAX_TICKS (~((clock_time_t)0) / 2)

#define TICK_INTERVAL (12000 / CLOCK_CONF_SECOND)

static volatile unsigned long seconds = 0;
static volatile unsigned int second_count = TICK_INTERVAL;
static volatile clock_time_t count = 0;

/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
	/* Set DCO (main clock) to F_CPU (8 MHz). */
	BCSCTL1 = CALBC1_8MHZ;
  DCOCTL = CALDCO_8MHZ;

  /* Set auxilary timer to VLO (12000 Hz). */
  BCSCTL3 |= LFXT1S_2;
  /* Set timer interrupt frequency. */
  TACCR1 = TICK_INTERVAL;
  /* Set timer mode to ACLK, continious mode. */
  TACTL = TASSEL_ACLK + MC_CONT;
  /* Enable timer interrupt. */
  TACCTL1 = CCIE;
  
  /* Reset tick count. */
  count = 0;
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  return count;
}
/*---------------------------------------------------------------------------*/
unsigned long
clock_seconds(void)
{
	return seconds;
}
/*---------------------------------------------------------------------------*/
void
clock_delay(unsigned int d)
{
  volatile unsigned int i;
  
  for(i = 0; i < d; i++)
  {
  }
}
/*---------------------------------------------------------------------------*/
interrupt(TIMERA1_VECTOR) wakeup
isr_timera1(void)
{
	/* Only when TA1 compare!. */
	if(!(TAIV & 0x02))
		return;
		
	/* Increase tick count. */
	count++;
	
	/* Count seconds. */
	if(second_count == 0)
	{
		second_count = TICK_INTERVAL;
		seconds++;
	}
	else
	{
		second_count--;
	}
	
	/* Schedule next tick. */
	TACCR1 += TICK_INTERVAL;
	
	/* Count timers. */
	if(etimer_pending() && (etimer_next_expiration_time() - count - 1) > MAX_TICKS)
	{
		etimer_request_poll();
		/* Exit LPM. */
		LPM4_EXIT;
	}
}
/*---------------------------------------------------------------------------*/
