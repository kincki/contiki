#include <stdio.h>
#include "rest.h"
#include "contiki.h"
#include "string.h"
#include "simplexml.h"
#include "logger.h" 
#include "type-defs.h"

#include <stdlib.h> //for atof

#include "dev/sht11.h"
#ifdef CONTIKI_TARGET_SKY
  #include "dev/light.h"
#endif //CONTIKI_TARGET_SKY

#include "dev/leds.h"

#define BUFF_SIZE 100
char buffer[BUFF_SIZE];

/* Resources are defined by RESOURCE macro, signature: resource name, the http methods it handles and its url*/
RESOURCE(helloworld, METHOD_GET, "/helloworld");

/* For each resource defined, there corresponds an handler method which should be defined too.
 * Name of the handler method should be [resource name]_handler
 * */
void helloworld_handler( ConnectionState_t* pConnectionState )
{
  LOG_INFO("WS Helloworld\n");

  http_server_set_representation(pConnectionState, TEXT_PLAIN );
  http_server_concatenate_str_to_response(pConnectionState, "Hello World!");
}

/*A simple actuator example, depending on the color parameter and post variable mode, corresponding led is activated or deactivated*/
RESOURCE(ledControl, METHOD_POST | METHOD_PUT , "/led/{color}");

void ledControl_handler( ConnectionState_t* pConnectionState )
{
  char mode[20];
  uint8_t led = 0;
  bool bSuccess = false;
  char* color = rest_get_attribute(pConnectionState, "color");

  if ( color )
  {
    if ( !strcmp(color,"red") )
    {
      led = LEDS_RED;
    }
    else if ( !strcmp(color,"green") )
    {
      led = LEDS_GREEN;
    }
    else if ( !strcmp(color,"blue") )
    {
      led = LEDS_BLUE;
    }
    else
    {
      http_server_set_http_status( pConnectionState, CLIENT_ERROR_NOT_FOUND );
      return;
    }
  }

  http_server_get_post_variable(pConnectionState, "mode",mode, sizeof(mode));

  if (led)
  {
    if ( !strcmp(mode,"on") )
    {
      leds_on(led);
      bSuccess = true;
    }
    else if ( !strcmp(mode,"off") )
    {
      leds_off(led);
      bSuccess = true;
    }
  }

  LOG_DBG("color=%s mode=%s\n",color, mode);

  if ( !bSuccess )
  {
    http_server_set_http_status( pConnectionState, CLIENT_ERROR_BAD_REQUEST );
  }
}

/*Depending on the query variable which, one of the sensor values (light or temperature) is read and given in an xml document.*/
RESOURCE(service, METHOD_GET, "/service");

enum Service{none,temperature,light};
void service_handler( ConnectionState_t* pConnectionState )
{
  enum Service service = none;
  http_server_get_query_variable(pConnectionState, "which",buffer, BUFF_SIZE);
  
  LOG_DBG("which=%s\n",buffer);
  
  if ( !strcmp(buffer,"temperature") )
  {
    service = temperature;
  }
  else if ( !strcmp(buffer,"light") )
  {
    service = light;
  }
  else
  {
    http_server_set_http_status( pConnectionState, CLIENT_ERROR_BAD_REQUEST );
    return;
  }
  
  XmlWriter xmlWriter;

  simpleXmlStartDocument(&xmlWriter, http_server_get_res_buf(pConnectionState), RESPONSE_BUFFER_SIZE);
  simpleXmlStartElement(&xmlWriter, NULL, "Service");
  simpleXmlStartElement(&xmlWriter, NULL, "Name");
  simpleXmlCharacters(&xmlWriter, buffer);
  simpleXmlEndElement(&xmlWriter, NULL, "Name");
  simpleXmlStartElement(&xmlWriter, NULL, "Value");

  uint16_t value = 0;
  if ( service == temperature )
  {
    #ifdef CONTIKI_TARGET_SKY
      value = sht11_temp();

      snprintf(buffer, sizeof(buffer),
       "%d.%d", (value / 10 - 396) / 10, (value / 10 - 396) % 10);
     #else
      snprintf(buffer, sizeof(buffer),
       "%d.%d", value, value);
     #endif
  }
  else
  {
    #ifdef CONTIKI_TARGET_SKY
      value = sensors_light1();
    #endif

    snprintf(buffer, sizeof(buffer), "%d", value);
  }

  simpleXmlCharacters(&xmlWriter, buffer);
  simpleXmlEndElement(&xmlWriter, NULL, "Value");
  simpleXmlEndElement(&xmlWriter, NULL, "Service");
  simpleXmlEndDocument(&xmlWriter);
  
  http_server_set_representation(pConnectionState, TEXT_XML );
}

