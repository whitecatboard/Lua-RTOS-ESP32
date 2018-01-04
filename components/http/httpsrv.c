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

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_HTTP_SERVER

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
#include "esp_log.h"
#include <pthread.h>
#include <esp_wifi.h>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/syslog.h>
#include <sys/mount.h>

#include <openssl/ssl.h>
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#define SERVER_ID      "lua-rtos-http-server/1.0"
#define PROTOCOL       "HTTP/1.1"
#define RFC1123FMT     "%a, %d %b %Y %H:%M:%S GMT"
#define HTTP_BUFF_SIZE 1024
#define CAPTIVE_SERVER_NAME	"config-esp32-settings"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <drivers/net.h>
#include <drivers/spi_eth.h>


char *strcasestr(const char *haystack, const char *needle);
driver_error_t *wifi_stat(ifconfig_t *info);
driver_error_t *wifi_check_error(esp_err_t error);

extern int captivedns_start(lua_State* L);
extern void captivedns_stop();
extern int captivedns_running();

static lua_State *LL=NULL;
static wifi_mode_t wifi_mode = WIFI_MODE_STA;
static u8_t http_refcount = 0;
static u8_t volatile http_shutdown = 0;
static u8_t http_captiverun = 0;
static char ip4addr[IP4ADDR_STRLEN_MAX];
static int socket_server_normal = 0;
static int socket_server_secure = 0;

typedef struct {
  int port;
  int *server; //socket
  const int secure;
  char *certificate;
  char *private_key;
} http_server_config;

#define HTTP_Normal_initializer { CONFIG_LUA_RTOS_HTTP_SERVER_PORT, &socket_server_normal, 0, NULL, NULL }
#define HTTP_Secure_initializer { CONFIG_LUA_RTOS_HTTP_SERVER_PORT_SSL, &socket_server_secure, 1, NULL, NULL } //cert and privkey need to be supplied from lua

typedef struct {
  http_server_config *config;
  FILE *file;
  SSL *ssl;
} http_request_handle;

#define HTTP_Request_Normal_initializer { config, stream, NULL };
#define HTTP_Request_Secure_initializer { config, NULL, ssl };

static http_server_config http_normal = HTTP_Normal_initializer;
static http_server_config http_secure = HTTP_Secure_initializer;

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

static int request_write(http_request_handle *request, char *buffer, int length) {
	return (request->config->secure) ? SSL_write(request->ssl, buffer, length) : fwrite(buffer, 1, length, request->file);
}

static int do_printf(http_request_handle *request, const char *fmt, ...) {
	int ret = 0;
	char *buffer;
	va_list args;

	buffer = (char *)malloc(2048);
	if (buffer) {
		*buffer = '\0';

		va_start(args, fmt);
		vsnprintf(buffer, 2048, fmt, args);

		int length = strlen(buffer);
		if(length) { //don't try to transfer "nothing"
			ret = request_write(request, buffer, length);
		}

		va_end(args);
		free(buffer);
	}
	return ret;
}

char *do_gets(char *s, int size, http_request_handle *request) {

	if (0 == request->config->secure)
		return fgets(s, size, request->file);

	int rc;
	char *c = s;
	int done = 0;
	while (c < (s + size - 1) && !done) {
  	rc = SSL_read(request->ssl, c, 1);
  	if (rc>0) {
  		if (*c == '\n') done=1;
  		c++;
  	}
  	else if (rc==0) {
  		//no data received or connection is closed
  		done=1;
  	}
  	else {
  		return NULL; //discard half-received data
  	}
  }
  *c = 0;
  return (c == s ? 0 : s);
}

void send_headers(http_request_handle *request, int status, char *title, char *extra, char *mime, int length) {
	do_printf(request, "%s %d %s\r\n", PROTOCOL, status, title);
	do_printf(request, "Server: %s\r\n", SERVER_ID);
	if (extra) do_printf(request, "%s\r\n", extra);
	if (mime) do_printf(request, "Content-Type: %s\r\n", mime);

	if (length >= 0) {
		do_printf(request, "Content-Length: %d\r\n", length);
	} else {
		do_printf(request, "Transfer-Encoding: chunked\r\n");
	}

	do_printf(request, "Connection: close\r\n");

	do_printf(request, "Cache-Control: no-cache, no-store, must-revalidate\r\n");
	do_printf(request, "no-cache\r\n");
	do_printf(request, "0\r\n");

	do_printf(request, "\r\n");
}

