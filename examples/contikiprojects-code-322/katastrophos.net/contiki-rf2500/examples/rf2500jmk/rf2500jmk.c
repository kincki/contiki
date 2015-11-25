/**
 * \file
 *         TI MSP430 RF2500 access point
 * \author
 *         Wincent Balin <wincent.balin@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "contiki.h"

#include "dev/button-sensor.h"
#include "dev/adc-sensors.h"
#include "dev/radio-sensor.h"
#include "dev/leds.h"

#include "shell.h"          // for shell_blink_init() (and possibly others)
#include "serial-shell.h"   // for serial_shell_init()

#include "shell-lsensors.h"
#include "shell-rsensors.h"

#include "dev/cc2500-jmk.h"

extern uint8_t cc2500_last_lqi;
extern uint8_t cc2500_last_rssi;

/* Prototypes of auxilary functions. */
static void receiver_callback(void);
static void send_beacon(void);

#define RCVBUFSIZE 64
static uint8_t rcvbuf[RCVBUFSIZE];
static uint8_t rcvlength;

#define SNDBUFSIZE 16
static uint8_t sndbuf[SNDBUFSIZE];


static struct etimer beacon_timer;

/*---------------------------------------------------------------------------*/
PROCESS(rf2500jmk_main_process, "RF2500 JMK main");
PROCESS(rf2500jmk_beacon_process, "RF2500 JMK beacon");
AUTOSTART_PROCESSES(&rf2500jmk_main_process, &rf2500jmk_beacon_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rf2500jmk_main_process, ev, data)
{
	static struct etimer pause_timer;

  PROCESS_BEGIN();
  
  serial_shell_init();
//  shell_blink_init();
  shell_ps_init();
//	shell_reboot_init();

	shell_lsensors_init();
	shell_rsensors_init();

	leds_on(LEDS_GREEN);
	
	/* Radio chip is configured with Data Whitening and no FEC; CRC is enabled. */
	/* Set radio chip to fixed packet length of 16 bytes. */
	cc2500_write_register(MRFI_CC2500_SPI_REG_PKTCTRL0, 1 << 6 | 1 << 2 | 0 << 0);
	cc2500_write_register(MRFI_CC2500_SPI_REG_PKTLEN, 16);
	
	/* Set receiver callback. */
	cc2500_set_receiver(receiver_callback);
	
	while(1)
	{
		PROCESS_YIELD();
		
		/* Receive frame. */
		cc2500_receive_frame(rcvbuf, &rcvlength, RCVBUFSIZE);
		
		/* If data received and CRC ok, print it. */
		if(rcvlength > 0 && (rcvbuf[rcvlength-1] & (1 << 7)))
		{
			leds_on(LEDS_GREEN);
			
			/* Prevent from sending further beacons. */
			etimer_restart(&beacon_timer);
			
			/* Store radio sensor values. */
			cc2500_last_lqi = rcvbuf[rcvlength-1] & ((1 << 7) - 1);
			cc2500_last_rssi = rcvbuf[rcvlength-2];
			
			/* Send reply. */
			etimer_set(&pause_timer, CLOCK_SECOND / 4);
			PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
			leds_off(LEDS_RED);
			
			/* Received command rsensors. */
			if(strncmp(rcvbuf, "rsensors", 8) == 0)
			{
				sndbuf[0] = 'R';
				sndbuf[1] = 'S';
				
				sndbuf[2] = (uint8_t) button_sensor.value(0);
				
				div_t volt = div((((long) battery_sensor.value(0)) * 25) / 512, 10);
				sndbuf[3] = '0' + volt.quot;
				sndbuf[4] = '0' + volt.rem;
				
				sndbuf[5] = ((temperature_sensor.value(0) - 673) * 423) / 1024;
				
				sndbuf[6] = radio_sensor.value(0);
				sndbuf[7] = radio_sensor.value(1);

				cc2500_send_frame(sndbuf, 8);
			}
			/* Received reply to rsensors. */
			else if(rcvbuf[0] == 'R' && rcvbuf[1] == 'S')
			{
				  printf("Values of remote sensors:\r\n");
  
					printf("Button sensor:\t");
					printf("%s\r\n", rcvbuf[2] ? "on" : "off");

					printf("Battery sensor:\t");
					printf("%c.%c V\r\n", rcvbuf[3], rcvbuf[4]);
	
					printf("Temperature sensor:\t");
					printf("%d Celsius\r\n", rcvbuf[5]);
	
					printf("Radio sensor:\t");
					printf("LQI=%d\t", rcvbuf[6]);
					int rssi = rcvbuf[7];
					printf("RSSI=%s%d\r\n", rssi >= 128 ? "-" : "",
	        				                rssi >= 128 ? 256 - rssi : rssi);

			}
			/* Everything else. */
			else
			{
				send_beacon();
			}

			leds_off(LEDS_GREEN);
		}
	}


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rf2500jmk_beacon_process, ev, data)
{
	PROCESS_BEGIN();
	
	etimer_set(&beacon_timer, CLOCK_SECOND * 2);
	
	while(1)
	{
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
		leds_toggle(LEDS_RED);
		send_beacon();
		etimer_reset(&beacon_timer);
	}
	
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void receiver_callback(void)
{
	process_post(&rf2500jmk_main_process, PROCESS_EVENT_POLL, 0);
}
/*---------------------------------------------------------------------------*/
static void send_beacon(void)
{
	static const uint8_t beacon[] = "ABCDEFGH";
	leds_on(LEDS_GREEN);
	cc2500_send_frame((uint8_t*) beacon, sizeof(beacon)-1);
	leds_off(LEDS_GREEN);
}
/*---------------------------------------------------------------------------*/
