#include "contiki.h"
#include <stdio.h>
#include "sys/mt.h"

PROCESS(mt_process, "multithreading example");
AUTOSTART_PROCESSES(&mt_process);

void
 thread_entry(void *data)
 {
   for(;;) {
     printf("Looping in thread_entry\n");
     mt_yield();
   }
 }

 PROCESS_THREAD(mt_process, ev, data)
 {
   static struct mt_thread thread;
   static int counter;

   PROCESS_BEGIN();
   mt_start(&thread, thread_entry, NULL);
   for(;;) {
     PROCESS_PAUSE();
     mt_exec(&thread);
     if(++counter == 10) {
       printf("Stopping the thread after %d calls\n",
              counter);
       mt_stop(&thread);
       break;
     }
   }
   PROCESS_END();
 }
