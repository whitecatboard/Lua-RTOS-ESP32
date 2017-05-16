/*
 * Lua RTOS, ssh module
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
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "luartos.h"

#if CONFIG_WIFI_ENABLED && CONFIG_LUA_RTOS_LUA_USE_NET

#include "libssh2_config.h"
#include <libssh2.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void l_ssh_shutdown(LIBSSH2_SESSION *session) {
    libssh2_session_disconnect(session, NULL);
    libssh2_session_free(session);
}

static int l_scp_get(lua_State* L) {
	const char* host = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);
	const char* src = luaL_checkstring(L, 3);
	const char* dst = luaL_checkstring(L, 4);
	const char* user = luaL_checkstring(L, 5);
	const char* password = luaL_checkstring(L, 6);

	// Resolve name
	char str_port[6];
	itoa(port,str_port,10);

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *res;

    int err = getaddrinfo(host, str_port, &hints, &res);
    if (err != 0 || res == NULL) {
    	return luaL_error(L, "can't resolve name");
    }

    // Init ssh
    int rc;

    rc = libssh2_init (0);
    if (rc != 0) {
    	return luaL_error(L, "can't init ssh");
    }

    // Open socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if(connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
    	return luaL_error(L, "can't connect");
    }

    // Create a session instance
    LIBSSH2_SESSION *session;
    session = libssh2_session_init();
    if (!session) {
    	return luaL_error(L, "can't create ssh session");
    }

    rc = libssh2_session_handshake(session, sock);
    if(rc) {
    	return luaL_error(L, "can't establish ssh session");
    }

    if (libssh2_userauth_password(session, user, password)) {
    	l_ssh_shutdown(session);
    	return luaL_error(L, "can't authenticate by password");
    }

    // Request file
    LIBSSH2_CHANNEL *channel;
    libssh2_struct_stat fileinfo;
    libssh2_struct_stat_size got = 0;

    channel = libssh2_scp_recv2(session, src, &fileinfo);
    if (!channel) {
    	l_ssh_shutdown(session);
    	return luaL_error(L, "can't get file");
    }

    // Open stream for save file
    FILE *fp;
    fp = fopen(dst, "w");
    if (!fp) {
    	l_ssh_shutdown(session);
        fclose(fp);
        libssh2_channel_free(channel);
        close(sock);
        libssh2_exit();
        return luaL_fileresult(L, 0, dst);
    }

    while(got < fileinfo.st_size) {
        char mem[1024];
        int amount=sizeof(mem);

        if ((fileinfo.st_size -got) < amount) {
            amount = (int)(fileinfo.st_size -got);
        }

        rc = libssh2_channel_read(channel, mem, amount);
        if(rc > 0) {
        	fwrite(mem, 1, amount, fp);
        }
        else if (rc < 0) {
        	l_ssh_shutdown(session);
            fclose(fp);
            libssh2_channel_free(channel);
            close(sock);
            libssh2_exit();

        	return luaL_error(L, "error receiving file");
        }
        got += rc;
    }

    fclose(fp);

    libssh2_channel_free(channel);
    l_ssh_shutdown(session);
    close(sock);
    libssh2_exit();

	return 0;
}

static const LUA_REG_TYPE scp_map[] = {
	{ LSTRKEY( "get" ),	 LFUNCVAL( l_scp_get   ) },
	{ LNILKEY, LNILVAL }
};

#endif

/*

Example:

net.scp.get("your_domain.com",22,"/server_path/server_file","/local_path/local_file","user","pass")

*/
