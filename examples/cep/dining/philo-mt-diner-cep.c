
/**
 * \file
 *        Illustrates an Event-Based  Solution to deadlock problem in Mote-1. 
 *        Each thread posts an event to the event queue whenever a resource 
 *        contention is experienced. The event would describe the other thread
 *        contented with and  the shared resource. Mote-3 PROCESS listens for 
 *        "RESOURCE_CONTENTION_EVENT" type of events; and tries to aggregate 
 *        those incoming events for resolving "Circular Resource Dependency" problem.
 *        It implements a new method that comes with "CEP_CONTEND" library as
 *        an empty function definition,  "resolve_contention(...)". In order to detect
 *        a "Circular Resource Dependency", we  need to use a data structure of size
 *        the  same as the total number of resources; because, regardless of total
 *        number of threads running on the system, dependency revolves around resources.
 *\author
 *        Koray INCKI <koray.incki@ozu.edu.tr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "contiki.h"
#include "contiki-net.h"
#include "sys/mt.h"
#include "rest-engine.h"


#define PRINTF(...)

/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern resource_t res_diners;

#define PHILOS 2
#define DELAY 5000
#define FOOD 50

static int chopsticks[PHILOS];

static void *philosopher (void *id); 
static int grab_chopstick (int, int, char *);
static void down_chopsticks (int, int);
static int food_on_table ();

static int sleep_second;

/**
 * \brief miCEP engine events
 */
typedef enum {
  CEP_EVENT_RES_CONTENTION,
  /* MQTT_EVENT_DISCONNECTED, */

  /* MQTT_EVENT_SUBACK, */
  /* MQTT_EVENT_UNSUBACK, */
  /* MQTT_EVENT_PUBLISH, */
  /* MQTT_EVENT_PUBACK, */

  /* Errors */
  /* MQTT_EVENT_ERROR = 0x80, */
  /* MQTT_EVENT_PROTOCOL_ERROR, */
  /* MQTT_EVENT_CONNECTION_REFUSED_ERROR, */
  /* MQTT_EVENT_DNS_ERROR, */
  /* MQTT_EVENT_NOT_IMPLEMENTED_ERROR, */
  /* Add more */
} micep_event_t;

/**
 * \brief miCEP engine event structure
 */
typedef struct {
  uint8_t myID;
  uint8_t contentionID;
  uint8_t resource;
} miCEP_event_str;

// event type for resolving mote based failures by miCEP
process_event_t miCEP_event;

PROCESS(philo_mt_deadlock_free_process, "Multi-Threaded Philosophers Deadlock Free Process");
AUTOSTART_PROCESSES(&philo_mt_deadlock_free_process);

void *philosopher(void *num)
{
  int id;
  int left_chopstick, right_chopstick, f;
  int eaten = 0;

  id = (int) num; 
  printf ("Philosopher %d is done thinking and now ready to eat.\n", id);
  right_chopstick = id;
  left_chopstick = id + 1;
  
  /* Wrap around the chopsticks. */
  if (left_chopstick == PHILOS)
    left_chopstick = 0;

  while ( f = food_on_table() ) {
  	
    /* Thanks to philosophers #1 who would like to take a nap
     * before picking up the chopsticks, the other philosophers
     * may be able to eat their dishes and not deadlock.  
     */
    /* if (id == 1) */
    /*   mt_yield(); */
    
    while (-1 == grab_chopstick (id, right_chopstick, "right "))
      { // try to get the chopstick before continueing 
	mt_yield();
      }
    
    // let other threads to contend for the chopsticks
    mt_yield();
    
    while( -1 == grab_chopstick (id, left_chopstick, "left ") )
      {
	mt_yield();
      }
    
    printf ("Philosopher %d: eating.\n", id);
    
    // let other threads to conted for the resources
    mt_yield();
    down_chopsticks(left_chopstick, right_chopstick);
    // let other threads to conted for the resources
    mt_yield();

    eaten = 1; 
  } // while  

  if (eaten)
    printf ("Philosopher %d is done eating.\n", id);
  else
    printf ("Philosopher %d has starved to death.\n", id);
  
  return 0;
} // philosopher(..)

