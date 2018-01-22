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
 * Lua RTOS, Lua SSH client net module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_LUA_USE_NET && CONFIG_LUA_RTOS_LUA_USE_SCP_NET

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
	int rc = libssh2_init (0);
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

	char mem[1024];
	while(got < fileinfo.st_size) {
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

static int l_scp_put(lua_State* L) {
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
	int rc = libssh2_init (0);
	if (rc != 0) {
		return luaL_error(L, "can't init ssh");
	}

	// Open stream for save file
	FILE *fp;
	fp = fopen(src, "rb");
	if (!fp) {
		fclose(fp);
		return luaL_fileresult(L, 0, src);
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
		fclose(fp);
		close(sock);
		libssh2_exit();
		return luaL_error(L, "can't authenticate by password");
	}

	// Send file
	LIBSSH2_CHANNEL *channel;
	libssh2_struct_stat fileinfo;
	stat(src, &fileinfo);

	channel = libssh2_scp_send_ex(session, dst, 0644, (size_t)fileinfo.st_size, 0, 0);
	if (!channel) {
		l_ssh_shutdown(session);
		fclose(fp);
		libssh2_channel_free(channel);
		close(sock);
		libssh2_exit();
		return luaL_error(L, "can't put file");
	}

	size_t nread;
	char *ptr;
	char mem[1024];
	do {
		nread = fread(mem, 1, sizeof(mem), fp);
		if (nread <= 0) {
			/* end of file */
			break;
		}
		ptr = mem;

		do {
			/* write the same data over and over, until error or completion */
			rc = libssh2_channel_write(channel, ptr, nread);
			if (rc < 0) {
								l_ssh_shutdown(session);
									fclose(fp);
									libssh2_channel_free(channel);
									close(sock);
									libssh2_exit();
				return luaL_error(L, "error sending file");
			}
			else {
				/* rc indicates how many bytes were written this time */
				ptr += rc;
				nread -= rc;
			}
		} while (nread);

	} while (1);

	fclose(fp);

	libssh2_channel_send_eof(channel);
	libssh2_channel_wait_eof(channel);
	libssh2_channel_wait_closed(channel);

	libssh2_channel_free(channel);
	l_ssh_shutdown(session);
	close(sock);
	libssh2_exit();

	return 0;
}

static int waitsocket(int socket_fd, LIBSSH2_SESSION *session)
{
	struct timeval timeout;
	int rc;
	fd_set fd;
	fd_set *writefd = NULL;
	fd_set *readfd = NULL;
	int dir;

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	FD_ZERO(&fd);

	FD_SET(socket_fd, &fd);

	/* now make sure we wait in the correct direction */
	dir = libssh2_session_block_directions(session);

	if(dir & LIBSSH2_SESSION_BLOCK_INBOUND)
		readfd = &fd;

	if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
		writefd = &fd;

	rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

	return rc;
}

static int l_ssh_exec(lua_State* L) {
	const char* host = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);
	const char* commandline = luaL_checkstring(L, 3);
	const char* user = luaL_checkstring(L, 4);
	const char* password = luaL_checkstring(L, 5);

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
	int rc = libssh2_init (0);
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

	/* Exec non-blocking on the remote host */
	while( (channel = libssh2_channel_open_session(session)) == NULL &&
			 libssh2_session_last_error(session,NULL,NULL,0) ==
			 LIBSSH2_ERROR_EAGAIN )	{
		waitsocket(sock, session);
	}

	if (!channel) {
		l_ssh_shutdown(session);
		libssh2_channel_free(channel);
		return luaL_error(L, "can't execute");
	}

	while( (rc = libssh2_channel_exec(channel, commandline)) ==
			 LIBSSH2_ERROR_EAGAIN ) {
		waitsocket(sock, session);
	}

	if (rc)	{
		l_ssh_shutdown(session);
		libssh2_channel_free(channel);
		return luaL_error(L, "can't execute");
	}

	int bytecount = 0;
	char buffer[1024];

	for( ;; ) {
		/* loop until we block */
		do {
			rc = libssh2_channel_read( channel, buffer, sizeof(buffer) );
			if( rc > 0 ) {
				bytecount += rc;
				for( int i=0; i < rc; ++i )
					fputc( buffer[i], stdout);
				//fprintf(stdout, "\n");
			}
			else if( rc != LIBSSH2_ERROR_EAGAIN && rc != LIBSSH2_ERROR_NONE ) {
				/* no need to output this for the EAGAIN case */
				fprintf(stderr, "libssh2_channel_read returned %d\n", rc);
			}
		}
		while( rc > 0 );

		/* this is due to blocking that would occur otherwise so we loop on
			 this condition */
		if( rc == LIBSSH2_ERROR_EAGAIN ) {
			waitsocket(sock, session);
		}
		else break;
	}

	while( (rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN )
		waitsocket(sock, session);

	char* exitsignal=0;
	int exitcode = 0;
	if (rc)	{
		exitcode = libssh2_channel_get_exit_status( channel );
		libssh2_channel_get_exit_signal(channel, &exitsignal,
										NULL, NULL, NULL, NULL, NULL);
	}

	if (exitsignal) {
		fprintf(stderr, "\nSignal: %s\n", exitsignal);
		free(exitsignal);
	}
	else {
		//fprintf(stderr, "\nEXIT: %d bytecount: %d\n", exitcode, bytecount);
		(void) exitcode;
	}

	libssh2_channel_free(channel);
	l_ssh_shutdown(session);
	close(sock);
	libssh2_exit();

	return 0;
}

static const LUA_REG_TYPE scp_map[] = {
	{ LSTRKEY( "get" ),	 LFUNCVAL( l_scp_get	 ) },
	{ LSTRKEY( "put" ),	 LFUNCVAL( l_scp_put	 ) },
	{ LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE ssh_map[] = {
	{ LSTRKEY( "exec" ),	 LFUNCVAL( l_ssh_exec	) },
	{ LNILKEY, LNILVAL }
};

#endif

/*

Example:

net.scp.get("your_domain.com",22,"/server_path/server_file","/local_path/local_file","user","pass")
net.scp.put("your_domain.com",22,"/local_path/local_file","/server_path/server_file","user","pass")
net.ssh.exec("your_domain.com",22,"/usr/bin/your_command and args","user","pass")

*/
