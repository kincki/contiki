#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "soap.h"
#include "contiki.h"
#include "simplexml.h"
#include "logger.h"
#include "type-defs.h"

#include "dev/sht11.h"
#ifdef CONTIKI_TARGET_SKY
  #include "dev/light.h"
#endif //CONTIKI_TARGET_SKY

#include "dev/leds.h"

//DY : FIX_ME : should set "text/xml" for soap 1.1 and application/xml for 1.2.

//DY : FIX_ME : url compare is inefficient (compares it for each service), can put services in a router as csoap
LIST(soapServices);

static const char * const soap_env_ns = "http://schemas.xmlsoap.org/soap/envelope/";
static const char * const soap_env_enc = "http://schemas.xmlsoap.org/soap/encoding/";
static const char * const soap_xsi_ns = "http://www.w3.org/1999/XMLSchema-instance";
static const char * const soap_xsd_ns = "http://www.w3.org/1999/XMLSchema";

//not used yet so commented for code space

//static char *fault_vm = "VersionMismatch";
//static char *fault_mu = "MustUnderstand";
//static char *fault_client = "Client";
static char *fault_server = "Server";

static const char* envelope = "Envelope";
static const char* header = "Header";
static const char* body = "Body";
static const char* fault = "Fault";

static const char* SOAP_ENV = "SOAP-ENV";
static const char* xmlns = "xmlns";
static const char* xsi = "xsi";
static const char* xsd = "xsd";
static const char* encodingStyle = "encodingStyle";
static const char* method = "m";
static const char* type = "type";

static const char* faultcode = "faultcode";
static const char* faultstring = "faultstring";
static const char* faultactor = "faultactor";
static const char* detail = "detail";

SoapContext_t requestCtx;
SoapContext_t responseCtx;

struct SoapHandlerData_t
{
  struct pt handlerpt;
  char* expectedTag;
  SimpleXmlEvent expectedEvent;
  bool bHeaderExists;
  bool bVerify;
};

typedef struct SoapHandlerData_t SoapHandlerData_t;

static PT_THREAD( handlerPT( struct SoapHandlerData_t* pData, SimpleXmlEvent event, const char* uri, const char* szName, const char** attr) )
{
  LOG_DBG("handlerPT -->\n");

  PT_BEGIN(&(pData->handlerpt));

  if ( event == ADD_SUBTAG && !strcmp(szName, envelope) )
  {
    pData->bVerify = true;
  }

  PT_YIELD(&(pData->handlerpt));

  if ( event == ADD_SUBTAG && !strcmp(szName, header) )
  {
    pData->bVerify = true;
  }
  else
  {
    pData->bHeaderExists = false;
  }

  if ( pData->bHeaderExists )
  {
    PT_YIELD(&(pData->handlerpt));

    if ( event == FINISH_TAG && !strcmp(szName, header) )
    {
      pData->bVerify = true;
    }
    PT_YIELD(&(pData->handlerpt));
  }

  LOG_DBG("Before Body\n");
  if ( event == ADD_SUBTAG && !strcmp(szName, body) )
  {
    pData->bVerify = true;
  }
  PT_YIELD(&(pData->handlerpt));

  LOG_DBG("Before Method\n");
  //Method start
  if ( event == ADD_SUBTAG )
  {
    LOG_DBG("**** Method %s URN %s\n",szName, uri);
    pData->bVerify = true;

    strcpy(requestCtx.env.body.method.urn, uri);
    strcpy(requestCtx.env.body.method.name, szName);
  }
  PT_YIELD(&(pData->handlerpt));

  //param start
  while (1)
  {
    if ( event == ADD_SUBTAG )
    {
      LOG_DBG("**** Param name %s\n",szName);
      pData->bVerify = true;

      strcpy( requestCtx.env.body.method.params[requestCtx.env.body.method.numOfParams].name, szName);
    }
    else if ( event == FINISH_TAG )
    {
      break;
    }

    PT_YIELD(&(pData->handlerpt));

    if ( event == ADD_CONTENT )
    {
      pData->bVerify = true;

      strcpy( requestCtx.env.body.method.params[requestCtx.env.body.method.numOfParams].value, szName);
      requestCtx.env.body.method.numOfParams++;
      LOG_DBG("Param value:%s\n",szName);
    }
    PT_YIELD(&(pData->handlerpt));

    if ( event == FINISH_TAG )
    {
      pData->bVerify = true;
    }
    PT_YIELD(&(pData->handlerpt));
  }


  if ( event == FINISH_TAG )
  {
    pData->bVerify = true;
  }
  PT_YIELD(&(pData->handlerpt));
  //Method end

  if ( event == FINISH_TAG && !strcmp(szName, body) )
  {
    pData->bVerify = true;
  }
  PT_YIELD(&(pData->handlerpt));

  if ( event == FINISH_TAG && !strcmp(szName, envelope) )
  {
    pData->bVerify = true;
  }
  PT_YIELD(&(pData->handlerpt));

  LOG_DBG("handlerPT <--\n");
  PT_END(&(pData->handlerpt));
}

