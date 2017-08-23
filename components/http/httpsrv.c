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

#include "preprocessor.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include <sys/panic.h>
#include <sys/delay.h>

#include "esp_wifi_types.h"
#include <pthread/pthread.h>
#include <esp_wifi.h>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/syslog.h>

#define SERVER         "lua-rtos-http-server/1.0"
#define PROTOCOL       "HTTP/1.1"
#define RFC1123FMT     "%a, %d %b %Y %H:%M:%S GMT"
#define HTTP_BUFF_SIZE 1024
#define CAPTIVE_SERVER_NAME	"config-esp32-settings"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <drivers/net.h>

char *strcasestr(const char *haystack, const char *needle);
driver_error_t *wifi_stat(ifconfig_t *info);
driver_error_t *wifi_check_error(esp_err_t error);

extern int captivedns_start(lua_State* L);
extern void captivedns_stop();
extern int captivedns_running();

static lua_State *LL=NULL;
static u8_t wifi_mode = WIFI_MODE_STA;
static u8_t http_refcount = 0;
static u8_t volatile http_shutdown = 0;
static u8_t http_captiverun = 0;
static char ip4addr[IP4ADDR_STRLEN_MAX];
static int server = 0;

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

		int length = strlen(buffer);
		if(length) { //a length of zero would end our whole transfer
			fprintf(f, "%x\r\n", length);
			fprintf(f, "%s\r\n", buffer);
		}

		va_end(args);

		free(buffer);
	}
}

int http_print(lua_State* L) {

		lua_getglobal(L, "http_stream_handle");
		if (!lua_islightuserdata(L, -1)) {
        return luaL_error(L, "this function may only be called inside a lua script served by httpsrv");
    }
    FILE *f = (FILE*)lua_touserdata(L, -1);

		if (!f) {
        return luaL_error(L, "this function may only be called inside a lua script served by httpsrv");
    }

    int nargs = lua_gettop(L);
    for (int i=1; i <= nargs; i++) {
        if (lua_isstring(L, i)) {
            chunk(f, "%s", lua_tostring(L, i));
        }
        else {
        		/* non-strings handling not reqired */
        }
    }

    return 0;
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
		fflush(f);

		lua_pushstring(LL, (requestdata && *requestdata) ? requestdata:"");
		lua_setglobal(LL, "http_request");

		lua_pushlightuserdata(LL, (void*)f);
		lua_setglobal(LL, "http_stream_handle");

		char ppath[PATH_MAX];

		strcpy(ppath, path);
		if (strlen(ppath) < PATH_MAX - 1) {
			strcat(ppath, "p");
		}

		http_process_lua_page(path,ppath);

		int rc = luaL_dofile(LL, ppath);
		(void) rc;

		fflush(f);

		fprintf(f, "0\r\n\r\n");

		lua_pushlightuserdata(LL, (void*)0);
		lua_setglobal(LL, "http_stream_handle");

	} else {
		int length = S_ISREG(statbuf->st_mode) ? statbuf->st_size : -1;
		send_headers(f, 200, "OK", NULL, get_mime_type(path), length);
		while ((n = fread(data, 1, sizeof (data), file)) > 0) fwrite(data, 1, n, f);
		fclose(file);
	}
}

//newpath must have a size of HTTP_BUFF_SIZE
//rootpath must not be relative, should be '/' or any valid file system path
//reqpath may be relative
//addpath must not be relative and should not start with '/', or should be NULL
int filepath_merge(char *newpath, const char *rootpath, const char *reqpath, const char *addpath) {
	char *pos = newpath;
	int seglen;

	//newpath has a size of HTTP_BUFF_SIZE
	memset(newpath, 0, HTTP_BUFF_SIZE);

  // treat null as an empty path.
  if (!reqpath)
      reqpath = "";

	// copy the root path
	seglen = strlen(rootpath);
	if(seglen==0) {
	}
	else if(seglen==1) {
		// no need to memcpy
		*pos = *rootpath;
		pos++;
	}
	else {
		memcpy(newpath, rootpath, seglen);
		pos += seglen;
	}

	// make sure the root path ends with a slash
	if(*(pos-1) != '/') {
		*pos = '/';
		pos++;
	}
	*pos = 0;

	// remove leading slashes from the request path
	while (*reqpath=='/') {
		reqpath++;
	}

	int rootlen = pos - newpath;
	int pathlen = rootlen;

	// add the request path segment by segment
	while (*reqpath) {
      // finding the closing '/'
      const char *next = reqpath;
      while (*next && (*next != '/')) {
          ++next;
      }
      seglen = next - reqpath;

      if (seglen == 0 || (seglen == 1 && reqpath[0] == '.')) {
          // noop segment (/ or ./) => skip
      }
      else if (seglen == 2 && reqpath[0] == '.' && reqpath[1] == '.') {
          // backpath (../)

          // try to crop the prior segment
          do {
              --pathlen;
          } while (pathlen && newpath[pathlen - 1] != '/');

          // now test if we are above root path length and
          // get back if necessary
          if (pathlen < rootlen) {
              pathlen = rootlen;
          }
      }
      else {
          // an actual segment, append to dest path
          if (*next) {
              seglen++;
          }
          memcpy(newpath + pathlen, reqpath, seglen);
          pathlen += seglen;
      }

      // skip over trailing slash => next segment
      if (*next) {
          ++next;
      }

      reqpath = next;
  }
  pos = newpath + pathlen;

	// add the additional path
	if (addpath) {
		// make sure the current path ends with a slash
		if(*(pos-1) != '/') {
			*pos = '/';
			pos++;
		}

		// remove leading slashes from the request path
		while (*addpath=='/') {
			addpath++;
		}

		seglen = strlen(addpath);
		if(pos - newpath + seglen < HTTP_BUFF_SIZE)
			memcpy(pos, addpath, seglen);

		pos += seglen;
	}
	*pos = 0;

	return 1;
}

