/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2018 - 2019, Thomas E. Horner (whitecatboard.org@horner.it)
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
 * Lua RTOS, Lua CPU module
 *
 */

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_USE_TELNET_SERVER

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
#include <sys/path.h>
#include <sys/socket.h>
#include <netdb.h>
#include <linux/in6.h>

#define TELNET_BUFF_SIZE 256

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <drivers/net.h>
#include <drivers/spi_eth.h>
#include <time.h>

#include "sys.h"

static u8_t volatile telnet_shutdown = 0;
static int socket_server_telnet = 0;
static lua_callback_t *telnet_callback = NULL;

typedef struct {
  int port;
  int *server; //socket
  int autoclose; //close after X seconds, 0 = never
} telnet_server_config;

#define TELNET_initializer { CONFIG_LUA_RTOS_TELNET_SERVER_PORT, &socket_server_telnet, 60 }

typedef struct {
  telnet_server_config *config;
  int socket;
  struct sockaddr_storage client;
  socklen_t client_len;
  char *outbuf;
} telnet_request_handle;

#define TELNET_Request_initializer { config, client, client_addr, client_addr_len, NULL };

static telnet_server_config telnetsrv = TELNET_initializer;

static char *do_gets(char *s, int size, telnet_request_handle *request) {
  int socket = request->socket;

  int rc;
  char *c = s;
  while (c < (s + size - 1)) {
    rc = recv(socket, c, 1, MSG_DONTWAIT);

    if (rc>0) {
      if (*c == '\n') {
        c++;
        break;
      }
      c++;
    }
    else if (rc==0) {
      //no data received or connection is closed
      syslog(LOG_DEBUG, "telnet: no data received or connection is closed\r");
      break;
    }
    else {
      return NULL; //discard half-received data
    }
  }
  *c = 0;
  return (c == s ? 0 : s);
}

static int process(telnet_request_handle *request) {
  char *reqbuf;
  char *outbuf;
  uint16_t len;

	if (request->client.ss_family==AF_INET6) {
			struct sockaddr_in6* sa6=(struct sockaddr_in6*)&request->client;
			if (IN6_IS_ADDR_V4MAPPED(&sa6->sin6_addr)) {
					struct sockaddr_in sa4;
					memset(&sa4,0,sizeof(sa4));
					sa4.sin_family=AF_INET;
					sa4.sin_port=sa6->sin6_port;
					memcpy(&sa4.sin_addr.s_addr,sa6->sin6_addr.s6_addr+12,4);
					memcpy(&request->client,&sa4,sizeof(sa4));
					request->client_len=sizeof(sa4);
			}
	}
	// -> now convert the address to human-readable form
	char *buffer = (char *)malloc(INET6_ADDRSTRLEN);
  if (!buffer) {
    syslog(LOG_ERR, "error allocating memory\n");
    return 0;
  }
	int err=getnameinfo((struct sockaddr*)&request->client,request->client_len,buffer,INET6_ADDRSTRLEN,0,0,NI_NUMERICHOST);
	if (err!=0) {
			snprintf(buffer,INET6_ADDRSTRLEN,"invalid address");
	}

  syslog(LOG_DEBUG, "telnet: new connection from %s\n", buffer);

  // Allocate space for buffers
  reqbuf = calloc(1, TELNET_BUFF_SIZE);
  if (!reqbuf) {
    free(buffer);
    syslog(LOG_ERR, "error allocating memory\n");
    return 0;
  }
  outbuf = calloc(1, TELNET_BUFF_SIZE);
  if (!outbuf) {
    free(buffer);
    free(reqbuf);
    syslog(LOG_ERR, "error allocating memory\n");
    return 0;
  }

  request->outbuf = outbuf;
  clock_t last = clock()/CLOCKS_PER_SEC;

  while(true) {

    if (do_gets(reqbuf, TELNET_BUFF_SIZE, request) && 0 != (len = strnlen(reqbuf, TELNET_BUFF_SIZE)) ) {

      while(len>0 && (reqbuf[len-1]=='\r' || reqbuf[len-1]=='\n')) len--;
      reqbuf[len] = '\0';

      if (strcasecmp(reqbuf,"QUIT")==0) {
        syslog(LOG_DEBUG, "telnet: closing connection with %s\n", buffer);
        break; //return
      }
      else {
        last = clock()/CLOCKS_PER_SEC;

        if (telnet_callback != NULL) {
          lua_State *state = luaS_callback_state(telnet_callback);

          // set the globals
          lua_pushlightuserdata(state, (void*)request);
          lua_setglobal(state, "telnet_internal_handle");

          // call the callback callback
          lua_pushstring(state, buffer); //client address
          lua_pushstring(state, reqbuf); //client string
          luaS_callback_call(telnet_callback, 2);

          // clear the globals
          lua_pushnil(state);
          lua_setglobal(state, "telnet_internal_handle");
        }

      }
    }
    else if (request->config->autoclose) {
      clock_t now = clock()/CLOCKS_PER_SEC;
      if (request->config->autoclose < (now - last)) {
        // not received any input, exiting
        syslog(LOG_DEBUG, "telnet: auto-closing connection with %s\n", buffer);
        break;
      }
      else {
        //syslog(LOG_DEBUG, "telnet: not yet auto-closing connection with %s\n", buffer);
        usleep(200*1000); //check again in 200ms
      }
    }
  }

  free(buffer);
  free(reqbuf);
  request->outbuf = NULL;
  free(outbuf);

  return 0;
}

