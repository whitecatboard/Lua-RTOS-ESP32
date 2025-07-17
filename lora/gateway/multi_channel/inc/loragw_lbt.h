/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech-Cycleo

Description:
    Functions used to handle the Listen Before Talk feature

License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Michael Coracin
*/

#ifndef _LORAGW_LBT_H
#define _LORAGW_LBT_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */

#include "loragw_hal.h"

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

#define LGW_LBT_SUCCESS 0
#define LGW_LBT_ERROR -1

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

/**
@brief Set the configuration parameters for LBT feature
@param conf structure containing the configuration parameters
@return LGW_LBT_ERROR id the operation failed, LGW_LBT_SUCCESS else
*/
int lbt_setconf(struct lgw_conf_lbt_s * conf);

/**
@brief Configure the concentrator for LBT feature
@return LGW_LBT_ERROR id the operation failed, LGW_LBT_SUCCESS else
*/
int lbt_setup(void);

/**
@brief Start the LBT FSM
@return LGW_LBT_ERROR id the operation failed, LGW_LBT_SUCCESS else
*/
int lbt_start(void);

/**
@brief Configure the concentrator for LBT feature
@param pkt_data pointer to downlink packet to be trabsmitted
@param tx_allowed pointer to receive permission for transmission
@return LGW_LBT_ERROR id the operation failed, LGW_LBT_SUCCESS else
*/
int lbt_is_channel_free(struct lgw_pkt_tx_s * pkt_data, bool * tx_allowed);

/**
@brief Check if LBT is enabled
@return true if enabled, false otherwise
*/
bool lbt_is_enabled(void);

#endif
/* --- EOF ------------------------------------------------------------------ */
