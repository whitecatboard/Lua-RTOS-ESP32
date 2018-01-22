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
 * Lua RTOS, network http client
 *
 */

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/syslog.h>

#include <drivers/net.h>
#include <drivers/net_http.h>

#define HTTP_CLIENT_GET "GET %s HTTP/1.1\r\nHost: %s:%s \r\n\r\n"
#define HTTP_CLIENT_GET_SIZE (strlen(HTTP_CLIENT_GET) - (3 * 2))

static int net_http_read_line(net_http_client_t *client, char *buffer, size_t len) {
	size_t size = 0;
	char c;

	while ((len > 0) && (SSL_read(client->ssl, &c, 1) > 0)) {
		*buffer++ = c;
		len--;
		size++;

		if (c == '\n') {
			break;
		}
	}

	*buffer = 0x00;

	if (size >= 2) {
		// Buffer must end by \r\n
		buffer--;
		buffer--;

		if (strcmp(buffer,"\r\n") == 0) {
			// Remove \r\n
			*buffer = 0x00;

			size = size - 2;

			return (size == 0?-1:size);
		} else {
			// Error
			return -2;
		}
	} else if (size == 1) {
		// Error
		return -2;
	} else {
		return 0;
	}
}

static int net_http_read_chunk(net_http_client_t *client, uint8_t *buffer, size_t len) {
	return SSL_read(client->ssl, buffer, len);
}

driver_error_t *net_http_create_client(const char *server, const char *port, net_http_client_t *client) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int connected = 0;

    // Obtain address matching host/port
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	if (getaddrinfo(server, port, &hints, &result) != 0) {
		return driver_error(NET_DRIVER, NET_ERR_NAME_CANNOT_BE_RESOLVED,NULL);
	}

	// Try each address until we successfully connect
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		if ((client->socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) < 0) {
			continue;
		}

		if (!connect(client->socket, rp->ai_addr, rp->ai_addrlen)) {
			connected = 1;
			break;
		}

		close(client->socket);
	}

	if (!connected) {
    	net_http_destroy_client(client);

    	return driver_error(NET_DRIVER, NET_ERR_NAME_CANNOT_CONNECT,NULL);
	}

	struct timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	setsockopt(client->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    client->ctx = SSL_CTX_new(TLSv1_1_client_method());
    if (!client->ctx) {
    	net_http_destroy_client(client);

    	return driver_error(NET_DRIVER, NET_ERR_CANNOT_CREATE_SSL,"context");
    }

    client->ssl = SSL_new(client->ctx);
    if (!client->ssl) {
    	net_http_destroy_client(client);

    	return driver_error(NET_DRIVER, NET_ERR_CANNOT_CREATE_SSL,"session");
    }

    SSL_set_fd(client->ssl, client->socket);

    if (!SSL_connect(client->ssl)) {
    	net_http_destroy_client(client);

		return driver_error(NET_DRIVER, NET_ERR_CANNOT_CONNECT_SSL,NULL);
    }

    client->host = server;
    client->port = port;

    return NULL;
}

driver_error_t *net_http_destroy_client(net_http_client_t *client) {
	if (client->ssl) {
		SSL_shutdown(client->ssl);
		SSL_free(client->ssl);

		client->ssl = NULL;
	}

	if (client->socket >= 0) {
		close(client->socket);

		client->socket = -1;
	}

	if (client->ctx) {
		SSL_CTX_free(client->ctx);

		client->ctx = NULL;
	}

	return NULL;
}

driver_error_t *net_http_get(net_http_client_t *client, const char *res, net_http_response_t *response) {
	char *http_request;
	char http_response[2048];
	char *protocol;
	char *version;
	char *code;
	char *header;
	char *content_type;
	char *content_length;
	int size;

	// Allocate space for buffer
	http_request = calloc(1, HTTP_CLIENT_GET_SIZE + strlen(res) + strlen(client->host) + strlen(client->port) + 1);
	if (!http_request) {
		return driver_error(NET_DRIVER, NET_ERR_NOT_ENOUGH_MEMORY, NULL);
	}

	// Send
    sprintf(http_request, HTTP_CLIENT_GET, res, client->host, client->port);
    if (SSL_write(client->ssl, http_request, strlen(http_request)) <= 0) {
    	free(http_request);

    	return NULL;
    }

    free(http_request);

    // Read
    if (net_http_read_line(client, http_response, sizeof(http_response)) <= 0) {
    	return NULL;
    }

	protocol = strtok(http_response, "/");
	if (strcmp(protocol,"HTTP") != 0) {
		return driver_error(NET_DRIVER, NET_ERR_INVALID_RESPONSE, "not HTTP protocol");
	}

	version = strtok(NULL, " ");
	(void)(version);

	code = strtok(NULL, " ");

	response->code = atoi(code);

	if (response->code == 200) {
		for(;;) {
			size = net_http_read_line(client, http_response, sizeof(http_response));
			if (size == -2) {
				// Error
				return driver_error(NET_DRIVER, NET_ERR_INVALID_RESPONSE, "http response");
			} else if (size == -1){
				// End headers
				break;
			} else if (size == 0) {
				// Error
				return driver_error(NET_DRIVER, NET_ERR_INVALID_RESPONSE, "http response");
			} else {
				// Header
				header = strtok(http_response, ":");
				if (strcmp(header,"Server") == 0) {
				} else if (strcmp(header,"Date") == 0) {
				} else if (strcmp(header,"Content-Type") == 0) {
					content_type = strtok(NULL, "");
					content_type++;
					if (strcmp(content_type, "application/octet-stream") != 0) {
						return driver_error(NET_DRIVER, NET_ERR_INVALID_CONTENT, "Content-Type");
					}
				} else if (strcmp(header,"Content-Length") == 0) {
					content_length = strtok(NULL, "");
					content_length++;
					response->size = atoi(content_length);
				} else if (strcmp(header,"Connection") == 0) {
				} else if (strcmp(header,"X-Powered-By") == 0) {
				} else if (strcmp(header,"Content-Description") == 0) {
				} else if (strcmp(header,"Content-Disposition") == 0) {
				} else if (strcmp(header,"Expires") == 0) {
				} else if (strcmp(header,"Pragma") == 0) {
				}
			}
		}

		response->client = client;
	}

    return NULL;
}

driver_error_t *net_http_read_response(net_http_response_t *response, uint8_t *buffer, size_t size) {
	response->len = 0;

	if (response->size > 0) {
		int readed = net_http_read_chunk(response->client, buffer, (response->size > size)?size:response->size);
		if (readed >= 0) {
			response->size = response->size - readed;
		}

		response->len = readed;
	}

	return NULL;
}
