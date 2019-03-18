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
 * Lua RTOS, Lua wifi net module
 *
 */

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET

#include <string.h>
#include <stdio.h>
#include <esp_wps.h>
#include <esp_wpa2.h>
#include <sys/syslog.h>

#if CONFIG_ESP32_WIFI_NVS_ENABLED
static lua_callback_t *wps_callback = NULL;
#endif
static lua_callback_t *sc_callback = NULL;

driver_error_t *wifi_check_error(esp_err_t error);

typedef struct {
    unsigned char *cacert;
    int cacert_len;
    unsigned char *certificate;
    int certificate_len;
    unsigned char *privkey;
    int privkey_len;
    unsigned char *privpwd;
    int privpwd_len;
} lenterprise_cert_t;

#define WPA2_CERT_initializer { NULL, 0, NULL, 0, NULL, 0, NULL, 0 }
static lenterprise_cert_t enterprise_cert_info = WPA2_CERT_initializer;
#define WIFI_MODE_STA_ENTERPRISE 999
static int lwifi_setup_enterprise(lua_State* L);

static int lwifi_setup(lua_State* L) {
    driver_error_t *error;

    int powersave = 0;
    int channel = 0;
    int hidden = 0;

    uint32_t ip = 0;
    uint32_t mask = 0;
    uint32_t gw = 0;
    uint32_t dns1 = 0;
    uint32_t dns2 = 0;

    int mode = luaL_checkinteger(L, 1);
    const char *ssid = luaL_checkstring(L, 2);
    const char *password = luaL_checkstring(L, 3);

    if (mode == WIFI_MODE_STA) {
        // Get IP info, if present
        ip = luaL_optinteger(L, 4, 0);
        mask = luaL_optinteger(L, 5, 0);
        gw = luaL_optinteger(L, 6, 0);
        dns1 = luaL_optinteger(L, 7, 0);
        dns2 = luaL_optinteger(L, 8, 0);
        powersave = luaL_optinteger(L, 9, 0);
        channel = luaL_optinteger(L, 10, 0);
    } else if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
        powersave = luaL_optinteger(L, 4, 0);
        channel = luaL_optinteger(L, 5, 0);

        if (lua_gettop(L) > 5) {
            luaL_checktype(L, 6, LUA_TBOOLEAN);
            if (lua_toboolean(L, 6)) {
                hidden = 1;
            }
        }
    } else if (mode == WIFI_MODE_STA_ENTERPRISE) {
        return lwifi_setup_enterprise(L);
    }

    // Setup wifi
    if ((error = wifi_setup(mode, (char *)ssid, (char *)password, ip, mask, gw, dns1, dns2, powersave, channel, hidden))) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

static int get_file_content(const char* filename, unsigned char **buffer, int* buflen) {
    int rc = ESP_OK;

    FILE *fp = fopen(filename, "rb");
    if ( fp != NULL )
    {
        fseek(fp, 0, SEEK_END);
        unsigned long fileLen = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        unsigned char *buf=(unsigned char *)malloc(fileLen+1);
        if (buf) {
            memset(buf, 0, fileLen+1); //make sure certs are properly delimited
            unsigned long bytesRead = (int)fread(buf, sizeof(char), fileLen, fp);
            *buffer = buf;
            // see esp_wpa2.h - The client_cert, private_key and private_key_passwd should be zero terminated.
            *buflen = bytesRead+1; // resp. https://gist.github.com/me-no-dev/2d2b51b17226f5e9c5a4d9a78bdc0720
            // The strlen() function does return the length of the certificate without the terminating 0x00.
            // The esp_wifi_sta_wpa2_ent_set_ca_cert() does need the terminating 0x00, so you must add +1 .
            if ( bytesRead != fileLen ) {
                rc = ESP_FAIL;
            }
        } else {
            rc = ESP_ERR_NO_MEM;
        }
        fclose(fp);
        fp = NULL;
    } else {
        rc = ESP_ERR_INVALID_ARG;
    }

    return rc;
}

