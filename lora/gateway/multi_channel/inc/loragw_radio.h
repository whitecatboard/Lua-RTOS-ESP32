/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
  (C)2013 Semtech-Cycleo

Description:
    Functions used to handle LoRa concentrator radios.

License: Revised BSD License, see LICENSE.TXT file include in the project
Maintainer: Michael Coracin
*/

#ifndef _LORAGW_RADIO_H
#define _LORAGW_RADIO_H

/* -------------------------------------------------------------------------- */
/* --- DEPENDANCIES --------------------------------------------------------- */

#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

#define LGW_REG_SUCCESS 0
#define LGW_REG_ERROR -1

#define SX125x_32MHz_FRAC 15625 /* irreductible fraction for PLL register caculation */

/* -------------------------------------------------------------------------- */
/* --- PUBLIC CONSTANTS ----------------------------------------------------- */

enum lgw_sx127x_rxbw_e {
    LGW_SX127X_RXBW_2K6_HZ,
    LGW_SX127X_RXBW_3K1_HZ,
    LGW_SX127X_RXBW_3K9_HZ,
    LGW_SX127X_RXBW_5K2_HZ,
    LGW_SX127X_RXBW_6K3_HZ,
    LGW_SX127X_RXBW_7K8_HZ,
    LGW_SX127X_RXBW_10K4_HZ,
    LGW_SX127X_RXBW_12K5_HZ,
    LGW_SX127X_RXBW_15K6_HZ,
    LGW_SX127X_RXBW_20K8_HZ,
    LGW_SX127X_RXBW_25K_HZ,
    LGW_SX127X_RXBW_31K3_HZ,
    LGW_SX127X_RXBW_41K7_HZ,
    LGW_SX127X_RXBW_50K_HZ,
    LGW_SX127X_RXBW_62K5_HZ,
    LGW_SX127X_RXBW_83K3_HZ,
    LGW_SX127X_RXBW_100K_HZ,
    LGW_SX127X_RXBW_125K_HZ,
    LGW_SX127X_RXBW_166K7_HZ,
    LGW_SX127X_RXBW_200K_HZ,
    LGW_SX127X_RXBW_250K_HZ
};

/* -------------------------------------------------------------------------- */
/* --- PUBLIC FUNCTIONS PROTOTYPES ------------------------------------------ */

int lgw_setup_sx125x(uint8_t rf_chain, uint8_t rf_clkout, bool rf_enable, uint8_t rf_radio_type, uint32_t freq_hz);

int lgw_setup_sx127x(uint32_t frequency, uint8_t modulation, enum lgw_sx127x_rxbw_e rxbw_khz, int8_t rssi_offset);

int lgw_sx127x_reg_w(uint8_t address, uint8_t reg_value);

int lgw_sx127x_reg_r(uint8_t address, uint8_t *reg_value);


#endif
/* --- EOF ------------------------------------------------------------------ */
