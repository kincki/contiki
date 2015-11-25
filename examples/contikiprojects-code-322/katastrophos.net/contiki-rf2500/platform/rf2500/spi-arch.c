/**
 * \file
 *         CC2500 ez430-RF2500 specific SPI driver
 * \author
 *         Wincent Balin <wincent.balin@gmail.com>
 */

#include "contiki.h"

#include "dev/spi.h"

/*---------------------------------------------------------------------------*/
void
spi_init(void)
{
	/* Setup SPI. */
	UCB0CTL1 = UCSWRST;
	UCB0CTL1 = UCSWRST | UCSSEL1;
	UCB0CTL0 = UCCKPH | UCMSB | UCMST | UCSYNC;
	UCB0BR0  = 2;
	UCB0BR1  = 0;	

	/* Initialize port pins. */
	P2SEL &= ~(1 << P2GDO0 | 1 << P2GDO2); /* Pins 2.6 and 2.7 are assigned to crystal by default. */
	
	P3OUT |= (1 << P3CSN);
	P3DIR |= (1 << P3CSN | 1 << P3MOSI | 1<< P3CLK);
	P3SEL |= (1 << P3CLK | 1 << P3MOSI | 1 << P3MISO);
	
	/* Initialize SPI. */
	UCB0CTL1 &= ~UCSWRST;

}
/*---------------------------------------------------------------------------*/
