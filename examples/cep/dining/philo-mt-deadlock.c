
/**
 * \file
 *        Illustrates deadlock problem in a  multi-threaded application environment,
 *        which regards/implements resource sharing as a synchronization problem. If a
 *        philosopher on this node can NOT attain both chopsticks, it will wait until both 
 *        chopsticks become available. The scenario has been setup such that each
 *        philosopher yields itself after it grabs one chopstick, and since there is no 
 *        mechanism in Contiki for deadlock prevention, all philosophers end up
 *        waiting for each other forever. In other words, each philosopher depends on 
 *        another one, thus resulting a PROCESS-wide "Circular Resource Dependency".
 *\author
 *        Koray INCKI <koray.incki@ozu.edu.tr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "contiki.h"
#include "sys/mt.h"

#define PHILOS 5
#define DELAY 5000
#define FOOD 50

static int chopsticks[PHILOS];

static void *philosopher (void *id); 
static int grab_chopstick (int, int, char *);
static void down_chopsticks (int, int);
static int food_on_table ();

static int sleep_second;

PROCESS(philo_mt_deadlock_process, "Multi-Threaded Philosophers w/ Deadlock Process");
AUTOSTART_PROCESSES(&philo_mt_deadlock_process);

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
      printf("%d. chopstick is already grabbed by philosopher-%d\n", c, chopsticks[c]);
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
PROCESS_THREAD(philo_mt_deadlock_process, ev, data)
{
  static struct mt_thread mt_philos[PHILOS];
 
  static struct etimer timer;
  static int toggle;

  // you can begin coding thread body from here on
  PROCESS_BEGIN();

  int i = 0;
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
	switch (toggle)
	  {
	  case 0:
	    mt_exec(&mt_philos[toggle]);
	    break;
	  case 1:
	    mt_exec(&mt_philos[toggle]);
	    break;
	  case 2:
	    mt_exec(&mt_philos[toggle]);
	    break;
	  case 3:
	    mt_exec(&mt_philos[toggle]);
	    break;
	  case 4:
	    mt_exec(&mt_philos[toggle]);
	    break;
	  default:
	    printf("Toggle Shouldn't Have Taken Value %d\n", toggle);
	  } // switch
	
	toggle++;
	// toggle should cycle back to 0, if greater than PHILOS
	toggle %= PHILOS;
      } // if

    etimer_set(&timer, sleep_second);
  } // while

  // stop all philosophers, they will be in MT_STATE_READY state
  for (i=0; i < PHILOS; i++)
    mt_stop(&mt_philos[i]); 

  // remove multi-threading library
  mt_remove();

  PROCESS_END();
} // PROCESS_THREAD
