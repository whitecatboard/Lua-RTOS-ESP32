/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech-Cycleo

Description:
    LoRa concentrator : Just In Time TX scheduling queue

License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Michael Coracin
*/


#ifndef _LORA_PKTFWD_JIT_H
#define _LORA_PKTFWD_JIT_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <sys/time.h>   /* timeval */

#include "loragw_hal.h"
#include "loragw_gps.h"

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

#define JIT_QUEUE_MAX           32  /* Maximum number of packets to be stored in JiT queue */
#define JIT_NUM_BEACON_IN_QUEUE 3   /* Number of beacons to be loaded in JiT queue at any time */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC TYPES --------------------------------------------------------- */

enum jit_pkt_type_e {
    JIT_PKT_TYPE_DOWNLINK_CLASS_A,
    JIT_PKT_TYPE_DOWNLINK_CLASS_B,
    JIT_PKT_TYPE_DOWNLINK_CLASS_C,
    JIT_PKT_TYPE_BEACON
};

enum jit_error_e {
    JIT_ERROR_OK,           /* Packet ok to be sent */
    JIT_ERROR_TOO_LATE,     /* Too late to send this packet */
    JIT_ERROR_TOO_EARLY,    /* Too early to queue this packet */
    JIT_ERROR_FULL,         /* Downlink queue is full */
    JIT_ERROR_EMPTY,        /* Downlink queue is empty */
    JIT_ERROR_COLLISION_PACKET, /* A packet is already enqueued for this timeframe */
    JIT_ERROR_COLLISION_BEACON, /* A beacon is planned for this timeframe */
    JIT_ERROR_TX_FREQ,      /* The required frequency for downlink is not supported */
    JIT_ERROR_TX_POWER,     /* The required power for downlink is not supported */
    JIT_ERROR_GPS_UNLOCKED, /* GPS timestamp could not be used as GPS is unlocked */
    JIT_ERROR_INVALID       /* Packet is invalid */
};

struct jit_node_s {
    /* API fields */
    struct lgw_pkt_tx_s pkt;        /* TX packet */
    enum jit_pkt_type_e pkt_type;   /* Packet type: Downlink, Beacon... */

    /* Internal fields */
    uint32_t pre_delay;             /* Amount of time before packet timestamp to be reserved */
    uint32_t post_delay;            /* Amount of time after packet timestamp to be reserved (time on air) */
};

struct jit_queue_s {
    uint8_t num_pkt;                /* Total number of packets in the queue (downlinks, beacons...) */
    uint8_t num_beacon;             /* Number of beacons in the queue */
    struct jit_node_s nodes[JIT_QUEUE_MAX]; /* Nodes/packets array in the queue */
};

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

/**
@brief Check if a JiT queue is full.

@param queue[in] Just in Time queue to be checked.
@return true if queue is full, false otherwise.
*/
bool jit_queue_is_full(struct jit_queue_s *queue);

/**
@brief Check if a JiT queue is empty.

@param queue[in] Just in Time queue to be checked.
@return true if queue is empty, false otherwise.
*/
bool jit_queue_is_empty(struct jit_queue_s *queue);

/**
@brief Initialize a Just in Time queue.

@param queue[in] Just in Time queue to be initialized. Memory should have been allocated already.

This function is used to reset every elements in the allocated queue.
*/
void jit_queue_init(struct jit_queue_s *queue);

/**
@brief Add a packet in a Just-in-Time queue

@param queue[in/out] Just in Time queue in which the packet should be inserted
@param time[in] Current concentrator time
@param packet[in] Packet to be queued in JiT queue
@param pkt_type[in] Type of packet to be queued: Downlink, Beacon
@return success if the function was able to queue the packet

This function is typically used when a packet is received from server for downlink.
It will check if packet can be queued, with several criterias. Once the packet is queued, it has to be
sent over the air. So all checks should happen before the packet being actually in the queue.
*/
enum jit_error_e jit_enqueue(struct jit_queue_s *queue, struct timeval *time, struct lgw_pkt_tx_s *packet, enum jit_pkt_type_e pkt_type);

/**
@brief Dequeue a packet from a Just-in-Time queue

@param queue[in/out] Just in Time queue from which the packet should be removed
@param index[in] in the queue where to get the packet to be removed
@param packet[out] that was at index
@param pkt_type[out] Type of packet dequeued: Downlink, Beacon
@return success if the function was able to dequeue the packet

This function is typically used when a packet is about to be placed on concentrator buffer for TX.
The index is generally got using the jit_peek function.
*/
enum jit_error_e jit_dequeue(struct jit_queue_s *queue, int index, struct lgw_pkt_tx_s *packet, enum jit_pkt_type_e *pkt_type);

/**
@brief Check if there is a packet soon to be sent from the JiT queue.

@param queue[in] Just in Time queue to parse for peeking a packet
@param time[in] Current concentrator time
@param pkt_idx[out] Packet index which is soon to be dequeued.
@return success if the function was able to parse the queue. pkt_idx is set to -1 if no packet found.

This function is typically used to check in JiT queue if there is a packet soon to be sent.
It search the packet with the highest priority in queue, and check if its timestamp is near
enough the current concentrator time.
*/
enum jit_error_e jit_peek(struct jit_queue_s *queue, struct timeval *time, int *pkt_idx);

/**
@brief Debug function to print the queue's content on console

@param queue[in] Just in Time queue to be displayed
@param show_all[in] Indicates if empty nodes have to be displayed or not
*/
void jit_print_queue(struct jit_queue_s *queue, bool show_all, int debug_level);

#endif
/* --- EOF ------------------------------------------------------------------ */