/*---------------------------------------------------------------*/
int
grab_chopstick (int phil, 
		int c, 
		char *hand)
{
  //find a fictitous way to lock chopstick mutex c
  if ( -1 != chopsticks[c] )
    {
      printf("Philosopher-%d cannot grab Chopstick-%d, it is already grabbed by philosopher-%d\n", phil, c, chopsticks[c]);
      
      // post a RESOURCE_CONTENTION_EVENT to the process
      miCEP_event_str *contention;
      contention->myID = phil;
      contention->contentionID = chopsticks[c];
      contention->resource = c;
      process_post(PROCESS_CURRENT(), miCEP_event, (void *)  contention);
      return -1; // indicates failure
    }
  else
    {
      chopsticks[c] = phil;
      printf("Philosopher %d: got %s chopstick %d\n", phil, hand, c);
      return c; // indicates success
    }
}

/*---------------------------------------------------------------*/
void
down_chopsticks (int c1,
		 int c2)
{
  // reset chopsticks array to -1 for index c1, c2
  chopsticks[c1] = chopsticks[c2] = -1;
}

/*---------------------------------------------------------------*/
int
food_on_table()
{
  static int food = FOOD;
  int myfood;

  if (food > 0)
    {
      food--;
    }
  myfood = food;
  return myfood;
}

/*-----------------------------------------------------------------*/
PROCESS_THREAD(philo_mt_deadlock_free_process, ev, data)
{
  static struct mt_thread mt_philos[PHILOS];
 
  static miCEP_event_str *event;

  static struct etimer timer;
  static int toggle;

  // you can begin coding thread body from here on
  PROCESS_BEGIN();

  PROCESS_PAUSE();

  PRINTF("Starting Erbium Example Server\n");

  PRINTF("uIP buffer: %u\n", UIP_BUFSIZE);
  PRINTF("LL header: %u\n", UIP_LLH_LEN);
  PRINTF("IP+UDP header: %u\n", UIP_IPUDPH_LEN);
  PRINTF("REST max chunk: %u\n", REST_MAX_CHUNK_SIZE);

  /* Initialize the REST engine. */
  rest_init_engine();

  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
  rest_activate_resource(&res_diners, "test/diners");

  int i = 0;

  // initialize micep_event
  miCEP_event = process_alloc_event();
  
  // initialize multi-threading library before using  
  mt_init(); 

  // initialize chopsticks array
  for (i = 0; i < PHILOS; i++)
    chopsticks[i] = -1;

  // start all philosophers, they will be in MT_STATE_READY state
  for (i=0; i < PHILOS; i++)
    mt_start(&mt_philos[i], philosopher, (void *) i); // we are sending NULL temporarily
 
  sleep_second = CLOCK_SECOND * 2;

  etimer_set(&timer, sleep_second);

  while (1) {
    PROCESS_WAIT_EVENT();

    // we will find a fictitous way to execute each thread randomly or in sequence
    if (PROCESS_EVENT_TIMER == ev)
      {
	// lets start by sequentially running all threads
	// we can use switch structure for later
	mt_exec(&mt_philos[toggle]);
	
	toggle++;
	// toggle should cycle back to 0, if greater than PHILOS
	toggle %= PHILOS;
      } // if (PROCESS_EVENT_TIMER..)
    else if ( miCEP_event == ev )
      {
	event = data;
	printf("Thread-%d contends with Thread-%d for resource %d\n",
	       event->myID, 
	       event->contentionID, 
	       event->resource);
      }

    etimer_set(&timer, sleep_second);
  } // while

  // stop all philosophers, they will be in MT_STATE_READY state
  for (i=0; i < PHILOS; i++)
    mt_stop(&mt_philos[i]); 

  // remove multi-threading library
  mt_remove();

  PROCESS_END();
} // PROCESS_THREAD
