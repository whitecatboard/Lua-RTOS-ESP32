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
 * Lua RTOS, Lua OpenVPN net module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_OPENVPN

#include "net.h"

#include <unistd.h>
#include <pthread.h>

#include <sys/driver.h>
#include <sys/delay.h>

#include <drivers/net.h>

int openvpn(int argc, char *argv[]);
u8_t volatile _openvpn_should_stop = 0;
u8_t volatile _openvpn_running = 0;

static void *openvpn_thread(void *arg) {
    // Wait for network
    if (!wait_for_network(20000)) {
        lua_State* L = (lua_State*)arg;
        luaL_exception(L, NET_ERR_NOT_AVAILABLE);
    }

    delay(1000); //probably not required

    char* argv[] = {
            "openvpn",
            "--config",
            CONFIG_LUA_RTOS_OPENVPN_CONFIG_FILE,
            "--ifconfig-nowarn",
            "--single-session"
    };

    _openvpn_running = 1;
    openvpn(5, argv);
    _openvpn_running = 0;

    return NULL;
}

static int lopenvpn_service_start(lua_State* L) {
    pthread_t thread;
    pthread_attr_t attr;

    _openvpn_should_stop = 0;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8192);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&thread, &attr, openvpn_thread, L)) {
        return 0;
    }

    pthread_setname_np(thread, "openvpn");

    return 0;
}

static int lopenvpn_service_stop(lua_State* L) {
    _openvpn_should_stop = 1;
    return 0;
}

static int lopenvpn_service_running( lua_State* L ) {
    lua_pushboolean(L, _openvpn_running);
    return 1;
}

driver_error_t *tun_if_stat(ifconfig_t *info);

static int lopenvpn_tun_stat(lua_State* L) {
    ifconfig_t info;
    driver_error_t *error;
    u8_t table = 0;

    // Check if user wants result as a table, or wants scan's result
    // on the console
    if (lua_gettop(L) == 1) {
        luaL_checktype(L, 1, LUA_TBOOLEAN);
        if (lua_toboolean(L, 1)) {
            table = 1;
        }
    }

    if ((error = tun_if_stat(&info))) {
        return luaL_driver_error(L, error);
    }

    if (!table) {
        char ipa[16];
        char maska[16];
        char gwa[16];

        strcpy(ipa, inet_ntoa(info.ip));
        strcpy(maska, inet_ntoa(info.netmask));
        strcpy(gwa, inet_ntoa(info.gw));

        printf(
                "tu: mac address %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                info.mac[0],info.mac[1],
                info.mac[2],info.mac[3],
                info.mac[4],info.mac[5]
        );

        printf("   ip address %s netmask %s\r\n", ipa, maska);
        printf("   gw address %s\r\n\r\n", gwa);
    } else {
        char tmp[18];

        lua_createtable(L, 0, 4);

        lua_pushstring(L, "tu");
        lua_setfield (L, -2, "interface");

        sprintf(tmp,"%d.%d.%d.%d", ip4_addr1_16(&info.ip),ip4_addr2_16(&info.ip),ip4_addr3_16(&info.ip),ip4_addr4_16(&info.ip));
        lua_pushstring(L, tmp);
        lua_setfield (L, -2, "ip");

        sprintf(tmp,"%d.%d.%d.%d", ip4_addr1_16(&info.gw),ip4_addr2_16(&info.gw),ip4_addr3_16(&info.gw),ip4_addr4_16(&info.gw));
        lua_pushstring(L, tmp);
        lua_setfield (L, -2, "gw");

        sprintf(tmp,"%d.%d.%d.%d", ip4_addr1_16(&info.netmask),ip4_addr2_16(&info.netmask),ip4_addr3_16(&info.netmask),ip4_addr4_16(&info.netmask));
        lua_pushstring(L, tmp);
        lua_setfield (L, -2, "netmask");

        sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x",
            info.mac[0],info.mac[1],
            info.mac[2],info.mac[3],
            info.mac[4],info.mac[5]
        );
        lua_pushstring(L, tmp);
        lua_setfield (L, -2, "mac");
    }

    return table;
}

driver_error_t *tun_if_stat(ifconfig_t *info) {
    tcpip_adapter_ip_info_t esp_info;
    uint8_t mac[6] = {0,0,0,0,0,0};

    // Get IP info
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_TUN, &esp_info);

    // Get MAC info
    tcpip_adapter_get_mac(TCPIP_ADAPTER_IF_TUN, mac);

    // Copy info
    info->gw = esp_info.gw;
    info->ip = esp_info.ip;
    info->netmask = esp_info.netmask;

    memcpy(info->mac, mac, sizeof(mac));

    return NULL;
}

static const LUA_REG_TYPE openvpn_map[] = {
    { LSTRKEY("start"  ), LFUNCVAL( lopenvpn_service_start   ) },
    { LSTRKEY("stop"   ), LFUNCVAL( lopenvpn_service_stop    ) },
    { LSTRKEY("running"), LFUNCVAL( lopenvpn_service_running ) },
    { LNILKEY,LNILVAL }
};

static const LUA_REG_TYPE tun_map[] = {
    { LSTRKEY( "stat"  ),     LFUNCVAL( lopenvpn_tun_stat    ) },
    { LNILKEY, LNILVAL }
};

#endif