void handler(
  SimpleXmlParser parser, 
  SimpleXmlEvent event, 
  const char* uri, 
  const char* szName, 
  const char** attr)
{
  uint8_t ledNo = 0;
  
  if ( event == ADD_SUBTAG )
  {
    //printf("Uri:%s Tag Name:%s NumOfAttrs:%d\n",uri, szName,simpleXmlGetNumOfAttrs(attr));
    
    if ( strcmp(szName,"led") == 0 )
    {
      unsigned char nAttrIndex = 0;
      unsigned char nNumOfAttrs = simpleXmlGetNumOfAttrs(attr);
      
      while( nAttrIndex < nNumOfAttrs )
      {
        const char* name = simpleXmlGetAttrName(nAttrIndex, attr);
        const char* value = simpleXmlGetAttrValue(nAttrIndex, attr);
        
        if ( strcmp( name,"name") == 0  )
        {
          if ( strcmp(value,"red") == 0  )
          {
            LOG_DBG("RED\n");
            ledNo = LEDS_RED;
          }
          else if ( strcmp(value,"green") == 0  )
          {
            LOG_DBG("GREEN\n");
            ledNo = LEDS_GREEN;
          }
          else if ( strcmp(value,"blue") == 0  )
          {
            LOG_DBG("BLUE\n");
            ledNo = LEDS_BLUE;
          }
        }
        else if ( strcmp(name,"mode") == 0  )
        {
          LOG_DBG("ledNo %d szValue %s\n",ledNo,value);
          if ( ledNo )
          {
            if ( strcmp(value,"ON") == 0  )
            {
              LOG_DBG("ON\n");
              leds_on(ledNo);
            }
            else if ( strcmp(value,"OFF") == 0  )
            {
              LOG_DBG("OFF\n");
              leds_off(ledNo);
            }
          }
        }
        //printf("###%s###::::%s=%s  ", simpleXmlGetAttrUri(nAttrIndex, attr), simpleXmlGetAttrName(nAttrIndex, attr), simpleXmlGetAttrValue(nAttrIndex, attr) );
        nAttrIndex++;
      }
    }
  }
  
  printf("\n");
}

/*Configures the leds by the information provided in the post data as an xml file*/
RESOURCE(configure, METHOD_POST, "/conf");

void configure_handler( ConnectionState_t* pConnectionState )
{
  SimpleXmlParser parser;
  int nResult;
  const char* xml = http_server_get_post_data(pConnectionState);

  if ( xml )
  {
    parser = simpleXmlCreateParser(xml, strlen(xml));
    if (parser == NULL)
    {
      LOG_DBG("couldn't create parser\n");
      http_server_set_http_status( pConnectionState, SERVER_ERROR_INTERNAL );

      return;
    }

    if ((nResult= simpleXmlParse(parser, handler)) != 0)
    {
      if (nResult >= SIMPLE_XML_USER_ERROR)
      {
        LOG_DBG("parse error %d\n", nResult );
      }
    }

    simpleXmlDestroyParser(parser);
  }
  else
  {
    http_server_set_http_status( pConnectionState, CLIENT_ERROR_BAD_REQUEST );
  }
}

PROCESS(rest_example, "Rest Example");
AUTOSTART_PROCESSES( &rest_example);


PROCESS_THREAD(rest_example, ev, data)
{
  PROCESS_BEGIN();
  
  /*start the http server*/
  process_start(&httpdProcess, NULL);
  /*Init the restful api*/
  rest_init();
  
  /*Resources should be activated*/
  rest_activate_resource( &resource_helloworld );
  rest_activate_resource( &resource_service );
  rest_activate_resource( &resource_ledControl );
  rest_activate_resource( &resource_configure );
  
  PROCESS_END();
}
