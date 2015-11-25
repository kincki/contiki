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
 *
 * Documentation : http://www.ece.fr/~duhart//emma/doc/html/
 *
 * \defgroup API E.M.M.A API
 * \brief This API allows an easy way to make hardware drivers with HARDWARE API and the Emma applications by using RESSOURCES API.\n
 *
 * \image html ../EMMA-API-overview.png
 * The drivers should be implemented in a specific named file ressources.c and the file Rest6-Agent.c should only contain ressources declarations and links.
 * 
 * \li /contiki-2.x/plateform/ { xxxx } /Rest6-Agent.c
 * \li /contiki-2.x/plateform/ { xxxx } /ressources.c
 * \li /contiki-2.x/plateform/ { xxxx } /ressources.h
 * 
 * @{
 */

/**
 * \file
 *         EMMA API - Application function
 *         This file contains the application function to construct a plateform of RestFull publication.
 * \author
 *         Clement DUHART <duhart.clement@gmail.com>
 */
#include "Emma-Rest6.h"
#include <stdio.h>

/**
 *\name Ressource functions API
 * @{
 */

/**
 * \brief      Loader of ressources in Emma-Rest6 Engine.\n
 *             This function should be implemented in the file : /contiki-2.x/plateform/{MyApps}/Agent-Rest6.c
 * \return     No return
 *
 */
#define RESSOURCES_LOADER() \
		void ressources_loader() 

/**
 * \brief      Init function called by Emma Core on starting device.\n
 *             This function should be implemented in the file : /contiki-2.x/plateform/{MyApps}/Agent-Rest6.c
 * \return     No return
 *
 */
#define RESSOURCES_INIT() \
	void ressources_init()

/**
 * \brief           Declaration of a ressource in the Emma-Rest6 Engine\n
 *             	    A new declaration of ressource will allocate slot for data ressource and data log
 * \param sensor    Name of the new ressource, this name will be used for the publish attribute
 * \return     	    No return value
 *
 */
#define RESSOURCE_INIT(sensor) \
	volatile struct Ressource sensor

/**
 * \brief           Initialization of the ressource unit.
 * \param sensor    Name of the ressource to set the unit.
 * \param unite	    Name of the unit, this name is used for published data attribute.
 * \return     	    No return value
 *
 */
#define RESSOURCE_UNIT(sensor, unite) \
			strcpy((char*)sensor.name,#sensor "\0"); \
			list_add(ressources_list, (char*)&sensor); \
			strcpy((char*)sensor.unit,#unite "\0")

/**
 * \brief           Initialization of the setter function for a ressource.
 * \param sensor    Name of the concerned ressource.
 * \param setFunc   Pointer on the setter function which must have the prototype : void func(int data);
 * \return     	    No return value
 *
 */
#define RESSOURCE_SET(sensor, setFunc) \
	sensor.setData = setFunc

/**
 * \brief           Initialization of the getter function for a ressource.
 * \param sensor    Name of the concerned ressource.
 * \param getFunc   Pointer on the getter function which must have the prototype : int func(void);
 * \return     	    Return the current value.
 *
 */
#define RESSOURCE_GET(sensor, getFunc) \
	sensor.getData = getFunc; \
	sensor.min = 0; \
	sensor.max = 100

/**
 * \brief           Initialization of the min value for the ressource.
 * \param sensor    Name of the concerned ressource.
 * \param min       Value of the minimum
 * \return     	    Return the current value.
 *
 */
#define RESSOURCE_MIN(sensor, Vmin) \
	sensor.min = Vmin

/**
 * \brief           Initialization of the max value for the ressource.
 * \param sensor    Name of the concerned ressource.
 * \param max       Value of the maximum
 * \return     	    Return the current value.
 *
 */
#define RESSOURCE_MAX(sensor, Vmax) \
	sensor.max = Vmax

/**
 * \brief           Get an external ressource.
 * \param sensor    Name of the concerned ressource.
 * \return     	    No return;
 *
 */
#define RESSOURCE_LOAD(sensor) \
	extern struct Ressource sensor

/**
 * \brief           Create a new ressource list
 * \param sensor    Name of the ressource list.
 * \return     	    No return;
 *
 */
#define RESSOURCE_LIST(name) \
         volatile void *LIST_CONCAT(name,_list) = NULL; \
         volatile list_t name = (list_t)&LIST_CONCAT(name,_list)

/**
 * \brief           Load an external ressource list
 * \param sensor    Name of the ressource list.
 * \return     	    No return;
 *
 */
#define RESSOURCE_LIST_LOAD(name) \
	extern list_t name

 /**
 * @}
 */

/**
 * 
 * \name Hardware Digit Functions API
 * \brief Abstract functions for getting or setting a specific pin state.
 *
 * @{
 */


/**
 * \brief           Return the current value of a specify pin
 * \param port      PORT name of the pin
 * \param pin       PIN number on the specify port
 * \return     	    The current value of the pin.
 *
 */
#define HARDWARE_DIGIT_GET(port, pin) \
	((port & ( 1 << pin)) > 0)

/**
 * \brief           Set up a specify pin.
 * \param port      PORT name of the pin
 * \param pin       PIN number on the specify port
 * \return     	    No return.
 *
 */
#define HARDWARE_DIGIT_SET_UP(port, pin) \
	 port |= (1 << pin)

/**
 * \brief           Set down a specify pin.
 * \param port      PORT name of the pin
 * \param pin       PIN number on the specify port
 * \return     	    No return;
 *
 */
#define HARDWARE_DIGIT_SET_DOWN(port,pin) \
	 port &= ~(1<<pin) 
/** @} */


/**
 *
 * \name Hardware PWM (Pulse Width Modulation) API
 * @{
 */

/**
 * \brief           Initialize the four standard PWM (OCR1A, OCR1B, OCR3A, OCR3B)
 * \return     	    No return;
 *
 */
#define HARDWARE_PWM_INIT() initPWM()

/**
 * \brief           Set the value of a specify pwm.
 * \param pin       PWM pin number target
 * \param val       PWM value between 0-1024
 * \return     	    No return;
 *
 */
#define HARDWARE_PWM_SET(pin,val) setPWM(pin,val)

/**
 * \brief           Get the value of a specify pwm
 * \param pin       PWM pin number target
 * \return     	    No return;
 *
 */
#define HARDWARE_PWM_GET(pin) getPWM(pin) 

/** @} */

/**
 *
 * \name Hardware Two Wire Interface (I2C) API
 * @{
 */
#define I2C_READ_FLAG    1
#define I2C_WRITE_FLAG   0
/*---------------------------------------------------------------------------*/
/**
 * \brief           Interface function for init TWI(I2C) interface, all plateforms must implement this function
 * \return     	    No return.
 *
 */
#define I2C_INIT()           i2c_init()

/*---------------------------------------------------------------------------*/
/**
 * \brief           Interface function for start TWI(I2C) communication, all plateform must implements this function
 * \param address   Address of the target device.
 * \retval 0        Success in target device access.
 * \retval 1        Fail in configuration of the communication
 * \retval 2        Fail in target device access. (Bad address)
 *
 */
#define I2C_START(addr)      i2c_start(addr)

/*---------------------------------------------------------------------------*/
/**
 * \brief           Waiting Interface function for start TWI(I2C) communication, all plateform must implements this function
 * \param address   Address of the target device.
 * \retval 0        Success in target device access.
 * \retval 1        Fail in configuration of the communication
 * \retval 2        Fail in target device access. (Bad address)
 *
 */
#define I2C_START_WAIT(addr) i2c_start_wait(addr)

/*---------------------------------------------------------------------------*/
/**
 * \brief           Interface function for repeating start TWI(I2C) communication, all plateform must implements this function
 * \param address   Address of the target device.
 * \retval 0        Success in target device access.
 * \retval 1        Fail in configuration of the communication
 * \retval 2        Fail in target device access. (Bad address)
 *
 */
#define I2C_REP_START(addr)  i2c_rep_start(addr)

/*---------------------------------------------------------------------------*/
/**
 * \brief           Interface function for stopping and releasing TWI(I2C) communication, all plateform must implements this function
 * \return          No return.
 *
 */
#define I2C_STOP()           i2c_stop()

/*---------------------------------------------------------------------------*/
/**
 * \brief           Interface function for sending a byte over TWI(I2C) communication, all plateform must implements this function
 * \param data      Byte to send.
 * \retval 0        Success in sending data.
 * \retval 1        Fail in sending data
 *
 */
#define I2C_WRITE(data)      i2c_write(data)

/*---------------------------------------------------------------------------*/
/**
 * \brief           Interface function for reading one byte from TWI(I2C) communication, all plateform must implements this function
 * \return          The readed data byte.
 *
 */
#define I2C_READ_ACK()       i2c_readAck()

/*---------------------------------------------------------------------------*/
/**
 * \brief           Interface function for reading one byte if followed by a stop condition from TWI(I2C) communication, all plateform must implements this function
 * \return          The readed data byte.
 *
 */
#define I2C_READ_NACK()      i2c_readNak()
/** @} */

/** 
 * \name Implemented functions depending of the CPU / Plateform
 * \brief This function must be implemented for using full EMMA HARDWARE API.
 * @{
 */

void initPWM(void);
void setPWM(char pin, int val);
int getPWM(char pin);

void i2c_init(void);
unsigned char i2c_start(unsigned char address);
int i2c_start_wait(unsigned char address);
unsigned char i2c_rep_start(unsigned char address);
void i2c_stop(void);
unsigned char i2c_write( unsigned char data );
unsigned char i2c_readAck(void);
unsigned char i2c_readNak(void);
/** @} */

/** 
 * \name Implemented functions for each EMMA Applications
 * \brief This must be never used.
 * \ref RESSOURCES_INIT
 * \ref RESSOURCES_LOADER
 * @{
 */
void ressources_init();
void ressources_loader();

/** @} */
/** @} */
