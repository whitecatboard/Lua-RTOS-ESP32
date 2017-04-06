/*
 * Lua RTOS, LoRa WAN implementation
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
 *
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
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

