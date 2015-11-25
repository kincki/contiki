/**
 * \file
 *	ADC config include file for the TI MSP430 RF2500 port.
 * \author
 * 	Wincent Balin <wincent.balin@gmail.com>
 */

#ifndef ADC_SENSORS_H
#define ADC_SENSORS_H

#include "contiki.h"

#include "dev/battery-sensor.h"

/*
 * Add temperature sensor infrastructure.
 * Essentially a copy of the dev/temperature-sensor.h
 * from the native target.
 */

#include "lib/sensors.h"

extern const struct sensors_sensor temperature_sensor;

#define TEMPERATURE_SENSOR "Temperature"


PROCESS_NAME(adc_sensors_process);

#endif
