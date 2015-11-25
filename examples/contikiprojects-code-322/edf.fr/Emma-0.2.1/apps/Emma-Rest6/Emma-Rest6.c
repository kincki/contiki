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
 *
 * \addtogroup EMMA
 *
 * @{
 */

/**
 * \file
 *         EMMA-Rest6 Engine
 * \author
 *         Clement DUHART <duhart.clement@gmail.com>
 *         
 * \link	 http://www.ece.fr/~duhart/pages/fr/1projets/aEMMA/EMMA-Node.pdf
 *
 * \brief
 *         EMMA-Rest6 Engine is the acronyme of Energy Monitoring & Management Agent for IPV6 RestFull HTTP Protocole.\n
 *         This engine allows publication of ressource data on the standard HTTP protocole with JSON representation.\n
 *         \n
 *         Report :\n
 *         \n
 *         This engine implements notification over HTTP by using report mechanism.\n
 *         This reports are inserted and stored in EEPROM memory and are periodically called to notify the subscriber.\n
 *         \n
 *         A log mechanism periodically store the current data of the ressource in eeprom slot log memory by using circle memory mapping.\n
 *         \n
 *         URI available :\n
 *         - GET /\n 
 *         - GET /data/{*|RessourceName}\/\n
 *         - GET /data/{*|RessourceName}\/report/{*|ReportId}\n
 *         - GET /data/{*|RessourceName}\/log/{*|LogId}/\n
 *         - POST /data/{*|RessourceName}\/report/\n
 *         - DELETE /data/{*|RessourceName}\/report/{*|IdReport}/\n
 *         - PUT /data/{*|RessourceName}\/\n
 *         - PUT /data/{*|RessourceName}\/report/{IdReport}/\n
 *
 *

 */
#include "Emma-Rest6.h"
#include "Emma-Protocole.h"

volatile struct System slash;
static struct Ressource RessourceTemp;
static char connection_flag;
RESSOURCE_LIST(ressources_list);
static struct etimer et;
MEMB(conns, struct RestConnection, INCOMMING_CONNECTION_MAX);
MEMB(noti, struct RestNotify, OUTCOMMING_CONNECTION_MAX);

char * Factory_http_header(char * buff, unsigned short int status_code){

switch (status_code){
	case 200 : 	sprintf_P(buff,header_http,status_code,"OK\0");				break;
	case 400 : 	sprintf_P(buff,header_http,status_code,"Bad Request\0");			break;
	case 403 : 	sprintf_P(buff,header_http,status_code,"Forbidden\0"); 			break;
	case 404 : 	sprintf_P(buff,header_http,status_code,"Not Found\0"); 			break;
	case 405 : 	sprintf_P(buff,header_http,status_code,"Method Not Allowed\0"); 		break;
	case 408 : 	sprintf_P(buff,header_http,status_code,"Request Timeout\0");		break;
	case 411 : 	sprintf_P(buff,header_http,status_code,"Length required\0");		break;
	case 413 : 	sprintf_P(buff,header_http,status_code,"Entity too long\0"); 		break;
	case 414 : 	sprintf_P(buff,header_http,status_code,"Uri too large\0"); 		break;
	default :		sprintf_P(buff,header_http,status_code,"Internal Error\0");					 	
}

return buff;
}
/*
#define N 2
int     getnbr(char *str)
{
  char  base[16] = "0123456789ABCDEF";
  int   i, res, pow;

// le i < 16, c'est moche, mais un strlen(base) ou un base[i] != NULL, c'est pas forcément plus optimisé 
  for (i = res = 0, pow = N - 1; i < 16 && pow > -1; i++)
    if (base[i] == *(str - pow + (N - 1)))
    {
      res *= 16;
      res += i;
      pow--;
      i = 0;
    }

  return res;
}


char extractUIP6(char * str, uip_ip6addr_t * dst)
{
   char   i = 0, r = 0;
 
   if (str[0] != '['){
		printf("No [ \n");
		return 1;
	}
	
		*str ++;

   // Extract : from str
   // TODO : Mieux faire !!!
   do {
			r = 0;
      for(i = 0; i < strlen(str); i++)
          if(str[i] == ':' || r) 
             {
                str[i] = str[i+1];
                r = 1;
             }
      }
   while(r);

printf("UIP FOUND (%c,%c): %s\n",str[31],str[32], str);

	if(str[32] != ']'){
		printf("No ] \n");
		return 1;
}

   // 1 Hex = 2 Char
   str[32]  = '\0';


  for (i = 0; i < 16; i++)
    ((char*)dst)[i] = (char)getnbr(str[i * 2]);


printf("UIP : ");
PRINT6ADDR(dst);
printf("\n-----------\"");
}
*/

char * Factory_http_balise_log(char * buff, int id,char*name, int value, int time){
sprintf_P(buff,balise_log,id,name,value,time);
return buff;
}

char * Factory_http_balise_data(char * buff, char*name,char *unit, char*timestamp,char *value){
sprintf_P(buff,balise_data,name,unit,value);
return buff;
}


char * Factory_http_balise_report_meta_1(char * buff){
sprintf_P(buff,balise_report_meta_1,URI_OUTCOMMING_MAX_SIZE);
return buff;
}

char * Factory_http_balise_report_meta_2(char * buff){
sprintf_P(buff,balise_report_meta_2);
return buff;
}

char * Factory_http_balise_report_meta_3(char * buff){
sprintf_P(buff,balise_report_meta_3,BODY_NOTIFY);
return buff;
}

char * Factory_http_balise_report_meta_4(char * buff){
sprintf_P(buff,balise_report_meta_4,REPORT_CMD_SIZE);
return buff;
}

char * Factory_http_balise_report_0(char * buff, char* host2){
sprintf_P(buff,balise_report_0,host2);
return buff;
}

char * Factory_http_balise_report_1(char * buff, char*uri, int id){
sprintf_P(buff,balise_report_1,uri,id);
return buff;
}

char * Factory_http_balise_report_2(char * buff,int port, char * method, char * body){
sprintf_P(buff,balise_report_2,port,method,body);
return buff;
}

char * Factory_http_balise_report_3(char * buff, char * condition){
sprintf_P(buff,balise_report_3, condition);
return buff;
}

char * Factory_http_balise_meta_1(char * buff,char *name){
sprintf_P(buff,balise_meta_1,name);
return buff;
}

char * Factory_http_balise_meta_2(char * buff, int min, int max, char readonly){
sprintf_P(buff,balise_meta_2, min, max, readonly);
return buff;
}

void nextUrlFoled(char * url, char * buff){
	char * temp;
	temp = strchr((char*) url,'/');
	sprintf(buff, "%s",(temp+1));
	sprintf(url,buff);
}

char* extractBodyValue(char * str1, const char * str2, char * dst){
	static int l=-1;
	static char * temp;

	temp  = strstr_P(str1,str2);
	if(temp != NULL){
		temp =strstr(temp,":\"");
		temp += 2;
		sprintf(dst,"%s",temp);
		l = 1;
		while((dst[l] != '\0' && dst[l] != '"') || dst[l-1] == '\\') l++;
		dst[l] = 0;

		return dst;
	}

return NULL;
}
void SubscriberFactory(struct Report * dst, char * str, char * buf){
	static unsigned short int Tint[16];

	if(extractBodyValue(str, emmal_attribute_host, buf) != NULL){
/*_PRINTF("Before");
     extractUIP6(buf, &(dst->host));
_PRINTF("After");
/*/			sscanf(buf,"[%02hx%02hx:%02hx%02hx:%02hx%02hx:%02hx%02hx:%02hx%02hx:%02hx%02hx:%02hx%02hx:%02hx%02hx]",
			&Tint[0],&Tint[1],&Tint[2],&Tint[3],
			&Tint[4],&Tint[5],&Tint[6],&Tint[7],
			&Tint[8],&Tint[9],&Tint[10],&Tint[11],
			&Tint[12],&Tint[13],&Tint[14],&Tint[15]);

			// Conversion two bytes to one byte
			((char *)(&dst->host))[0] = (char)Tint[0];
			((char *)(&dst->host))[1] = (char)Tint[1];
			((char *)(&dst->host))[2] = (char)Tint[2];
			((char *)(&dst->host))[3] = (char)Tint[3];
			((char *)(&dst->host))[4] = (char)Tint[4];
			((char *)(&dst->host))[5] = (char)Tint[5];
			((char *)(&dst->host))[6] = (char)Tint[6];
			((char *)(&dst->host))[7] = (char)Tint[7];
			((char *)(&dst->host))[8] = (char)Tint[8];
			((char *)(&dst->host))[9] = (char)Tint[9];
			((char *)(&dst->host))[10] = (char)Tint[10];
			((char *)(&dst->host))[11] = (char)Tint[11];
			((char *)(&dst->host))[12] = (char)Tint[12];
			((char *)(&dst->host))[13] = (char)Tint[13];
			((char *)(&dst->host))[14] = (char)Tint[14];
			((char *)(&dst->host))[15] = (char)Tint[15];
		}
		if(extractBodyValue(str, emmal_attribute_port, buf) != NULL)
				dst->port = atoi(buf);

		if(extractBodyValue(str, emmal_attribute_uri, buf) != NULL)
				sprintf(dst->uri,"%s",buf);

		if(extractBodyValue(str, emmal_attribute_method, buf) != NULL){
			if(strncmp_P(buf, get,3) == 0)
				dst->method = GET;
			else if(strncmp_P(buf, put,3) == 0)
				dst->method = PUT;
			else if(strncmp_P(buf, post,4) == 0)
				dst->method = POST;
			else if(strncmp_P(buf, delete,6) == 0)
				dst->method = DELETE;
			}

		if(extractBodyValue(str, emmal_attribute_body, buf) != NULL){
				sprintf((char*)dst->body,"%s",buf);
		}

		if(extractBodyValue(str, emmal_attribute_id, buf) != NULL)
				dst->id = atoi(buf);

		if(extractBodyValue(str, emmal_attribute_condition, buf) != NULL)
				sprintf((char*)dst->condition,"%s",buf);

}

void lookup_method(struct RestConnection * conn){
  if(strncmp_P((char*)conn->SocketBuff,get, 4) == 0)				conn->method = GET;
	else if(strncmp_P((char*)conn->SocketBuff,put, 4) == 0)			conn->method = PUT;
	else if(strncmp_P((char*)conn->SocketBuff, post, 6) == 0)		conn->method = POST;
	else if(strncmp_P((char*)conn->SocketBuff, delete, 7) == 0)		conn->method = DELETE;
}

static PT_THREAD(handle_connection(struct RestConnection * conn)){
	static struct Ressource * PtRessource = NULL;
	static char * temp;
	static int i=-1,j=-1,k=-1,DataFlag=0;

	PT_BEGIN(&conn->SocketPt);

	//  Starting socket, this thread will block on this point while the PSOCK execution is running
	PSOCK_BEGIN(&conn->Socket);

	// Abort the connection if the timeout expired
	if(conn->timeout++ > TIMEOUT){
		_PRINTF("Input HTTP request : timeout...\n");
		uip_abort();
		PSOCK_CLOSE_EXIT(&conn->Socket);
	}

	_PRINTF("Input HTTP request : process...\n");

	// Looking for method involve
 	PSOCK_READTO(&conn->Socket, ' ');
	lookup_method(conn);

	// Looking for url involve
	PSOCK_READTO(&conn->Socket, ' ');
	sprintf((char*)conn->url, (char*)conn->SocketBuff);
	nextUrlFoled((char*) conn->url, (char*) conn->Buff);

	/**************************************************************************
	* GET METHOD :
	*		{
	*		"name":"temperature",
	*		"uri":"http://[cccc::3332:3334:2000:0000]/data/temperature/"}
	*		}
	*
	* GET /
	*	
	* GET /data/\*
	* GET /data/{RESSOURCE_name}/
	* 	{
	*		"unit":"lux",
	*		"InstatenousRate":"105"
	*		}
	* GET /data/{RESSOURCE_name}/report/
	*		{
	* 	"uri":"http://[3ffe:0501:ffff:0100:0206:98ff:fe00:0231]/notify.php",
	* 	"name":"brightness",
	* 	"period":"-1",
	* 	}
	* GET /data/\*\/report
	* GET /data/{*|RESSOURCE_name}/log/
	*
	**************************************************************************/
	if(conn->method == GET){

				DataFlag = NONE;
				j = 0;

				// GET /data/
				if(strncmp_P((char*)conn->url,url_data,5) == 0){
					nextUrlFoled((char*) conn->url, (char*) conn->Buff);
 		
				// Check if the request is about all the sensor
				if(strncmp_P((char*)conn->url,url_all,1) == 0)
					DataFlag |= (1 << GET_ALL_DATA); 

				// Looking for data method
				sprintf((char*)conn->Buff2,(char*)conn->url);
				nextUrlFoled((char*) conn->url, (char*) conn->Buff);

				if(strncmp_P((char*)conn->url,url_log,3) == 0)
					DataFlag |= (1 << GET_LOG);
				else if(strncmp_P((char*)conn->url,url_meta,4) == 0)
					DataFlag |= (1 << GET_META);
				else DataFlag |= (1 << GET_DATA);

				sprintf((char*)conn->url,(char*)conn->Buff2);

				// Send header
				if(DataFlag != NONE)
					SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 200));

				// Browse the sensor list
				if (DataFlag & (1 << GET_ALL_DATA))
					SEND_STRING(&conn->Socket,"[");

				for(PtRessource = list_head(ressources_list);PtRessource != NULL; PtRessource = PtRessource->next) {

						// Send sensor data if there is necessary
						if(strncmp(PtRessource->name,(char*) conn->url, strlen(PtRessource->name)) == 0 || DataFlag & (1 << GET_ALL_DATA)){

							// GET /data/{RESSOURCE_name}/
							if(DataFlag & (1 << GET_DATA)){
								#if SEND_NOTIFY
								sprintf((char*)conn->MiniBuff,"%d",PtRessource->value); 
								#else
								sprintf((char*)conn->MiniBuff,"%d",PtRessource->getData()); 
								#endif
								Factory_http_balise_data((char*)conn->Buff,PtRessource->name, PtRessource->unit,"0",(char*)conn->MiniBuff);
								SEND_STRING(&conn->Socket,(char*)conn->Buff);
								

								if(PtRessource->next != NULL && (DataFlag & (1 << GET_ALL_DATA)))
									SEND_STRING(&conn->Socket,",");

								}

							// GET /data/{RESSOURCE_name}/meta/
							else if(DataFlag & (1 << GET_META)){
								Factory_http_balise_meta_1((char*)conn->Buff,PtRessource->name);
								SEND_STRING(&conn->Socket,(char*)conn->Buff);

								Factory_http_balise_meta_2((char*)conn->Buff, PtRessource->min, PtRessource->max, (PtRessource->setData==NULL)?1:0);
								SEND_STRING(&conn->Socket,(char*)conn->Buff);

								if(PtRessource->next != NULL && (DataFlag & (1 << GET_ALL_DATA)))
									SEND_STRING(&conn->Socket,",");
							}

							// GET /data/{RESSOURCE_name}/log/
							#if SEND_NOTIFY
							else if (DataFlag & (1 << GET_LOG)){
								_PRINTF("**LOG**");
								nextUrlFoled((char*) conn->url, (char*) conn->Buff);
								nextUrlFoled((char*) conn->url, (char*) conn->Buff);

								if(((char*)conn->url)[0] != ' ' && ((char*)conn->url)[0] != '*'){
									if(LOG_OPEN(&RessourceTemp, atoi((char*) conn->url), &conn->root) == 0){
										Factory_http_balise_log((char*)conn->Buff, RessourceTemp.id, RessourceTemp.name, RessourceTemp.value,RessourceTemp.time);
										SEND_STRING(&conn->Socket,(char*)conn->Buff);
									}
								break;
								}

								// All log for all or a specify ressource
								i = 0;
								if(LOG_READ(&RessourceTemp, CFS_BEGIN, &conn->root) == 0){
									
									if( strcmp(PtRessource->name, RessourceTemp.name) == 0 || DataFlag & (1 << GET_ALL_DATA)){
										Factory_http_balise_log((char*)conn->Buff, RessourceTemp.id, RessourceTemp.name, RessourceTemp.value,RessourceTemp.time);
										SEND_STRING(&conn->Socket,(char*)conn->Buff);
										i++;
										}

									if(LOG_READ(&RessourceTemp, CFS_NEXT, &conn->root) == 0)
									do {
										if(i > 0)
											SEND_STRING(&conn->Socket,",");

										if( strcmp(PtRessource->name, RessourceTemp.name) == 0 || DataFlag & (1 << GET_ALL_DATA)){
											Factory_http_balise_log((char*)conn->Buff, RessourceTemp.id, RessourceTemp.name, RessourceTemp.value,RessourceTemp.time);
											SEND_STRING(&conn->Socket,(char*)conn->Buff);
											i++;
											}
									}
									while(LOG_READ(&RessourceTemp, CFS_NEXT, &conn->root) == 0);

								break;
								}
							}
					#endif
							}	
					}	

				if (DataFlag & (1 << GET_ALL_DATA))
					SEND_STRING(&conn->Socket,"]");		
				}

			//GET /report/
			else if(strncmp_P((char*)conn->url,report,6) == 0){
				nextUrlFoled((char*) conn->url, (char*) conn->Buff);
				DataFlag = GET_ALL_DATA;

				if(strncmp_P((char*)conn->url,url_all,1) == 0)
					DataFlag |= (1 << GET_ALL_DATA); 

				else if(strncmp_P((char*)conn->url,url_meta,4) == 0)
					DataFlag |= (1 << GET_META); 

					// Send header HTTP
				if(DataFlag != NONE)
					SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 200));
				else SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 404));

			  /* Reading and sending subscriber(s) */
				_PRINTF("**REPORT**");
				if (DataFlag & (1 << GET_ALL_DATA))
					SEND_STRING(&conn->Socket,"[");

					if(DataFlag & (1 << GET_META)){
						Factory_http_balise_report_meta_1((char*)conn->Buff);
						SEND_STRING(&conn->Socket, (char*)conn->Buff);	

						Factory_http_balise_report_meta_2((char*)conn->Buff);
						SEND_STRING(&conn->Socket, (char*)conn->Buff);	

						Factory_http_balise_report_meta_3((char*)conn->Buff);
						SEND_STRING(&conn->Socket, (char*)conn->Buff);	

						Factory_http_balise_report_meta_4((char*)conn->Buff);
						SEND_STRING(&conn->Socket, (char*)conn->Buff);	
					}

					else if(REPORT_READ(&conn->Subscriber, CFS_BEGIN) == 0){
						do {					

							if (DataFlag & (1 << GET_ALL_DATA)){

								// Make the URI for get updated data
								// WARNING : Memory over space => Limited by conn->buff2[MAX_BUFFER_SIZE]
								sprintf_P(	(char*)conn->Buff2,host, 	
																		((u8_t *)(&conn->Subscriber.host))[0],((u8_t *)(&conn->Subscriber.host))[1], 	
																		((u8_t *)(&conn->Subscriber.host))[2],((u8_t *)(&conn->Subscriber.host))[3], 	
																		((u8_t *)(&conn->Subscriber.host))[4],((u8_t *)(&conn->Subscriber.host))[5], 	
																		((u8_t *)(&conn->Subscriber.host))[6],((u8_t *)(&conn->Subscriber.host))[7],
																		((u8_t *)(&conn->Subscriber.host))[8],((u8_t *)(&conn->Subscriber.host))[9],
																		((u8_t *)(&conn->Subscriber.host))[10],((u8_t *)(&conn->Subscriber.host))[11],
																		((u8_t *)(&conn->Subscriber.host))[12],((u8_t *)(&conn->Subscriber.host))[13],
																		((u8_t *)(&conn->Subscriber.host))[14],((u8_t *)(&conn->Subscriber.host))[15]);				
												
								// Make the response	
								// WARNING : Memory over space => Limited by conn->buff[MAX_BUFFER_SIZE]
								Factory_http_balise_report_0((char*)conn->Buff,(char*)conn->Buff2);
								SEND_STRING(&conn->Socket, (char*)conn->Buff);	

								Factory_http_balise_report_1((char*)conn->Buff,conn->Subscriber.uri, conn->Subscriber.id);
								SEND_STRING(&conn->Socket, (char*)conn->Buff);	

								if(conn->Subscriber.method == GET)
									sprintf_P((char*)conn->Buff2,get);
								else if(conn->Subscriber.method == PUT)
									sprintf_P((char*)conn->Buff2,put);
								else if(conn->Subscriber.method == POST)
									sprintf_P((char*)conn->Buff2,post);
								else if(conn->Subscriber.method == DELETE)
									sprintf_P((char*)conn->Buff2,delete);
								else sprintf((char*)conn->Buff2,no_define);
			
								// Remove the ' ' of the program constant
								conn->Buff2[strlen((char*)conn->Buff2) - 1 ] = '\0';

								Factory_http_balise_report_2((char*)conn->Buff,conn->Subscriber.port,(char*)conn->Buff2,conn->Subscriber.body);
								SEND_STRING(&conn->Socket, (char*)conn->Buff);	

								Factory_http_balise_report_3((char*)conn->Buff,conn->Subscriber.condition);
								SEND_STRING(&conn->Socket, (char*)conn->Buff);	
								_PRINTF("Report sended for the ressource\n");
							}
						}
						while(REPORT_READ(&conn->Subscriber, CFS_NEXT) == 0);
					} 
				if (DataFlag & (1 << GET_ALL_DATA))
					SEND_STRING(&conn->Socket,"]");
			}
			// GET /
			// Return the access data link from uri
			else if(conn->url[0] == ' '){
				SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 200));

				SEND_STRING(&conn->Socket,"[\n");
				// Browse the sensor list
				for(PtRessource = list_head(ressources_list);PtRessource != NULL; PtRessource = PtRessource->next) {

				// Looking for host address
				for(i = 0; i < UIP_CONF_NETIF_MAX_ADDRESSES; i ++)
						if(uip_netif_physical_if.addresses[i].state != NOT_USED && uip_netif_physical_if.addresses[i].state != TENTATIVE){

						sprintf_P(	(char*)conn->Buff2,url_ressource, 	
											((u8_t *)(&uip_netif_physical_if.addresses[i].ipaddr))[0],((u8_t *)(&uip_netif_physical_if.addresses[i].ipaddr))[1],		
										  ((u8_t *)(&uip_netif_physical_if.addresses[i].ipaddr))[8],((u8_t *)(&uip_netif_physical_if.addresses[i].ipaddr))[9],
											((u8_t *)(&uip_netif_physical_if.addresses[i].ipaddr))[10],((u8_t *)(&uip_netif_physical_if.addresses[i].ipaddr))[11],
											((u8_t *)(&uip_netif_physical_if.addresses[i].ipaddr))[12],((u8_t *)(&uip_netif_physical_if.addresses[i].ipaddr))[13],
											((u8_t *)(&uip_netif_physical_if.addresses[i].ipaddr))[14],((u8_t *)(&uip_netif_physical_if.addresses[i].ipaddr))[15],										
											PtRessource->name);				
						}
					
					sprintf_P((char*)conn->Buff,racine_uri,(char*)PtRessource->name,(char*)conn->Buff2);
					SEND_STRING(&conn->Socket,(char*)conn->Buff);

					if(PtRessource->next != NULL)
						SEND_STRING(&conn->Socket,",");
				}
			SEND_STRING(&conn->Socket,"]\n");
			}
		else SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 404));
		}
	/**************************************************************************
	* PUT METHOD :
	* PUT /data/{RESSOURCE_name}/
	*
	* Body example : 
	* {
	* "value":"100"
	* }
	**************************************************************************/
	else if (conn->method == PUT) {
		_PRINTF("[HTTP] Put method\n");
		DataFlag = NONE;

		_PRINTF("[HTTP] Read URI ...\n");
		if(strncmp_P((char*)conn->url,report,6) == 0)
			DataFlag |= (1 << GET_REPORT);
		else if(strncmp_P((char*)conn->url,url_data,4) == 0) 
			DataFlag |= (1 << GET_DATA);

		// Check if the request is about all the sensor
		nextUrlFoled((char*) conn->url, (char*) conn->Buff);
		if(strncmp_P((char*)conn->url,url_all,1) == 0)
			DataFlag |= (1 << GET_ALL_DATA); 

		if(DataFlag == NONE)
			SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 404));

		else	{

			/* Store the name of the ressource*/
			i =0;
			while(((char*)conn->url)[i] != '/' && ((char*)conn->url)[i] != '\0'){
				conn->Buff2[i] = conn->url[i];
				i++;
				}
			conn->Buff2[i] = 0;

			_PRINTF("[HTTP] Read http header ...\n");
			/* Store the size of content*/
			do {
				PSOCK_READTO(&conn->Socket, '\n');

				if(strncmp_P((char*) conn->SocketBuff, content_length,15) == 0){
						temp = strchr((char*) conn->SocketBuff, ':');
						temp = strchr((char*) conn->SocketBuff, ' ');
						i=0;
						while(temp[i] != '\r' && temp[i] != 0 && temp[i] != ' ' ) i++;
						temp[i-1] = 0;

						// Save the content length
						j = atoi(temp);
				}

				// End of the header
				if(((char*)conn->SocketBuff)[0] == '\r')
					break;
			}
			while(1);

			// **ERROR** If no content length found, we leave the connection
			if(j == -1)			{
				SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 411));
				uip_abort();
				PSOCK_CLOSE_EXIT(&conn->Socket);
			}
			i = 0;

			// Looking for the subsection (Report)
			nextUrlFoled((char*) conn->url, (char*) conn->Buff);

			// Looking for ressource access
			for(PtRessource = list_head(ressources_list);PtRessource != NULL; PtRessource = PtRessource->next) 

				// PUT on data or the data
				if(strcmp(PtRessource->name,(char*)conn->Buff2) == 0 || strncmp(url_all,(char*)conn->Buff2,1) == 0){

					// Report update
					if (DataFlag & (1 << GET_REPORT)){
						_PRINTF("[HTTP] Update report\n");
						// Get the ID & load the report
						nextUrlFoled((char*) conn->url, (char*) conn->Buff);

						REPORT_OPEN(&conn->Subscriber, atoi((char*) conn->url));
					}

					/* Parse of HTTP Body 
					 * Note : the body muse be terminated by \n for leaving loop			
					*/
					_PRINTF("[HTTP] read http body ...\n");

					do {
					// Read line of content
					PSOCK_READTO(&conn->Socket,'\n');

					for (k =0; k < (PSOCK_DATALEN(&conn->Socket)/BUFFER_SIZE)+1; k++){
							memcpy(conn->Buff,conn->SocketBuff + ((k)*BUFFER_SIZE),PSOCK_DATALEN(&conn->Socket) > BUFFER_SIZE ? BUFFER_SIZE-2: PSOCK_DATALEN(&conn->Socket));
							conn->Buff[(PSOCK_DATALEN(&conn->Socket) > BUFFER_SIZE ? BUFFER_SIZE-2: PSOCK_DATALEN(&conn->Socket))-1] = 0;
							i += (PSOCK_DATALEN(&conn->Socket) > BUFFER_SIZE ? BUFFER_SIZE-2: PSOCK_DATALEN(&conn->Socket));

							// PUT /data/{sensor}/report/
							if (DataFlag & (1 << GET_REPORT))
								SubscriberFactory(&conn->Subscriber, (char*) conn->Buff, (char*)conn->Buff2);

							// PUT /data/{sensor}/
							else {
								_PRINTF("[HTTP] Update ressource request found...\n");

								// Check data value
								if(extractBodyValue((char *)conn->Buff, emmal_attribute_value, (char*)conn->Buff2) != NULL){
									if(PtRessource->setData != NULL){
										// Set the new value if it's a writable data
										_PRINTF("[HTTP] Updating ressource data...\n");
										PtRessource->setData(Emmal_evaluate((char*)conn->Buff2));
										SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 200));
										}
									else SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 403));
									i = j;
									break;
									}
								} // GET /data/{sensor}/
							}
						}
					while(i+1 < j);

					break;
					}
		}	

	if (DataFlag & (1 << GET_REPORT)){
		if(REPORT_UPDATE(&conn->Subscriber, (char*)conn->Buff2) == 0)		
			SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 200));
		else	SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 500));
		}
			
	}
	/**************************************************************************
	* POST METHOD :
	* POST /data/{RESSOURCE_name}/report/
	*
	* Body example : 
	* {
	* "host":"[cccc::3332:3334:2000:0]",
	* "uri":"notify/",
	* "name":"temperature",
	* "period":"1000",
	* }
	**************************************************************************/
	else if (conn->method == POST) {

				if(strncmp_P((char*) conn->url,report,6) != 0)
					SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 404));

				else	{
				nextUrlFoled((char*) conn->url, (char*) conn->Buff);

				uip_ip6addr(&conn->Subscriber.host, 0x0000, 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000);
				conn->Subscriber.port = 80;
				conn->Subscriber.id = -1;
				conn->Subscriber.uri[0] = '\0';
				conn->Subscriber.method = 1;
				sprintf(conn->Subscriber.body,"\0");

				conn->Subscriber.time 	= 0;
				conn->Subscriber.condition[0] 	= '\0';
				// BODY PARSING

			/* Parse of HTTP header*/
			do {
				PSOCK_READTO(&conn->Socket, '\n');

				if(strncmp_P((char*) conn->SocketBuff, content_length,15) == 0){
						temp = strchr((char*) conn->SocketBuff, ':');
						temp = strchr((char*) conn->SocketBuff, ' ');
						i=0;
						while(temp[i] != '\r' && temp[i] != 0 && temp[i] != ' ' ) i++;
						temp[i-1] = 0;

						// Save the content length
						j = atoi(temp);
				}

				// End of the header
				if(((char*)conn->SocketBuff)[0] == '\r')
					break;
			}
			while(1);

			// **ERROR** If no content length found, we leave the connection
			if(j == -1 || j == 0){
				SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 411));
				uip_abort();
				PSOCK_CLOSE_EXIT(&conn->Socket);
			}
			i = 0;

			/* Parse of HTTP Body 
			 * Note : the body muse be terminated by \n for leaving loop			
			*/
			do {
			// Read line of content
			PSOCK_READTO(&conn->Socket,'\n');

			for (k =0; k < (PSOCK_DATALEN(&conn->Socket)/BUFFER_SIZE)+1; k++){
					memcpy(conn->Buff,conn->SocketBuff + ((k)*BUFFER_SIZE),PSOCK_DATALEN(&conn->Socket) > BUFFER_SIZE ? BUFFER_SIZE-2: PSOCK_DATALEN(&conn->Socket));
					conn->Buff[(PSOCK_DATALEN(&conn->Socket) > BUFFER_SIZE ? BUFFER_SIZE-2: PSOCK_DATALEN(&conn->Socket))-1] = 0;
					i += (PSOCK_DATALEN(&conn->Socket) > BUFFER_SIZE ? BUFFER_SIZE-2: PSOCK_DATALEN(&conn->Socket));

					SubscriberFactory(&conn->Subscriber, (char*) conn->Buff, (char*)conn->Buff2);
					}
		}
			while(i+1 < j);

				if(REPORT_NEW(&conn->Subscriber, (char*)conn->Buff2) == 0)		
							SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 200));
				else	SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 500));
			
			}
		}
	/**************************************************************************
	* DELETE METHOD :
	* POST /data/\*\/report/
	*
	**************************************************************************/
	else if(conn->method == DELETE){
		if(strncmp_P((char*)conn->url,report,6) == 0){

					// Jump to the {IdReport | *}
					nextUrlFoled((char*) conn->url, (char*) conn->Buff);
					if(strncmp_P((char*)conn->url,url_all,1) == 0){
						if(REPORT_FORMAT (&conn->Subscriber,(char*)conn->Buff) == 0)
							SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 200));
						else SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 500));
					}
					else if(atoi((char*)conn->url) != 0) {
						if(REPORT_DELETE (&conn->Subscriber,(char*)conn->Buff, atoi((char*) conn->url)) == 0)
							SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 200));
						else SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 500));
					}				
		}
		// Bad URL structure
		else 	SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 404));
	}
	else SEND_STRING(&conn->Socket,Factory_http_header((char*)conn->Buff, 405));
	
	_PRINTF("Input HTTP request : Terminated [SUCCESS]");

	PSOCK_CLOSE_EXIT(&conn->Socket);
	PSOCK_END(&conn->Socket);

	PT_END(&conn->SocketPt);
}
/*---------------------------------------------------------------------------*/
static PT_THREAD(notify_connection(struct RestNotify * conn))
{
	static char * temp;
	static int i;

	PT_BEGIN(&conn->SocketPt);
	PSOCK_BEGIN(&conn->Socket);
	_PRINTF("[NOTIFY] Sending request...\n");
	// Abort the connection if the timeout expired
	if(conn->timeout++ > TIMEOUT){
		uip_abort();
		_PRINTF("[NOTIFY] Timeout during sending...\n");
		PSOCK_CLOSE_EXIT(&conn->Socket);
	}

	//  Starting socket, this thread will block on this point while the PSOCK execution is running
	sprintf_P((char*)conn->Buff,header_notify,&conn->Subscriber.method, conn->Subscriber.uri);
	SEND_STRING(&conn->Socket,(char*)conn->Buff);
	// Send body

	// Extract back slash if necessary
	for(i=0; i<strlen(conn->Subscriber.body); i++)
		if(conn->Subscriber.body[i] == '\\'){
			for(; i<strlen(conn->Subscriber.body)-1; i++)
				conn->Subscriber.body[i] = conn->Subscriber.body[i+1];
			i = 0;
		}
//printf("BODY : %s\n",conn->Subscriber.body);
			
	SEND_STRING(&conn->Socket,conn->Subscriber.body);

	/* Parse of HTTP header*/
	_PRINTF("[NOTIFY] Response header reading ...\n");
	do {
		PSOCK_READTO(&conn->Socket, '\n');

				if(strncmp_P((char*) conn->SocketBuff, content_length,15) == 0){
						temp = strchr((char*) conn->SocketBuff, ':');
						temp = strchr((char*) conn->SocketBuff, ' ');
						conn->i=0;
						while(temp[conn->i] != '\r' && temp[conn->i] != 0 && temp[conn->i] != ' ' ) conn->i++;
						temp[conn->i-1] = 0;

						// Save the content length
						conn->j = atoi(temp);
				}

				// End of the header
				if(((char*)conn->SocketBuff)[0] == '\r')
					break;
			}
	
			while(1);
	/* END of HTTP header */

		_PRINTF("[NOTIFY] Response body reading ...\n");
			/* Parse of HTTP Body 
			 * Note : the body muse be terminated by \n for leaving loop			
			*/
			do {
			// Read line of content
			PSOCK_READTO(&conn->Socket,'\n');

			for (conn->k =0; conn->k < (PSOCK_DATALEN(&conn->Socket)/BUFFER_SIZE)+1; conn->k++){
					memcpy(conn->Buff,conn->SocketBuff + ((conn->k)*BUFFER_SIZE),PSOCK_DATALEN(&conn->Socket) > BUFFER_SIZE ? BUFFER_SIZE-2: PSOCK_DATALEN(&conn->Socket));
					conn->Buff[(PSOCK_DATALEN(&conn->Socket) > BUFFER_SIZE ? BUFFER_SIZE-2: PSOCK_DATALEN(&conn->Socket))-1] = 0;
					conn->i += (PSOCK_DATALEN(&conn->Socket) > BUFFER_SIZE ? BUFFER_SIZE-2: PSOCK_DATALEN(&conn->Socket));

				//	printf("R:%s\n",conn->Buff);
					}
		}
			while(conn->i+1 < conn->j);
	_PRINTF("[NOTIFY] Response reading [SUCCESS]\n");
	/* END HTTP Body*/

	// Close the connection
	uip_close();

	PSOCK_CLOSE_EXIT(&conn->Socket);
	PSOCK_END(&conn->Socket);
	PT_END(&conn->SocketPt);
}
/*---------------------------------------------------------------------------*/
void app_rest(void * data){
  int i=0;
  struct RestConnection * conn = (struct RestConnection *) data;
_PRINTF("**AppRest handle**\n");
if(uip_connected()){
	_PRINTF("New connexion : ");

	// If data are associated to the connection, so this connection is already instanciate by this node : !Notify request
	if(data != NULL)
		if (((struct RestNotify*)data)->method == NOTIFY){
			PT_INIT(&((struct RestNotify *)data)->SocketPt);
			PSOCK_INIT(&((struct RestNotify*)data)->Socket, ((struct RestNotify*)data)->SocketBuff, sizeof(((struct RestNotify*)data)->SocketBuff) - 1);
			_PRINTF("[NOTIFY] Responce processing...\n");

			notify_connection(((struct RestNotify*)data));
		return;
	}

	_PRINTF("HTTP Request\n");
	connection_flag = 1;

	// No data associated, so we create data connection : !Incomming request
	conn = (struct RestConnection *) memb_alloc(&conns);
	if (conn == NULL){
		_PRINTF("Not able to allocate a new connection\n");
		uip_abort();
		return;
	}

	conn->method = NO_SPECIFY;
	conn->timeout = 0;
	conn->url[0] = '\0';
	while(i < BUFFER_SIZE)	conn->SocketBuff[i++] = '\0';
	tcp_markconn(uip_conn, conn);

	PSOCK_INIT(&conn->Socket, conn->SocketBuff, sizeof(conn->SocketBuff) - 1);
	PT_INIT(&conn->SocketPt);

	handle_connection(data);
}
else if(uip_aborted()){
		_PRINTF("aborted\n");
		if(data != NULL)		memb_free(&noti, data);
		if(conn != NULL)		memb_free(&conns, conn);
		connection_flag = 0;
		return;
    }
else if(uip_closed()){
		_PRINTF("closed\n");
		if(data != NULL)		memb_free(&noti, data);
		if(conn != NULL)		memb_free(&conns, conn);
		connection_flag = 0;
		return;
    }
else if (uip_timedout()){
		_PRINTF("Timeout\n");
		if(data != NULL)		memb_free(&noti, data);
		if(conn != NULL)		memb_free(&conns, conn);
		connection_flag = 0;
		return;
	}

else if(conn != NULL){

	// Process for NOTIFY feedback	
	if( ((struct RestNotify*)data)->method == NOTIFY ){
		_PRINTF("UIP Poll : Notify feedback\n");
  
	  if(uip_poll()) {
      ((struct RestNotify*)data)->timeout++;

       if(((struct RestNotify*)data)->timeout >= 20) {
				uip_abort();
				if(data != NULL)		memb_free(&noti, data);
				if(conn != NULL)		memb_free(&conns, conn);
				_PRINTF("[NOTIFY] Timeout ...\n");
				return;		    
				}
		  }
		else   ((struct RestNotify*)data)->timeout = 0;
		notify_connection(((struct RestNotify*)conn));
	}

	// Process for new income connection
	else {
		_PRINTF("UIP Poll : Incomming HTTP Request\n");

    if(uip_poll()) {
      conn->timeout++;

       if(conn->timeout >= TIMEOUT) {
				_PRINTF("Timeout connexion ...\n");
				uip_abort();
				if(data != NULL)		memb_free(&noti, data);
				if(conn != NULL)		memb_free(&conns, conn);
				return ;
		    }
		  }
		else   conn->timeout = 0;
		handle_connection(data);

	}
}
else {
	_PRINTF("UIP abord \n"); 
	uip_abort();
	if(data != NULL)		memb_free(&noti, data);
	if(conn != NULL)		memb_free(&conns, conn);
	return;
	}
}
/*---------------------------------------------------------------------------*/


