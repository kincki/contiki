#ifndef HTTPCOMMON_H_
#define HTTPCOMMON_H_

/*includes*/
#include "contiki-net.h"
#include "type-defs.h" /*needed by rest.h*/

/*current state of the request, waiting: handling request, output: sending response*/
#define STATE_WAITING 0
#define STATE_OUTPUT  1

/*definitions of the line ending characters*/
#define LINE_FEED_CHAR '\n'
#define CARRIAGE_RETURN_CHAR '\r'

/*needed for web services giving all path (http://172.16.79.0/services/light1)
 * instead relative (/services/light1) in HTTP request. Ex: Restlet lib. does it*/
extern const char* httpString;

/*HTTP method strings*/
extern const char* httpGetString;
extern const char* httpHeadString;
extern const char* httpPostString;
extern const char* httpPutString;
extern const char* httpDeleteString;

extern const char* spaceString;

extern const char* httpv1_1;
extern const char* lineEnd;
extern const char* contiki;
extern const char* close;

/*header names*/
extern const char* HTTP_HEADER_NAME_CONTENT_TYPE;
extern const char* HTTP_HEADER_NAME_CONTENT_LENGTH;
extern const char* HTTP_HEADER_NAME_LOCATION;
extern const char* HTTP_HEADER_NAME_CONNECTION;
extern const char* HTTP_HEADER_NAME_SERVER;
extern const char* HTTP_HEADER_NAME_HOST;
extern const char* HTTP_HEADER_NAME_IF_NONE_MATCH;
extern const char* HTTP_HEADER_NAME_ETAG;

extern const char* headerDelimiter;


/*Configuration parameters*/
#define PORT 8080
#define RESPONSE_BUFFER_SIZE 500
#define MAX_REQUEST_HEADERS 6
#define MAX_RESPONSE_HEADERS 6
#define REQUEST_BUFFER_SIZE 1000
#define MAX_URL_MATCHED_ATTRS 4
#define INCOMING_DATA_BUFF_SIZE 102 /*100+2, 100 = max url len, 2 = space char+'\0'*/

/*error definitions*/
typedef enum
{
  NO_ERROR,

  /*Memory errors*/
  MEMORY_ALLOC_ERR,
  MEMORY_BOUNDARY_EXCEEDED,

  /*specific errors*/
  XML_NOT_VALID,
  SOAP_MESSAGE_NOT_VALID,
  URL_TOO_LONG,
  URL_INVALID
} Error_t;

/*HTTP Method (Verb) type*/
typedef enum
{
  METHOD_GET=(1<<0),
  METHOD_HEAD=(1<<1),
  METHOD_POST=(1<<2),
  METHOD_PUT=(1<<3),
  METHOD_DELETE=(1<<4)
} HttpMethod_t;

/*Media Types and Statuses (both copied from Restlet)*/
typedef enum
{
  TEXT_PLAIN, TEXT_XML, APPLICATION_XML, APPLICATION_JSON, APPLICATION_WWW_FORM, APPLICATION_ATOM_XML
} MediaType_t;

