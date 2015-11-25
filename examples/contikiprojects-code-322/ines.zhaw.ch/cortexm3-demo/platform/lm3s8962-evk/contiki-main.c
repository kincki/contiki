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
 *						Includes main(), initializes UC, starts Contiki
 * \author
 *						Urs Beerli
 * \version
 *						0.1
 * \since
 *						05.04.2009
 */
/*---------------------------------------------------------------------------*/

#include "contiki.h"
#include "contiki-net.h"
#include "webserver-nogui.h"
#include "sys/clock.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"

#include "drivers/ethernet_drv.h"
#include "display.h"


#define DEBUG 1
#if DEBUG
#include "utils/uartstdio.h"
#define PRINTF(...) UARTprintf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define USEUART1 0

PROCINIT(&etimer_process, &ethernet_process, &tcpip_process);

/*---------------------------------------------------------------------------*/
/**
 * \brief	Start entry point.
 * \param	void There are no parameters needed.
 * \return Program exit code value.
 */
int
main(void)
{
	/* Set the clocking to run directly from the crystal. */
	SysCtlClockSet(	SYSCTL_SYSDIV_4 |
					SYSCTL_OSC_MAIN |
					SYSCTL_USE_PLL |
					SYSCTL_XTAL_8MHZ);

	/*Set Interrupt Priorities*/
	IntPrioritySet(INT_ETH, 1);
	IntPrioritySet(INT_SYSCTL, 2);

#if USEUART1
	/* Initialize the UART1 interface on PORT D */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);
	UARTStdioInit(1);
#else
    /* Initialize the UART0 interface on PORT A*/
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);
#endif

	/* Enable processor interrupts. */
	IntMasterEnable();

	PRINTF("Starting Contiki...\n");

	/* Initialize System Clock */
	clock_init();

	/* Process subsystem */
	process_init();

	/* Register initial processes */
	procinit_init();

	/* Configure the IP Stack. */
	ip_init();

	/* Autostart processes
	 * - Webserver
	 */
	autostart_start(autostart_processes);

	/*Start Process for OLED display*/
	process_start(&display_process, NULL);

	PRINTF("Contiki is ready!\n");
	while(1) {
		process_run();
  }

  return 0;
}
