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
 * \addtogroup EMMA
 *
 * @{
 */

/**
 * \file
 *         Configuration file of EMMA-Rest6 Engine
 * \author
 *         Clement DUHART <duhart.clement@gmail.com>
 *
 *         This file contains all the static configuration of Emma-Rest6 Engine.
 */

// Debug mode to print the different step/state of the machine
#define DEBUG 1
// HTTP Notification mode is a light web client to send simple request to another (light) web server.
#define SEND_NOTIFY 1

// Port for listenning incomming request 
#define PORT 80

// Periodic time for calling the getter function of each ressource
#define PROCESS_TIME 3
// Periodic time for calling the log process of all ressources
#define PROCESS_TIME_LOG 5
// Periodic time for calling the report process for sending report with HTTP notification
#define PROCESS_TIME_REPORT 5

// Size max of a ressource name
#define RESSOURCE_NAME_SIZE 32
// Length of the max id number
#define RESSOURCE_ID_MAX_DIGIT 3

// Size of the outcomming body request ({\n\"value\":\"xxxx"\n} = 23)
#define BODY_NOTIFY 32

// Notification buffer size define
// Max size of URI for sending request (/data/{ressource}/{report | log}/{id} = 15 + sizeof({ressource}) + sizeof({id}))
#define URI_OUTCOMMING_MAX_SIZE (15 + RESSOURCE_NAME_SIZE + RESSOURCE_ID_MAX_DIGIT)
#define URI_INCOMMING_MAX_SIZE (15 + RESSOURCE_NAME_SIZE + RESSOURCE_ID_MAX_DIGIT)


// There are two buffer of size BUFFER_SIZE 
#define BUFFER_SIZE 65
#define BUFFER_SOCKET_SIZE 65
#define BUFFERMINI_SIZE 6
#define INCOMMING_CONNECTION_MAX 1
#define OUTCOMMING_CONNECTION_MAX 1
#define TIMEOUT 20
#define REPORT_CMD_SIZE 30


/** @} */
