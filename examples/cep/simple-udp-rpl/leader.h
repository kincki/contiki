#include "contiki.h"
#include "lib/random.h"

#include <stdio.h>
#include <string.h>



/*-------------------------LEADERSHIP DATA TYPES--------------------------------------*/
/*
 * This enumaration defines MESSAGE_TYPES used when exchaning leadership status 
 * in a broadcast message
 */
typedef enum {
  LDR_BEACON = 10, 	// VERY FIRST BEACON MESSAGE FOR THIS LEADER
  LDR_REFRESH = 20, 	// FOLLOW-UP MESSAGES FOR SAME LEADER	
  LDR_REVOKE = 30,	// REVOKE UNREACHABLE LEADER	
  LDR_CONFLICT = 40    // RELAY MESSAGE FOR LEADER BY SOME OTHER MOTE
} eLdrMsgType;

/* 
 * This structure represents the data type that is used in xferring leadership beacons.
 * @param <refreshTime> assumes that all motes are time synchronized.
 */
typedef struct {
  uint8_t leader;	// the id of leader mote
  uint8_t lSource; // the source of leadership beacon
  clock_time_t refreshTime;
  eLdrMsgType lMsgType; // determines the type of message 	
} strLeader;

/*
 * A data structure to represent conflicting leadership situations
 */
typedef struct {
  uint8_t confLeader; // conflicting leader ID
  uint8_t sourceID; // who reported this conflict
  struct etimer confTimer;
  bool bSet; // determines if timer is set
} confStr;

/*-----------------------Function Prototypes--------------------------*/

// This method is used for sending out leadership beacons that contains
// leadership structure contents.
void send_leadership_beacon(strLeader*);

// This method sets the Leader structure with new values and current clock_time
void set_leader(uint8_t, uint8_t, eLdrMsgType, clock_time_t);

// This method retrieves the leader id from the leader struct
uint8_t get_leader_id();

// This method retrieves the source of leadership structure
uint8_t get_source_id();

// This method retrieves the refresh time from the leadership structure
clock_time_t get_refreshTime();

// This method checks whether the incoming message is fresh
bool is_msg_fresh(clock_time_t);

// This method retrieves the leadership message type
uint8_t get_leader_status();

// This method converts a strLeader struct to a String
void leader2String(const strLeader sLeader, char *buf);

// This method converts a String to a strLeader struct
void string2Leader(const char *buf, strLeader *sLeader);
