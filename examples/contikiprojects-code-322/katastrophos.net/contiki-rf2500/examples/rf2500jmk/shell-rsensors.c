/**
 * \file
 *         rsensors Contiki shell command
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "shell.h"

#include "dev/cc2500-jmk.h"


/*---------------------------------------------------------------------------*/
PROCESS(shell_rsensors_process, "rsensors");
SHELL_COMMAND(rsensors_command,
	      "rsensors",
	      "rsensors: remote sensors",
	      &shell_rsensors_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_rsensors_process, ev, data)
{	
  PROCESS_BEGIN();

  shell_output_str(&rsensors_command, "Send command...", "");

  cc2500_send_frame("rsensors", 8);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
shell_rsensors_init(void)
{
  shell_register_command(&rsensors_command);
}
/*---------------------------------------------------------------------------*/
