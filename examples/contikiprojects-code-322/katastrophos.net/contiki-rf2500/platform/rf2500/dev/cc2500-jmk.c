/**
 * \file
 *         CC2500 low-level driver implementation file
 * \author
 *         Jean-Marc Koller
 *         Wincent Balin <wincent.balin@gmail.com>
 */

#include <io.h>
#include <signal.h>

#include <stdio.h>

#include "contiki.h"

#include "dev/cc2500-jmk.h"

#include "dev/leds.h"


/* Useful constants. */
#define DUMMY_BYTE 0xDB
#define READ_BIT   0x80
#define BURST_BIT  0x40

/* SPI communication macros. */
#define SPI_CS_SET(enable) do { if(enable) P3OUT &= ~(1 << P3CSN); else P3OUT |= (1 << P3CSN); } while(0)
#define SPI_WRITE_BYTE(x) do { IFG2 &= ~UCB0RXIFG; UCB0TXBUF = x; } while(0)
#define SPI_READ_BYTE()   UCB0RXBUF
#define SPI_WAIT_DONE()   while(!(IFG2 & UCB0RXIFG))


/* CC2500 special settings. */
#define MRFI_GDO_SYNC           6
#define MRFI_GDO_CCA            9
#define MRFI_GDO_PA_PD          27  /* low when transmit is active, low during sleep */
#define MRFI_GDO_LNA_PD         28  /* low when receive is active, low during sleep */

/* CC2500 configuration. */
static const uint8_t radio_regs_config[][2] =
{
  /* internal radio configuration */
  { MRFI_CC2500_SPI_REG_IOCFG0,    MRFI_GDO_SYNC   },
  { MRFI_CC2500_SPI_REG_IOCFG2,    MRFI_GDO_PA_PD   },
  { MRFI_CC2500_SPI_REG_MCSM1,     0x3F    },
  { MRFI_CC2500_SPI_REG_MCSM0,     0x10 | (SMARTRF_SETTING_MCSM0 & (1<<2|1<<3))    },
/*  { MRFI_CC2500_SPI_REG_PKTLEN,    MRFI_SETTING_PKTLEN   },*/
  { MRFI_CC2500_SPI_REG_PKTCTRL0,  0x05 | (SMARTRF_SETTING_PKTCTRL0 & 1<<6) }, 
/*  { MRFI_CC2500_SPI_REG_PATABLE,   SMARTRF_SETTING_PATABLE0 }, */
  { MRFI_CC2500_SPI_REG_CHANNR,    SMARTRF_SETTING_CHANNR   },
/*  { MRFI_CC2500_SPI_REG_FIFOTHR,   0x07 | (SMARTRF_SETTING_FIFOTHR & (1<<4|1<<5|1<<6))  },*/
  /* imported SmartRF radio configuration */
  { MRFI_CC2500_SPI_REG_FSCTRL1,   SMARTRF_SETTING_FSCTRL1  },
  { MRFI_CC2500_SPI_REG_FSCTRL0,   SMARTRF_SETTING_FSCTRL0  },
  { MRFI_CC2500_SPI_REG_FREQ2,     SMARTRF_SETTING_FREQ2    },
  { MRFI_CC2500_SPI_REG_FREQ1,     SMARTRF_SETTING_FREQ1    },
  { MRFI_CC2500_SPI_REG_FREQ0,     SMARTRF_SETTING_FREQ0    },
  { MRFI_CC2500_SPI_REG_MDMCFG4,   SMARTRF_SETTING_MDMCFG4  },
  { MRFI_CC2500_SPI_REG_MDMCFG3,   SMARTRF_SETTING_MDMCFG3  },
  { MRFI_CC2500_SPI_REG_MDMCFG2,   SMARTRF_SETTING_MDMCFG2  },
  { MRFI_CC2500_SPI_REG_MDMCFG1,   SMARTRF_SETTING_MDMCFG1  },
  { MRFI_CC2500_SPI_REG_MDMCFG0,   SMARTRF_SETTING_MDMCFG0  },
  { MRFI_CC2500_SPI_REG_DEVIATN,   SMARTRF_SETTING_DEVIATN  },
  { MRFI_CC2500_SPI_REG_FOCCFG,    SMARTRF_SETTING_FOCCFG   },
  { MRFI_CC2500_SPI_REG_BSCFG,     SMARTRF_SETTING_BSCFG    },
  { MRFI_CC2500_SPI_REG_AGCCTRL2,  SMARTRF_SETTING_AGCCTRL2 },
  { MRFI_CC2500_SPI_REG_AGCCTRL1,  SMARTRF_SETTING_AGCCTRL1 },
  { MRFI_CC2500_SPI_REG_AGCCTRL0,  SMARTRF_SETTING_AGCCTRL0 },
  { MRFI_CC2500_SPI_REG_FREND1,    SMARTRF_SETTING_FREND1   },
  { MRFI_CC2500_SPI_REG_FREND0,    SMARTRF_SETTING_FREND0   },
  { MRFI_CC2500_SPI_REG_FSCAL3,    SMARTRF_SETTING_FSCAL3   },
  { MRFI_CC2500_SPI_REG_FSCAL2,    SMARTRF_SETTING_FSCAL2   },
  { MRFI_CC2500_SPI_REG_FSCAL1,    SMARTRF_SETTING_FSCAL1   },
  { MRFI_CC2500_SPI_REG_FSCAL0,    SMARTRF_SETTING_FSCAL0   },
  { MRFI_CC2500_SPI_REG_TEST2,     SMARTRF_SETTING_TEST2    },
  { MRFI_CC2500_SPI_REG_TEST1,     SMARTRF_SETTING_TEST1    },
  { MRFI_CC2500_SPI_REG_TEST0,     SMARTRF_SETTING_TEST0    },
  
  /*JMK:*/
  { MRFI_CC2500_SPI_REG_IOCFG0, 6 },
  { MRFI_CC2500_SPI_REG_FIFOTHR, 2 /*RX>8*/},
  { MRFI_CC2500_SPI_REG_PKTCTRL1, 1<<2 /*Append status, no addr check*/ },
  { MRFI_CC2500_SPI_REG_MCSM0, 1<<4|1<<2 },
//  { MRFI_CC2500_SPI_REG_PKTLEN, 16 },
  { MRFI_CC2500_SPI_REG_PATABLE, 0xfe },
};


