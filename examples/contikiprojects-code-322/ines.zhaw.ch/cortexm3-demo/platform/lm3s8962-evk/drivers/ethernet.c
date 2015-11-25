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
 *						Initializes ethernet driver, ISR etc.
 * \author
 *						Urs Beerli
 * \version
 *						0.1
 * \since
 *						29.01.2010
 */
/*---------------------------------------------------------------------------*/


#include "contiki.h"
#include "contiki-net.h"

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/ethernet.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"

#include "ethernet.h"
#include "ethernet_drv.h"

#define DEBUG 1
#if DEBUG
#include "utils/uartstdio.h"
#define PRINTF(...) UARTprintf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


/*---------------------------------------------------------------------------*/
/**
 * \brief	Interrupt handler for the ethernet device.
 * \param	void There are no parameters needed.
 * \return	No return values.
 *
 * Initializes peripherials of Cortex M3 for integrated ethernet module usage.
 * Sets all needed ports and default values etc.
 */
void
eth_int_handler(void)
{
	u32_t ulTemp;

	/* Read and Clear the interrupt. */
	ulTemp = EthernetIntStatus(ETH_BASE, false);
	EthernetIntClear(ETH_BASE, ulTemp);

	/* Check to see if an RX Interrupt has occured. */
	if(ulTemp & ETH_INT_RX) {
		EthernetIntDisable(ETH_BASE, ETH_INT_RX);
		process_poll(&ethernet_process);
	}
}

/*---------------------------------------------------------------------------*/
/**
 * \brief	Initialize some ethernet stuff of Cortex M3
 * \param	void There are no parameters needed.
 * \return	No return values.
 *
 * Initializes peripherials of Cortex M3 for integrated ethernet module usage.
 * Sets all needed ports and default values etc.
 */
void
eth_init(void)
{
	unsigned long ulTemp;
	u8_t ethMacAddr[]   = {0x00, 0x15, 0x12, 0x15, 0x04, 0x10};

	/*
	* Enable and Reset the Ethernet Controller.
	*/
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
	SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);

	/*
	* Enable Port F for Ethernet LEDs.
	*  LED0        Bit 3   Output
	*  LED1        Bit 2   Output
	*/
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_DIR_MODE_HW);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3,
					GPIO_STRENGTH_2MA,
  					GPIO_PIN_TYPE_STD);

	/*
	* Intialize the Ethernet Controller and disable all Ethernet Controller
	* interrupt sources.
	*/
	EthernetIntDisable(ETH_BASE, (ETH_INT_PHY | ETH_INT_MDIO | ETH_INT_RXER |
					ETH_INT_RXOF | ETH_INT_TX | ETH_INT_TXER | ETH_INT_RX));
	ulTemp = EthernetIntStatus(ETH_BASE, 0);
	EthernetIntClear(ETH_BASE, ulTemp);

	/* Initialize the Ethernet Controller for operation. */
	EthernetInitExpClk(ETH_BASE, SysCtlClockGet());

	/*
	* Configure the Ethernet Controller for normal operation.
	* - No reception of Packets with CRC Errors
	* - Promiscuous mode reception
	* - Full Duplex
	* - TX CRC Auto Generation
	* - TX Padding Enabled
	*/
	EthernetConfigSet(ETH_BASE, (ETH_CFG_RX_BADCRCDIS | ETH_CFG_RX_PRMSEN |
			ETH_CFG_TX_DPLXEN | ETH_CFG_TX_CRCEN | ETH_CFG_TX_PADEN));

	/* Register interrupt handler. */
	EthernetIntRegister(ETH_BASE, eth_int_handler);

	EthernetEnable(ETH_BASE);	/* Enable the Ethernet Controller. */
	IntEnable(INT_ETH);				/* Enable the Ethernet interrupt. */

	/* Enable the Ethernet RX Packet interrupt source. */
	EthernetIntEnable(ETH_BASE, ETH_INT_RX);


	/* Set MAC Address */
	uip_ethaddr.addr[0] = ethMacAddr[0];
	uip_ethaddr.addr[1] = ethMacAddr[1];
	uip_ethaddr.addr[2] = ethMacAddr[2];
	uip_ethaddr.addr[3] = ethMacAddr[3];
	uip_ethaddr.addr[4] = ethMacAddr[4];
	uip_ethaddr.addr[5] = ethMacAddr[5];

	/*Program the hardware with it's MAC address (for filtering).*/
	EthernetMACAddrSet(ETH_BASE, ethMacAddr);

	PRINTF("Ethernet MAC Address: %x:%x:%x:%x:%x:%x\n", ethMacAddr[0],
		ethMacAddr[1], ethMacAddr[2], ethMacAddr[3], ethMacAddr[4],
		ethMacAddr[5]);

}
