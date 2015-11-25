/*   Copyright (c) 2008, Swedish Institute of Computer Science
 *  All rights reserved.
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the copyright holders nor the names of
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 *  \brief This module contains code to interface a Contiki-based
 *  project on the AVR Raven platform's ATMega1284P chip to the LCD
 *  driver chip (ATMega3290P) on the Raven.
 *  
 *  \author Durvy Mathilde <mdurvy@cisco.com>
 *
 */

/**  \addtogroup raven
 * @{ 
 */

/**
 *  \defgroup ravenserial Serial interface between Raven processors
 * @{
 */

/**
 *  \file
 *  This file contains code to connect the two AVR Raven processors via
 *  a serial connection for the IPSO interop application
 *
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include "mac.h"

#include "raven-lcd.h"

#include <string.h>
#include <stdio.h>

#define CMD_LEN 8
#define DATA_LEN 20

#define SERVER_PORT HTONS(0xF0B0)

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",lladdr->addr[0], lladdr->addr[1], lladdr->addr[2], lladdr->addr[3],lladdr->addr[4], lladdr->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

static uint8_t seqno;
static uip_ipaddr_t server_addr;
static uip_ipaddr_t locaddr;
static uint16_t addr[8];
static struct uip_udp_conn *send_conn;
static struct uip_udp_conn *node_conn;

static struct etimer periodic_timer;
static int periodic_interval;

static char udp_data[DATA_LEN];

static char last_temp[6];

static uint8_t invert_led;

static struct{
  u8_t frame[CMD_LEN];
  u8_t ndx;
  u8_t len;
  u8_t cmd;
  u8_t done;
} cmd;

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_ICMP_BUF ((struct uip_icmp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])

#define CMD_PING 0x81
#define CMD_TEMP 0x80

#define SOF_CHAR 1
#define EOF_CHAR 4

void rs232_send(uint8_t port, unsigned char c);


/*------------------------------------------------------------------*/
/* Sends a ping packet out the radio */
static void
raven_ping6(uip_ipaddr_t *addr, int len)
{
     
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = UIP_TTL;
  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, addr);
  uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);
 
  UIP_ICMP_BUF->type = ICMP6_ECHO_REQUEST;
  UIP_ICMP_BUF->icode = 0;
  /* set identifier and sequence number to 0 */
  memset((void *)UIP_ICMP_BUF + UIP_ICMPH_LEN, 0, UIP_ICMP6_ECHO_REQUEST_LEN);
  /* put len bytes of all one data */
  memset((void *)UIP_ICMP_BUF + UIP_ICMPH_LEN + UIP_ICMP6_ECHO_REQUEST_LEN,
         1, len);

  uip_len = UIP_ICMPH_LEN + UIP_ICMP6_ECHO_REQUEST_LEN + UIP_IPH_LEN + len;
  UIP_IP_BUF->len[0] = (u8_t)((uip_len - 40) >> 8);
  UIP_IP_BUF->len[1] = (u8_t)((uip_len - 40) & 0x00FF);
  
  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();
  
  tcpip_ipv6_output();
}

/*------------------------------------------------------------------*/
/* Send a serial command frame to the ATMega3290 Processor on Raven
   via serial port */
static void
send_frame(uint8_t cmd, uint8_t len, uint8_t *payload)
{
  uint8_t i;

  rs232_send(0, SOF_CHAR);    /* Start of Frame */
  rs232_send(0, len);
  rs232_send(0, cmd);
  for (i=0;i<len;i++)
    rs232_send(0,*payload++);
  rs232_send(0, EOF_CHAR);
}

/*------------------------------------------------------------------*/
/* Send a UDP packet to the specified destination */
static void
send_packet(uip_ipaddr_t *dest, uint16_t destport, void *data, int data_size)
{
  if(send_conn != NULL) {
    uip_ipaddr_copy(&send_conn->ripaddr, dest);
    send_conn->rport = destport;
    PRINTF("[APP] Sending UPD packet %s (%d) to", (char *)data, data_size);
    PRINT6ADDR(dest);
    PRINTF("\n");
    uip_udp_packet_send(send_conn, data, data_size);
  }
}

