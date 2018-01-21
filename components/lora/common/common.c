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
 * Lua RTOS, LoRa WAN implementation
 *
 */

#include "common.h"

// Convert a buffer coded into an hex string (hbuff) into a byte buffer (vbuff)
// Length of byte buffer is len
void hex_string_to_val(char *hbuff, char *vbuff, int len, int rev) {
    int  i;
    char c;

	// If reverse, put hbuff at the last byte
	if (rev) {
		while(*hbuff) hbuff++;
		hbuff -= 2;
	}

    for(i=0;i<len;i++) {
        c = 0;

        if ((*hbuff >= '0') && (*hbuff <= '9')) {
            c = (0 + (*hbuff - '0')) << 4;
        }

        if ((*hbuff >= 'A') && (*hbuff <= 'F')) {
            c = (10 + (*hbuff - 'A')) << 4;
        }

        hbuff++;

        if ((*hbuff >= '0') && (*hbuff <= '9')) {
            c |= 0 + (*hbuff - '0');
        }

        if ((*hbuff >= 'A') && (*hbuff <= 'F')) {
            c |= 10 + (*hbuff - 'A');
        }

        *vbuff = c;

		if (rev) {
			hbuff -= 3;
		} else {
	        hbuff++;
		}

        vbuff++;
    }
}

// Convert byte buffer (vbuff argument) of len argument size into a hex
// string buffer (hbuff argument) into a )
void val_to_hex_string(char *hbuff, char *vbuff, int len, int reverse) {
    int i;

	if (reverse) {
		vbuff += (len - 1);
	}

    for(i=0;i<len;i++) {
        if ((((*vbuff & 0xf0) >> 4) >= 0) && (((*vbuff & 0xf0) >> 4) <= 9)) {
            *hbuff = '0' + ((*vbuff & 0xf0) >> 4);
        }

        if ((((*vbuff & 0xf0) >> 4) >= 10) && (((*vbuff & 0xf0) >> 4) <= 15)) {
            *hbuff = 'A' + (((*vbuff & 0xf0) >> 4) - 10);
        }
        hbuff++;

        if (((*vbuff & 0x0f) >= 0) && ((*vbuff & 0x0f) <= 9)) {
            *hbuff = '0' + (*vbuff & 0x0f);
        }

        if (((*vbuff & 0x0f) >= 10) && ((*vbuff & 0x0f) <= 15)) {
            *hbuff = 'A' + ((*vbuff & 0x0f) - 10);
        }
        hbuff++;

		if (reverse) {
	        vbuff--;
		} else {
	        vbuff++;
		}
    }

	*hbuff = 0x00;
}

