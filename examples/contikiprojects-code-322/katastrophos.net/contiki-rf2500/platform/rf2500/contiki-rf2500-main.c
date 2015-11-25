/**
 * \file
 *	Main system file for the TI MSP430 RF2500 port.
 * \author
 * 	Wincent Balin <wincent.balin@gmail.com>
 */

#include <io.h>
#include <signal.h>

#include "contiki.h"

#include "dev/watchdog.h"
#include "dev/leds.h"
#include "dev/uart1.h"
#include "dev/serial-line.h"
#include "dev/button-sensor.h"
#include "dev/spi.h"
#include "dev/cc2500-jmk.h"
#include "dev/adc-sensors.h"
#include "dev/radio-sensor.h"

SENSORS(&button_sensor, &battery_sensor, &temperature_sensor, &radio_sensor);


#if 0
#include <stdio.h>
static void
print_processes(struct process * const processes[])
{
  /*  const struct process * const * p = processes;*/
  printf("Starting");
  while(*processes != NULL) {
    printf(" '%s'", (*processes)->name);
    processes++;
  }
  putchar('\n');
}
#endif


/*---------------------------------------------------------------------------*/
extern int _end;		/* Not in sys/unistd.h */
static char *cur_break = (char *)&_end;

/* Workaround for msp430-ld bug! Copied from cpu/msp430/msp430.c . */
void
align_workaround(void)
{
  if((uintptr_t)cur_break & 1)
  {
    cur_break++;
  }
}
/*---------------------------------------------------------------------------*/

int
main(void)
{
	/* Stop watchdog. */
	watchdog_stop();
	
	/* Initialize clock subsystem. */
	clock_init();
	
	/* Align workaround for msp430-ld. */
	align_workaround();
	
	/* Initialize peripherals. */
	leds_init();
	leds_off(LEDS_ALL);
	uart1_init(BAUD2UBR(9600)); /* Parameter is discarded in the rewritten UART driver anyway. */
  uart1_set_input(serial_line_input_byte);
  serial_line_init();	
  spi_init();
  cc2500_init();

	/* Initialize system services. */
	rtimer_init();
  process_init();
  process_start(&etimer_process, NULL);
  process_start(&sensors_process, NULL);
  
  process_start(&adc_sensors_process, NULL);

	/* Autostart user processes. */
	//print_processes(autostart_processes);
  autostart_start(autostart_processes);
  
 	/* Activate sensors. */
	button_sensor.activate();
	battery_sensor.activate();
	temperature_sensor.activate();
	radio_sensor.activate();

 
  /* Enable interrupts. */
  eint();
  
  /* Start watchdog. */
  watchdog_start();

  /* Main loop. */
  while(1)
  {
	  int r;
	  
	  /* Scheduler. */
	  do
	  {
		  /* Reset watchdog before running a process. */
		  watchdog_periodic();
		  /* Run process. */
		  r = process_run();
	  }
	  while(r > 0);

	  /* If no events to process, go into LPM. */
    if(process_nevents() == 0)
   	{
	   	LPM4;
   	}
  }
}

