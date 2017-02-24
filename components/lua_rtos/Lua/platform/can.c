/*
 * Lua RTOS, platform functions for lua CAN module
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
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
 * whatsoever resulting from loss of use, ƒintedata or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "luartos.h"

#if LUA_USE_CAN

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "drivers/can/can.h"

#include <strings.h>

extern struct can can[];

int platform_can_exists( unsigned id ) {
    unsigned char rx, tx;

    if ((id >= 1) && (id <= NCAN)) {
        can_pins(id, &rx, &tx);  
        
        return ((rx != 0) && (tx != 0));
    } else {
        return 0;
    }
}

int platform_can_pins( lua_State* L ) {
    int i;
    unsigned char rx, tx;
    luaL_Buffer buff;
    char *cbuff;
    char *tmpbuff[80];
    
    luaL_buffinit( L, &buff );
    cbuff = buff.b;
    
    for(i=1;i<=NCAN;i++) {
        can_pins(i, &rx, &tx);
        
        if ((rx != 0) && (tx != 0)) {
            sprintf((char *)tmpbuff,"can%d: rx=%s%d\t(pin %2d)\ttx=%s%d\t(pin %2d)\n", i,
                            gpio_portname(rx), gpio_name(rx),cpu_pin_number(rx),
                            gpio_portname(tx), gpio_name(tx),cpu_pin_number(tx)
            );

            memcpy(cbuff, tmpbuff, strlen((const char *)tmpbuff));
            buff.n = buff.n + strlen((const char *)tmpbuff);
            cbuff = cbuff + strlen((const char *)tmpbuff);            
        }        
    }
    
    luaL_pushresult( &buff );
    
    return 1;
}

u32 platform_can_setup( unsigned id, u32 clock ) {
    if (can_init(id, clock) < 0) {
        return 0;
    }

    if (can_configure_rx_channel(id, 0, 0) < 0) {
        return 0;
    }
    
    if (can_configure_tx_channel(id, 1, CAN_HIGH_MED_PRIO, 0) < 0) {
        return 0;
    }
    
    return clock;
}

int platform_can_send( unsigned id, u32 canid, u8 idtype, u8 len, const u8 *data ) {
    canMessage frame;
    register struct can_fifo_reg *fifo;
    register int i;
    
    bzero(&frame, sizeof(canMessage));
    
    // Populate message
    frame.CMSGSID.SID = canid;
    frame.CMSGEID.DLC = len;
    
    for (i=0; i < len; i++) {
        frame.data[i] = *data++;
    }
        
    can_tx(id, 1, &frame);      
    
    return 1;
}

int platform_can_rawsend( unsigned id, canMessage *frame ) {
    can_tx(id, 1, frame);      
    
    return 1;
}

int platform_can_recv( unsigned id, u32 *canid, u8 *idtype, u8 *len, u8 *data ) {
    canMessage frame;
    u8 i;
        
    can_rx(id, 0, &frame);

    *canid = frame.CMSGSID.SID;
    *len = frame.CMSGEID.DLC;
    *idtype = 0;

    for(i=0; i < *len; i++) {
        *data++ = frame.data[i];
    }   

    return 1;
}

int platform_can_rawrecv( unsigned id, canMessage *frame ) {
    can_rx(id, 0, frame);

    return 1;
}

int platform_can_stats(unsigned id, struct can_stats *stats) {
    memcpy(stats, &can[id - 1].stats, sizeof(struct can_stats));

    return 1;
}

#endif