/* Prototypes of internal functions. */
static uint8_t cc2500_cmd_strobe(uint8_t address);
static uint8_t cc2500_register(uint8_t address, uint8_t value);
static void cc2500_fifo(uint8_t address, uint8_t* data, uint8_t length);

/* Pointer to the receiver callback. */
static void (* receiver_callback)(void);

/*---------------------------------------------------------------------------*/
interrupt(PORT2_VECTOR)
isr_port2(void)
{
	if(P2IFG & (1 << P2GDO0))
	{
		if(receiver_callback)
		{
			(*receiver_callback)();
		}
		
		P2IFG &= ~(1 << P2GDO0);
	}
}
/*---------------------------------------------------------------------------*/
static uint8_t
cc2500_cmd_strobe(uint8_t address)
{
	if(!(address >= 0x30 && address <= 0x3D))
	{
		return -1;
	}
	
	SPI_CS_SET(0);
	SPI_CS_SET(1);
	
	SPI_WRITE_BYTE(address);
	SPI_WAIT_DONE();
	
	uint8_t status = SPI_READ_BYTE();
	SPI_CS_SET(0);
	
	return status;
}
/*---------------------------------------------------------------------------*/
static uint8_t
cc2500_register(uint8_t address, uint8_t value)
{
	SPI_CS_SET(0);
	SPI_CS_SET(1);
	
	SPI_WRITE_BYTE(address);
	SPI_WAIT_DONE();
	
	SPI_WRITE_BYTE(value);
	SPI_WAIT_DONE();
	
	uint8_t result = SPI_READ_BYTE();
	SPI_CS_SET(0);
	
	return result;
}
/*---------------------------------------------------------------------------*/
static void
cc2500_fifo(uint8_t address, uint8_t* data, uint8_t length)
{
	/* If nothing to transmit, do so. */
	if(length == 0)
	{
		return;
	}
	
	/* Address of the FIFO must be higher than one of a register. */
	if(!(address & BURST_BIT))
	{
		return;
	}
	
	SPI_CS_SET(0);
	SPI_CS_SET(1);
	
	SPI_WRITE_BYTE(address);
	SPI_WAIT_DONE();
	
	for(; length > 0; --length, ++data)
	{
		SPI_WRITE_BYTE(*data);
		SPI_WAIT_DONE();
		
		if(address & READ_BIT)
		{
			*data = SPI_READ_BYTE();
		}
	}
	
	SPI_CS_SET(0);
}
/*---------------------------------------------------------------------------*/
uint8_t
cc2500_read_register(uint8_t address)
{
	return cc2500_register(address | BURST_BIT | READ_BIT, DUMMY_BYTE);
}
/*---------------------------------------------------------------------------*/
void
cc2500_write_register(uint8_t address, uint8_t value)
{
	cc2500_register(address, value);
}
/*---------------------------------------------------------------------------*/
void
cc2500_write_tx_fifo(const uint8_t* data, uint8_t length)
{
	cc2500_fifo(MRFI_CC2500_SPI_REG_TXFIFO | BURST_BIT, (uint8_t*) data, length);
}
/*---------------------------------------------------------------------------*/
void
cc2500_read_rx_fifo(uint8_t* data, uint8_t length)
{
	cc2500_fifo(MRFI_CC2500_SPI_REG_RXFIFO | BURST_BIT | READ_BIT, data, length);
}
/*---------------------------------------------------------------------------*/
void
cc2500_write_patable01(uint8_t pa0, uint8_t pa1)
{
	uint8_t buf[2] = {pa0, pa1};
	cc2500_fifo(MRFI_CC2500_SPI_REG_PATABLE | BURST_BIT, buf, 2);
}
/*---------------------------------------------------------------------------*/
void
cc2500_init(void)
{
	/* Initialize radio chip. */
	SPI_CS_SET(1);
	clock_delay(40);
	SPI_CS_SET(0);
	clock_delay(400);
	/* pull CSn low and wait for SO to go low */
	SPI_CS_SET(1);
	while(P3IN & (1 << P3MISO));
	
	SPI_WRITE_BYTE(MRFI_CC2500_SPI_STROBE_SRES);
	SPI_WAIT_DONE();
	
	while(P3IN & (1 << P3MISO));
	SPI_CS_SET(0);
	
	/* Test connection to the radio chip. */
	#define TEST_VALUE 0xA5
	cc2500_write_register(MRFI_CC2500_SPI_REG_PKTLEN, TEST_VALUE);
	if(cc2500_read_register(MRFI_CC2500_SPI_REG_PKTLEN) != TEST_VALUE)
	{
		puts("Error while initializing CC2500 chip!");
		leds_on(LEDS_RED);
		return;
	}
		
	uint8_t i;
	for(i = 0; i < sizeof(radio_regs_config) / sizeof(radio_regs_config[0]); ++i)
		cc2500_write_register(radio_regs_config[i][0], radio_regs_config[i][1]);
		
	cc2500_cmd_strobe(MRFI_CC2500_SPI_STROBE_SRX);

	
	/* Set P2GDO0 as input. CC2500 will strobe it when receiving data. */
	P2IES = (1 << P2GDO0);
	P2IFG &= ~(1 << P2GDO0);
	P2IE = (1 << P2GDO0);
}
/*---------------------------------------------------------------------------*/
int
cc2500_receive_frame(uint8_t* data, uint8_t* length, uint8_t maxlength)
{
	uint8_t rxlen = cc2500_read_register(MRFI_CC2500_SPI_REG_RXBYTES);
	
	while(1)
	{
		uint8_t rxlen_v = cc2500_read_register(MRFI_CC2500_SPI_REG_RXBYTES);
		
		if(rxlen_v == rxlen)
			break;
			
		rxlen = rxlen_v;
	}
	
	*length = rxlen;
	
	if(rxlen == 0)
	{
		return 0;
	}
	
	if(rxlen > maxlength)
	{
		*length = maxlength;
	}
	
	cc2500_read_rx_fifo(data, *length);
	
	if(rxlen > *length)
	{
		cc2500_cmd_strobe(MRFI_CC2500_SPI_STROBE_SIDLE);
		cc2500_cmd_strobe(MRFI_CC2500_SPI_STROBE_SFRX);
		cc2500_cmd_strobe(MRFI_CC2500_SPI_STROBE_SRX);
	}
	
	return 0;
}
/*---------------------------------------------------------------------------*/
int
cc2500_send_frame(uint8_t* data, uint8_t length)
{
	cc2500_write_register(MRFI_CC2500_SPI_REG_PKTLEN, length);
	cc2500_write_tx_fifo(data, length);
	cc2500_cmd_strobe(MRFI_CC2500_SPI_STROBE_STX);
	
	return 0;
}
/*---------------------------------------------------------------------------*/
void
cc2500_set_receiver(void (* recv)(void))
{
	receiver_callback = recv;
}
/*---------------------------------------------------------------------------*/
