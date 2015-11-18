#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "contiki.h"
#include "pt-sem.h"

#define PHILOS 4
#define DELAY 5000
#define FOOD 50

/* static void *philosopher (void *id); */
/* static void grab_chopstick (int, */
/*                      int, */
/* 		     char *); */
/* static void down_chopsticks (int, */
/*                       int); */
/* static int food_on_table (); */

/* /\* pthread_mutex_t chopstick[PHILOS]; */
/* pthread_t philo[PHILOS]; */
/* pthread_mutex_t food_lock; */

static struct pt_sem chopstick1, chopstick2, chopstick3, chopstick4, food_lock;
static int sleep_seconds = 0;
static int food = FOOD;	

static
PT_THREAD(philosopher1(struct pt *pt))
{
  static int id;
  static int left_chopstick, right_chopstick;
  static int myfood;
  static int eaten = 0;

  PT_BEGIN(pt);

  id = 1; // hard-coded in the function name
  printf ("Philosopher %d is done thinking and now ready to eat.\n", id);
  right_chopstick = id;
  left_chopstick = id + 1;
  
  /* Wrap around the chopsticks. */
  if (left_chopstick == PHILOS)
    left_chopstick = 0;

  printf("try to lock food\n");

  PT_SEM_WAIT(pt, &food_lock);

  printf("food lock acquired\n");

  if (food > 0) {
    food--;
  }
  myfood = food;

  printf("myfood is %d\n", myfood);

  PT_SEM_SIGNAL(pt, &food_lock);


  while (myfood) {
  	
    /* Thanks to philosophers #1 who would like to take a nap
     * before picking up the chopsticks, the other philosophers
     * may be able to eat their dishes and not deadlock.  
     */
    if (id == 1)
      usleep(sleep_seconds);
    
    PT_SEM_WAIT(pt, &chopstick1);
    printf("Philosopher-1 got Left Chopstick-1\n");
    //    grab_chopstick (id, right_chopstick, "right ", pt);
    //    grab_chopstick (id, left_chopstick, "left", pt);
    PT_SEM_WAIT(pt, &chopstick2);
    printf("Philosopher-1 got Right Chopstick-2\n");
	
    printf ("Philosopher %d: eating.\n", id);
    usleep (DELAY * (FOOD - myfood + 1));
    //    down_chopsticks (left_chopstick, right_chopstick, pt);
    PT_SEM_SIGNAL(pt, &chopstick1);
    printf("Philosopher-1 Dropped Left Chopstick-1\n");
    PT_SEM_SIGNAL(Pt, &chopstick2);
    printf("Philosopher-1 Dropped Right Chopstick-2\n");

    // check if there is still food
    {
      PT_SEM_WAIT(pt, &food_lock);
      if (food > 0) {
	food--;
      }
      myfood = food;

      printf("myfood is %d\n", myfood);
      
      eaten = 1;

      PT_SEM_SIGNAL(pt, &food_lock);
    }
  }  

  if (eaten)
    printf ("Philosopher %d is done eating.\n", id);
  else
    printf ("Philosopher %d has starved to death.\n", id);

  PT_END(pt);
}

