
#ifndef __FAILURE_H__
#define __FAILURE_H__

#define MAX_NODES 1000

int failure_delay(void);
int failure_corrupt_packet(void);
int failure_drop_packet(void);
int failure_duplicate_packet(void);
int failure_halt_node(void);
int failure_reboot_node(void);

unsigned node_failure_delay[MAX_NODES];
unsigned corrupt_packet[MAX_NODES];
unsigned drop_packet[MAX_NODES];
unsigned drop_subsequent_packet[MAX_NODES];
unsigned duplicate_packet[MAX_NODES];
unsigned halt_node[MAX_NODES];
unsigned reboot_node[MAX_NODES];

#endif /* __FAILURE_H__ */

