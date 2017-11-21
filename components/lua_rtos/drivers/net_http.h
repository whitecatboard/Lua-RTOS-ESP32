/*
 * Lua RTOS, network http client
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

#ifndef NET_HTTP_H_
#define NET_HTTP_H_

#include "openssl/ssl.h"
#include "lwip/sockets.h"

typedef struct {
	int socket;
    SSL_CTX *ctx;
    SSL *ssl;
    const char *host;
    const char *port;
} net_http_client_t;

typedef struct {
	net_http_client_t *client;
	uint32_t code;
	uint32_t size;
	uint32_t len;
} net_http_response_t;

#define HTTP_CLIENT_INITIALIZER {-1,NULL,NULL,"",""}

driver_error_t *net_http_create_client(const char *server, const char *port, net_http_client_t *client);
driver_error_t *net_http_destroy_client(net_http_client_t *client);
driver_error_t *net_http_get(net_http_client_t *client, const char *res, net_http_response_t *response);
driver_error_t *net_http_read_response(net_http_response_t *response, uint8_t *buffer, size_t size);

#endif
