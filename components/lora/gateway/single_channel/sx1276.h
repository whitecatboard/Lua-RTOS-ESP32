/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS, simple channel LoRa WAN gateway
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1276 || CONFIG_LUA_RTOS_LORA_HW_TYPE_SX1272

#ifndef _SX1276_H
#define _SX1276_H

#include <stdint.h>

#define SX1276_REG_FIFO                    0x00
#define SX1276_REG_OP_MODE                 0x01
#define SX1276_REG_FR_MSB                  0x06
#define SX1276_REG_FR_MID                  0x07
#define SX1276_REG_FR_LSB                  0x08
#define SX1276_REG_PA_CONFIG               0x09
#define SX1276_REG_PA_RAMP                 0x0A
#define SX1276_REG_OCP                     0x0B
#define SX1276_REG_LNA                     0x0C
#define SX1276_REG_FIFO_ADDR_PTR           0x0D
#define SX1276_REG_FIFO_TX_BASE_ADDR       0x0E
#define SX1276_REG_FIFO_RX_BASE_ADDR       0x0F
#define SX1276_REG_FIFO_RX_CURRENT_ADDR    0x10
#define SX1276_REG_IRQ_FLAGS_MASK          0x11
#define SX1276_REG_IRQ_FLAGS               0x12
#define SX1276_REG_RX_NB_BYTES             0x13
#define SX1276_REG_RX_HEADER_CNT_VALUE_MSB 0x14
#define SX1276_REG_RX_HEADER_CNT_VALUE_LSB 0x15
#define SX1276_REG_RX_PACKET_CNT_VALUE_MSB 0x16
#define SX1276_REG_RX_PACKET_CNT_VALUE_LSB 0x17
#define SX1276_REG_MODEM_STAT              0x18
#define SX1276_REG_PKT_SNR_VALUE           0x19
#define SX1276_REG_PKT_RSSI_VALUE          0x1A
#define SX1276_REG_RSSI_VALUE              0x1B
#define SX1276_REG_HOP_CHANNEL             0x1C
#define SX1276_REG_MODEM_CONFIG_1          0x1D
#define SX1276_REG_MODEM_CONFIG_2          0x1E
#define SX1276_REG_SYMB_TIMEOUT_LSB        0x1F
#define SX1276_REG_PREAMBLE_MSB            0x20
#define SX1276_REG_PREAMBLE_LSB            0x21
#define SX1276_REG_PAYLOAD_LENGTH          0x22
#define SX1276_REG_MAX_PAYLOAD_LENGTH      0x23
#define SX1276_REG_HOP_PERIOD              0x24
#define SX1276_REG_FIFO_RX_BYTE_ADDR       0x25
#define SX1276_REG_MODEM_CONFIG_3          0x26
#define PPM_CORRECTION              	   0x27
#define SX1276_REG_FEI_MSB                 0x28
#define SX1276_REG_FEI_MID                 0x29
#define SX1276_REG_FEI_LSB                 0x2A
#define SX1276_REG_RSSI_WIDEBAND           0x2C
#define SX1276_REG_DETECT_OPTIMIZE         0x31
#define SX1276_REG_INVERT_IQ               0x33
#define SX1276_REG_DETECTION_THRESHOLD     0x37
#define SX1276_REG_SYNC_WORD               0x39
#define SX1276_REG_VERSION                 0x42

#define OPMODE_LORA      0x80
#define OPMODE_CONTINOUS 0X85
#define OPMODE_MASK      0x07
#define OPMODE_SLEEP     0x00
#define OPMODE_STANDBY   0x01
#define OPMODE_FSTX      0x02
#define OPMODE_TX        0x03
#define OPMODE_FSRX      0x04
#define OPMODE_RX        0x05
#define OPMODE_RX_SINGLE 0x06
#define OPMODE_CAD       0x07

void sx1276_reset(uint8_t val);
void stx1276_read_reg(int spi_device, uint8_t addr, uint8_t *data);
void stx1276_write_reg(int spi_device, uint8_t addr, uint8_t data);
void stx1276_read_buff(int spi_device, uint8_t addr, uint8_t *data, uint8_t len);

#endif

#endif
