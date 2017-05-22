/*******************************************************************************
 * Copyright (c) 2015, http://www.jbox.dk/
 * All rights reserved. Released under the BSD license.
 * httpsrv.c 1.0 01/01/2016 (Simple Http Server)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/
#include "luartos.h"

#if LUA_USE_HTTP

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include <sys/panic.h>

#include <pthread/pthread.h>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/syslog.h>

#define PORT           80
#define SERVER         "lua-rtos-http-server/1.0"
#define PROTOCOL       "HTTP/1.1"
#define RFC1123FMT     "%a, %d %b %Y %H:%M:%S GMT"
#define HTTP_BUFF_SIZE 1024
#define CAPTIVE_SERVER_NAME	"config-esp32-settings"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

char *strcasestr(const char *haystack, const char *needle);

static lua_State *LL=NULL;

int is_lua(char *name) {
	char *ext = strrchr(name, '.');
	if (!ext) return 0;

	if (strcmp(ext, ".lua")  == 0) return 1;
	return 0;
}

char *get_mime_type(char *name) {

	char *ext = strrchr(name, '.');
	if (!ext) return NULL;
	if (strcmp(ext, ".html") == 0) return "text/html";
	if (strcmp(ext, ".htm")  == 0) return "text/html";
	if (strcmp(ext, ".txt")  == 0) return "text/html";
	if (strcmp(ext, ".jpg")  == 0) return "image/jpeg";
	if (strcmp(ext, ".jpeg") == 0) return "image/jpeg";
	if (strcmp(ext, ".gif")  == 0) return "image/gif";
	if (strcmp(ext, ".png")  == 0) return "image/png";
	if (strcmp(ext, ".css")  == 0) return "text/css";
	if (strcmp(ext, ".au")   == 0) return "audio/basic";
	if (strcmp(ext, ".wav")  == 0) return "audio/wav";
	if (strcmp(ext, ".avi")  == 0) return "video/x-msvideo";
	if (strcmp(ext, ".mpeg") == 0) return "video/mpeg";
	if (strcmp(ext, ".mpg")  == 0) return "video/mpeg";
	if (strcmp(ext, ".mp3")  == 0) return "audio/mpeg";
	if (strcmp(ext, ".svg")  == 0) return "image/svg+xml";
	if (strcmp(ext, ".pdf")  == 0) return "application/pdf";
	return NULL;
}

void send_headers(FILE *f, int status, char *title, char *extra, char *mime, int length) {
	fprintf(f, "%s %d %s\r\n", PROTOCOL, status, title);
	fprintf(f, "Server: %s\r\n", SERVER);
	if (extra) fprintf(f, "%s\r\n", extra);
	if (mime) fprintf(f, "Content-Type: %s\r\n", mime);

	if (length >= 0) {
		fprintf(f, "Content-Length: %d\r\n", length);
	} else {
		fprintf(f, "Transfer-Encoding: chunked\r\n");
	}

	fprintf(f, "Connection: close\r\n");

	fprintf(f, "Cache-Control: no-cache, no-store, must-revalidate\r\n");
	fprintf(f, "no-cache\r\n");
	fprintf(f, "0\r\n");

	fprintf(f, "\r\n");
}

#define HTTP_STATUS_LEN     3
#define HTTP_ERROR_LINE_1   "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\r\n"
#define HTTP_ERROR_LINE_2   "<BODY><H4>%d %s</H4>\r\n"
#define HTTP_ERROR_LINE_3   "%s\r\n"
#define HTTP_ERROR_LINE_4   "</BODY></HTML>\r\n"
#define HTTP_ERROR_VARS_LEN (2 * 5)

void send_error(FILE *f, int status, char *title, char *extra, char *text) {
	int len = strlen(title) * 2 +
			  HTTP_STATUS_LEN * 2 +
			  strlen(text) +
			  strlen(HTTP_ERROR_LINE_1) +
			  strlen(HTTP_ERROR_LINE_2) +
			  strlen(HTTP_ERROR_LINE_3) +
			  strlen(HTTP_ERROR_LINE_4) -
			  HTTP_ERROR_VARS_LEN;

	send_headers(f, status, title, extra, "text/html", len);
	fprintf(f, HTTP_ERROR_LINE_1, status, title);
	fprintf(f, HTTP_ERROR_LINE_2, status, title);
	fprintf(f, HTTP_ERROR_LINE_3, text);
	fprintf(f, HTTP_ERROR_LINE_4);
}

static void chunk(FILE *f, const char *fmt, ...) {
	char *buffer;
	va_list args;

	buffer = (char *)malloc(2048);
	if (buffer) {
		*buffer = '\0';

		va_start(args, fmt);

		vsnprintf(buffer, 2048, fmt, args);

		fprintf(f, "%x\r\n", strlen(buffer));
		fprintf(f, "%s\r\n", buffer);

		va_end(args);

		free(buffer);
	}
}

void send_file(FILE *f, char *path, struct stat *statbuf, char *requestdata) {
	int n;
	char data[HTTP_BUFF_SIZE];
	FILE *file = fopen(path, "r");

	if (!file) {
		send_error(f, 403, "Forbidden", NULL, "Access denied.");
	} else if (is_lua(path)) {
		fclose(file);
		
		send_headers(f, 200, "OK", NULL, "text/html", -1);

		lua_pushstring(LL, (requestdata && *requestdata) ? requestdata:"");
		lua_setglobal(LL, "http_request");

		int rc = luaL_dofile(LL, path);
		(void) rc;

		lua_getglobal(LL, "http_response");
		size_t rlen = 0;
		const char *response = lua_tostring(LL, -1);
		if (!lua_isnil(LL,-1)) {
		rlen = lua_rawlen(LL, -1);
			if (rlen>0) {
				fprintf(f, "%x\r\n", rlen);
				fwrite(response, 1, rlen, f);
				fprintf(f, "\r\n");
			}
		}
		lua_pop(LL, 1);
				
		fprintf(f, "0\r\n\r\n");
	} else {
		int length = S_ISREG(statbuf->st_mode) ? statbuf->st_size : -1;
		send_headers(f, 200, "OK", NULL, get_mime_type(path), length);
		while ((n = fread(data, 1, sizeof (data), file)) > 0) fwrite(data, 1, n, f);
		fclose(file);
	}
}

int process(FILE *f) {
	char buf[HTTP_BUFF_SIZE];
	char *method;
	char *path;
	char *data = NULL;
	char *host = NULL;
	char *protocol;
	struct stat statbuf;
	char hostbuf[HTTP_BUFF_SIZE];
	char pathbuf[HTTP_BUFF_SIZE];
	int len;

	if (!fgets(buf, sizeof (buf), f)) return -1;

	method = strtok(buf, " ");
	path = strtok(NULL, " ");
	protocol = strtok(NULL, "\r");

	if(path) {
		data = strchr(path, '?');
	  if (data) {
	  	*data = 0; //cut off the path
	  	data++; //point to start of params
	  }
	}
		
	while (fgets(hostbuf, sizeof (hostbuf), f)) {
		host = strcasestr(hostbuf, "Host:");
		if (host) {
			host = strtok(host, " "); //Host:
	  	host = strtok(NULL, "\r"); //the actual host

			if (0 == strcasecmp(CAPTIVE_SERVER_NAME, host)) {
				break; //done parsing headers
			}
			else {
				//redirect
				snprintf(pathbuf, sizeof (pathbuf), "Location: http://%s/", CAPTIVE_SERVER_NAME);
				send_headers(f, 302, "Found", pathbuf, "text/html", -1);
				return 0;
			}
		}
	}
		
	if (!method || !path || !protocol) return -1;

	syslog(LOG_DEBUG, "http: %s %s %s\r", method, path, protocol);

	fseek(f, 0, SEEK_CUR); // Force change of stream direction

	if (strcasecmp(method, "GET") != 0) {
		send_error(f, 501, "Not supported", NULL, "Method is not supported.");
	} else if (stat(path, &statbuf) < 0) {
		send_error(f, 404, "Not Found", NULL, "File not found.");
		syslog(LOG_DEBUG, "http: %s Not found\r", path);
	} else if (S_ISDIR(statbuf.st_mode)) {
		len = strlen(path);
		if (len == 0 || path[len - 1] != '/') {
			snprintf(pathbuf, sizeof (pathbuf), "Location: %s/", path);
			send_error(f, 302, "Found", pathbuf, "Directories must end with a slash.");
		} else {
			snprintf(pathbuf, sizeof (pathbuf), "%sindex.lua", path);
			if (stat(pathbuf, &statbuf) >= 0) {
				send_file(f, pathbuf, &statbuf, data);
			} else {
				  snprintf(pathbuf, sizeof (pathbuf), "%sindex.html", path);
				  if (stat(pathbuf, &statbuf) >= 0) {
					  send_file(f, pathbuf, &statbuf, data);
				  } else {
					  DIR *dir;
					  struct dirent *de;

					  send_headers(f, 200, "OK", NULL, "text/html", -1);
					  chunk(f, "<HTML><HEAD><TITLE>Index of %s</TITLE></HEAD><BODY>", path);
					  chunk(f, "<H4>Index of %s</H4>", path);

					  chunk(f, "<TABLE>");
					  chunk(f, "<TR>");
					  chunk(f, "<TH style=\"width: 250;text-align: left;\">Name</TH><TH style=\"width: 100px;text-align: right;\">Size</TH>");
					  chunk(f, "</TR>");

					  if (len > 1) {
						  chunk(f, "<TR>");
					  	chunk(f, "<TD><A HREF=\"..\">..</A></TD><TD></TD>");
						  chunk(f, "</TR>");
					  }

					  dir = opendir(path);
					  while ((de = readdir(dir)) != NULL) {
						  strcpy(pathbuf, path);
						  strcat(pathbuf, de->d_name);

						  stat(pathbuf, &statbuf);

						  chunk(f, "<TR>");
						  chunk(f, "<TD>");
						  chunk(f, "<A HREF=\"%s%s\">", de->d_name, S_ISDIR(statbuf.st_mode) ? "/" : "");
						  chunk(f, "%s%s", de->d_name, S_ISDIR(statbuf.st_mode) ? "/</A>" : "</A> ");
						  chunk(f, "</TD>");
						  chunk(f, "<TD style=\"text-align: right;\">");
						  if (!S_ISDIR(statbuf.st_mode)) {
								chunk(f, "%d", (int)statbuf.st_size);
						  }
						  chunk(f, "</TD>");
						  chunk(f, "</TR>");
					  }
					  closedir(dir);


					  chunk(f, "</TABLE>");

					  chunk(f, "</BODY></HTML>");

					  fprintf(f, "0\r\n\r\n");
				  }
			   }
		}
	} else {
		send_file(f, path, &statbuf, data);
	}

	return 0;
}

static void *http_thread(void *arg) {
	struct sockaddr_in sin;
	int server;

	server = socket(AF_INET, SOCK_STREAM, 0);


	sin.sin_family      = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port        = htons(PORT);
	bind(server, (struct sockaddr *) &sin, sizeof (sin));

	listen(server, 5);
	syslog(LOG_INFO, "http: server listening on port %d\n", PORT);

	while (1) {
		int client;

		// Wait for a request ...
		if ((client = accept(server, NULL, NULL)) != -1) {
			// We waiting for send all data before close socket's stream
			struct linger so_linger;

			so_linger.l_onoff  = 1;
			so_linger.l_linger = 10;

			setsockopt(client, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));

			// Set a timeout for send / receive
			struct timeval tout;

			tout.tv_sec = 10;
			tout.tv_usec = 0;

			setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout));
			setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &tout, sizeof(tout));

			// Create the socket stream
			FILE *stream = fdopen(client, "a+");
			process(stream);
			fclose(stream);
		}
	}

	close(server);

	pthread_exit(NULL);
}

void http_start(lua_State* L) {
	pthread_attr_t attr;
	struct sched_param sched;
	pthread_t thread;
	int res;

	LL=L;

	// Init thread attributes
	pthread_attr_init(&attr);

	// Set stack size
	pthread_attr_setstacksize(&attr, CONFIG_LUA_RTOS_LUA_STACK_SIZE);

	// Set priority
	sched.sched_priority = CONFIG_LUA_RTOS_LUA_TASK_PRIORITY;
	pthread_attr_setschedparam(&attr, &sched);

	// Set CPU
	cpu_set_t cpu_set = CONFIG_LUA_RTOS_LUA_TASK_CPU;
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);

	// Create thread
	res = pthread_create(&thread, &attr, http_thread, NULL);
	if (res) {
		panic("Cannot start http_thread");
	}
}

void http_stop() {

}

#endif