/*------------------------------------------------------------------*/
/* Reply to a UDP packet */
static void
reply_packet(void *data, int data_size)
{
  send_packet(&UIP_IP_BUF->srcipaddr, UIP_UDP_BUF->srcport,
              data, data_size);
}

/*------------------------------------------------------------------*/
/* Copy sensor data to local buffer */
static void
copy_value(uint8_t *packet_data, char *local_data, int max)
{
  /* Copy data until white space or end of string. */
  while(*packet_data > 32 && (--max > 0)) {
    *local_data++ = *packet_data++;
  }
  *local_data = 0;
}

/*------------------------------------------------------------------*/
static u8_t
raven_gui_loop(process_event_t ev, process_data_t data)
{
  uint8_t *ptr;
  int len = 0;
  
  if(ev == tcpip_event) {
    PRINTF("[APP] TCPIP event lport %u rport %u raddress\n",uip_udp_conn->lport, uip_udp_conn->rport);
    if(uip_udp_conn == node_conn) {
      PRINTF("[APP] UDP packet\n");
      if(uip_newdata()) {
        ptr = (uint8_t *)uip_appdata;
        /* Make sure the data is terminated */
        ptr[uip_datalen()] = 0;

        /* Command to this node */
        switch(*ptr) {
          case 'T':
            send_frame(TEMP_REQUEST, 0, 0);
            len = snprintf(udp_data,
                           DATA_LEN, "%c%s\r\n", *ptr, last_temp);
            if(len > 0) {
              reply_packet(udp_data, len);
            }
            break;
          case 'A':
            if(ptr[1] > 32) {
              invert_led = ptr[1] == '1';
              send_frame(LED_REQUEST, 1, &invert_led);
            }
            len = sprintf(udp_data, "A%u\r\n", invert_led ? 1 : 0);
            if(len > 0) {
              reply_packet(udp_data, len);
            }
            break;
          case 'P':
            PRINTF("[APP] Request for periodic sending of temperatures\n"); 
            if(sscanf((char *)ptr+1,"%d", &periodic_interval)){
              PRINTF("[APP] Period %d\n", periodic_interval);
              if(periodic_interval == 0){
                etimer_stop(&periodic_timer);
              } else {
                etimer_set(&periodic_timer, periodic_interval * CLOCK_SECOND);
              }
              len = snprintf(udp_data,
                             DATA_LEN, "P%d\r\n", periodic_interval);
              if(len > 0) {
                reply_packet(udp_data, len);
              }
            } 
            break;
          case 'C':
            PRINTF("[APP] Command %c\n",ptr[1]);
            if(sscanf((char *)ptr+2,
                      "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x %d",
                      &addr[0],&addr[1],&addr[2],&addr[3],&addr[4],
                      &addr[5],&addr[6],&addr[7],&len) >= 8){
              uip_ip6addr(&locaddr, addr[0], addr[1],addr[2],
                          addr[3],addr[4],addr[5],addr[6],addr[7]);
              if(ptr[1] == 'E'){
                PRINTF("[APP] Send ping to");
                PRINT6ADDR(&locaddr);
                PRINTF("len %d\n", len);
                raven_ping6(&locaddr, len);
              } else if (ptr[1] == 'T'){
                PRINTF("[APP] Send temperature to");
                PRINT6ADDR(&locaddr);
                PRINTF("\n");
                len = snprintf(udp_data, DATA_LEN, "%c\r\n", ptr[1]);
                if(len > 0) {
                  send_packet(&locaddr, HTONS(0xF0B0), udp_data, len);
                }
              } else {
                PRINTF("[APP] Command unknown\n");
              }
            }
            break;
          default:
            break;
        }
      }
    } 
  } else {
    switch (ev){
      case ICMP6_ECHO_REQUEST:
        /* We have received a ping request over the air.
           Send frame back to 3290 */
        send_frame(PING_REQUEST, 0, 0);
        break;
      case ICMP6_ECHO_REPLY:
        /* We have received a ping reply over the air.
           Send frame back to 3290 */
        send_frame(PING_REPLY, 1, &seqno);
        break;
      case PROCESS_EVENT_TIMER:
        if(data == &periodic_timer) {
          PRINTF("[APP] Periodic Timer\n");
          /* Send to server if time to send */
          len = sprintf(udp_data, "T%s\r\n", last_temp);
          if(len > 0) {
            send_packet(&server_addr, SERVER_PORT, udp_data, len);
          }
          etimer_reset(&periodic_timer);
          send_frame(TEMP_REQUEST, 0, 0);
        }
        break;
      case PROCESS_EVENT_POLL:
        /* Check for command from serial port, execute it. */
        if (cmd.done){
          /* Execute the waiting command */
          switch (cmd.cmd){
            case CMD_PING:
              /* Send ping request over the air */
              seqno = cmd.frame[0];
              raven_ping6(&server_addr, 0);
              break;
            case CMD_TEMP:
              /* Copy sensor data until white space or end of
                 string. The ATMega3290 sends temperature with both
                 value and type (25 C) */
              copy_value(cmd.frame, last_temp, sizeof(last_temp));
              PRINTF("[APP] New temp value %s\n", last_temp);
              break;
            default:
              break;
          }
          /* Reset command done flag. */
          cmd.done = 0;
        }
        break;
      default:
        break;
    }
  }
  return 0;
}

