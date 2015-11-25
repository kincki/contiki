#include "http-common.h"

/*needed for web services giving all path (http://172.16.79.0/services/light1)
 * instead relative (/services/light1) in HTTP request. Ex: Restlet lib.*/
const char* httpString = "http";

/*HTTP method strings*/
const char* httpGetString = "GET";
const char* httpHeadString = "HEAD";
const char* httpPostString = "POST";
const char* httpPutString = "PUT";
const char* httpDeleteString = "DELETE";

const char* spaceString = " ";

const char* httpv1_1 = "HTTP/1.1";
const char* lineEnd = "\r\n";
const char* contiki = "Contiki";
const char* close = "close";

/*header names*/
const char* HTTP_HEADER_NAME_CONTENT_TYPE = "Content-Type";
const char* HTTP_HEADER_NAME_CONTENT_LENGTH = "Content-Length";
const char* HTTP_HEADER_NAME_LOCATION = "Location";
const char* HTTP_HEADER_NAME_CONNECTION = "Connection";
const char* HTTP_HEADER_NAME_SERVER = "Server";
const char* HTTP_HEADER_NAME_HOST = "Host";
const char* HTTP_HEADER_NAME_IF_NONE_MATCH = "If-None-Match";
const char* HTTP_HEADER_NAME_ETAG = "ETag";

const char* headerDelimiter = ": ";

void http_common_init_connection(ConnectionState_t* pConnectionState)
{
  pConnectionState->state = STATE_WAITING;

  pConnectionState->resourceUrl = NULL;
  pConnectionState->connBuffer[0] = 0;
  pConnectionState->connBuffUsedSize = 0;

  pConnectionState->userData.numOfAttrs = 0;
  pConnectionState->numOfRequestHeaders = 0;

  pConnectionState->userData.queryString = NULL;
  pConnectionState->userData.queryStringSize = 0;

  pConnectionState->userData.postData = NULL;
  pConnectionState->userData.postDataSize = 0;

  pConnectionState->response.responseBuffer[0] = 0;
  pConnectionState->response.responseBuffIndex = 0;
  pConnectionState->response.statusString = NULL;
  pConnectionState->response.numOfResponseHeaders = 0;
}