static
PT_THREAD(philosopher2(struct pt *pt))
{
  static int id;
  static int left_chopstick, right_chopstick;
  static int myfood;
  static int eaten;

  PT_BEGIN(pt);

  id = 2; // hard-coded in the function name
  printf ("Philosopher %d is done thinking and now ready to eat.\n", id);
  right_chopstick = id;
  left_chopstick = id + 1;
  
  /* Wrap around the chopsticks. */
  if (left_chopstick == PHILOS)
    left_chopstick = 0;

  PT_SEM_WAIT(pt, &food_lock);

  if (food > 0) {
    food--;
  }
  myfood = food;

  printf("myfood is %d\n", myfood);

  PT_SEM_SIGNAL(pt, &food_lock);


  while (myfood) {
  	
    /* Thanks to philosophers #1 who would like to take a nap
     * before picking up the chopsticks, the other philosophers
     * may be able to eat their dishes and not deadlock.  
     */
    if (id == 1)
      sleep (sleep_seconds);
    
    PT_SEM_WAIT(pt, &chopstick2);
    printf("Philosopher-2 got Left Chopstick-2\n");
    //    grab_chopstick (id, right_chopstick, "right ", pt);
    //    grab_chopstick (id, left_chopstick, "left", pt);
    PT_SEM_WAIT(pt, &chopstick3);
    printf("Philosopher-2 got Right Chopstick-3\n");
	
    printf ("Philosopher %d: eating.\n", id);
    usleep (DELAY * (FOOD - myfood + 1));
    //    down_chopsticks (left_chopstick, right_chopstick, pt);
    PT_SEM_SIGNAL(pt, &chopstick2);
    printf("Philosopher-2 Dropped Left Chopstick-2\n");
    PT_SEM_SIGNAL(Pt, &chopstick3);
    printf("Philosopher-2 Dropped Right Chopstick-3\n");

    // check if there is still food
    {
      PT_SEM_WAIT(pt, &food_lock);
      if (food > 0) {
	food--;
      }
      myfood = food;
      
      eaten = 1;

      printf("myfood is %d\n", myfood);
      PT_SEM_SIGNAL(pt, &food_lock);
    }
  }
  

  if (eaten)
    printf ("Philosopher %d is done eating.\n", id);
  else
    printf ("Philosopher %d has starved to death.\n", id);

  PT_END(pt);
}

static
PT_THREAD(philosopher3(struct pt *pt))
{
  static int id;
  static int left_chopstick, right_chopstick;
  static int myfood;
  static int eaten;

  PT_BEGIN(pt);

  id = 3; // hard-coded in the function name
  printf("Philosopher %d is done thinking and now ready to eat.\n", id);
  right_chopstick = id;
  left_chopstick = id + 1;
  
  /* Wrap around the chopsticks. */
  if (left_chopstick == PHILOS)
    left_chopstick = 0;

  PT_SEM_WAIT(pt, &food_lock);

  if (food > 0) {
    food--;
  }
  myfood = food;

  printf("myfood is %d\n", myfood);
  PT_SEM_SIGNAL(pt, &food_lock);


  while (myfood) {
  	
    /* Thanks to philosophers #1 who would like to take a nap
     * before picking up the chopsticks, the other philosophers
     * may be able to eat their dishes and not deadlock.  
     */
    if (id == 1)
      sleep (sleep_seconds);
    
    PT_SEM_WAIT(pt, &chopstick3);
    printf("Philosopher-3 got Left Chopstick-3\n");
    //    grab_chopstick (id, right_chopstick, "right ", pt);
    //    grab_chopstick (id, left_chopstick, "left", pt);
    PT_SEM_WAIT(pt, &chopstick4);
    printf("Philosopher-3 got Right Chopstick-4\n");
	
    printf ("Philosopher %d: eating.\n", id);
    usleep (DELAY * (FOOD - myfood + 1));
    //    down_chopsticks (left_chopstick, right_chopstick, pt);
    PT_SEM_SIGNAL(pt, &chopstick3);
    printf("Philosopher-3 Dropped Lef Chopstick-3\n");
    PT_SEM_SIGNAL(Pt, &chopstick4);
    printf("Philosopher-3 Dropped Right Chopstick-4\n");

    // check if there is still food
    {
      PT_SEM_WAIT(pt, &food_lock);
      if (food > 0) {
	food--;
      }
      myfood = food;

      printf("myfood value is %d\n", myfood);

      eaten = 1;

      PT_SEM_SIGNAL(pt, &food_lock);
    }
  }
  

  if (eaten)
    printf ("Philosopher %d is done eating.\n", id);
  else
    printf ("Philosopher %d has starved to death.\n", id);

  PT_END(pt);
}

