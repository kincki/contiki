
#include "contiki.h"
#include "sys/rtimer.h"

#include "dev/leds.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*/
PROCESS(test_rtimer_process, "Test RT");
AUTOSTART_PROCESSES(&test_rtimer_process);

struct fade {
  struct rtimer rt;
  struct pt pt;
  int led;
  rtimer_clock_t ontime, offtime;
  int addend;
};
/*---------------------------------------------------------------------------*/
static char
fade(struct rtimer *t, void *ptr)
{
  struct fade *f = ptr;

  PT_BEGIN(&f->pt);

  while(1) {
    leds_on(f->led);
    rtimer_set(t, RTIMER_TIME(t) + f->ontime, 1,
	       (rtimer_callback_t)fade, ptr);
    PT_YIELD(&f->pt);
    
    leds_off(f->led);
    rtimer_set(t, RTIMER_TIME(t) + f->offtime, 1,
	       (rtimer_callback_t)fade, ptr);

    f->ontime += f->addend;
    f->offtime -= f->addend;
    if(f->offtime <= 4 || f->offtime >= 100) {
      f->addend = -f->addend;
    }
    PT_YIELD(&f->pt);
  }

  PT_END(&f->pt);
}
/*---------------------------------------------------------------------------*/

static struct rtimer r,r2;

void func(struct rtimer *t, void *ptr);
void func2(struct rtimer *t, void *ptr);

PROCESS_THREAD(test_rtimer_process, ev, data)
{
  
  PROCESS_BEGIN();
  
  rtimer_init();
  
  
  rtimer_set(&r, RTIMER_NOW()+ RTIMER_SECOND, 1, func, NULL);
  rtimer_set(&r2, RTIMER_NOW()+ RTIMER_SECOND/4, 1, func2, NULL);
  
  
  while(1) {
    PROCESS_WAIT_EVENT();
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

void func(struct rtimer *t, void *ptr)
{
  printf("FUNC %ld\r\n",(unsigned long)RTIMER_NOW());
}

void func2(struct rtimer *t, void *ptr)
{
  printf("FUNC2 %ld\r\n",(unsigned long)RTIMER_NOW());
}