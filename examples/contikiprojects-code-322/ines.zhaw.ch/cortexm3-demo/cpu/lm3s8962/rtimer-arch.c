/*******************************************************************************
 *  _____       ______   ____
 * |_   _|     |  ____|/ ____|  Institute of Embedded Systems
 *   | |  _ __ | |__  | (___    Wireless Group
 *   | | | '_ \|  __|  \___ \   Zuercher Hochschule Winterthur
 *  _| |_| | | | |____ ____) |  (University of Applied Sciences)
 * |_____|_| |_|______|_____/   8401 Winterthur, Switzerland
 *
 *******************************************************************************
 *
 * Copyright (c) 2010, Institute Of Embedded Systems at Zurich University
 * of Applied Sciences. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
 /**
 * \file
 *						Implementation of timers for scheduler
 * \author
 *						Urs Beerli, Silvan Fischer
 * \version
 *						0.1
 * \since
 *						05.04.2009
 */
/*---------------------------------------------------------------------------*/

#include "sys/rtimer.h"

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"

#include "rtimer-arch.h"


/*---------------------------------------------------------------------------*/
/**
 * \brief	Interrupt handler for the timer0
 * \param	void There are no parameters needed.
 * \return	No return values.
 */
void
timer0_int_handler(void)
{
	/* Clear the timer interrupt. */
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	/* Call rtimer callback */
  rtimer_run_next();
}

/*---------------------------------------------------------------------------*/
/**
 * \brief	Initialize timer0
 * \param	void There are no parameters needed.
 * \return	No return values.
 */
void
rtimer_arch_init(void)
{

	/* Enable the peripherals used by this example. */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

	/* Configure the two 32-bit periodic timers. */
	TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER);

	/* Configure the handler routine. */
	TimerIntRegister(TIMER0_BASE, TIMER_A, timer0_int_handler);

	/* Setup the interrupts for the timer timeouts. */
	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	/* Enable the timers. */
	TimerEnable(TIMER0_BASE, TIMER_A);
}

/*---------------------------------------------------------------------------*/
/**
 * \brief	Interrupt handler for the timer0
 * \param	t Time to reload the timer value.
 * \return	No return values.
 */
void
rtimer_arch_schedule(rtimer_clock_t t)
{
	TimerLoadSet(TIMER0_BASE, TIMER_A, t);
}