/*--------------------------------------------------------------------*/
/* Process an input character from serial port.  
 * ** This is called from an ISR!!
 */
int raven_lcd_serial_input(unsigned char ch)
{
  /* Parse frame,  */
  switch (cmd.ndx){
    case 0:
      /* first byte, must be 0x01 */
      cmd.done = 0;
      if (ch != 0x01){
        return 0;
      }
      break;
    case 1: 
      /* Second byte, length of payload */
      cmd.len = ch;
      break;
    case 2:
      /* Third byte, command byte */
      cmd.cmd = ch;
      break;
    default:
      /* Payload and ETX */
      if (cmd.ndx >= cmd.len+3){
        /* all done, check ETX */
        if (ch == 0x04){
          cmd.done = 1;
          PRINTF("[APP] Serial input\n");
          process_poll(&raven_lcd_process);
          /*     process_post(&raven_lcd_process, SERIAL_CMD, 0); */
        } else {
          /* Failed ETX */
          cmd.ndx = 0;
        }
      } else {
        /* Just grab and store payload */
        cmd.frame[cmd.ndx - 3] = ch;
      }
      break;
  }

  cmd.ndx++;

  return 0;
}

/*---------------------------------------------------------------------------*/
PROCESS(raven_lcd_process, "Raven LCD interface process");
PROCESS_THREAD(raven_lcd_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize the sensor data history */
  memcpy(last_temp, "20.0", sizeof("20.0"));

  /*Create a udp connection to the server*/
  uip_ip6addr(&server_addr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);

  /* set destination parameters */
  send_conn = udp_new(&server_addr, SERVER_PORT, NULL);
  /*set local port */
  udp_bind(send_conn, HTONS(0xF0B0+1));
  
  /* Listen to port 0xF0B0 for commands */
  node_conn = udp_new(NULL, 0, NULL);
  udp_bind(node_conn, HTONS(0xF0B0));

  if(icmp6_new(NULL) != NULL) {
    while(1) {
      PROCESS_YIELD();
      raven_gui_loop(ev, data);
    } 
  }
  PROCESS_END();
}
/** @} */
/** @} */