//DY : FIX_ME : should do trimming, may include whitespace chars
void handler (SimpleXmlParser parser, SimpleXmlEvent event, const char* uri,
  const char* szName, const char** attr)
{
  LOG_DBG("event %d szName %s uri %s\n",event, szName, uri);

  bool bIgnore = false;
  //ignore add_content with only whitespace in it.
  if ( event == ADD_CONTENT )
  {
    unsigned short nLen = strlen(szName);
    unsigned short nIndex = 0;

    bIgnore = true;
    for ( nIndex = 0 ; nIndex < nLen ; nIndex++ )
    {
      LOG_DBG("CHAR %d\n",szName[nIndex]);
      if ( !isspace(szName[nIndex]) )
      {
        bIgnore = false;
        break;
      }
      LOG_DBG("WS\n");
    }
  }

  if (!bIgnore)
  {
    SoapHandlerData_t* pData = (SoapHandlerData_t*)simpleXmlGetUserData(parser);
    if ( pData )
    {
      pData->bVerify = false;

      handlerPT(pData, event, uri, szName, attr);

      if ( pData->bVerify == false )
      {
        LOG_ERR("PARSE ERROR Tag:%s Event:%d\n",szName, event);
        simpleXmlParseAbort(parser, SIMPLE_XML_USER_ERROR_XML_NOT_VALID);
      }
    }
  }
}

Error_t parse(char* pData, long nMaxDataLen)
{
  Error_t result = NO_ERROR;

  SimpleXmlParser parser;
  int nResult = 0;

  parser = simpleXmlCreateParser(pData, nMaxDataLen);

  if (parser)
  {
    struct SoapHandlerData_t soapHandlerData;
    soapHandlerData.expectedTag = NULL;
    soapHandlerData.expectedEvent = ADD_SUBTAG;
    soapHandlerData.bVerify = true;
    soapHandlerData.bHeaderExists = true;

    PT_INIT(&(soapHandlerData.handlerpt));

    simpleXmlSetUserData(parser,(void*)&soapHandlerData);

    if ((nResult = simpleXmlParse(parser, handler)) != 0)
    {
      LOG_DBG("parse error %d\n", nResult );
      if ( nResult >= SIMPLE_XML_USER_ERROR  )
      {
        result = SOAP_MESSAGE_NOT_VALID;
      }
      else
      {
        result = XML_NOT_VALID;
      }
    }

    simpleXmlDestroyParser(parser);
  }
  else
  {
    LOG_DBG("couldn't create parser\n");
    result = MEMORY_ALLOC_ERR;
  }

  return result;
}

void initCtx(SoapContext_t* ctx)
{
  ctx->env.buffer = NULL;
  ctx->env.body.method.numOfParams = 0;
  ctx->env.body.method.name[0] = 0;
  ctx->env.body.method.urn[0] = 0;

  ctx->env.body.fault.faultcode = NULL;
  ctx->env.body.fault.faultstring = NULL;
  ctx->env.body.fault.faultactor = NULL;
  ctx->env.body.fault.detail = NULL;
}