#define HTTP_STATUS_LEN     3
#define HTTP_ERROR_LINE_1   "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\r\n"
#define HTTP_ERROR_LINE_2   "<BODY><H4>%d %s</H4>\r\n"
#define HTTP_ERROR_LINE_3   "%s\r\n"
#define HTTP_ERROR_LINE_4   "</BODY></HTML>\r\n"
#define HTTP_ERROR_VARS_LEN (2 * 5)

void send_error(http_request_handle *request, int status, char *title, char *extra, char *text) {
	int len = strlen(title) * 2 +
			  HTTP_STATUS_LEN * 2 +
			  strlen(text) +
			  strlen(HTTP_ERROR_LINE_1) +
			  strlen(HTTP_ERROR_LINE_2) +
			  strlen(HTTP_ERROR_LINE_3) +
			  strlen(HTTP_ERROR_LINE_4) -
			  HTTP_ERROR_VARS_LEN;

	send_headers(request, status, title, extra, "text/html", len);
	do_printf(request, HTTP_ERROR_LINE_1, status, title);
	do_printf(request, HTTP_ERROR_LINE_2, status, title);
	do_printf(request, HTTP_ERROR_LINE_3, text);
	do_printf(request, HTTP_ERROR_LINE_4);
}

static void chunk(http_request_handle *request, const char *fmt, ...) {
	char *buffer;
	va_list args;

	buffer = (char *)malloc(2048);
	if (buffer) {
		*buffer = '\0';

		va_start(args, fmt);

		vsnprintf(buffer, 2048, fmt, args);

		int length = strlen(buffer);
		if(length) { //a length of zero would end our whole transfer
			do_printf(request, "%x\r\n", length);
			do_printf(request, "%s\r\n", buffer);
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
    http_request_handle *request = (http_request_handle*)lua_touserdata(L, -1);

		if (!request) {
        return luaL_error(L, "this function may only be called inside a lua script served by httpsrv");
    }

    int nargs = lua_gettop(L);
    for (int i=1; i <= nargs; i++) {
        if (lua_isstring(L, i)) {
            chunk(request, "%s", lua_tostring(L, i));
        }
        else {
        		/* non-strings handling not reqired */
        }
    }

    return 0;
}

void send_file(http_request_handle *request, char *path, struct stat *statbuf, char *requestdata) {
	int n;
	char *data;

	FILE *file = fopen(path, "r");

	if (!file) {
		send_error(request, 403, "Forbidden", NULL, "Access denied.");
	} else if (is_lua(path)) {
		fclose(file);

		char ppath[PATH_MAX];

		strcpy(ppath, path);
		if (strlen(ppath) < PATH_MAX - 1) {
			strcat(ppath, "p");

			//on fat use the modification time
			if (strcmp((const char *)mount_device(ppath),"fat") == 0) {
				time_t src_mtime = statbuf->st_mtime;

				if (stat(ppath, statbuf) == 0) {
					if (src_mtime > statbuf->st_mtime) {
						//syslog(LOG_WARNING, "re-preprocessing %s due to mtime %i > %i\n", path, src_mtime, statbuf->st_mtime);
						http_preprocess_lua_page(path,ppath);
					}
				}
				else {
					http_preprocess_lua_page(path,ppath);
				}
			}
			else {
				// after modifying a .lua file, the developer
				// is responsible for deleting the preprocessed
				// file as there is no "date" in the file system

				// we preprocess in case the preprocessed file does not exist
				if (stat(ppath, statbuf) != 0) {
					http_preprocess_lua_page(path,ppath);
				}
				else {
					// but do our best to find out if the source file was modified
					char *dirsep = (char*)ppath + strlen(ppath) - 1;
					while (dirsep > ppath && *dirsep!=0 && *dirsep!='/') {
						dirsep--;
					}
					if (*dirsep=='/') {
						int filename_length = ppath + strlen(ppath) - (dirsep + 1);
						char* filename = dirsep + 1;

						*dirsep = 0;
						DIR *dir = opendir(ppath);
						if (dir) {
							struct dirent *ent;
							while ((ent = readdir(dir)) != NULL) {
								if (0==strcmp(filename, ent->d_name)) {
									//syslog(LOG_WARNING, "re-preprocessing %s due file system order, found %s first\n", path, filename);
									*dirsep = '/'; //fix ppath before using it
									http_preprocess_lua_page(path,ppath);
									break;
								}
								if (0==strncmp(filename, ent->d_name, filename_length-1)) {
									//syslog(LOG_WARNING, "not re-preprocessing %s due file system order\n", path);
									break;
								}
							}
						}
						closedir(dir);
						*dirsep = '/';
					}
				}
			}

			if (S_ISDIR(statbuf->st_mode)) {
				send_error(request, 500, "Internal Server Error", NULL, "Folder found where a precompiled file is expected.");
			}
			else {
				send_headers(request, 200, "OK", NULL, "text/html", -1);
				if (!request->config->secure) fflush(request->file);

				lua_pushstring(LL, (requestdata && *requestdata) ? requestdata:"");
				lua_setglobal(LL, "http_request");

				lua_pushlightuserdata(LL, (void*)request);
				lua_setglobal(LL, "http_stream_handle");

				lua_pushinteger(LL, request->config->port);
				lua_setglobal(LL, "http_port");

				lua_pushinteger(LL, request->config->secure);
				lua_setglobal(LL, "http_secure");

				int rc = luaL_dofile(LL, ppath);
				(void) rc;

				lua_pushnil(LL);
				lua_setglobal(LL, "http_request");

				lua_pushnil(LL);
				lua_setglobal(LL, "http_port");

				lua_pushnil(LL);
				lua_setglobal(LL, "http_secure");

				lua_pushnil(LL);
				lua_setglobal(LL, "http_stream_handle");

				if (!request->config->secure) fflush(request->file);
				do_printf(request, "0\r\n\r\n");
			}
		}
		else {
			send_error(request, 500, "Internal Server Error", NULL, "Path too long.");
		}

	} else {
		data = calloc(1, HTTP_BUFF_SIZE);
		if (data) {
			int length = S_ISREG(statbuf->st_mode) ? statbuf->st_size : -1;
			send_headers(request, 200, "OK", NULL, get_mime_type(path), length);
			while ((n = fread(data, 1, sizeof (data), file)) > 0) request_write(request, data, n);
			fclose(file);

			free(data);
		}
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

int process(http_request_handle *request) {
	char buf[HTTP_BUFF_SIZE];
	char *method;
	char *path;
	char *data = NULL;
	char *host = NULL;
	char *protocol;
	struct stat statbuf;
	char pathbuf[HTTP_BUFF_SIZE];
	int len;

	if (!do_gets(buf, sizeof (buf), request) || 0 == strlen(buf) ) {
		send_error(request, 400, "Bad Request", NULL, "Got empty request buffer.");
		return 0;
	}

	method = strtok(buf, " ");
	path = strtok(NULL, " ");
	protocol = strtok(NULL, "\r");

	if(!path) {
		len = strlen(method)-1;
		while(len>0 && (method[len]=='\r' || method[len]=='\n')) {
			method[len] = 0;
			len--;
		}
		path = "/";
	}

	//in case the protocol wasn't given we need to fix the path
	if(!protocol) {
		len = strlen(path)-1;
		while(len>=0 && (path[len]=='\r' || path[len]=='\n')) {
			path[len] = 0;
			len--;
		}
		if(!strlen(path)) path = "/";
	}

	if(path) {
		data = strchr(path, '?');
	  if (data) {
	  	*data = 0; //cut off the path
	  	data++; //point to start of params
	  }
	}

	//only in AP mode we redirect arbitrary host names to our own host name
	if (wifi_mode == WIFI_MODE_AP) {
		//find the Host: header and check if it matches our IP or captive server name
		while (do_gets(pathbuf, sizeof (pathbuf), request) && strlen(pathbuf)>0 ) {

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
						send_headers(request, 302, "Found", pathbuf, NULL, 0);
						return 0;
					}
				}
			}

		} // while
	} // AP mode

	if (!method || !path) return -1; //protocol may be omitted
	syslog(LOG_DEBUG, "http: %s %s %s\r", method, path, protocol ? protocol:"");

	if (!request->config->secure) fseek(request->file, 0, SEEK_CUR); // force change of stream direction

	if (strcasecmp(method, "GET") != 0) {
		syslog(LOG_DEBUG, "http: %s not supported\r", method);
		send_error(request, 501, "Not supported", NULL, "Method is not supported.");
	} else if (!filepath_merge(pathbuf, CONFIG_LUA_RTOS_HTTP_SERVER_DOCUMENT_ROOT, path, NULL)) {
		send_error(request, 404, "Not Found", NULL, "File not found.");
		syslog(LOG_DEBUG, "http: invalid path requested: %s\r", path);
	} else if (stat(pathbuf, &statbuf) < 0) {
		send_error(request, 404, "Not Found", NULL, "File not found.");
		syslog(LOG_DEBUG, "http: %s Not found\r", pathbuf);
	} else if (S_ISDIR(statbuf.st_mode)) {
		len = strlen(path);
		if (len == 0 || path[len - 1] != '/') {
			//send a redirect
			snprintf(pathbuf, sizeof (pathbuf), "Location: %s/", path);
			send_error(request, 302, "Found", pathbuf, "Directories must end with a slash.");
		} else {
			filepath_merge(pathbuf, CONFIG_LUA_RTOS_HTTP_SERVER_DOCUMENT_ROOT, path, "index.lua");
			if (stat(pathbuf, &statbuf) >= 0) {
				send_file(request, pathbuf, &statbuf, data);
			} else {
			      filepath_merge(pathbuf, CONFIG_LUA_RTOS_HTTP_SERVER_DOCUMENT_ROOT, path, "index.html");
				  if (stat(pathbuf, &statbuf) >= 0) {
					  send_file(request, pathbuf, &statbuf, data);
				  } else {
					  DIR *dir;
					  struct dirent *de;

					  send_headers(request, 200, "OK", NULL, "text/html", -1);
					  chunk(request, "<HTML><HEAD><TITLE>Index of %s</TITLE></HEAD><BODY>", path);
					  chunk(request, "<H4>Index of %s</H4>", path);

					  chunk(request, "<TABLE>");
					  chunk(request, "<TR>");
					  chunk(request, "<TH style=\"width: 250;text-align: left;\">Name</TH><TH style=\"width: 100px;text-align: right;\">Size</TH>");
					  chunk(request, "</TR>");

					  if (len > 1) {
						  chunk(request, "<TR>");
					  	chunk(request, "<TD><A HREF=\"..\">..</A></TD><TD></TD>");
						  chunk(request, "</TR>");
					  }

					  filepath_merge(pathbuf, CONFIG_LUA_RTOS_HTTP_SERVER_DOCUMENT_ROOT, path, NULL); //restore folder pathbuf
					  dir = opendir(pathbuf);
					  while ((de = readdir(dir)) != NULL) {
					  	filepath_merge(pathbuf, CONFIG_LUA_RTOS_HTTP_SERVER_DOCUMENT_ROOT, path, de->d_name);
						  stat(pathbuf, &statbuf);

						  chunk(request, "<TR>");
						  chunk(request, "<TD>");
						  chunk(request, "<A HREF=\"%s%s\">", de->d_name, S_ISDIR(statbuf.st_mode) ? "/" : "");
						  chunk(request, "%s%s", de->d_name, S_ISDIR(statbuf.st_mode) ? "/</A>" : "</A> ");
						  chunk(request, "</TD>");
						  chunk(request, "<TD style=\"text-align: right;\">");
						  if (!S_ISDIR(statbuf.st_mode)) {
								chunk(request, "%d", (int)statbuf.st_size);
						  }
						  chunk(request, "</TD>");
						  chunk(request, "</TR>");
					  }
					  closedir(dir);


					  chunk(request, "</TABLE>");

					  chunk(request, "</BODY></HTML>");

					  do_printf(request, "0\r\n\r\n");
				  }
			   }
		}
	} else {
		send_file(request, pathbuf, &statbuf, data);
	}

	return 0;
}

static void http_net_callback(system_event_t *event){
	//syslog(LOG_DEBUG, "event: %d\r\n", event->event_id);

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
			if (wifi_mode != WIFI_MODE_STA) {
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

static void mbedtls_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

static void *http_thread(void *arg) {
	http_server_config *config = (http_server_config*) arg;
	struct sockaddr_in6 sin;
  SSL_CTX *ctx = NULL;
  SSL *ssl = NULL;

  	net_init();
	if(0 == *config->server) {
		*config->server = socket(AF_INET6, SOCK_STREAM, 0);
		if(0 > *config->server) {
			syslog(LOG_ERR, "couldn't create server socket\n");
			return NULL;
		}

		u8_t one = 1;
		setsockopt(*config->server, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

		memset(&sin, 0, sizeof(sin));
		sin.sin6_family = AF_INET6;
		memcpy(&sin.sin6_addr.un.u32_addr, &in6addr_any, sizeof(in6addr_any));
		sin.sin6_port   = htons(config->port);
		int rc = bind(*config->server, (struct sockaddr *) &sin, sizeof (sin));
		if(0 != rc) {
			syslog(LOG_ERR, "couldn't bind to port %d\n", config->port);
			return NULL;
		}

		listen(*config->server, 5);
		LWIP_ASSERT("httpd_init: listen failed", *config->server >= 0);
		if(0 > *config->server) {
			syslog(LOG_ERR, "couldn't listen on port %d\n", config->port);
			return NULL;
		}

		// Set the timeout for accept
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		setsockopt(*config->server, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	}

	if (config->secure) {
		ctx = SSL_CTX_new(TLS_server_method());
		if (!ctx) {
			syslog(LOG_ERR, "couldn't create SSL context\n");
			return NULL;
		}

		//load cert
		size_t certificate_bytes;
		unsigned char *certificate_buf;
		if( mbedtls_pk_load_file( config->certificate, &certificate_buf, &certificate_bytes ) != 0 ) {
			syslog(LOG_ERR, "couldn't load SSL certificate\n");
		  SSL_CTX_free(ctx);
		  ctx = NULL;
			return NULL;
		}
		if (!SSL_CTX_use_certificate_ASN1(ctx, certificate_bytes, certificate_buf)) {
			syslog(LOG_ERR, "couldn't set SSL certificate\n");
			mbedtls_zeroize( certificate_buf, certificate_bytes );
			mbedtls_free( certificate_buf );
		  SSL_CTX_free(ctx);
		  ctx = NULL;
			return NULL;
		}
		mbedtls_zeroize( certificate_buf, certificate_bytes );
		mbedtls_free( certificate_buf );

		//load privkey
		size_t private_key_bytes;
		unsigned char *private_key_buf;
		if( mbedtls_pk_load_file( config->private_key, &private_key_buf, &private_key_bytes ) != 0 ) {
			syslog(LOG_ERR, "couldn't load SSL certificate\n");
		  SSL_CTX_free(ctx);
		  ctx = NULL;
			return NULL;
		}
		if (!SSL_CTX_use_PrivateKey_ASN1(0, ctx, private_key_buf, private_key_bytes)) {
			syslog(LOG_ERR, "couldn't load SSL private key\n");
			mbedtls_zeroize( private_key_buf, private_key_bytes );
			mbedtls_free( private_key_buf );
		  SSL_CTX_free(ctx);
		  ctx = NULL;
			return NULL;
		}
		mbedtls_zeroize( private_key_buf, private_key_bytes );
		mbedtls_free( private_key_buf );
	}

	syslog(LOG_INFO, "http: server listening on port %d\n", config->port);

	http_refcount++;
	while (!http_shutdown) {
		int client;

		if (config->secure) {
		  ssl = SSL_new(ctx);
		  if (!ssl) {
				syslog(LOG_ERR, "couldn't create SSL session\n");
				break; //exit the loop to shutdown the server
			}
		}

		// Wait for a request ...
		if ((client = accept(*config->server, NULL, NULL)) != -1) {

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

			if (config->secure) {
				SSL_set_fd(ssl, client);

				if (!SSL_accept(ssl)) {
					syslog(LOG_ERR, "couldn't accept SSL connection\n");
				} else {
					http_request_handle request = HTTP_Request_Secure_initializer;
					process(&request);
				}

				SSL_shutdown(ssl);
			}
			else
			{
				// Create the socket stream
				FILE *stream = fdopen(client, "a+");
				http_request_handle request = HTTP_Request_Normal_initializer;
				process(&request);
				fclose(stream);
			}

			close(client);
			client = -1;
		}

		if (config->secure) {
			SSL_free(ssl);
			ssl = NULL;
		}
	}

	if (config->secure) {
		SSL_CTX_free(ctx);
		ctx = NULL;

		free(config->certificate);
		config->certificate = NULL;

		free(config->private_key);
		config->private_key = NULL;
	}

	syslog(LOG_INFO, "http: server shutting down on port %d\n", config->port);

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

	if (0 == http_refcount) {
		//last one needs to unregister the callback
		driver_error_t *error;
		if ((error = net_event_unregister_callback(http_net_callback))) {
			syslog(LOG_WARNING, "couldn't unregister net callback\n");
		}
	}

	return NULL;
}

int http_start(lua_State* L) {
	//wait until an ongoing shutdown has been finished
	while(http_refcount && http_shutdown) delay(10);

	if(!http_refcount) {
		pthread_attr_t attr;
		struct sched_param sched;
		pthread_t thread_normal;
		pthread_t thread_secure;
		ifconfig_t info;
		int res;
		driver_error_t *error;

		LL=L;
		strcpy(ip4addr, "0.0.0.0");

		esp_log_level_set("wifi", ESP_LOG_NONE);

		if ((error = wifi_check_error(esp_wifi_get_mode(&wifi_mode)))) {
			esp_log_level_set("wifi", ESP_LOG_ERROR);
			free(error);
		} else {
			if ((error = wifi_stat(&info))) {
				esp_log_level_set("wifi", ESP_LOG_ERROR);
				free(error);
			}
			strcpy(ip4addr, inet_ntoa(info.ip));
		}

		esp_log_level_set("wifi", ESP_LOG_ERROR);

		if ((error = net_event_register_callback(http_net_callback))) {
			syslog(LOG_WARNING, "couldn't register net callback, please restart http service from lua after changing connectivity\n");
		}

		// Init thread attributes
		pthread_attr_init(&attr);

		// Set stack size
		pthread_attr_setstacksize(&attr, CONFIG_LUA_RTOS_HTTP_SERVER_STACK_SIZE);

		// Set priority
		sched.sched_priority = CONFIG_LUA_RTOS_HTTP_SERVER_TASK_PRIORITY;
		pthread_attr_setschedparam(&attr, &sched);

		// Set CPU
		cpu_set_t cpu_set = CPU_INITIALIZER;

		CPU_SET(CONFIG_LUA_RTOS_HTTP_SERVER_TASK_CPU, &cpu_set);

		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);

		// Create threads
		http_shutdown = 0;

		http_normal.port = luaL_optinteger( L, 1, CONFIG_LUA_RTOS_HTTP_SERVER_PORT );
		if (http_normal.port) {
			res = pthread_create(&thread_normal, &attr, http_thread, &http_normal);
			if (res) {
				return luaL_error(L, "couldn't start http_thread");
			}

			pthread_setname_np(thread_normal, "http");
		}

		http_secure.port = luaL_optinteger( L, 2, CONFIG_LUA_RTOS_HTTP_SERVER_PORT_SSL );
		const char *certificate = luaL_optstring( L, 3, NULL );
		const char *private_key = luaL_optstring( L, 4, NULL );
		http_secure.certificate = certificate ? strdup(certificate) : NULL;
		http_secure.private_key = private_key ? strdup(private_key) : NULL;

		if ( http_secure.port && http_secure.certificate && http_secure.private_key ) {
			res = pthread_create(&thread_secure, &attr, http_thread, &http_secure);
			if (res) {
				return luaL_error(L, "couldn't start secure http_thread");
			}

			pthread_setname_np(thread_normal, "ssl_http");
		}

		pthread_attr_destroy(&attr);
	}

	return 0;
}

void http_stop() {
	if(http_refcount) {
		http_shutdown++;
	}
}

#endif
