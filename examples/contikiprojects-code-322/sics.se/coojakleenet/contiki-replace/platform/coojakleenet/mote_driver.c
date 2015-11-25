#include <stdio.h>
#include <string.h>

#include "contiki.h"

#include "lib/simEnvChange.h"
#include "sys/clock.h"

/* This file should be linked with a COOJA Contiki library. */

extern int simMoteID;
int simProcessRunValue;
static int reboot_once = 1;
static int halt_once = 1;

clock_time_t simNextExpirationTime;
clock_time_t simTimeDrift;

extern clock_time_t simCurrentTime;

extern int failure_drop_packet(void);
extern int failure_reboot_node(void);
extern int failure_delay(void);

extern void contiki_init(void);

#define MILLISECOND 1000UL

#define PRINTF(...) /*printf(__VA_ARGS__)*/
#define NODE_TIME() (kleenet_get_virtual_time()/MILLISECOND - simTimeDrift)
#define SIM_TIME() (kleenet_get_virtual_time()/MILLISECOND)

static void exit_all_processes(void);

/*---------------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
  PRINTF(">> %s:main()\n", __FILE__);

start:
  /* Yield once during bootup */
  kleenet_schedule_state(MILLISECOND);
  kleenet_yield_state();
  simCurrentTime = NODE_TIME();

  PRINTF("> Executing Contiki at simulation time %lu\n", clock_time());

  /* Code from contiki-cooja-main.c:process_run_thread_loop (before loop) */
  doActionsBeforeTick();
  contiki_init();
  doActionsAfterTick();

  while (1) {

    /* reboot */
    if ((clock_seconds() >= failure_delay()) && reboot_once) {
      /* symbolic node reboot every 10 seconds */
      if (clock_seconds() % 10 == 0) {
        if (failure_reboot_node()) {
          PRINTF("main(): rebooting node once @ %lu, %lu seconds\n",
            clock_time(), clock_seconds());
          reboot_once = 0;
          exit_all_processes();
          goto start;
        }
      }
    }
    /* halt */
    if ((clock_seconds() >= failure_delay()) && halt_once) {
      /* symbolic node outage every 10 seconds */
      if (clock_seconds() % 10 == 0) {
        if (failure_halt_node()) {
          PRINTF("main(): halting node once @ %lu, %lu seconds\n",
            clock_time(), clock_seconds());
          halt_once = 0;
          exit_all_processes();
          /* loop forerver */
          while(1) {
            kleenet_schedule_state(MILLISECOND);
            kleenet_yield_state();
          }
        }
      }
    }

    /* Update time */
    simCurrentTime = NODE_TIME();
    simProcessRunValue = 0;

    /* Do actions before tick */
    doActionsBeforeTick();

    /* Poll etimer process */
    if (etimer_pending()) {
      etimer_request_poll();
    }

    /* Code from contiki-cooja-main.c:process_run_thread_loop */
    simProcessRunValue = process_run();
    while(simProcessRunValue-- > 0) {
      process_run();
    }
    simProcessRunValue = process_nevents();

    /* Do actions after tick */
    doActionsAfterTick();

    /* Request next tick for remaining events / timers */
    if (simProcessRunValue != 0) {
      kleenet_schedule_state(MILLISECOND);
      kleenet_yield_state();
      continue;
    }

    /* Request tick next wakeup time */
    if (etimer_pending()) {
      simNextExpirationTime = etimer_next_expiration_time() - simCurrentTime;
    } else {
      simNextExpirationTime = 0;
      PRINTF("WARNING: No more timers pending\n");
      kleenet_schedule_state(MILLISECOND);
      kleenet_yield_state();
      continue;
    }

    /* check next expiration time */
    if (simNextExpirationTime <= 0) {
      PRINTF("WARNING: Event timer already expired, but has been delayed: %lu\n",
        simNextExpirationTime);
      kleenet_schedule_state(MILLISECOND);
    } else {
      kleenet_schedule_state(simNextExpirationTime*MILLISECOND);
    }

    /* yield active state */
    kleenet_yield_state();

  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
exit_all_processes()
{
  struct process *q;
  for(q = process_list; q != NULL; q = q->next) {
    process_exit(q);
  }
}
/*---------------------------------------------------------------------------*/
