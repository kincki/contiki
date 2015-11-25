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
 * \brief This file is an example to make a simple E.M.M.A sensor application.
 * \see ressources.c
 */
#include "Emma-API.h"
#include "ressources.h"

#define LED0 5
#define LED1 6
#define LED2 7

RESSOURCE_LIST_LOAD(ressources_list);

// Declaration of the ressource
RESSOURCE_INIT(temperature);
RESSOURCE_INIT(brightness);
RESSOURCE_INIT(led0);

void setLed0(int val){
  if(val) HARDWARE_DIGIT_SET_UP(PORTB, LED0);
  else    HARDWARE_DIGIT_SET_DOWN(PORTB,LED0);
}

int getLed0(){
  return HARDWARE_DIGIT_GET(PORTB, LED0);
}

RESSOURCES_INIT(){
  /* I2C Sensor interface*/
  configureTemperature();
}

// Connection between the driver API and the REST API
RESSOURCES_LOADER(){

	RESSOURCE_UNIT(brightness,lux);
	RESSOURCE_GET(brightness, &getLight);
	RESSOURCE_MIN(brightness, 0);
	RESSOURCE_MAX(brightness, 255);

	RESSOURCE_UNIT(temperature,celcius);
	RESSOURCE_GET(temperature, &getTemperature);
	RESSOURCE_MIN(temperature, 0);
	RESSOURCE_MAX(temperature, 200);

	RESSOURCE_UNIT(led0,boolean);
	RESSOURCE_GET(led0,&getLed0);
	RESSOURCE_SET(led0,&setLed0);
	RESSOURCE_MIN(led0, 0);
	RESSOURCE_MAX(led0, 1);

}
