/**
 *  Energy Monitoring & Management Agent for IPV6 RestFull HTTP 
 *  Copyright (C) 2010  DUHART Clément <duhart.clement@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * All rights reserved.
 */

 /**
 * \brief This file is an example to make two sensor drivers for brightness and temperature on the TWI (I2C) bus.
 */
#include "Emma-API.h"
#include <avr/pgmspace.h>

#define LightSensorAddress  0b01110010 // 0x39
#define MAXLUX 1846 // standard mode max

// Table for channel ratio (i.e. channel1 / channel0)
static unsigned char ratioLut[129] PROGMEM = {
100,100,100,100,100,100,100,100,
100,100,100,100,100,100,99,99,
99,99,99,99,99,99,99,99,
99,99,99,98,98,98,98,98,
98,98,97,97,97,97,97,96,
96,96,96,95,95,95,94,94,
93,93,93,92,92,91,91,90,
89,89,88,87,87,86,85,84,
83,82,81,80,79,78,77,75,
74,73,71,69,68,66,64,62,
60,58,56,54,52,49,47,44,
42,41,40,40,39,39,38,38,
37,37,37,36,36,36,35,35,
35,35,34,34,34,34,33,33,
33,33,32,32,32,32,32,31,
31,31,31,31,30,30,30,30,
30};

// Table to convert channel values to counts
static unsigned short countLut[128] PROGMEM = {
0, 1, 2, 3, 4, 5, 6, 7,
8, 9, 10, 11, 12, 13, 14, 15,
16, 18, 20, 22, 24, 26, 28, 30,
32, 34, 36, 38, 40, 42, 44, 46,
49, 53, 57, 61, 65, 69, 73, 77,
81, 85, 89, 93, 97, 101, 105, 109,
115, 123, 131, 139, 147, 155, 163, 171,
179, 187, 195, 203, 211, 219, 227, 235,
247, 263, 279, 295, 311, 327, 343, 359,
375, 391, 407, 423, 439, 455, 471, 487,
511, 543, 575, 607, 639, 671, 703, 735,
767, 799, 831, 863, 895, 927, 959, 991,
1039,1103,1167,1231,1295,1359,1423,1487,
1551,1615,1679,1743,1807,1871,1935,1999,
2095,2223,2351,2479,2607,2735,2863,2991,
3119,3247,3375,3503,3631,3759,3887,4015};

void sleepModeLight(void){
    I2C_START_WAIT(LightSensorAddress+I2C_WRITE_FLAG);
    I2C_WRITE(0x00);            // Power-down state

	return ;
}

unsigned char WakeUpModeLight(void){
	unsigned char ret = -1;
	unsigned char count = 0;

	do {
	ret = I2C_START(LightSensorAddress+I2C_WRITE_FLAG); 

        if(ret == 0){

        I2C_WRITE(0x03); 

        I2C_REP_START(LightSensorAddress+I2C_READ_FLAG);
	ret = I2C_READ_ACK();
        }

        if(count++ > 10) 
            return 1;

	} while(ret != 0x03);			// While Response of command WakeUp is not 0x03

	return 0;
}

void setExtendedMode(void){
    I2C_START_WAIT(LightSensorAddress+I2C_WRITE_FLAG);
	I2C_WRITE(0x1D);			// Extended range mode
}


void ResetToStandardMode(void){
    I2C_START_WAIT(LightSensorAddress+I2C_WRITE_FLAG);
	I2C_WRITE(0x18);			// Extended range mode
}

unsigned char readAdc0(void){
	unsigned char readValue = 0;

	I2C_START_WAIT(LightSensorAddress+I2C_WRITE_FLAG);
	I2C_WRITE(0x43);

        I2C_REP_START(LightSensorAddress+I2C_READ_FLAG);
	readValue = I2C_READ_ACK(); 

	return readValue;
}


unsigned char readAdc1(void){
	unsigned char readValue = 0;

	I2C_START_WAIT(LightSensorAddress+I2C_WRITE_FLAG);
	I2C_WRITE(0x83);

        I2C_REP_START(LightSensorAddress+I2C_READ_FLAG);
	readValue = I2C_READ_ACK(); 

	return readValue;
}

