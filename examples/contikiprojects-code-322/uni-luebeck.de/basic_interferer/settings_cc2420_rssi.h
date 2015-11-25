/*
 * Copyright (c) 2012, University of Luebeck, Germany.
 * All rights reserved.
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
 *
 * Author: Carlo Alberto Boano <cboano@iti.uni-luebeck.de>
 *
 */
 
#include "contiki.h"
#include "dev/spi.h"
#include "dev/cc2420.h"
#include "dev/cc2420_const.h"	
#include <io.h>

/*---------------------------------------------------------------------------*/

// Constant: amount of microseconds necessary to obtain a single RSSI sample
#define SINGLE_RSSI_SAMPLE_US 21.1395f					

/*---------------------------------------------------------------------------*/

// Prototype: get one quick RSSI sample from the cc2420 register
void get_fast_rssi(signed char registro);

/*---------------------------------------------------------------------------*/

// Fast SPI operations: NOP
#define NOP_DELAY() \
  do {\
    _NOP(); _NOP(); _NOP(); _NOP(); \
    _NOP(); _NOP(); _NOP(); _NOP(); \
    _NOP(); _NOP(); _NOP(); _NOP(); \
    _NOP(); _NOP(); _NOP(); _NOP(); \
	_NOP(); _NOP(); _NOP(); _NOP(); \
  } while (0);
  
/*---------------------------------------------------------------------------*/

// Fast SPI operations: get RSSI from the register
#define FASTSPI_GETRSSI(v)\
  do {\
    /* Flush the radio. Looks necessary to avoid the wrong RSSI readings due to interrupts */\
	SPI_TXBUF = CC2420_SFLUSHRX;\
	/* Request CCA THRESHOLD + RSSI */\
    SPI_TXBUF = (CC2420_RSSI) | 0x40;\
    NOP_DELAY();\
    SPI_RXBUF;\
    /* Receive LSB: CCA THRESHOLD */\
    SPI_TXBUF = 0;\
    /*while ((IFG1 & URXIFG0) == 0);*/\
    NOP_DELAY();\
    SPI_RXBUF;\
    /* Receive LSB: **RSSI** */\
    SPI_TXBUF = 0;\
    NOP_DELAY();\
    v = SPI_RXBUF;\
  } while (0) 

/*---------------------------------------------------------------------------*/
