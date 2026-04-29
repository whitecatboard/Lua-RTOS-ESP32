/*
 * Copyright (C) 2015 - 2020, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2020, Jaume Olivé Petrus (jolive@whitecatboard.org)
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
 * Lua RTOS, LoRaWAN driver for Semtech Stack
 *
 */

#include "lora.h"


// Register driver and messages
void _lora_init();

DRIVER_REGISTER_BEGIN(LORA, lora, 0, _lora_init, NULL);
DRIVER_REGISTER_ERROR(LORA, lora, KeysNotConfigured, "keys are not configured", LORA_ERR_KEYS_NOT_CONFIGURED);
DRIVER_REGISTER_ERROR(LORA, lora, JoinDenied, "join denied", LORA_ERR_JOIN_DENIED);
DRIVER_REGISTER_ERROR(LORA, lora, UnexpectedResponse, "unexpected response", LORA_ERR_UNEXPECTED_RESPONSE);
DRIVER_REGISTER_ERROR(LORA, lora, NotJoined, "not joined", LORA_ERR_NOT_JOINED);
DRIVER_REGISTER_ERROR(LORA, lora, NotSetup, "not setup", LORA_ERR_NOT_SETUP);
DRIVER_REGISTER_ERROR(LORA, lora, NotEnoughtMemory, "not enough memory", LORA_ERR_NO_MEM);
DRIVER_REGISTER_ERROR(LORA, lora, CannotSetup, "can't setup", LORA_ERR_CANT_SETUP);
DRIVER_REGISTER_ERROR(LORA, lora, TransmissionFailACK, "transmission fail ack not received", LORA_ERR_TX_ERROR_ACK);
DRIVER_REGISTER_ERROR(LORA, lora, TransmissionFail, "transmission error", LORA_ERR_TX_ERROR);
DRIVER_REGISTER_ERROR(LORA, lora, InvalidArgument, "invalid argument", LORA_ERR_INVALID_ARGUMENT);
DRIVER_REGISTER_ERROR(LORA, lora, InvalidDataRate, "invalid data rate for your location", LORA_ERR_INVALID_DR);
DRIVER_REGISTER_ERROR(LORA, lora, InvalidBand, "invalid band for your location", LORA_ERR_INVALID_BAND);
DRIVER_REGISTER_ERROR(LORA, lora, NotAllowed, "not allowed", LORA_ERR_NOT_ALLOWED);
DRIVER_REGISTER_ERROR(LORA, lora, InvalidFreq, "invalid frequency for your location", LORA_ERR_INVALID_FREQ);
DRIVER_REGISTER_ERROR(LORA, lora, CryptoError, "crypto error", LORA_ERR_CRYPTO_ERROR);
DRIVER_REGISTER_ERROR(LORA, lora, Timeout, "timeout", LORA_ERR_TIMEOUT);
DRIVER_REGISTER_END(LORA, lora, 0, _lora_init, NULL);

void _lora_init() {
}