// simplified lux equation approximation using tables
unsigned short calculateLux(unsigned char channel0, unsigned char channel1)
{
	unsigned long lux = 0;
	// lookup count from channel value
	unsigned short count0 = pgm_read_byte_near(countLut[channel0&0b01111111]);
	unsigned short count1 = pgm_read_byte_near(countLut[channel1&0b01111111]);

	// calculate ratio
	unsigned char ratio = 128; // default

	// avoid division by zero - count1 cannot be greater than count0
	if((count0) && (count1 <= count0))
		ratio = ((count1 * 128) / count0);

	// calculate lux
	lux = (((count0-count1) * pgm_read_byte_near(ratioLut[ratio]))  / 256);

	// if lux is over MAXLUX, put MAXLUX as maximum
	if(lux > MAXLUX)
		lux = MAXLUX;

	return ((unsigned short) lux);
}

int getLight(void){
	int light=0;
	unsigned char temp, adc0, adc1;

	/* Wait Data Light available */
	if(WakeUpModeLight() == 1) {
		return -1;
	}

    setExtendedMode();
    do{
	adc0 = readAdc0(); 
    adc1 = readAdc1(); 
    }
    while(! (adc0 & 128 && adc1 & 128));
    light = calculateLux(adc0,adc1);

    sleepModeLight();
    I2C_STOP();

	return light;
}


/*****************************************************************************/

#define TemperatureSensorAddress  0b10010010       

int sleepMode(void){
	unsigned char ret = -1;
	ret = I2C_START(TemperatureSensorAddress);  

	if(ret != 0)
		return ret;

     I2C_WRITE(0x01);                        // write on confiuration register
     I2C_WRITE(0xC0);                        // Put SuhtDown mode, start one shot convertion
     I2C_STOP();

return 0;
}

int WakeUpMode(void){
	unsigned char ret = -1;
	ret = I2C_START(TemperatureSensorAddress);  

	if(ret != 0)
		return ret;

	I2C_WRITE(0x01);			// CONTROLS/STATUS REGISTER 
	I2C_WRITE(0x40);            //WakeUP
	I2C_STOP();

return 0;
}

int configureTemperature(void){
    int t;
    I2C_INIT();

    t=I2C_START(TemperatureSensorAddress+I2C_WRITE_FLAG);
	if(t!=0)
        return t;

	I2C_WRITE(0x04);			// CONTROLS/STATUS REGISTER 
	I2C_WRITE(0xE0);			// 14bits resolution & Timeout Off0xE0
	
	I2C_WRITE(0x01);			// CONTROLS/STATUS REGISTER 
	I2C_WRITE(0x40);			

	I2C_STOP();
return 0;
}

int getTemperature(void){
	int temperature=0;
	unsigned char temp;

	WakeUpMode();

	/* Wait data temperature available */
	do {
	I2C_START_WAIT(TemperatureSensorAddress+I2C_WRITE_FLAG);  

	I2C_WRITE(0x04);			// CONTROLS/STATUS REGISTER 
	I2C_REP_START(TemperatureSensorAddress+I2C_READ_FLAG); 	// Send a responce request
	temp = I2C_READ_NACK();			// Read the response

	I2C_STOP();
	}
	while(!(temp & 0x01));			// While Data Available Flag is not set

	/* Read the TEMPERATURE DATA REGISTER */
	I2C_START_WAIT(TemperatureSensorAddress+I2C_WRITE_FLAG);  

	I2C_WRITE(0x00);                        // Write the temperature data register 0x00
	I2C_REP_START(TemperatureSensorAddress+I2C_READ_FLAG);       // Send a responce request
	temperature = I2C_READ_NACK()<<8;            // Read the response
	
	I2C_WRITE(0x10);                        // Write the temperature hight byte data register 0x00
	I2C_REP_START(TemperatureSensorAddress+I2C_READ_FLAG);       // Send a responce request
	temperature = I2C_READ_NACK()<<8;            // Read the response
	
	I2C_WRITE(0x11);                        // Write the temperature low byte data register 0x00
	I2C_REP_START(TemperatureSensorAddress+I2C_READ_FLAG);       // Send a responce request
	temperature |= I2C_READ_NACK();            // Read the response

	I2C_STOP();

	/* Go to sleep mode */
	sleepMode();

return temperature/128;
}
