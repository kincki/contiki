
#include "failure.h"

enum { FALSE = 0, TRUE = 1 };

int failure_delay(void) {
  return node_failure_delay[kleenet_get_node_id()];
}

int failure_corrupt_packet() {
  if (--corrupt_packet[kleenet_get_node_id()] == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

int failure_drop_packet() {
  static int dropped_single = 0;
  static int dropped_subsequent = 0;
  /* drop single packet */
  if (dropped_single && !dropped_subsequent) {
    /* drop subsequent packets */
    if (drop_subsequent_packet[kleenet_get_node_id()]-- > 0) {
      return TRUE;
    } else {
      dropped_subsequent = 1;
      return FALSE;
    }
  } else if (!dropped_single) {
    if (--drop_packet[kleenet_get_node_id()] == 0) {
      dropped_single = 1;
      return TRUE;
    } else {
      return FALSE;
    }
  } else {
    return FALSE;
  }
}

int failure_duplicate_packet() {
  if (--duplicate_packet[kleenet_get_node_id()] == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

int failure_halt_node() {
  if (--halt_node[kleenet_get_node_id()] == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

int failure_reboot_node() {
  if (--reboot_node[kleenet_get_node_id()] == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