static int lwifi_setup_enterprise(lua_State* L) {
    driver_error_t *error;

    int powersave = 0;
    int channel = 0;

    uint32_t ip = 0;
    uint32_t mask = 0;
    uint32_t gw = 0;
    uint32_t dns1 = 0;
    uint32_t dns2 = 0;

    const char *ssid = luaL_checkstring(L, 2);
    const char *identity = luaL_optstring(L, 3, NULL);
    const char *username = luaL_optstring(L, 4, NULL);
    const char *password = luaL_optstring(L, 5, NULL);

    const char *cacert_file = luaL_optstring(L, 6, NULL);
    const char *certificate_file = luaL_optstring(L, 7, NULL);
    const char *privkey_file = luaL_optstring(L, 8, NULL);
    const char *privkey_password = luaL_optstring(L, 9, NULL);

    int disabletimecheck = luaL_optinteger(L, 10, 2); //default = 2 = don't explicitly enable or disable

    // Get IP info, if present
    ip = luaL_optinteger(L, 11, 0);
    mask = luaL_optinteger(L, 12, 0);
    gw = luaL_optinteger(L, 13, 0);
    dns1 = luaL_optinteger(L, 14, 0);
    dns2 = luaL_optinteger(L, 15, 0);
    powersave = luaL_optinteger(L, 16, 0);
    channel = luaL_optinteger(L, 17, 0);

    esp_wifi_sta_wpa2_ent_clear_ca_cert(); //make sure our memory is no longer referenced by the espressif layer
    esp_wifi_sta_wpa2_ent_clear_cert_key(); //make sure our memory is no longer referenced by the espressif layer

    if (enterprise_cert_info.cacert) {
        free(enterprise_cert_info.cacert);
        enterprise_cert_info.cacert = NULL;
        enterprise_cert_info.cacert_len = 0;
    }
    if (cacert_file != NULL && *cacert_file != 0) { // also checking for *empty* string
        int rc = get_file_content(cacert_file, &enterprise_cert_info.cacert, &enterprise_cert_info.cacert_len);
        if (ESP_OK != rc) {
            syslog(LOG_ERR, "wifi_setup_enterprise: get_file_content(cacert) error %s\n", esp_err_to_name(rc));
            return luaL_driver_error(L, wifi_check_error(rc));
        }
    }
    if (enterprise_cert_info.certificate) {
        free(enterprise_cert_info.certificate);
        enterprise_cert_info.certificate = NULL;
        enterprise_cert_info.certificate_len = 0;
    }
    if (certificate_file != NULL && *certificate_file != 0) { // also checking for *empty* string
        int rc = get_file_content(certificate_file, &enterprise_cert_info.certificate, &enterprise_cert_info.certificate_len);
        if (ESP_OK != rc) {
            syslog(LOG_ERR, "wifi_setup_enterprise: get_file_content(certificate) error %s\n", esp_err_to_name(rc));
            return luaL_driver_error(L, wifi_check_error(rc));
        }
    }
    if (enterprise_cert_info.privkey) {
        free(enterprise_cert_info.privkey);
        enterprise_cert_info.privkey = NULL;
        enterprise_cert_info.privkey_len = 0;
    }
    if (privkey_file != NULL && *privkey_file != 0) { // also checking for *empty* string
        int rc = get_file_content(privkey_file, &enterprise_cert_info.privkey, &enterprise_cert_info.privkey_len);
        if (ESP_OK != rc) {
            syslog(LOG_ERR, "wifi_setup_enterprise: get_file_content(privkey) error %s\n", esp_err_to_name(rc));
            return luaL_driver_error(L, wifi_check_error(rc));
        }
    }
    if (enterprise_cert_info.privpwd) {
        free(enterprise_cert_info.privpwd);
        enterprise_cert_info.privpwd = NULL;
        enterprise_cert_info.privpwd_len = 0;
    }
    if (privkey_password != NULL && *privkey_password != 0) {
        enterprise_cert_info.privpwd = (unsigned char *)strdup(privkey_password);
        // see esp_wpa2.h - The client_cert, private_key and private_key_passwd should be zero terminated.
        enterprise_cert_info.privpwd_len = strlen(privkey_password)+1;
    }

    // Setup wifi
    if ((error = wifi_setup_enterprise((char *)ssid, (char *)identity, (char *)username, (char *)password,
                                       enterprise_cert_info.cacert, enterprise_cert_info.cacert_len,
                                       enterprise_cert_info.certificate, enterprise_cert_info.certificate_len,
                                       enterprise_cert_info.privkey, enterprise_cert_info.privkey_len,
                                       enterprise_cert_info.privpwd, enterprise_cert_info.privpwd_len,
                                       disabletimecheck, ip, mask, gw, dns1, dns2, powersave, channel))) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

static int lwifi_scan(lua_State* L) {
    driver_error_t *error;
    wifi_ap_record_t *list;
    uint16_t count, i;
    u8_t table = 0;
    char auth[13];

    // Check if user wants scan's result as a table, or wants scan's result
    // on the console
    if (lua_gettop(L) == 1) {
        luaL_checktype(L, 1, LUA_TBOOLEAN);
        if (lua_toboolean(L, 1)) {
            table = 1;
        }
    }

    // Scan wifi
    if ((error = wifi_scan(&count, &list))) {
        return luaL_driver_error(L, error);
    }

    // Show / get APs
    if (!table) {
        printf("\r\n                           SSID  RSSI          AUTH  CH1  CH2\r\n");
        printf("-------------------------------------------------------------\r\n");
    } else {
        lua_createtable(L, count, 0);
    }

    for(i=0;i<count;i++) {
        if (!table) {
            switch (list[i].authmode) {
                case WIFI_AUTH_OPEN: strcpy(auth, "OPEN"); break;
                case WIFI_AUTH_WEP: strcpy(auth, "WEP"); break;
                case WIFI_AUTH_WPA_PSK: strcpy(auth, "WPA_PSK"); break;
                case WIFI_AUTH_WPA2_PSK: strcpy(auth, "WPA2_PSK"); break;
                case WIFI_AUTH_WPA_WPA2_PSK: strcpy(auth, "WPA_WPA2_PSK"); break;
                default:
                    break;
            }

            printf("%31.31s  %4d %13.13s   %2d   %2d\r\n",list[i].ssid, list[i].rssi, auth, list[i].primary, list[i].second);
        } else {
            lua_pushinteger(L, i);

            lua_createtable(L, 0, 3);

            lua_pushstring(L, (char *)list[i].ssid);
            lua_setfield (L, -2, "ssid");

            lua_pushinteger(L, list[i].rssi);
            lua_setfield (L, -2, "rssi");

            lua_pushinteger(L, list[i].authmode);
            lua_setfield (L, -2, "auth");

            lua_pushinteger(L, list[i].primary);
            lua_setfield (L, -2, "ch1");

            lua_pushinteger(L, list[i].second);
            lua_setfield (L, -2, "ch2");

            lua_settable(L,-3);
        }
    }

    if (list) {
        free(list);
    }

    if (!table) {
        printf("\r\n");
    }

    return table;
}

static int lwifi_start(lua_State* L) {
    driver_error_t *error;

    // Determine if the interface start must be synchronous
    // or asynchronous
    uint8_t async = 1;

    if (lua_gettop(L) >= 1) {
        if (lua_gettop(L) == 1) {
            luaL_checktype(L, 1, LUA_TBOOLEAN);
            if (lua_toboolean(L, 1)) {
                async = 1;
            } else {
                async = 0;
            }
        } else {
            return luaL_exception(L, WIFI_ERR_INVALID_ARGUMENT);
        }
    }

    if ((error = wifi_start(async))) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

static int lwifi_stop(lua_State* L) {
    driver_error_t *error;

    if ((error = wifi_stop())) {
        return luaL_driver_error(L, error);
    }

    return 0;
}

static int lwifi_stat(lua_State* L) {
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

    if ((error = wifi_stat(&info))) {
        if (error->exception != WIFI_ERR_WIFI_NOT_INIT) {
            return luaL_driver_error(L, error);
        }

        free(error);
        memset(&info, 0, sizeof(ifconfig_t));
    }

    if (!table) {
        char ipa[16];
        char maska[16];
        char gwa[16];

        strcpy(ipa, inet_ntoa(info.ip));
        strcpy(maska, inet_ntoa(info.netmask));
        strcpy(gwa, inet_ntoa(info.gw));

        printf( "wf: mac address %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            info.mac[0],info.mac[1],
            info.mac[2],info.mac[3],
            info.mac[4],info.mac[5]
        );

        printf("   ip address %s / netmask %s\r\n", ipa, maska);
        printf("   gw address %s\r\n", gwa);

        if (!ip6_addr_cmp(IP6_ADDR_ANY6, &info.ip6)) {
            char tmp[46];
            snprintf(tmp, sizeof(tmp), IPV6STR, IPV62STR(info.ip6));
            printf("  ip6 address %s\r\n", tmp);
        }

        printf("\r\n");

    } else {
        char tmp[46];
        tmp[0] = 0;

        if (!ip6_addr_cmp(IP6_ADDR_ANY6, &info.ip6)) {
            snprintf(tmp, sizeof(tmp), IPV6STR, IPV62STR(info.ip6));
        }

        lua_createtable(L, 0, strlen(tmp) > 0 ? 6:5);

        lua_pushstring(L, "wf");
        lua_setfield (L, -2, "interface");

        snprintf(tmp, sizeof(tmp), IPSTR, ip4_addr1_16(&info.ip),ip4_addr2_16(&info.ip),ip4_addr3_16(&info.ip),ip4_addr4_16(&info.ip));
        lua_pushstring(L, tmp);
        lua_setfield (L, -2, "ip");

        snprintf(tmp, sizeof(tmp), IPSTR, ip4_addr1_16(&info.gw),ip4_addr2_16(&info.gw),ip4_addr3_16(&info.gw),ip4_addr4_16(&info.gw));
        lua_pushstring(L, tmp);
        lua_setfield (L, -2, "gw");

        snprintf(tmp, sizeof(tmp), IPSTR, ip4_addr1_16(&info.netmask),ip4_addr2_16(&info.netmask),ip4_addr3_16(&info.netmask),ip4_addr4_16(&info.netmask));
        lua_pushstring(L, tmp);
        lua_setfield (L, -2, "netmask");

        snprintf(tmp, sizeof(tmp), "%02x:%02x:%02x:%02x:%02x:%02x",
            info.mac[0],info.mac[1],
            info.mac[2],info.mac[3],
            info.mac[4],info.mac[5]
        );
        lua_pushstring(L, tmp);
        lua_setfield (L, -2, "mac");

        if (strlen(tmp) > 0 ) {
            lua_pushstring(L, tmp);
            lua_setfield (L, -2, "ip6");
        }
    }

    return table;
}

#if CONFIG_ESP32_WIFI_NVS_ENABLED
static void lwifi_wps_pin(char* pin) {
    if (wps_callback != NULL) {
        lua_State *state = luaS_callback_state(wps_callback);
        lua_pushstring(state, pin);
        luaS_callback_call(wps_callback, 1);
    }
    free(pin);
}

static int lwifi_wps(lua_State* L) {
    driver_error_t *error;

    if (wps_callback != NULL) {
        luaS_callback_destroy(wps_callback);
        wps_callback = NULL;
    }

    int wpsmode = luaL_optinteger(L, 1, WPS_TYPE_PBC);
    if (wpsmode != WPS_TYPE_PBC) {
        luaL_checktype(L, 2, LUA_TFUNCTION);
        wps_callback = luaS_callback_create(L, 2);
    }

    if ((error = wifi_wps(wpsmode, lwifi_wps_pin))) {
        return luaL_driver_error(L, error);
    }

    return 0;
}
#endif

static void lwifi_sc_config(char* ssid, char* password) {
    if (sc_callback != NULL) {
        lua_State *state = luaS_callback_state(sc_callback);
        lua_pushstring(state, ssid);
        lua_pushstring(state, password);
        luaS_callback_call(sc_callback, 2);
    }
    /* ssid and password are allocated (and freed) by esp_smartconfig_... */
}

static int lwifi_smartconfig(lua_State* L) {
    driver_error_t *error;

    if (sc_callback != NULL) {
        luaS_callback_destroy(sc_callback);
        sc_callback = NULL;
    }

    luaL_checktype(L, 1, LUA_TFUNCTION);
    sc_callback = luaS_callback_create(L, 1);

    if ((error = wifi_smartconfig(lwifi_sc_config))) {
        return luaL_driver_error(L, error);
    }

    return 0;
}


static const LUA_REG_TYPE wifi_auth_map[] = {
    { LSTRKEY( "OPEN"         ), LINTVAL( WIFI_AUTH_OPEN ) },
    { LSTRKEY( "WEP"          ), LINTVAL( WIFI_AUTH_WEP ) },
    { LSTRKEY( "WPA_PSK"      ), LINTVAL( WIFI_AUTH_WPA_PSK ) },
    { LSTRKEY( "WPA2_PSK"     ), LINTVAL( WIFI_AUTH_WPA2_PSK ) },
    { LSTRKEY( "WPA_WPA2_PSK" ), LINTVAL( WIFI_AUTH_WPA_WPA2_PSK ) },
};

static const LUA_REG_TYPE wifi_mode_map[] = {
    { LSTRKEY( "STA"          ), LINTVAL( WIFI_MODE_STA ) },
    { LSTRKEY( "AP"           ), LINTVAL( WIFI_MODE_AP ) },
    { LSTRKEY( "APSTA"        ), LINTVAL( WIFI_MODE_APSTA ) },
    { LSTRKEY( "STAENT"       ), LINTVAL( WIFI_MODE_STA_ENTERPRISE ) },
};

static const LUA_REG_TYPE wifi_powersave_map[] = {
    { LSTRKEY( "NONE"          ), LINTVAL( WIFI_PS_NONE ) },
    { LSTRKEY( "MODEM"         ), LINTVAL( WIFI_PS_MIN_MODEM ) },
    { LSTRKEY( "MODEM_MIN"     ), LINTVAL( WIFI_PS_MIN_MODEM ) },
    { LSTRKEY( "MODEM_MAX"     ), LINTVAL( WIFI_PS_MAX_MODEM ) },
};

static const LUA_REG_TYPE wifi_wpstype_map[] = {
    { LSTRKEY( "DISABLE"       ), LINTVAL( WPS_TYPE_DISABLE ) },
    { LSTRKEY( "PBC"           ), LINTVAL( WPS_TYPE_PBC ) },
    { LSTRKEY( "PIN"           ), LINTVAL( WPS_TYPE_PIN ) },
};

static const LUA_REG_TYPE wifi_timecheck_map[] = {
    { LSTRKEY( "DEFAULT"       ), LINTVAL( 2 ) },
    { LSTRKEY( "ENABLE"        ), LINTVAL( 0 ) },
    { LSTRKEY( "DISABLE"       ), LINTVAL( 1 ) },
};

static const LUA_REG_TYPE wifi_map[] = {
    { LSTRKEY( "setup"      ),     LFUNCVAL( lwifi_setup            ) },
    { LSTRKEY( "scan"       ),     LFUNCVAL( lwifi_scan             ) },
    { LSTRKEY( "start"      ),     LFUNCVAL( lwifi_start            ) },
    { LSTRKEY( "stop"       ),     LFUNCVAL( lwifi_stop             ) },
    { LSTRKEY( "stat"       ),     LFUNCVAL( lwifi_stat             ) },
#if CONFIG_ESP32_WIFI_NVS_ENABLED
    // wps ssid+auth can only be stored to nvs
    // so if nvs is disabled, wps would have to be repeated on every boot
    { LSTRKEY( "startwps"  ),     LFUNCVAL( lwifi_wps          ) },
#endif
    { LSTRKEY( "startsc"   ),     LFUNCVAL( lwifi_smartconfig  ) },
    { LSTRKEY( "auth"      ),     LROVAL  ( wifi_auth_map      ) },
    { LSTRKEY( "mode"      ),     LROVAL  ( wifi_mode_map      ) },
    { LSTRKEY( "powersave" ),     LROVAL  ( wifi_powersave_map ) },
    { LSTRKEY( "wpstype"   ),     LROVAL  ( wifi_wpstype_map   ) },
    { LSTRKEY( "timecheck" ),     LROVAL  ( wifi_timecheck_map ) },

    DRIVER_REGISTER_LUA_ERRORS(wifi)
    { LNILKEY, LNILVAL }
};

#endif
