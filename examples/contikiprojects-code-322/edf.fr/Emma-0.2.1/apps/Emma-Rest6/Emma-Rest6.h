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
 * \defgroup EMMA E.M.M.A Rest6 core
 * @{
 */

/**
 * \file
 *         Header file of EMMA-Rest6 Engine
 * \author
 *         Clement DUHART <duhart.clement@gmail.com>
 */

#ifndef __HEADER_REST6__
#define __HEADER_REST6__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "contiki.h"
#include "process.h"
#include "contiki-net.h"
#include "uip-netif.h"
#include "cfs.h"
#include "dev/eeprom.h"

#include "Emma-CFS-API.h"
#include "Emma-conf.h"
#include "Emma-API.h"

#ifndef EMMA_REST6_VERSION
 #define EMMA_REST6_VERSION 0.2
#endif

#ifndef EMMA_CPU_AVR
   #define _PROGMEM
   #define sprintf_P sprintf
   #define scanf_P   scanf
   #define strcpy_P  strcpy
   #define strcmp_P  strcmp
   #define printf_P  printf
   #define strstr_P  strstr
   #define strncmp_P  strncmp
   #define sscanf_P  sscanf
   #define PSTR
#else 
   #include <avr/pgmspace.h>
   #define _PROGMEM PROGMEM
#endif

#define SEND_STRING(s, str) PSOCK_SEND(s, (uint8_t *)str, (unsigned int)strlen(str))

#if DEBUG
#define _PRINTF(...) printf_P(PSTR(__VA_ARGS__))
#else 
#define _PRINTF(...)
#endif

#define PRINT6ADDR(addr) printf(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])

#define MIN(a,b) a < b ? a : b
#define MAX(a,b) a > b ? a : b

#ifdef SEND_NOTIFY
	#if URI_OUTCOMMING_MAX_SIZE < 17
		#error "URI_OUTCOMMING_MAX_SIZE must be greater than 17 for report with HTTP notification (Sending request mode)"
	#endif
	#if BODY_NOTIFY < 18
		#error "BODY_NOTIFY must be greater than 18 for outcomming request"
	#endif
#endif 




#if URI_INCOMMING_MAX_SIZE < 17
	#error "URI_INTCOMMING_MAX_SIZE must be greater than 17 for request"
#endif

#if BUFFER_SIZE - 14 < 11
#error "BUFFER_SIZE must be greater than 25"
#endif

#if BUFFER_SIZE - 14 < 40
#warning "The buffer are lower than 40 bytes"
#endif

enum {
	NO_SPECIFY,
	GET,
	PUT,
	POST,
	DELETE,
	NOTIFY
	};

enum {
	READY,
	SENDING
	};

enum {
	NONE,
	GET_DATA,
	GET_ALL_DATA,
	GET_REPORT,
	GET_ALL_REPORT,
	GET_LOG,
	GET_ALL_LOG,
	GET_META
	};

struct Report{
	LOGGABLE();

	uip_ip6addr_t host;
	int port;
	char uri[URI_OUTCOMMING_MAX_SIZE];
	char method;
	char body[BODY_NOTIFY];
	char condition[REPORT_CMD_SIZE];
	};

struct Ressource {
	LOGGABLE();

	char name[RESSOURCE_NAME_SIZE];
	char unit[15];
	int min;
	int max;
	int (*getData) ();
	void (*setData) ();
	int value;
	};

struct RestConnection {
	struct File root;
	unsigned char method;

	unsigned char url[URI_INCOMMING_MAX_SIZE];

	struct psock Socket;
	struct pt SocketPt;

	unsigned char SocketBuff[BUFFER_SOCKET_SIZE];
	unsigned char Buff[BUFFER_SIZE];
	unsigned char Buff2[BUFFER_SIZE];
	unsigned char MiniBuff[BUFFERMINI_SIZE];

	unsigned short int timeout;
	 struct Report Subscriber;
	};

struct RestNotify {
	struct File root;
	unsigned char method;

	struct psock Socket;
	struct pt SocketPt;
	
	unsigned char Buff[BUFFER_SIZE];
	unsigned char SocketBuff[BUFFER_SIZE];
	unsigned short int timeout;
	struct Report Subscriber;
	int i,j,k;
	};

struct System {
	short int logPt;
	char LogLoop;

	char ReportLoop;
	short int ReportPt;

	int time;
	struct File LogCfsPt;
	char Buf[sizeof(struct Loggable)];
};

int Emmal_evaluate (char* str);
#endif

/*---------------------------------------------------------------------------*/

/** @} */