static void *telnet_request_thread(void *arg) {
  telnet_request_handle* request = (telnet_request_handle*) arg;
  process(request);
  shutdown(request->socket, SHUT_RDWR);

  close(request->socket);
  request->socket = -1;

  free(request);
  request = NULL;

  //pthread_exit(NULL);
  return NULL;
}

static void *telnet_thread(void *arg) {
  telnet_server_config *config = (telnet_server_config*) arg;
  struct sockaddr_in6 sin;
  int rc = 0;

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
    rc = bind(*config->server, (struct sockaddr *) &sin, sizeof (sin));
    if(0 != rc) {
      syslog(LOG_ERR, "couldn't bind to port %d\n", config->port);
      return NULL;
    }

    listen(*config->server, 5);
    LWIP_ASSERT("telnetd_init: listen failed", *config->server >= 0);
    if(0 > *config->server) {
      syslog(LOG_ERR, "couldn't listen on port %d\n", config->port);
      return NULL;
    }

    // Set the timeout for accept
    struct timeval timeout = {0L, 1000L}; /* check for shutdown every 1ms */
    setsockopt(*config->server, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  }

  syslog(LOG_INFO, "telnet: server listening on port %d\n", config->port);


  pthread_attr_t attr;
  struct sched_param sched;

  // Init thread attributes
  pthread_attr_init(&attr);

  // Set stack size
  pthread_attr_setstacksize(&attr, CONFIG_LUA_RTOS_TELNET_SERVER_CHILD_STACK_SIZE);

  // Set priority
  sched.sched_priority = CONFIG_LUA_RTOS_TELNET_SERVER_TASK_PRIORITY;
  pthread_attr_setschedparam(&attr, &sched);

  // Set CPU
  cpu_set_t cpu_set = CPU_INITIALIZER;
  CPU_SET(CONFIG_LUA_RTOS_TELNET_SERVER_TASK_CPU, &cpu_set);

  pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  while (!telnet_shutdown) {
    int client;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Wait for a request ...
    if ((client = accept(*config->server, (struct sockaddr *)&client_addr, &client_addr_len)) != -1) {

      // We wait for send all data before close socket's stream
      struct linger so_linger;
      so_linger.l_onoff  = 1;
      so_linger.l_linger = 2;
      setsockopt(client, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));

      // Set a timeout for send / receive
      struct timeval timeout = {10L, 0L}; /* 10 secs to send all data */
      setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
      setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

      telnet_request_handle request = TELNET_Request_initializer;
      telnet_request_handle* thread_request = (telnet_request_handle*)malloc(sizeof(telnet_request_handle));
      if (thread_request) {
        memcpy(thread_request, &request, sizeof(telnet_request_handle));

        pthread_t thread_telnet_request;
        int res = pthread_create(&thread_telnet_request, &attr, telnet_request_thread, thread_request);
        if (res) {
          free(thread_request);
          close(client);
          syslog(LOG_ERR, "couldn't start telnet_request_thread");
          return NULL;
        }

        char thread_name[configMAX_TASK_NAME_LEN];
        snprintf(thread_name, configMAX_TASK_NAME_LEN, "telnet-%i", client);
        pthread_setname_np(thread_telnet_request, thread_name);
      }
      else {
        close(client);
        syslog(LOG_ERR, "couldn't allocate thread_request");
      }

    }
  }

  pthread_attr_destroy(&attr);

  syslog(LOG_INFO, "telnet: server shutting down on port %d\n", config->port);

  /* it's not ideal to keep the server_socket open as it is blocked
     but now at least the telnetsrv can be restarted from lua scripts.
     if we'd properly close the socket we would need to bind() again
     when restarting the telnetsrv but bind() will succeed only after
     several (~4) minutes: http://lwip.wikia.com/wiki/Netconn_bind
     "Note that if you try to bind the same address and/or port you
      might get an error (ERR_USE, address in use), even if you
      delete the netconn. Only after some time (minutes) the
      resources are completely cleared in the underlying stack due
      to the need to follow the TCP specification and go through
      the TCP timewait state."
  */

  return NULL;
}

