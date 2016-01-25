
/*
* \file
*		Light and temperature sensor native demo
*
* \author
*		Koray INCKI <koray.incki@ozu.edu.tr
*/

#include "contiki.h"
#include "sys/node-id.h"
#include <stdio.h>
//#include "dev/sht11/sht11-sensor.h"

/* Available Sensors */
#include "dev/light-sensor.h"
#include "dev/temperature-sensor.h"
#include "dev/battery-sensor.h"
#include "dev/radio-sensor.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"


PROCESS(sky_sensor_process, "Light & Temp Sensor Demo");
AUTOSTART_PROCESSES(&sky_sensor_process);

//extern const struct sensors_sensor temperature_sensor;

/*---------------------------------------------------------------------------*/
static int
get_light(void)
{
  //return 10 * light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC) / 7;
  return 10 * light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR) / 7;

  //return 3125 * light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR) / 512;

}

static int
get_temperature(void)
{
	return temperature_sensor.value(1); // input param type -> 1, no effect
}

static int
get_battery()
{
	return battery_sensor.value(1); // input param type -> 1, no effect
}

static int
get_button()
{
	return button_sensor.value(1); // input param type -> 1, no effect
}

static int
get_radioSSI()
{
	return radio_sensor.value(RADIO_SENSOR_LAST_VALUE);
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*  Following functions does not provide emulated data on Cooja.
 *  You can use them on a real device, though!
 *
 * static int
 * get_temp(void)
 * {
 *   return ((sht11_sensor.value(SHT11_SENSOR_TEMP) / 10) - 396) / 10;
 * }
 *
 * static int
 * get_humidity(void)
 * {
 * 	return sht11_sensor.value(SHT11_SENSOR_HUMIDITY);
 * }
 *
 * static int
 * get_battery(void)
 * {
 * 	return sht11_sensor.value(SHT11_SENSOR_BATTERY_INDICATOR);
 * }
 *
 * --------------------------------------------------------------------------*/

PROCESS_THREAD(sky_sensor_process, ev, data)
{
  static struct etimer timer;
  PROCESS_BEGIN();


  etimer_set(&timer, CLOCK_SECOND * 2);

  SENSORS_ACTIVATE(light_sensor);
  SENSORS_ACTIVATE(temperature_sensor);
  SENSORS_ACTIVATE(button_sensor);
  SENSORS_ACTIVATE(battery_sensor);
  SENSORS_ACTIVATE(radio_sensor);
  //SENSORS_ACTIVATE(sht11_sensor);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);

	printf("<KORAY> Node-%d: light sensor value is %d\n", node_id, get_light());
	printf("<KORAY> Node-%d: temperature sensor value is %d\n", node_id, get_temperature());
	printf("<KORAY> Node-%d: battery sensor value is %d\n", node_id, get_battery());
	printf("<KORAY> Node-%d: button sensor value is %d\n", node_id, get_button());
	printf("<KORAY> Node-%d: radioSSI sensor value is %d\n", node_id, get_radioSSI());

	/*
	 * SHT11 sensors has no effect on Cooja simulation
	 *
	 * printf("<KORAY> Node-%d: temp sensor value is %d\n", node_id, get_temp());
	 * printf("<KORAY> Node-%d: humidity sensor value is %d\n", node_id, get_humidity());
	 * printf("<KORAY> Node-%d: battery sensor value is %d\n", node_id, get_battery());
	 *
	 */


  }

  PROCESS_END();
}

