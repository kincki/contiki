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
 *						Shows picture and scrolling text
 * \author
 *						Urs Beerli
 * \version
 *						0.1
 * \since
 *						28.01.2010
 */
/*---------------------------------------------------------------------------*/

#include "contiki.h"

#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "drivers/rit128x96x4.h"

#include "display.h"
#include "zhawbmp.h"

#define DEBUG 1
#if DEBUG
#include "utils/uartstdio.h"
#define PRINTF(...) UARTprintf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static unsigned short id;

/*---------------------------------------------------------------------------*/
PROCESS(display_process, "Display");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(display_process, ev, data) {
PROCESS_BEGIN();

	unsigned long row, col;
	static unsigned char lineBuffer[64];
	static struct etimer timer;
	static char message[] =
	{
		"                      "
		"www.ines.zhaw.ch - Wireless Group - Contikiv2.4 Demo"
		"                      "
	};

	/*Init display*/
	RIT128x96x4Init(1000000);

	/*Display image*/
	PRINTF("DRAW IMAGE....\n");
	for(row = 5; row < 98; ++row) {

		for(col = 0; col < 64; ++col) {
			lineBuffer[col] = ~zhawlogo[row][col];
		}

		RIT128x96x4ImageDraw(lineBuffer, 20, (100 - row), 108, 1);
	}

	/*Scrolling text*/
	id = 0;
	while(1) {

		RIT128x96x4StringDraw(&message[id++], 0, 1, 11);

		etimer_set(&timer, 10);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

		/*Set index back*/
		if(id > 75) {
			id = 0;
		}

	}

PROCESS_END();
}