void setFaultMessage( SoapContext_t* ctx,
                      const char *faultcode,
                      const char *faultstring,
                      const char *faultactor,
                      const char *detail )
{
  ctx->env.body.fault.faultcode = faultcode;
  ctx->env.body.fault.faultstring = faultstring;
  ctx->env.body.fault.faultactor = faultactor;
  ctx->env.body.fault.detail = detail;
}

typedef enum {Type_Method, Type_Fault } SoapMessageType_t;
void createSoapMessage(SoapEnvelope_t* env, SoapMessageType_t soapMessageType)
{
  if ( env )
  {
    XmlWriter xmlWriter;

    simpleXmlStartDocument(&xmlWriter, env->buffer, RESPONSE_BUFFER_SIZE);
    simpleXmlStartElement(&xmlWriter, SOAP_ENV, envelope);
      simpleXmlAddAttribute(&xmlWriter, xmlns, SOAP_ENV,soap_env_ns);
      simpleXmlAddAttribute(&xmlWriter, SOAP_ENV, encodingStyle, soap_env_enc);
      simpleXmlAddAttribute(&xmlWriter, xmlns, xsi,soap_xsi_ns);
      simpleXmlAddAttribute(&xmlWriter, xmlns, xsd,soap_xsd_ns);

    simpleXmlStartElement(&xmlWriter, SOAP_ENV, header);
    simpleXmlEndElement(&xmlWriter, SOAP_ENV, header);

    simpleXmlStartElement(&xmlWriter, SOAP_ENV, body);

    LOG_DBG("xml buf position body start:%lu\n",xmlWriter.xmlWriteBuffer.nPosition);

    if ( soapMessageType == Type_Method )
    {
      simpleXmlStartElement(&xmlWriter, method ,env->body.method.name);
      simpleXmlAddAttribute(&xmlWriter, xmlns, method ,env->body.method.urn);

      unsigned int i = 0;
      for ( i=0; i< env->body.method.numOfParams;i++ )
      {
        simpleXmlStartElement(&xmlWriter, method, env->body.method.params[i].name);
        simpleXmlAddAttribute(&xmlWriter, xsi, type, env->body.method.params[i].type);
        simpleXmlCharacters(&xmlWriter, env->body.method.params[i].value);
        simpleXmlEndElement(&xmlWriter, method, env->body.method.params[i].name);
      }

      simpleXmlEndElement(&xmlWriter, method ,env->body.method.name);
    }
    else if ( soapMessageType == Type_Fault )
    {
      simpleXmlStartElement(&xmlWriter, SOAP_ENV, fault);

      simpleXmlStartElement(&xmlWriter, NULL, faultcode);
      simpleXmlCharacters(&xmlWriter, env->body.fault.faultcode);
      simpleXmlEndElement(&xmlWriter, NULL, faultcode);

      simpleXmlStartElement(&xmlWriter, NULL, faultstring);
      simpleXmlCharacters(&xmlWriter, env->body.fault.faultstring);
      simpleXmlEndElement(&xmlWriter, NULL, faultstring);

      simpleXmlStartElement(&xmlWriter, NULL, faultactor);
      simpleXmlCharacters(&xmlWriter, env->body.fault.faultactor);
      simpleXmlEndElement(&xmlWriter, NULL, faultactor);

      simpleXmlStartElement(&xmlWriter, NULL, detail);
      simpleXmlCharacters(&xmlWriter, env->body.fault.detail);
      simpleXmlEndElement(&xmlWriter, NULL, detail);

      simpleXmlEndElement(&xmlWriter, SOAP_ENV, fault);
    }

    simpleXmlEndElement(&xmlWriter, SOAP_ENV, body);

    simpleXmlEndElement(&xmlWriter, SOAP_ENV, envelope);
    simpleXmlEndDocument(&xmlWriter);

    LOG_DBG("xml buf position at end:%lu\n XML doc:\n",xmlWriter.xmlWriteBuffer.nPosition);
  }
}