int telnet_start(lua_State* L) {

  pthread_attr_t attr;
  struct sched_param sched;
  pthread_t thread_telnet;
  int res;

  // Init thread attributes
  pthread_attr_init(&attr);

  // Set stack size
  pthread_attr_setstacksize(&attr, CONFIG_LUA_RTOS_TELNET_SERVER_STACK_SIZE);

  // Set priority
  sched.sched_priority = CONFIG_LUA_RTOS_TELNET_SERVER_TASK_PRIORITY;
  pthread_attr_setschedparam(&attr, &sched);

  // Set CPU
  cpu_set_t cpu_set = CPU_INITIALIZER;
  CPU_SET(CONFIG_LUA_RTOS_TELNET_SERVER_TASK_CPU, &cpu_set);

  pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  // Create threads
  telnet_shutdown = 0;

  telnetsrv.port = luaL_optinteger( L, 1, CONFIG_LUA_RTOS_TELNET_SERVER_PORT );
  if (telnetsrv.port) {

    if (telnet_callback != NULL) {
        luaS_callback_destroy(telnet_callback);
        telnet_callback = NULL;
    }

    luaL_checktype(L, 2, LUA_TFUNCTION);
    telnet_callback = luaS_callback_create(L, 2);

    telnetsrv.autoclose = luaL_optinteger( L, 3, 60 ); //auto-close connection after 60 seconds of inactivity

    res = pthread_create(&thread_telnet, &attr, telnet_thread, &telnetsrv);
    if (res) {
      return luaL_error(L, "couldn't start telnet_thread");
    }

    pthread_setname_np(thread_telnet, "telnet");
  }

  pthread_attr_destroy(&attr);

  return 0;
}

void telnet_stop() {
  telnet_shutdown++;
}

int telnet_print(lua_State* L) {

	lua_getglobal(L, "telnet_internal_handle");
	if (!lua_islightuserdata(L, -1)) {
		return luaL_error(L, "this function may only be called inside a lua callback served by telnetsrv");
	}
	telnet_request_handle *request = (telnet_request_handle*)lua_touserdata(L, -1);

	if (!request) {
		return luaL_error(L, "this function may only be called inside a lua callback served by telnetsrv");
	}

	int nargs = lua_gettop(L);
	for (int i=1; i <= nargs; i++) {
		if (lua_isstring(L, i)) {
			snprintf(request->outbuf, TELNET_BUFF_SIZE, "%s", lua_tostring(L, i));
		  send(request->socket, request->outbuf, strnlen(request->outbuf, TELNET_BUFF_SIZE), MSG_DONTWAIT);
		}
		else {
			/* non-strings handling not reqired */
		}
	}

	lua_pop(L, 1);
	return 0;
}

#endif
