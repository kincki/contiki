/**
 * \file
 *         lsensors Contiki shell command
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include <stdio.h>
#include <stdlib.h>

#include "contiki.h"
#include "shell.h"

#include "dev/button-sensor.h"
#include "dev/adc-sensors.h"
#include "dev/radio-sensor.h"

/*---------------------------------------------------------------------------*/
PROCESS(shell_lsensors_process, "lsensors");
SHELL_COMMAND(lsensors_command,
	      "lsensors",
	      "lsensors: local sensors",
	      &shell_lsensors_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_lsensors_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Values of local sensors:\r\n");
  
	printf("Button sensor:\t");
	printf("%s\r\n", button_sensor.value(0) ? "on" : "off");

	printf("Battery sensor:\t");
	div_t volt = div((((long) battery_sensor.value(0)) * 25) / 512, 10);
	printf("%c.%c V\r\n", '0' + volt.quot, '0' + volt.rem);
	
	printf("Temperature sensor:\t");
	printf("%d Celsius\r\n", ((temperature_sensor.value(0) - 673) * 423) / 1024);
	
	printf("Radio sensor:\t");
	printf("LQI=%d\t", radio_sensor.value(0));
	int rssi = radio_sensor.value(1);
	printf("RSSI=%s%d\r\n", rssi >= 128 ? "-" : "",
	                        rssi >= 128 ? 256 - rssi : rssi);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
shell_lsensors_init(void)
{
  shell_register_command(&lsensors_command);
}
/*---------------------------------------------------------------------------*/