bool soap_invoke_service( ConnectionState_t* pConnectionState )
{
  bool bReturnValue = false;
  HttpMethod_t requestType = http_server_get_http_method(pConnectionState);
  char* pUrl = pConnectionState->resourceUrl;
  char* pUrn = NULL;
  char* pMethodName = NULL;

  Error_t parseError = NO_ERROR;

  LOG_DBG("invokeSoapBasedWebService--> url:%s requestType:%d\n",pUrl,requestType);

  //DY : FIX_ME : if method is get and there is WSDL should return it here but no WSDL store done yet.

  if (requestType != METHOD_POST)
  {
    pConnectionState->response.statusCode = SUCCESS_OK;

    http_server_concatenate_str_to_response( pConnectionState, "I only talk with POST!!!");

    return false;
  }

  initCtx(&requestCtx);
  initCtx(&responseCtx);

  responseCtx.env.buffer = pConnectionState->response.responseBuffer;

  parseError = parse(pConnectionState->userData.postData, pConnectionState->userData.postDataSize);


  if ( parseError == NO_ERROR )
  {
    pUrn = requestCtx.env.body.method.urn;
    pMethodName = requestCtx.env.body.method.name;

    if ( pUrn && pMethodName && *pUrn && *pMethodName )
    {
      SoapWebService_t* webService = NULL;

      LOG_DBG("Requested url:%s urn:%s method:%s\n", pUrl, pUrn, pMethodName);

      for ( webService=list_head(soapServices); webService ; webService = webService->next )
      {
        LOG_DBG("Web Service url:%s urn:%s method:%s\n", webService->pUrl, webService->pUrn, webService->pMethodName);
        //if urls matches
        if ( !strcmp( webService->pUrl, pUrl )
             && !strcmp( webService->pUrn, pUrn )
             && !strcmp( webService->pMethodName, pMethodName ) )
        {
          //found the service, break.
          bReturnValue = true;

          pConnectionState->response.statusCode = SUCCESS_OK;

          //"suggested" method return name
          //DY : FIX_ME : later get it from WSDL?? says easysoap::SOAPServerDispatcher
          strcpy(responseCtx.env.body.method.name, requestCtx.env.body.method.name);
          strcat(responseCtx.env.body.method.name, "Response");
          strcpy(responseCtx.env.body.method.urn, requestCtx.env.body.method.urn);

          webService->handler(&(requestCtx.env.body.method), &(responseCtx.env.body.method));

          createSoapMessage(&(responseCtx.env), Type_Method);

          break;
        }
      }
    }
    else
    {
      LOG_DBG("Urn or method name is NULL!!!!\n");
    }

    if ( bReturnValue == false )
    {
      http_server_set_http_status(pConnectionState, CLIENT_ERROR_NOT_FOUND);
      LOG_DBG("No SOAP-Based Service Found!!!\n");
    }
  }

  if ( parseError != NO_ERROR )
  {
    //DY : FIX_ME : another status code maybe?
    http_server_set_http_status(pConnectionState, SERVER_ERROR_INTERNAL);
    http_server_set_representation(pConnectionState, TEXT_XML);

    setFaultMessage( &responseCtx, fault_server, "Error", "SoapServer", NULL );

    createSoapMessage(&(responseCtx.env), Type_Fault);
  }

  return bReturnValue;
}

void soap_set_method_name(const char *pcUrn, const char *pcName, SoapMethod_t* pMethod)
{
  strcpy( pMethod->name, pcName);
  strcpy( pMethod->urn, pcUrn);
}

void soap_add_param(const char *pcName, const char *pcValue, const char *pcType, SoapMethod_t* pMethod)
{
  strcpy( pMethod->params[pMethod->numOfParams].name, pcName);
  strcpy( pMethod->params[pMethod->numOfParams].value, pcValue);
  strcpy( pMethod->params[pMethod->numOfParams].type, pcType);

  pMethod->numOfParams++;
}

void soap_activate_service( SoapWebService_t* pWebService )
{
  list_add(soapServices, pWebService);
}

void soap_init(void)
{
  list_init(soapServices);
  http_server_set_service_callback(soap_invoke_service);

  //start the http server
  process_start(&httpdProcess, NULL);
}