static 
void thread_publish_func(void)
{
	static struct uip_conn *TCPconn;
	static struct RestNotify * conn;
	static struct Ressource * PtRessource;

	static int j=0,allocUsed;

	_PRINTF("Process ...\n");
	
	// Increment count loop
	slash.LogLoop++;

	// Update sensor value and log the new value
	for(PtRessource = list_head(ressources_list);PtRessource != NULL; PtRessource = PtRessource->next) 
		if(PtRessource->getData != NULL){

			// Get the new value of the sensor
			_PRINTF("[UPDATE] Ressource value\n");
			PtRessource->value = PtRessource->getData();
			PtRessource->time = slash.time;
			
			// If time to be log
			if(slash.LogLoop * PROCESS_TIME > PROCESS_TIME_LOG){

				if(PtRessource->next == NULL)
					slash.LogLoop = 0;
				_PRINTF("[LOG] Ressource value\n");

				LOG_DELETE(&RessourceTemp,(char*)slash.Buf,(struct File * )&slash.LogCfsPt,slash.logPt);
				LOG_NEW(PtRessource,(char*)slash.Buf,(struct File * )&slash.LogCfsPt);

				// Increment the pointeur on the next slot for log
				slash.logPt ++;
				slash.logPt = slash.logPt % CFS_LOG_COUNT;
				}
			}
			
		else PtRessource->value = -1;

	// Check for report
	if(slash.ReportLoop++ * PROCESS_TIME > PROCESS_TIME_REPORT){
		slash.ReportLoop = 0;

		j = 0;
		/* If there are not other entries in the file, we break the loop of reading*/
		conn = (struct RestNotify *) memb_alloc(&noti);
		if (conn == NULL){
			_PRINTF("Not enough memory for notify connexion\n");
			etimer_set(&et, CLOCK_SECOND * PROCESS_TIME);
			return;
			}
		else {
			allocUsed = 0;
			_PRINTF("[NOTIFY] : memory allocation [SUCCESS]\n");
		}

		if(REPORT_READ(&conn->Subscriber, CFS_BEGIN) == 0) {
		do {					
//printf("%d|%d\n",slash.ReportPt,j);
					/* Looking for sensors value if a notification is necessary*/
				if(conn->Subscriber.state == USED && j++ >= slash.ReportPt)
						// Check condition of subscribe
						if(	Emmal_evaluate (conn->Subscriber.condition)){

								_PRINTF("[NOTIFY] Connecting ...\n");
								// Init the connection parameter
								conn->Subscriber.time = 0;
								conn->timeout = 0;
								conn->method = NOTIFY;
								memcpy(&conn->Subscriber, &conn->Subscriber, sizeof(struct Report));

								// try to establish TCP connection
								TCPconn = tcp_connect(&conn->Subscriber.host, HTONS(conn->Subscriber.port),conn);  
								if(TCPconn == NULL) {
									_PRINTF("[NOTIFY] Could not open TCP connection\n");
									uip_abort();
								}
								else {		
									_PRINTF("[NOTIFY] Notify scheduled...\n");
									allocUsed = 1;
									// Set pointer to next notify report
									slash.ReportPt++;
									if(slash.ReportPt >= CFS_REPORT_COUNT)
										slash.ReportPt = 0;

										/* If there are not other entries in the file, we break the loop of reading*/
									conn = (struct RestNotify *) memb_alloc(&noti);
									if (conn == NULL){
											_PRINTF("[NOTIFY] Not enough memory : \n");
											break;
										}
									else {
										allocUsed = 0;
										_PRINTF("[NOTIFY] : memory allocation [SUCCESS]\n");
										}
									}	
						// Break the loop sensor	
						break;
						}
		if(conn == NULL) break;
		}
		while(REPORT_READ(&conn->Subscriber, CFS_NEXT) == 0);
		}
		else {
			_PRINTF("Notify : Free memory [SUCCESS]\n");
			memb_free(&noti, conn);
		}

	// If no other report must be send and a allocation has been opened
	if(allocUsed == 0) {
			slash.ReportPt = 0;

			_PRINTF("Notify : Free memory [SUCCESS]\n");
			memb_free(&noti, conn);
		}

	} // END PROCESS REPORT

	_PRINTF("End process : [SUCCESS]\n");
	etimer_set(&et, CLOCK_SECOND * PROCESS_TIME);
}

