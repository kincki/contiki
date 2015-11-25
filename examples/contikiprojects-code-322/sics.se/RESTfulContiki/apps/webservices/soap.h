#ifndef SOAP_H_
#define SOAP_H_

/*includes*/
#include "http-server.h"

/*Representation of a SOAP Method*/
typedef struct
{
  char name[50];
  char urn[50];

  /*DY : FIX_ME : should add type here and put linked list instead of arrays*/
  uint8_t numOfParams;
  #define MAX_PARAMS 8
  struct Param_t
  {
    char name[20];
    char value[20];
    char type[20];
  } params[MAX_PARAMS];

  char *action; //not impl yet, will be gotten from http header - would it be better to put it in method struct?
} SoapMethod_t;

/*Representation of a SOAP Fault*/
typedef struct
{
  char* faultcode;
  char* faultstring;
  char* faultactor;
  char* detail;
} SoapFault_t;

/*Representation of a SOAP Body*/
typedef struct
{
  SoapMethod_t method;
  SoapFault_t fault;
} SoapBody_t;

/*Representation of a SOAP Envelope*/
typedef struct
{
  //SoapHeader_t header;
  SoapBody_t body;
  char* buffer;
} SoapEnvelope_t;

/*Representation of a SOAP Context*/
typedef struct
{
  SoapEnvelope_t env;
//  hrequest_t *http;
//  attachments_t *attachments;
}SoapContext_t;

/*Signature of handler function*/
typedef void (*SoapHandler) (SoapMethod_t* request, SoapMethod_t* response);

typedef struct
{
  struct SoapWebService_t *next;
  const char* pUrl;
  const char* pUrn;
  const char* pMethodName;
  /*uint8_t acceptedEncoding;*/ /*DY : FIX_ME : not implemented yet, may need to be bigger than 8 bits*/
  SoapHandler handler;
} SoapWebService_t;

/*
 * Macro to define a SOAP-Based Web service
 */
#define SOAP_WEB_SERVICE(name,url,urn,method) \
void name##_handler(SoapMethod_t* request, SoapMethod_t* response); \
SoapWebService_t soapWebService_##name = { NULL, url, urn, method, name##_handler }


/*
 * Initializes SOAP library and starts HTTP process
 */
void soap_init(void);

/*
 * Web services should be activated with the following code to be accessible
 */
void soap_activate_service( SoapWebService_t* pWebService );

/*
 * To be called by HTTP server as a callback function when a new HTTP connection appears.
 * This function dispatches the corresponding Web service.
 */
bool soap_invoke_service( ConnectionState_t* pConnectionState );

/*
 * Sets the name and urn of the method to be called over SOAP.
 */
void soap_set_method_name(const char *pcUrn, const char *pcName, SoapMethod_t* pMethod);

/*
 * Adds new parameter(name, value and type of it) in the SOAP Method.
 */
void soap_add_param(const char *pcName, const char *pcValue, const char *pcType, SoapMethod_t* pMethod);

#endif /*SOAP_H_*/
