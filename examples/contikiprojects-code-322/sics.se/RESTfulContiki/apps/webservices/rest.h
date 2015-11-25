#ifndef REST_H_
#define REST_H_

/*includes*/
#include "http-server.h"

PROCESS_NAME(restWebServicesProcess);

/*Signature of handler functions*/
typedef void (*RestfulHandler) (ConnectionState_t* pConnectionState);
typedef bool (*RestfulPreHandler) (ConnectionState_t* pConnectionState);
typedef void (*RestfulPostHandler) (ConnectionState_t* pConnectionState);

/*
 * Data structure representing a resource in REST.
 */
struct Resource_t
{
  struct Resource_t *next; /*points to next resource defined*/
  HttpMethod_t requestTypesToHandle; /*handled HTTP methods*/
  const char* pUrlPattern; /*simple template of handled URLs
                           ex: "/task/{id}" id is parameterized*/
  RestfulHandler handler; /*handler function*/
  RestfulPreHandler preHandler; /*to be called before handler, may perform initializations*/
  RestfulPostHandler postHandler; /*to be called after handler, may perform finalizations (cleanup, etc)*/
  void* pUserData; /*pointer to user specific data*/
};
typedef struct Resource_t Resource_t;

/*
 * Macro to define a Resource
 * Resources are statically defined for the sake of efficiency and better memory management.
 */
#define RESOURCE(name,typesToHandle,url) \
void name##_handler(ConnectionState_t* pConnectionState); \
Resource_t resource_##name = { NULL, typesToHandle, url, name##_handler, NULL, NULL, NULL }

/*
 * Initializes REST framework and starts HTTP process
 */
void rest_init(void);

/*
 * Resources wanted to be accessible should be activated with the following code.
 */
void rest_activate_resource(Resource_t* pResource);

/*
 * To be called by HTTP server as a callback function when a new HTTP connection appears.
 * This function dispatches the corresponding RESTful service.
 */
bool rest_invoke_restful_service( ConnectionState_t* pConnectionState);

/*
 * Sets "Location" header
 */
bool rest_set_url(ConnectionState_t* pConnectionState, const char* pcUrl);

/*
 * Returns the value of the attribute mapped to the template URL.
 * Ex: Template URL "/task/{id}" matches "/task/5" and so calling this function with
 * "id" pattern will return "5".
 */
char* rest_get_attribute(ConnectionState_t* pConnectionState, const char* pcPattern);

/*
 * Setter method for user specific data.
 */
void rest_set_user_data(Resource_t* pResource, void* pUserData);

/*
 * Getter method for user specific data.
 */
void* rest_get_user_data(Resource_t* pResource);

/*
 * Sets the pre handler function of the Resource.
 * If set, this function will be called just before the original handler function.
 * Can be used to setup work before resource handling.
 */
void rest_set_pre_handler(Resource_t* pResource, RestfulPreHandler preHandler);

/*
 * Sets the post handler function of the Resource.
 * If set, this function will be called just after the original handler function.
 * Can be used to do cleanup (deallocate memory, etc) after resource handling.
 */
void rest_set_post_handler(Resource_t* pResource, RestfulPostHandler postHandler);

#endif /*REST_H_*/