/*---------------------------------------------------------------------------*/
void Emma_init(){
	static char Buf[sizeof(struct Loggable)];
	static struct File LogCfsPt;

	
	_PRINTF("[Emma init] :\n");
	slash.LogLoop = 0;
	slash.logPt = 1;
	slash.ReportLoop = 0;
	slash.ReportPt = 0;

	slash.time = 0;
	connection_flag = 0;

	#if SEND_NOTIFY
	_PRINTF("[INIT CFS] : ");
//	printf("%d,%d,%d,%d\n",sizeof(struct Report),sizeof(struct Ressource),sizeof(struct RestNotify),sizeof(struct RestConnection));
	_PRINTF("[LOG] Format ");
	LOG_FORMAT(&RessourceTemp,Buf,&LogCfsPt);
	_PRINTF("[OK]\n");
	#endif
}

PROCESS(rest6_server_process, "Rest IPV6 HTTP process");
AUTOSTART_PROCESSES(&rest6_server_process);

PROCESS_THREAD(rest6_server_process, ev, data)
{
  PROCESS_BEGIN();

  // Init listening on the port 80 for TCP/IP packet
  ressources_init();
  tcp_listen(HTONS(PORT));
  memb_init(&noti);
  memb_init(&conns);

	#if SEND_NOTIFY
		etimer_set(&et, CLOCK_SECOND * PROCESS_TIME);
	#endif

	list_init(ressources_list);
	ressources_loader();
	Emma_init();

  // Waiting for incomming packet or timer event for check notification
  while(1) {

    PROCESS_WAIT_EVENT_UNTIL(ev == tcpip_event || etimer_expired(&et));

		if(ev == tcpip_event)    			app_rest(data);
		else {
			etimer_set(&et, CLOCK_SECOND * PROCESS_TIME);
			slash.time += PROCESS_TIME;

		//	if (!connection_flag) 		
				thread_publish_func();
			}
  }


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/** @} */
