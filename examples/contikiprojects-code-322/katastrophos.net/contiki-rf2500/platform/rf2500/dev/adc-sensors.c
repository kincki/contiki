/**
 * \file
 *	ADC config file for the TI MSP430 RF2500 port.
 * \author
 * 	Wincent Balin <wincent.balin@gmail.com>
 */

#include <io.h>
#include <signal.h>

#include "contiki.h"

#include "adc-sensors.h"


enum adc_state_t
{
	BATTERY,
	TEMPERATURE,
	SENSORS,
	IDLE
};

struct adc_channel_t
{
	uint16_t value;
	bool active;
};


static enum adc_state_t adc_state = IDLE;

static struct adc_channel_t adc_sensors[SENSORS];

PROCESS(adc_sensors_process, "ADC sensors");
AUTOSTART_PROCESSES(&adc_sensors_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(adc_sensors_process, ev, data)
{
	static struct etimer pause_timer;
	unsigned int channel;

	PROCESS_BEGIN();

	/* Setup pause timer. */
	etimer_set(&pause_timer, 1);
	
	while(1)
	{
		if(adc_state == IDLE)
		{
			/* Search for a first active sensor. */
			for(channel = 0; channel < SENSORS; channel++)
			{
				if(adc_sensors[channel].active)
				{
					break;
				}
			}
			
			/* If found, set ADC state accordingly. */
			if(channel < SENSORS)
			{
				adc_state = channel;
			}
		}
		else
		{
			/* Search for the next active sensor. */
			for(channel = adc_state + 1; channel < SENSORS; channel++)
			{
				if(adc_sensors[channel].active)
				{
					break;
				}
			}
			
			/* If no following active sensors found, look for former sensors. */
			if(channel == SENSORS)
			{
				for(channel = 0; channel <= adc_state; channel++)
				{
					if(adc_sensors[channel].active)
					{
						break;
					}
				}
				
				/* If still no active sensor found, set ADC state to idle. */
				if(channel == adc_state + 1)
				{
					adc_state = IDLE;
				}
				else	/* Active sensor found, set ADC state. */
				{
					adc_state = channel;
				}
			}
			else	/* Active sensor found, set ADC state. */
			{
				adc_state = channel;
			}
		}
		
		if(adc_state != IDLE)
		{
			/* Setup the ADC. Taken from demo_AP.c of rf2500_wsm project. */
			if(adc_state == BATTERY)
			{
				/* Measure AVCC / 2 */
	      ADC10CTL1 = INCH_11;
      	/* VR+ = VCC - VSS, 32*ADC10CLKs, Ref on, ADC on, interrupt enabled,
      	   VREF = 2.5V . */	      
  	    ADC10CTL0 = SREF_1 + ADC10SHT_2 + REFON + ADC10ON + ADC10IE + REF2_5V;
			}
			else if(adc_state == TEMPERATURE)
			{
				/* Temperature sensor, ADC10CLK / 5 */
      	ADC10CTL1 = INCH_10 + ADC10DIV_4;
      	/* VR+ = VCC - VSS, 64*ADC10CLKs, Ref on, ADC on, interrupt enabled,
      	   low sample rate. */
      	ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ADC10IE + ADC10SR;
			}

     	/* Wait for reference to settle. */
     	clock_delay(240);
     	
			/* Enable conversion and start it. */
			ADC10CTL0 |= ENC + ADC10SC;
		}

		/* Pause to let ADC make a conversion. */
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
		
		/* Reset pause timer. */
		etimer_reset(&pause_timer);
	}
	
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
interrupt(ADC10_VECTOR)
isr_adc10(void)
{
	/* Store value into the appropriate sensor structure. */
	adc_sensors[adc_state].value = ADC10MEM;
	
	/* Switch off everything that is possible. */
	ADC10CTL0 &= ~(ENC);
	ADC10CTL0 &= ~(REFON + ADC10ON);
}
/*---------------------------------------------------------------------------*/
static void
battery_init(void)
{
}
/*---------------------------------------------------------------------------*/
static int
battery_irq(void)
{
	return 0;
}
/*---------------------------------------------------------------------------*/
static void
battery_activate(void)
{
	adc_sensors[BATTERY].active = TRUE;
}
/*---------------------------------------------------------------------------*/
static void
battery_deactivate(void)
{
	adc_sensors[BATTERY].active = FALSE;
}
/*---------------------------------------------------------------------------*/
static int
battery_active(void)
{
  return adc_sensors[BATTERY].active;
}
/*---------------------------------------------------------------------------*/
static unsigned int
battery_value(int type)
{
  return adc_sensors[BATTERY].value;
}
/*---------------------------------------------------------------------------*/
static int
battery_configure(int type, void* c)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void*
battery_status(int type)
{
  return NULL;
}
/*---------------------------------------------------------------------------*/
static void
temperature_init(void)
{
}
/*---------------------------------------------------------------------------*/
static int
temperature_irq(void)
{
	return 0;
}
/*---------------------------------------------------------------------------*/
static void
temperature_activate(void)
{
	adc_sensors[TEMPERATURE].active = TRUE;
}
/*---------------------------------------------------------------------------*/
static void
temperature_deactivate(void)
{
	adc_sensors[TEMPERATURE].active = FALSE;
}
/*---------------------------------------------------------------------------*/
static int
temperature_active(void)
{
  return adc_sensors[TEMPERATURE].active;
}
/*---------------------------------------------------------------------------*/
static unsigned int
temperature_value(int type)
{
  return adc_sensors[TEMPERATURE].value;
}
/*---------------------------------------------------------------------------*/
static int
temperature_configure(int type, void* c)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void*
temperature_status(int type)
{
  return NULL;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(battery_sensor, BATTERY_SENSOR,
	       battery_init, battery_irq,
	       battery_activate, battery_deactivate, battery_active,
	       battery_value, battery_configure, battery_status);

SENSORS_SENSOR(temperature_sensor, TEMPERATURE_SENSOR,
	       temperature_init, temperature_irq,
	       temperature_activate, temperature_deactivate, temperature_active,
	       temperature_value, temperature_configure, temperature_status);
/*---------------------------------------------------------------------------*/
