#include "contiki.h"

#include <string.h> //for string operations in match_addresses

#include "logger.h"
#include "type-defs.h"
//#include "simplexml.h" //not needed yet for rest framework
#include "rest.h"

LIST(restfulServices);

void rest_activate_resource( Resource_t* pResource )
{
  //add it to the restful web service link list
  list_add(restfulServices, pResource);
}

#include "dev/leds.h"

//DY : FIX_ME : missing most of the validity controls
//examples (of missing controls):
//should start with "/"
//'{' and '}' should match, '}' can not come first, etc
//
static bool match_addresses(ConnectionState_t* pConnectionState, const char* pPattern, char* pAddress)
{
  bool bMatchAll = false;
  bool bReturnValue = true;
  uint16_t nPatternIndex = 0;
  uint16_t nAddressIndex = 0;

  LOG_DBG("match_addresses -->\n pPattern:%s pAddress:%s\n", pPattern, pAddress);

  while ( bReturnValue )
  {
    if ( pPattern[nPatternIndex] && pAddress[nAddressIndex] )
    {
      if ( pPattern[nPatternIndex] == '{' )
      {
        bMatchAll = true;
        nPatternIndex++;
        continue;
      }
      else
      {
        if ( bMatchAll == false ) //normal comparison
        {
          if ( pPattern[nPatternIndex] != pAddress[nAddressIndex] )
          {
            bReturnValue = false;
          }

          ++nPatternIndex;
          ++nAddressIndex;
        }
        else
        {
          char temp[20]; temp[0] = 0;
          char temp2[20]; temp2[0] = 0;

          LOG_DBG("nPatternIndex %d pPattern+index %s \n", nPatternIndex, pPattern + nPatternIndex );

  //          sscanf( pPattern + nPatternIndex, "%[^}]",temp);
  //          LOG_DBG("pPattern brace ici %s\n",temp);
  //          //DY : buraya bi kontrol konabilir, scanf okuyabilmisse anca (return 1 ise) strlen yaparsin mesela
  //          nPatternIndex += strlen( temp );
  //          sscanf( pAddress + nAddressIndex, "%[^/]",temp2);
  //          LOG_DBG("adress slashe kadar %s\n",temp2);
  //          nAddressIndex += strlen( temp2 );

          char* pEndOfPatternArg = strchr(pPattern + nPatternIndex, '}');

          //DY : FIX_ME : '}' yoksa ne bok yiyecek bu program?
          if ( pEndOfPatternArg )
          {
            uint8_t nNumOfAttrs = pConnectionState->userData.numOfAttrs;

            bMatchAll = false;
            uint16_t nPatternArgLen = pEndOfPatternArg - ( pPattern + nPatternIndex );

            strncpy(temp, pPattern + nPatternIndex ,nPatternArgLen );
            temp[nPatternArgLen] = 0;

            nPatternIndex += ( nPatternArgLen + 1 ); // +1 is for consuming '}' character as well.

            LOG_DBG("pPattern brace ici %s new pPattern index %d endofarg %s\n", temp, nPatternIndex, pEndOfPatternArg );

            if ( nNumOfAttrs < MAX_URL_MATCHED_ATTRS )
            {
              pConnectionState->userData.attributes[nNumOfAttrs].pattern = http_server_put_in_conn_buffer(pConnectionState, temp);

              LOG_DBG("pattern %s\n", pConnectionState->userData.attributes[nNumOfAttrs].pattern);

              char* pEndOfAddressArg = strchr(pAddress + nAddressIndex, '/');

              if ( pEndOfAddressArg )
              {
                uint16_t nAddressArgLen = pEndOfAddressArg - ( pAddress + nAddressIndex );

                strncpy(temp, pAddress + nAddressIndex ,nAddressArgLen );
                temp[nAddressArgLen] = 0;

                nAddressIndex += nAddressArgLen;

                LOG_DBG("address1 / ici %s new adress index %d\n", temp, nAddressIndex);

                pConnectionState->userData.attributes[nNumOfAttrs].realValue = http_server_put_in_conn_buffer(pConnectionState, temp);
                pConnectionState->userData.numOfAttrs++;

                LOG_DBG("address1 real value %s\n", pConnectionState->userData.attributes[pConnectionState->userData.numOfAttrs-1].realValue);
              }
              else
              {
                strcpy(temp, pAddress + nAddressIndex );

                nAddressIndex += strlen(temp);

                LOG_DBG("address2 / ici %s new adress index %d paddress %s\n", temp, nAddressIndex, pAddress );

                pConnectionState->userData.attributes[nNumOfAttrs].realValue = http_server_put_in_conn_buffer(pConnectionState, temp);
                pConnectionState->userData.numOfAttrs++;
              }
            }
            else
            {
              LOG_ERR("MAX_URL_MATCHED_ATTRS(%d) is exceeded!!\n", MAX_URL_MATCHED_ATTRS);
            }
          }
        }
      }
    }
    //added just the handle the last '/' character in which only one of the urls may have but still they are matching
    //DY : FIX_ME : buna baska bi cozum bul, bole cok gereksiz ve kotu oldu.
    else
    {
      if ( pPattern[nPatternIndex] )
      {
        //if url still has more characters or current character is not '/' then urls do not match
        if ( pPattern[nPatternIndex] != '/' || strlen(pPattern) != ++nPatternIndex )
        {
          bReturnValue = false;
        }
      }
      else if ( pAddress[nAddressIndex] )
      {
       if ( pAddress[nAddressIndex] != '/' || strlen(pAddress) != ++nAddressIndex )
        {
          bReturnValue = false;
        }
      }
      //if both urls finished, then they match, just get out of the loop
      else
      {
        //strings are equal so just stop
        break;
      }
    }
  }

  LOG_DBG("Match Addresses Result %d\n",bReturnValue);

  return bReturnValue;
}
//DY : FIX_ME : can add accepted encoding here
bool rest_invoke_restful_service( ConnectionState_t* pConnectionState )
{
  bool bReturnValue = false;

  HttpMethod_t requestType = http_server_get_http_method( pConnectionState );
  char* pUrl = pConnectionState->resourceUrl;

  LOG_DBG("invokeRestfulWebService---> Url : %s\n", pUrl );

  Resource_t* pResource = NULL;

  for ( pResource=(Resource_t*)list_head(restfulServices); pResource ; pResource = pResource->next )
  {
    //if the web service handles that kind of requests and urls matches
    if ( match_addresses( pConnectionState, pResource->pUrlPattern, pUrl ) )
    {
      //found the service, break.
      bReturnValue = true;

      //if method is handled by the resource, then call the corresponding function, or else
      //then send CLIENT_ERROR_METHOD_NOT_ALLOWED status.
      if ( ( pResource->requestTypesToHandle ) & requestType )
      {
        //set to http status to success, user can change it later.
        http_server_set_http_status(pConnectionState, SUCCESS_OK);

        /*if preHandler is not set or it returns true if it is set.*/
        if ( !pResource->preHandler || pResource->preHandler(pConnectionState) )
        {
          pResource->handler(pConnectionState);

          /*call post handler if exists*/
          if (pResource->postHandler)
          {
            pResource->postHandler(pConnectionState);
          }
        }
      }
      else
      {
        http_server_set_http_status(pConnectionState, CLIENT_ERROR_METHOD_NOT_ALLOWED);
      }
      break;
    }
  }

  if ( bReturnValue == false )
  {
    http_server_set_http_status(pConnectionState, CLIENT_ERROR_NOT_FOUND);
    LOG_DBG("No Restful Service Found!!!\n");
  }

  return bReturnValue;
}

