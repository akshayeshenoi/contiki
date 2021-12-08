// /**
//  * \addtogroup timesynch
//  * @{
//  */


/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *        Resilient Clustering in Contiki
 * \author
 *         Nitin Shivaraman <nitin.shivaraman@tum-create.edu.sg>
 */

#include "contiki.h"
#include "random.h"
#include "net/rime/rime.h"
#include "dev/cc2420/cc2420.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "sys/rtimer.h"
#include "node-id.h"
#include "apps/powertrace/powertrace.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include <stdio.h>

#include "project-conf.h"


#define FIXED_ROOT_TIMEOUT         3      // number of iterations after root is fixed to stop transmission
#define ERROR_VALID_THRESHOLD      3      // Threshold of error from consensus to initiate sleep
#define CYCLE_LIMIT                6      // Number of rounds for a non-CH to transmit
#define NODE_FAIL_LIMIT            6      // Number of cycles to know if there a node failure
#define ROUND_LIMIT                1      // Number of seconds/transmission window for all nodes to trasmit atleast once
#define FALSE                      0      // Error state
#define TRUE                       1      // OK state
#define PROTOCOL_DISPATCH          0x01   // Identifier for the protocol
#define UNKNOWN_NODE               0xFF   // Default value of a node
#define RSSI_THRESHOLD             65     // Signal strength for neighbours
#define SKEW_ERR_THRESHOLD         10     // The threshold after which consensus is stopped
#define ROOT_RESET_LIMIT           5      // Iterations to wait if there is a root failure
#define RSSI_OFFSET                -45    // As mentioned in the datasheet of cc2420
#define MAX_NEIGHBOURS             64     // Maximum number of neighbour nodes for each node
#define MAX_ACTIVE                 24     // Maximum number of nodes that is tracked for active nodes
#define XMISSION_ROUND             1      // Number of rounds each node can complete transmission
#define XFER_CHANNEL               7      // Channel used for data transfer
#define EPOCH                      10     // Number of rounds per epoch
// A message is sent every 1.1 seconds by the root
// #define MIN_INTERVAL CLOCK_SECOND * 0.6
#define MIN_INTERVAL CLOCK_SECOND * 2

/**
 *  Message format
 */
struct message_t
{
    char msg[10];
};

PROCESS(deric_process, "Clustering process");
AUTOSTART_PROCESSES(&deric_process);


/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
    // synchronization message
    struct message_t msg;
    int16_t received_rssi = cc2420_last_rssi;
    received_rssi += RSSI_OFFSET;

    /* The packetbuf_dataptr() returns a pointer to the first data byte
       in the received packet. */
    memcpy(&msg, packetbuf_dataptr(), sizeof(struct message_t));
    printf("Received RSSI is %d\n", received_rssi);

    // if(abs(received_rssi) > RSSI_THRESHOLD) {
    //     // do something 
    //     printf("BYTE RECV: %s ", msg.msg);
    //     printf("SIZE RECV: %d\n", packetbuf_datalen());
    // }

    printf("BYTE RECV: %s", msg.msg);
    printf("SIZE RECV: %d\n", packetbuf_datalen());
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(deric_process, ev, data)
{
  static struct etimer sendtimer;
  static clock_time_t interval;
  struct message_t beacon;


  /* Variables for residual energy */
  // uint16_t seconds=100;// warning: if this variable is changed, then the kinect variable the count the minutes should be changed
  // double fixed_perc_energy = 0.002;// 0 - 1
  // uint16_t variation = 2;//0 - 99


  // static uint16_t residual = 0;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
  PROCESS_BEGIN();

  //powertrace_start(CLOCK_SECOND * seconds, seconds, fixed_perc_energy, variation);
//   powertrace_start(CLOCK_SECOND * 10);

  broadcast_open(&broadcast, XFER_CHANNEL, &broadcast_call);
  // node_id_burn(node_id); // To be used for hardware

  interval = MIN_INTERVAL;

  while(1)
  {

    etimer_set(&sendtimer, interval);

    PROCESS_WAIT_UNTIL(etimer_expired(&sendtimer));

    // residual = powertrace_getresidual();

    // if(!residual)
    // {
    //     printf("Residual energy of node %d is 0\n", node_id);
    //     break;
    // }

    memcpy(&beacon, "Hello!", 6);

    printf("String: %s\n", beacon.msg);
    /* Send the packet */
    packetbuf_copyfrom(&beacon, sizeof(beacon));
    broadcast_send(&broadcast);
  }
  PROCESS_END();
  return 1;
}
