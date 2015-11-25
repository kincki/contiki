/**
 * \file
 *	Port of uart1.c for TI MSP430 RF2500.
 * \author
 * 	Wincent Balin <wincent.balin@gmail.com>
 * \brief
 *  A mix of uart1.c for MSP430 CPU and main.c from rf2500_jmk project by Jean-Marc Koller.
 */

#include <io.h>
#include <signal.h>

#include "dev/uart1.h"
#include "dev/watchdog.h"

#include "lib/ringbuf.h"


static int (*uart1_input_handler)(unsigned char c);

//#define TX_WITH_INTERRUPT 1

#if TX_WITH_INTERRUPT
	static struct ringbuf txbuf;
	#define TXBUFSIZE 64
	static uint8_t txbuf_data[TXBUFSIZE];
#endif /* TX_WITH_INTERRUPT */

/* UART pins. */
#define P3TXD0 4
#define P3RXD0 5

/*---------------------------------------------------------------------------*/
void
uart1_init(unsigned long ubr)
{
	/* Parameter not used, because of arbitrary settings. */
	(void) ubr;
	
	/* Select UART mode for pins. */
	P3OUT |= (1 << P3TXD0);
	P3DIR |= (1 << P3TXD0);
	P3SEL |= (1 << P3TXD0 | 1 << P3RXD0);
	
	/* Use SMCLK. */
	UCA0CTL1 = UCSSEL_SMCLK;

	/* 9600 baud @ 8 MHz. */
	UCA0BR0 = 0x41;
	UCA0BR1 = 0x3;
	UCA0MCTL = UCBRS_2;                       

	/* Reset USCI FSM. */
	UCA0CTL1 &= ~UCSWRST;
	
	/* Enable RX interrupt. */
	IE2 |= UCA0RXIE;

#ifdef TX_WITH_INTERRUPT
	/* Initialize TX ring buffer. */
	ringbuf_init(&txbuf, txbuf_data, sizeof(txbuf_data));
#endif
}
/*---------------------------------------------------------------------------*/
uint8_t
uart1_active(void)
{
  return UCA0STAT & UCBUSY;
}
/*---------------------------------------------------------------------------*/
void
uart1_set_input(int (*input)(unsigned char c))
{
  uart1_input_handler = input;
}
/*---------------------------------------------------------------------------*/
void
uart1_writeb(unsigned char c)
{
  watchdog_periodic();
  
#if TX_WITH_INTERRUPT
  /* Put the outgoing byte on the transmission buffer. If the buffer
     is full, we just keep on trying to put the byte into the buffer
     until it is possible to put it there. */
  while(ringbuf_put(&txbuf, c) == 0);

  /* If there is no transmission going, we need to start it by putting
     the first byte into the UART. */
  if(IFG2 & UCA0TXIFG)
  {
    UCA0TXBUF = ringbuf_get(&txbuf);
		/* Enable TX interrupt. */
		IE2 |= UCA0TXIE;
  }

#else /* TX_WITH_INTERRUPT */

  /* Loop until the transmission buffer is available. */
  while(!(IFG2 & UCA0TXIFG));

  /* Transmit the data. */
  UCA0TXBUF = c;
#endif /* TX_WITH_INTERRUPT */
}
/*---------------------------------------------------------------------------*/
#if ! WITH_UIP /* If WITH_UIP is defined, putchar() is defined by the SLIP driver */
int
putchar(int c)
{
  uart1_writeb((char) c);
  return c;
}
#endif /* ! WITH_UIP */
/*---------------------------------------------------------------------------*/
interrupt(USCIAB0RX_VECTOR)
isr_usci0rx(void)
{
	if(IFG2 & UCA0RXIFG)
	{
		if(UCA0STAT & UCRXERR)
		{
			/* If an error as detected, clear error flags with a dummy read. */
			volatile unsigned dummy;
			dummy = UCA0RXBUF;
		}
		else
		{
			/* Receive character, if receive handler is set. */
		  if(uart1_input_handler)
  		{
				if(uart1_input_handler(UCA0RXBUF))
				{
		  		LPM4_EXIT;
				}
			}
		}
	}
}
/*---------------------------------------------------------------------------*/
#if TX_WITH_INTERRUPT
interrupt(USCIAB0TX_VECTOR)
isr_usci0tx(void)
{
	/* Check whether the interrupt came from the wrong source. */
	if(!(IFG2 & UCA0RXIFG))
	{
		return;
	}
	
  if(ringbuf_elements(&txbuf) == 0)
  {
	  /* Stop transmitting when there is no data. */
    /* To do so, forbid further TX interrupts. */
    IE2 &= ~UCA0TXIE;
  }
  else
  {
    UCA0TXBUF = ringbuf_get(&txbuf);
  }
}
#endif /* TX_WITH_INTERRUPT */
/*---------------------------------------------------------------------------*/