static
PT_THREAD(philosopher4(struct pt *pt))
{
  static int id;
  static int left_chopstick, right_chopstick;
  static int myfood;
  static int eaten;

  PT_BEGIN(pt);

  id = 4; // hard-coded in the function name
  printf ("Philosopher %d is done thinking and now ready to eat.\n", id);
  right_chopstick = id;
  left_chopstick = id + 1;
  
  /* Wrap around the chopsticks. */
  if (left_chopstick == PHILOS)
    left_chopstick = 0;

  PT_SEM_WAIT(pt, &food_lock);

  if (food > 0) {
    food--;
  }
  myfood = food;
  
  printf("myfood is %d\n", myfood);
  PT_SEM_SIGNAL(pt, &food_lock);


  while (myfood) {
  	
    /* Thanks to philosophers #1 who would like to take a nap
     * before picking up the chopsticks, the other philosophers
     * may be able to eat their dishes and not deadlock.  
     */
    if (id == 1)
      sleep (sleep_seconds);
    
    PT_SEM_WAIT(pt, &chopstick4);
    printf("Philosopher-4 got Left Chopstick-4\n");
    //    grab_chopstick (id, right_chopstick, "right ", pt);
    //    grab_chopstick (id, left_chopstick, "left", pt);
    PT_SEM_WAIT(pt, &chopstick1);
    printf("Philosopher-4 got Right Chopstick-1\n");
	
    printf ("Philosopher %d: eating.\n", id);
    usleep (DELAY * (FOOD - myfood + 1));
    //    down_chopsticks (left_chopstick, right_chopstick, pt);
    PT_SEM_SIGNAL(pt, &chopstick4);
    printf("Philosopher-4 Dropped Left Chopstick-4\n");
    PT_SEM_SIGNAL(Pt, &chopstick1);
    printf("Philosopher-4 Dropped Right Chopstick-1\n");

    // check if there is still food
    {
      PT_SEM_WAIT(pt, &food_lock);
      if (food > 0) {
	food--;
      }
      myfood = food;
    
      eaten = 1;

      printf("myfood is %d\n", myfood);
      PT_SEM_SIGNAL(pt, &food_lock);
    }
  }
  
  if (eaten)
    printf ("Philosopher %d is done eating.\n", id);
  else
    printf ("Philosopher %d has starved to death.\n", id);

  PT_END(pt);
}

static
PT_THREAD(driver_thread(struct pt *pt))
{
  static struct pt pt_philo1, pt_philo2, pt_philo3, pt_philo4;

  PT_BEGIN(pt);

  PT_SEM_INIT(&chopstick1, 1);
  PT_SEM_INIT(&chopstick2, 1);
  PT_SEM_INIT(&chopstick3, 1);
  PT_SEM_INIT(&chopstick4, 1);
  PT_SEM_INIT(&food_lock, 1);

  PT_INIT(&pt_philo1);
  PT_INIT(&pt_philo2);
  PT_INIT(&pt_philo3);
  PT_INIT(&pt_philo4);

  PT_WAIT_THREAD(pt, philosopher1(&pt_philo1) & philosopher2(&pt_philo2) & philosopher3(&pt_philo3) & philosopher4(&pt_philo4));

  PT_END(pt);
}


int
main (void)
{
  struct pt driver_pt;
  
  PT_INIT(&driver_pt);

  sleep_seconds = CLOCK_SECOND/2;

  while (PT_SCHEDULE(driver_thread(&driver_pt))) {
    usleep(10);
  }

  return 0;

  /* pthread_mutex_init (&food_lock, NULL); */
  /* for (i = 0; i < PHILOS; i++) */
  /*   pthread_mutex_init (&chopstick[i], NULL); */
  /* for (i = 0; i < PHILOS; i++) */
  /*   pthread_create (&philo[i], NULL, philosopher, (void *)i); */
  /* for (i = 0; i < PHILOS; i++) */
  /*   pthread_join (philo[i], NULL); */
  /* return 0; */
}