typedef enum
{
/**
     * The request could not be understood by the server due to malformed
     * syntax.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.4.1">HTTP RFC - 10.4.1 400 Bad Request</a>
     */
    CLIENT_ERROR_BAD_REQUEST = 400,

    /**
     * The method specified in the Request-Line is not allowed for the resource
     * identified by the Request-URI.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.4.6">HTTP RFC - 10.4.6 405 Method Not Allowed</a>
     */
    CLIENT_ERROR_METHOD_NOT_ALLOWED = 405,

    /**
     * The resource identified by the request is only capable of generating
     * response entities whose content characteristics do not match the user's
     * requirements (in Accept* headers).
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.4.7">HTTP RFC - 10.4.7 406 Not Acceptable</a>
     */
    CLIENT_ERROR_NOT_ACCEPTABLE = 406,

    /**
     * The server has not found anything matching the Request-URI or the server
     * does not wish to reveal exactly why the request has been refused, or no
     * other response is applicable.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.4.5">HTTP RFC - 10.4.5 404 Not Found</a>
     */
    CLIENT_ERROR_NOT_FOUND = 404,

    /**
     * The server is refusing to service the request because the Request-URI is
     * longer than the server is willing to interpret.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.4.15">HTTP RFC - 10.4.15 414 Request-URI Too Long</a>
     */
    CLIENT_ERROR_REQUEST_URI_TOO_LONG =
            414,

    /**
     * The server is refusing to service the request because the entity of the
     * request is in a format not supported by the requested resource for the
     * requested method.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.4.16">HTTP RFC - 10.4.16 415 Unsupported Media Type</a>
     */
    CLIENT_ERROR_UNSUPPORTED_MEDIA_TYPE =
            415,

    /**
     * Status code sent by the server in response to a conditional GET request
     * in case the document has not been modified.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.3.5">HTTP RFC - 10.3.5 304 Not Modified</a>
     */
    NOT_MODIFIED = 304,

     /**
     * The server encountered an unexpected condition which prevented it from
     * fulfilling the request.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.5.1">HTTP RFC - 10.5.1 500 Internal Server Error</a>
     */
    SERVER_ERROR_INTERNAL = 500,

    /**
     * The server does not support the functionality required to fulfill the
     * request.
     * The client tried to use a feature of HTTP (possibly an extended
     * feature) which the server doesn’t support.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.5.2">HTTP * RFC - 10.5.2 501 Not Implemented</a>
     */
    SERVER_ERROR_NOT_IMPLEMENTED = 501,

    /**
     * The server is currently unable to handle the request due to a temporary
     * overloading or maintenance of the server.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.5.4">HTTP RFC - 10.5.4 503 Service Unavailable</a>
     */
    SERVER_ERROR_SERVICE_UNAVAILABLE =
            503,

    /**
     * The request has been accepted for processing, but the processing has not
     * been completed.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.2.3">HTTP RFC - 10.2.3 202 Accepted</a>
     */
    SUCCESS_ACCEPTED = 202,

    /**
     * The request has been fulfilled and resulted in a new resource being
     * created.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.2.2">HTTP RFC - 10.2.2 201 Created</a>
     */
    SUCCESS_CREATED = 201,

    /**
     * The server has fulfilled the request but does not need to return an
     * entity-body (for example after a DELETE), and might want to return
     * updated metainformation.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.2.5">HTTP RFC - 10.2.5 204 No Content</a>
     */
    SUCCESS_NO_CONTENT = 204,


    /**
     * The request has succeeded.
     *
     * @see <a href="http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.2.1">HTTP RFC - 10.2.1 200 OK</a>
     */
    SUCCESS_OK = 200,

} StatusCode_t;

/*User Data type*/
struct UserData_t
{
  char* queryString;
  uint16_t queryStringSize;

  /*DY : FIX_ME : this is rest specific data actually, so maybe should be moved from here.*/
  uint8_t numOfAttrs;
  struct Attr_t
  {
    char* pattern;
    char* realValue;
  } attributes[ MAX_URL_MATCHED_ATTRS ];

  char* postData;
  uint16_t postDataSize;
};

/*Header type*/
struct Header_t
{
  char* name;
  char* value;
};

/*Response type*/
struct Response_t
{
  StatusCode_t statusCode;
  char* statusString;
  const char* url;
  uint8_t numOfResponseHeaders;
  struct Header_t headers[ MAX_RESPONSE_HEADERS ];
  uint16_t responseBuffIndex;
  char responseBuffer[RESPONSE_BUFFER_SIZE];
};

/*This structure contains information about the HTTP request.*/
typedef struct
{
  struct psock sin, sout; /*Protosockets for incoming and outgoing communication*/
  struct pt outputpt;
  char inputBuf[INCOMING_DATA_BUFF_SIZE]; /*to put incoming data in*/
  char* resourceUrl;
  uint8_t state;
  HttpMethod_t requestType; /* GET, POST, etc */

  struct Response_t response;

  char connBuffer[REQUEST_BUFFER_SIZE];
  uint16_t connBuffUsedSize;

  uint8_t numOfRequestHeaders;
  struct Header_t headers[ MAX_REQUEST_HEADERS ];

  struct UserData_t userData;
} ConnectionState_t;

/**
 * Initializes the connection state by clearing out the data structures
 */
void http_common_init_connection(ConnectionState_t* pConnectionState);

#endif /*HTTPCOMMON_H_*/