int process(FILE *f) {
	char buf[HTTP_BUFF_SIZE];
	char *method;
	char *path;
	char *data = NULL;
	char *host = NULL;
	char *protocol;
	struct stat statbuf;
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

	//only in AP mode we redirect arbitrary host names to our own host name
	if (wifi_mode != WIFI_MODE_STA) {
	
		//find the Host: header and check if it matches our IP or captive server name
		while (fgets(pathbuf, sizeof (pathbuf), f)) {

			//quick check if the first char matches, only then do strcasestr
			if(pathbuf[0]=='h' || pathbuf[0]=='H') {			
				host = strcasestr(pathbuf, "Host:");
				
				//check if the line begins with "Host:"
				if (host==(char *)pathbuf) {
					host = strtok(host, ":");  //Host:
					host = strtok(NULL, "\r"); //the actual host
					while(*host==' ') host++;  //skip spaces after the :

					if (0 == strcasecmp(CAPTIVE_SERVER_NAME, host) ||
							0 == strcasecmp(ip4addr, host)) {
						break; //done parsing headers
					}
					else {
						//redirect
						snprintf(pathbuf, sizeof (pathbuf), "Location: http://%s/", CAPTIVE_SERVER_NAME);
						send_headers(f, 302, "Found", pathbuf, NULL, 0);
						return 0;
					}
				}
			}

		} // while
	} // AP mode

	if (!method || !path) return -1; //protocol may be omitted

	syslog(LOG_DEBUG, "http: %s %s %s\r", method, path, protocol);

	fseek(f, 0, SEEK_CUR); // force change of stream direction

	if (strcasecmp(method, "GET") != 0) {
		send_error(f, 501, "Not supported", NULL, "Method is not supported.");
	} else if (!filepath_merge(pathbuf, CONFIG_LUA_RTOS_HTTP_SERVER_DOCUMENT_ROOT, path, NULL)) {
		send_error(f, 404, "Not Found", NULL, "File not found.");
		syslog(LOG_DEBUG, "http: invalid path requested: %s\r", path);
	} else if (stat(pathbuf, &statbuf) < 0) {
		send_error(f, 404, "Not Found", NULL, "File not found.");
		syslog(LOG_DEBUG, "http: %s Not found\r", pathbuf);
	} else if (S_ISDIR(statbuf.st_mode)) {
		len = strlen(path);
		if (len == 0 || path[len - 1] != '/') {
			//send a redirect
			snprintf(pathbuf, sizeof (pathbuf), "Location: %s/", path);
			send_error(f, 302, "Found", pathbuf, "Directories must end with a slash.");
		} else {
			filepath_merge(pathbuf, CONFIG_LUA_RTOS_HTTP_SERVER_DOCUMENT_ROOT, path, "index.lua");
			if (stat(pathbuf, &statbuf) >= 0) {
				send_file(f, pathbuf, &statbuf, data);
			} else {
					filepath_merge(pathbuf, CONFIG_LUA_RTOS_HTTP_SERVER_DOCUMENT_ROOT, path, "index.html");
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

					  filepath_merge(pathbuf, CONFIG_LUA_RTOS_HTTP_SERVER_DOCUMENT_ROOT, path, NULL); //restore folder pathbuf
					  dir = opendir(pathbuf);
					  while ((de = readdir(dir)) != NULL) {
					  	filepath_merge(pathbuf, CONFIG_LUA_RTOS_HTTP_SERVER_DOCUMENT_ROOT, path, de->d_name);
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
		send_file(f, pathbuf, &statbuf, data);
	}

	return 0;
}

static void http_net_callback(system_event_t *event){
	syslog(LOG_DEBUG, "event: %d\r\n", event->event_id);

	switch (event->event_id) {
		case SYSTEM_EVENT_STA_START:                /**< ESP32 station start */
			if (wifi_mode != WIFI_MODE_STA) {
				wifi_mode = WIFI_MODE_STA;

				syslog(LOG_DEBUG, "http: switched to non-captive mode\n");
				http_captiverun = captivedns_running();
				if (http_captiverun) {
					syslog(LOG_DEBUG, "http: auto-stopping captive dns service\n");
					captivedns_stop();
				}
			}
			break;

		case SYSTEM_EVENT_AP_START:                 /**< ESP32 soft-AP start */
			if (wifi_mode != WIFI_MODE_AP) {
				driver_error_t *error;
				ifconfig_t info;

				wifi_mode = WIFI_MODE_AP;
				if ((error = wifi_stat(&info))) {
					strcpy(ip4addr, "0.0.0.0");
				}
				else {
					strcpy(ip4addr, inet_ntoa(info.ip));
				}
				syslog(LOG_DEBUG, "http: switched to captive mode on %s\n", ip4addr);
				if (http_captiverun) {
					syslog(LOG_DEBUG, "http: auto-restarting captive dns service\n");
					captivedns_start(LL);
				}
			}
			break;

		default :
			break;
	}
}

static void *http_thread(void *arg) {
	struct sockaddr_in sin;

	if(0 == server) {
		server = socket(AF_INET, SOCK_STREAM, 0);
		LWIP_ASSERT("httpd_init: socket failed", server >= 0);
		if(0 > server) {
			syslog(LOG_ERR, "couldn't create server socket\n");
			pthread_exit(NULL);
		}

		u8_t one = 1;
		setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

		memset(&sin, 0, sizeof(sin));
		sin.sin_family      = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port        = htons(CONFIG_LUA_RTOS_HTTP_SERVER_PORT);
		int rc = bind(server, (struct sockaddr *) &sin, sizeof (sin));
		LWIP_ASSERT("httpd_init: bind failed", rc == 0);
		if(0 != rc) {
			syslog(LOG_ERR, "couldn't bind to port %d\n", CONFIG_LUA_RTOS_HTTP_SERVER_PORT);
			pthread_exit(NULL);
		}

		listen(server, 5);
		LWIP_ASSERT("httpd_init: listen failed", server >= 0);
		if(0 > server) {
			syslog(LOG_ERR, "couldn't listen on port %d\n", CONFIG_LUA_RTOS_HTTP_SERVER_PORT);
			pthread_exit(NULL);
		}

		// Set the timeout for accept
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	}
	
	syslog(LOG_INFO, "http: server listening on port %d\n", CONFIG_LUA_RTOS_HTTP_SERVER_PORT);

	http_refcount++;
	while (!http_shutdown) {
		int client;

		// Wait for a request ...
		if ((client = accept(server, NULL, NULL)) != -1) {

			// We wait for send all data before close socket's stream
			struct linger so_linger;
			so_linger.l_onoff  = 1;
			so_linger.l_linger = 2;
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

	syslog(LOG_INFO, "http: server shutting down on port %d\n", CONFIG_LUA_RTOS_HTTP_SERVER_PORT);

	driver_error_t *error;
	if ((error = net_event_unregister_callback(http_net_callback))) {
		syslog(LOG_WARNING, "couldn't unregister net callback\n");
	}

	/* it's not ideal to keep the server_socket open as it is blocked
	   but now at least the httpsrv can be restarted from lua scripts.
	   if we'd properly close the socket we would need to bind() again
	   when restarting the httpsrv but bind() will succeed only after
	   several (~4) minutes: http://lwip.wikia.com/wiki/Netconn_bind
	   "Note that if you try to bind the same address and/or port you
	    might get an error (ERR_USE, address in use), even if you
	    delete the netconn. Only after some time (minutes) the
	    resources are completely cleared in the underlying stack due
	    to the need to follow the TCP specification and go through
	    the TCP timewait state."
	*/

	http_refcount--;
	pthread_exit(NULL);
}

int http_start(lua_State* L) {
	//wait until an ongoing shutdown has been finished
	while(http_refcount && http_shutdown) delay(10);
	
	if(!http_refcount) {
		pthread_attr_t attr;
		struct sched_param sched;
		pthread_t thread;
		ifconfig_t info;
		int res;
		driver_error_t *error;

		LL=L;
		strcpy(ip4addr, "0.0.0.0");

#if CONFIG_WIFI_ENABLED
		if ((error = wifi_check_error(esp_wifi_get_mode((wifi_mode_t*)&wifi_mode)))) {
			return luaL_error(L, "couldn't get wifi mode");
		}
		if (wifi_mode != WIFI_MODE_STA) {
			if ((error = wifi_stat(&info))) {
				return luaL_error(L, "couldn't get wifi IP");
			}
			strcpy(ip4addr, inet_ntoa(info.ip));
		}
#else
#if CONFIG_SPI_ETHERNET
		if ((error = spi_eth_stat(&info))) {
			return luaL_error(L, "couldn't get spi eth IP");
		}
		strcpy(ip4addr, inet_ntoa(info.ip));
#endif
#endif

		if ((error = net_event_register_callback(http_net_callback))) {
			syslog(LOG_WARNING, "couldn't register net callback, please restart http service from lua after changing connectivity\n");
		}

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
		http_shutdown = 0;
		res = pthread_create(&thread, &attr, http_thread, NULL);
		if (res) {
			return luaL_error(L, "couldn't start http_thread");
		}
	}
	
	return 0;
}

void http_stop() {
	if(http_refcount) {
		http_shutdown++;
	}
}

#endif
