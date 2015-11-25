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
 *						IO functions of ethernet driver for IP stack
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

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/ethernet.h"

#include "ethernet_drv.h"
#include "ethernet.h"

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

#define DEBUG 0
#if DEBUG
#include "utils/uartstdio.h"
#define PRINTF(...) UARTprintf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


/*---------------------------------------------------------------------------*/
/**
 * \brief	Ethernet IO function.
 * \param	void There are no parameters needed.
 * \return	No return values.
 */
static void
eth_poll(void)
{
	/*Get Buffer*/
	uip_len = (u16_t)EthernetPacketGet(ETH_BASE, uip_buf, UIP_BUFSIZE);

	if(uip_len > 0) {
		if(BUF->type == HTONS(UIP_ETHTYPE_IP)) {
			uip_arp_ipin();
			uip_input();
			if(uip_len > 0) {
				uip_arp_out();
				EthernetPacketPut(ETH_BASE, uip_buf, uip_len);
			}
		} else if(BUF->type == HTONS(UIP_ETHTYPE_ARP)) {
			uip_arp_arpin();
			if(uip_len > 0) {
				EthernetPacketPut(ETH_BASE, uip_buf, uip_len);
			}
		}
	}

	EthernetIntEnable(ETH_BASE, ETH_INT_RX);
}

/*---------------------------------------------------------------------------*/
/**
 * \brief	Initialize the uIP startup settings
 * \param	void There are no parameters needed.
 * \return	No return values.
 */
void
ip_init(void)
{

	uip_ipaddr_t ip_addr, subnet;

	/*Init uIP Stack*/
	uip_init();

	/*Set IPv4 Address*/
	uip_ipaddr(&ip_addr, 192, 168, 2, 2);
	uip_sethostaddr(&ip_addr);
	/*Set (IPv4) Subnet*/
	uip_ipaddr(&subnet, 255, 255, 255, 0);
	uip_setnetmask(&subnet);

}

/*---------------------------------------------------------------------------*/
PROCESS(ethernet_process, "Ethernet driver");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ethernet_process, ev, data)
{
	PROCESS_POLLHANDLER(eth_poll());
	PROCESS_BEGIN();
	eth_init();

	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXIT);

  PROCESS_END();
}
