/**
 * \file
 *	Radio sensor implementation for the TI MSP430 RF2500 port
 * \author
 * 	Wincent Balin <wincent.balin@gmail.com>
 */

#include "contiki.h"

#include "dev/radio-sensor.h"

uint8_t cc2500_last_lqi = 0;
uint8_t cc2500_last_rssi = 0;

const struct sensors_sensor radio_sensor;

/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
static int
irq(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
activate(void)
{
}
/*---------------------------------------------------------------------------*/
static void
deactivate(void)
{
}
/*---------------------------------------------------------------------------*/
static int
active(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static unsigned int
value(int type)
{
  switch(type)
  {
	  case 0:
  	  return cc2500_last_lqi;
  	  
  	case 1:
  	default:
    	return cc2500_last_rssi;
  }
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, void *c)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void *
status(int type)
{
  return NULL;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(radio_sensor, RADIO_SENSOR,
	       init, irq, activate, deactivate, active,
	       value, configure, status);
