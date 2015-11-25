#ifndef __NETWORK_DRIVER_H__
#define __NETWORK_DRIVER_H__

#define COOJA_RADIO_BUFSIZE 128

typedef struct {
  char *simReceiving;
  int *simInSize;
  char *simInDataBuffer[];
} in_buffer;

/* drop packet is defined in KleeNet failure model */
extern unsigned drop_packet[100];
extern unsigned drop_subsequent_packet[100];
extern unsigned duplicate_packet[100];
extern unsigned reboot_node[100];
extern unsigned halt_node[100];
extern unsigned node_failure_delay[100];

#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)

#define MOTE_EXTERNS(id)                         \
extern int mote##id##_simMoteID;                 \
extern char mote##id##_simMoteIDChanged;         \
extern int mote##id##_simRandomSeed;             \
extern unsigned long mote##id##_simTimeDrift;    \
extern char mote##id##_simReceiving;             \
extern int mote##id##_simInSize;                 \
extern char mote##id##_simInDataBuffer[COOJA_RADIO_BUFSIZE]; \
in_buffer mote##id##_in_buffer =                 \
  { &mote##id##_simReceiving, &mote##id##_simInSize, {mote##id##_simInDataBuffer} };
#define MOTE_CASE(id, seed, drift, drop, subsequent_drop, duplicate, reboot, halt, delay) \
case id:                                         \
mote##id##_simMoteID = id;                       \
mote##id##_simRandomSeed = seed + id;            \
mote##id##_simTimeDrift = -drift/1000;           \
mote##id##_simMoteIDChanged = 1;                 \
kleenet_set_node_id(id);                         \
unsigned mote##id##_drop;                        \
klee_make_symbolic(&mote##id##_drop,             \
  sizeof(mote##id##_drop), QUOTEME(mote##id##_drop)); \
klee_assume(mote##id##_drop <= drop);            \
drop_packet[id] = mote##id##_drop;               \
unsigned mote##id##_subsequent_drop;             \
klee_make_symbolic(&mote##id##_subsequent_drop,  \
  sizeof(mote##id##_subsequent_drop), QUOTEME(mote##id##_subsequent_drop)); \
klee_assume(mote##id##_subsequent_drop <= subsequent_drop); \
drop_subsequent_packet[id] = mote##id##_subsequent_drop; \
unsigned mote##id##_duplicate;             \
klee_make_symbolic(&mote##id##_duplicate,  \
  sizeof(mote##id##_duplicate), QUOTEME(mote##id##_duplicate)); \
klee_assume(mote##id##_duplicate <= duplicate); \
duplicate_packet[id] = mote##id##_duplicate; \
unsigned mote##id##_reboot;                      \
klee_make_symbolic(&mote##id##_reboot,           \
  sizeof(mote##id##_reboot), QUOTEME(mote##id##_reboot)); \
klee_assume(mote##id##_reboot <= reboot);        \
reboot_node[id] = mote##id##_reboot;             \
unsigned mote##id##_halt;                      \
klee_make_symbolic(&mote##id##_halt,           \
  sizeof(mote##id##_halt), QUOTEME(mote##id##_halt)); \
klee_assume(mote##id##_halt <= halt);        \
halt_node[id] = mote##id##_halt;             \
unsigned mote##id##_failure_delay;               \
klee_make_symbolic(&mote##id##_failure_delay,    \
  sizeof(mote##id##_failure_delay), QUOTEME(mote##id##_failure_delay)); \
klee_assume(mote##id##_failure_delay == delay);  \
node_failure_delay[id] = mote##id##_failure_delay; \
kleenet_schedule_boot_state(-drift);             \
extern int mote##id##_main(int, char**);         \
mote##id##_main(argc, argv);                     \
break

/* Defines ALL_MOTE_EXTERNS(), ALL_MOTE_CASES(), ALL_IN_BUFFERS(),
 * DGRM_NR_LINKS, DGRM_LINKS_FROM(), and DGRM_LINKS_TO() */
#include "network_driver_conf.h"

#endif /* __NETWORK_DRIVER_H__ */
