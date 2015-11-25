#include "network_driver.h"

#include <stdio.h>

/* extern int mote1_simMoteID;
 * extern char mote1_simMoteIDChanged;
 * extern int mote1_simInSize;
 * extern char mote1_simInDataBuffer[COOJA_RADIO_BUFSIZE];
 * in_buffer mote1_in_buffer = { &mote1_simInSize, mote1_simInDataBuffer }; */
ALL_MOTE_EXTERNS();

/* in_buffer * const in_buffers[] = { 0, &mote1_in_buffer, &mote2_in_buffer }; */
ALL_IN_BUFFERS();

/* Exported DGRM configuration */
int dgrm_nr_links = DGRM_NR_LINKS;
int dgrm_from[] = DGRM_LINKS_FROM();
int dgrm_to[] = DGRM_LINKS_TO();

/*---------------------------------------------------------------------------*/
int
dgrm_has_link(int from, int to)
{
  int i;
  for(i=0; i < dgrm_nr_links; i++) {
    if(dgrm_from[i] == from && dgrm_to[i] == to) {
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
dgrm_print_links(void)
{
  int i;
  for(i=0; i < dgrm_nr_links; i++) {
    printf("DGRM: %i -> %i\n", dgrm_from[i], dgrm_to[i]);
  }
}
/*---------------------------------------------------------------------------*/
int main(int argc, char **argv) {

  /*dgrm_print_links();*/

  int sym = klee_int("sym");
  switch (sym) {
    /* case 1:
     * mote1_simMoteID = 1;
     * ...
     * mote1_main();
     * break;
     */
    ALL_MOTE_CASES();

    default:
      klee_silent_exit(0);
  }
  return 0;
}
