#include "contiki.h"
#include <stdio.h>
#include "sys/ctimer.h"
#include "sys/timer.h"
#include "cfs/cfs.h"

PROCESS(callback_example, "Callback example");
AUTOSTART_PROCESSES(&callback_example);

struct ctimer c;

#define MY_TIMEOUT 3 * CLOCK_SECOND

static void
my_timer_callback(void *in)
{
    printf("\nIn the callback!\nYou'll need to wait for 5 seconds\n");

	struct timer timeMe;
	
	//set timer for five seconds
	timer_set(&timeMe, CLOCK_SECOND * 5);
	
	while (1)
	{
		if ( timer_expired(&timeMe) )
		{
			printf("\ntimer has expired after 5 Seconds\n");
			break;
		}
	}
    
    int fd;
	char buf[] = "Hello, World!";


	fd = cfs_open("test", CFS_READ | CFS_WRITE);
	if(fd >= 0) {
	  cfs_write(fd, buf, sizeof(buf));
	  cfs_seek(fd, 0, CFS_SEEK_SET);
	  cfs_read(fd, buf, sizeof(buf));
	  
	  printf("\nRead message: %s\n", buf);
	  cfs_close(fd);
	}
	
	exit(1);
}

PROCESS_THREAD(callback_example, ev, data)
{
    PROCESS_BEGIN();

    printf("Starting up!");

    ctimer_set(&c, MY_TIMEOUT, my_timer_callback, (void *)NULL);   
    
    PROCESS_END();
}