void rest_init(void)
{
  list_init(restfulServices);
  http_server_set_service_callback(rest_invoke_restful_service);

  //start the http server
  process_start(&httpdProcess, NULL);
}

bool rest_set_url(ConnectionState_t* pConnectionState, const char* pcUrl)
{
  //FIX_ME : problematic to add response header directly since user may add too, change later.
  //pConnectionState->response.url = putInConnBuffer(pConnectionState, pUrl);
  //true stands for copy the value instead of using it like a const string
  return http_server_add_res_header(pConnectionState, HTTP_HEADER_NAME_LOCATION, pcUrl, true);
}

char* rest_get_attribute(ConnectionState_t* pConnectionState, const char* pcPattern)
{
  uint8_t nIndex = 0;
  char* pValue = NULL;

  LOG_DBG("numOfAttrs %d \n",pConnectionState->userData.numOfAttrs);

  for ( nIndex = 0 ; nIndex < pConnectionState->userData.numOfAttrs ; nIndex++ )
  {
    LOG_DBG("pattern %s value %s \n", pConnectionState->userData.attributes[nIndex].pattern,pConnectionState->userData.attributes[nIndex].realValue);
    if (!strcmp(pConnectionState->userData.attributes[nIndex].pattern, pcPattern))
    {
      pValue = pConnectionState->userData.attributes[nIndex].realValue;
      break;
    }
  }

  return pValue;
}

void rest_set_user_data(Resource_t* pResource, void* pUserData)
{
  pResource->pUserData = pUserData;
}

void* rest_get_user_data(Resource_t* pResource)
{
  return pResource->pUserData;
}

void rest_set_pre_handler(Resource_t* pResource, RestfulPreHandler preHandler)
{
  pResource->preHandler = preHandler;
}

void rest_set_post_handler(Resource_t* pResource, RestfulPostHandler postHandler)
{
  pResource->postHandler = postHandler;
}
