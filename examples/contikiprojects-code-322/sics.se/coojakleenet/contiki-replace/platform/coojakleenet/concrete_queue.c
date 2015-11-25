#include "contiki.h"
#include "concrete_queue.h"

#include "sys/cooja_mt.h"

#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Concrete failures values */
extern int failure_delay[];
extern int reboot_once[];
extern int halt_once[];
extern int mote_reboot[];
extern int mote_halt[];
extern int packet_drop[];
extern int packet_subsequent_drop[];
extern int packet_duplicate[];
/*---------------------------------------------------------------------------*/
/* Variables accessed from COOJA */
int concrete_queue_monitor_active = 0;
int concrete_queue_requesting = 0;
int concrete_queue_dest_mote = 0;
char* concrete_queue_lastvar[64]; /* XXX */
char* concrete_queue_lastvalue[2048]; /* max sym buffer */
int concrete_queue_error = 0;
/*---------------------------------------------------------------------------*/
void
concrete_queue_getnext(void* address, int size, char* varname)
{
  if (!concrete_queue_monitor_active) {
    printf("warning: no symbolic access monitor: unknown concrete value (-1)\n");
    return;
  }
  if (size > 2048) {
    printf("error: max sym buffer size is 2048!\n");
    return;
  }
  /* Request value from monitor (should return immediately) */
  concrete_queue_requesting = 1;
  memset(concrete_queue_lastvar, 0, 64);
  memset(concrete_queue_lastvalue, 0, 1024);
  memcpy(concrete_queue_lastvar, varname, strlen(varname));
  while (concrete_queue_requesting) {
    cooja_mt_yield();
  }
  /* Copy result */
  memcpy(address, concrete_queue_lastvalue, size);
}
/*---------------------------------------------------------------------------*/
int concrete_queue_getsymbol(int dest_mote_id, char* varname)
{
  /* XXX: only integers supported */
  int result;
  concrete_queue_dest_mote = dest_mote_id;
  concrete_queue_getnext(&result, sizeof(result), varname);
  return result;
}
/*---------------------------------------------------------------------------*/
void
concrete_queue_init(int simMoteID)
{
  char buffer[64];
  /* Request packet drop value */
  memset(buffer, 0, 64);
  memset(concrete_queue_lastvalue, 0, 1024);
  sprintf(buffer, "mote%d_drop", simMoteID);
  concrete_queue_requesting = 1;
  memset(packet_drop, 0, 1000);
  concrete_queue_getnext(&packet_drop[simMoteID], sizeof(packet_drop[0]), buffer);
  /* Request subsequent packet drop value */
  memset(buffer, 0, 64);
  memset(concrete_queue_lastvalue, 0, 1024);
  sprintf(buffer, "mote%d_subsequent_drop", simMoteID);
  concrete_queue_requesting = 1;
  memset(packet_subsequent_drop, 0, 1000);
  concrete_queue_getnext(&packet_subsequent_drop[simMoteID],
    sizeof(packet_subsequent_drop[0]), buffer);
  /* Request packet duplicate value */
  memset(buffer, 0, 64);
  memset(concrete_queue_lastvalue, 0, 1024);
  sprintf(buffer, "mote%d_duplicate", simMoteID);
  concrete_queue_requesting = 1;
  memset(packet_duplicate, 0, 1000);
  concrete_queue_getnext(&packet_duplicate[simMoteID], sizeof(packet_duplicate[0]), buffer);
  /* Request mote reboot value */
  memset(buffer, 0, 64);
  memset(concrete_queue_lastvalue, 0, 1024);
  sprintf(buffer, "mote%d_reboot", simMoteID);
  concrete_queue_requesting = 1;
  memset(mote_reboot, 0, 1000);
  memset(reboot_once, 1, 1000);
  concrete_queue_getnext(&mote_reboot[simMoteID], sizeof(mote_reboot[0]), buffer);
  /* Request mote halt value */
  memset(buffer, 0, 64);
  memset(concrete_queue_lastvalue, 0, 1024);
  sprintf(buffer, "mote%d_halt", simMoteID);
  concrete_queue_requesting = 1;
  memset(mote_halt, 0, 1000);
  memset(halt_once, 1, 1000);
  concrete_queue_getnext(&mote_halt[simMoteID], sizeof(mote_halt[0]), buffer);
  /* Request mote failure delay */
  memset(buffer, 0, 64);
  memset(concrete_queue_lastvalue, 0, 1024);
  sprintf(buffer, "mote%d_failure_delay", simMoteID);
  concrete_queue_requesting = 1;
  memset(failure_delay, 0, 1000);
  concrete_queue_getnext(&failure_delay[simMoteID], sizeof(failure_delay[0]), buffer);
}
/*---------------------------------------------------------------------------*/
