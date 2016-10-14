/*
 * Whitecat, Lua net module
 * 
 * This module contains elua parts
 *
 * Copyright (C) 2015 - 2016
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
 * whatsoever resulting from loss of use, ƒintedata or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#include "whitecat.h"

#if LUA_USE_NET

#include "lualib.h"
#include "lauxlib.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/opt.h"

int platform_net_exists(const char *interface);
void platform_net_stat_iface(const char *interface);
const char *platform_net_error(int code);
int platform_net_start(const char *interface);
int platform_net_stop(const char *interface);
int platform_net_sntp_start();

typedef s16 net_size;

enum {
  NET_ERR_OK = 0,
  NET_ERR_TIMEDOUT,
  NET_ERR_CLOSED,
  NET_ERR_ABORTED,
  NET_ERR_OVERFLOW,
  NET_ERR_OTHER,
};

typedef union {
  u32     ipaddr;
  u8      ipbytes[ 4 ];
  u16     ipwords[ 2 ];
} net_ip;

#define NET_SOCK_STREAM          0
#define NET_SOCK_DGRAM           1
#define NET_DHCP                 2
#define NET_STATIC               3

static int errno_to_err() {
    switch (errno) {
        case 0: return NET_ERR_OK;
        case EWOULDBLOCK: return NET_ERR_TIMEDOUT;
        case ECONNABORTED: return NET_ERR_ABORTED;
        case ENOTCONN: return NET_ERR_CLOSED;
        default:
            return NET_ERR_OTHER;
    }
}

static int net_accept( lua_State *L ) {
    int newsockfd;
    int ret = NET_ERR_OK;
    int clilen;
    
    struct sockaddr_in serv_addr, cli_addr;

    u16 sock = ( u16 )luaL_checkinteger( L, 1 );
    u16 port = ( u16 )luaL_checkinteger( L, 2 );
    u16 timeout = ( u16 )luaL_checkinteger( L, 3 );
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
   
     // Get socket type
    int type;
    int length = sizeof( int );
    
    if ((ret = getsockopt(sock, SOL_SOCKET, SO_TYPE, &type, &length)) < 0) {
        ret = errno_to_err();
        goto exit;
    }
    
    if (bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
         ret = errno_to_err();
         goto exit;
    }
      
    if (type == SOCK_STREAM) {
        listen(sock,5);
        clilen = sizeof(cli_addr);
    }
    
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    if (type == SOCK_STREAM) {
        newsockfd = accept(sock, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
             ret = errno_to_err();
             goto exit;
        }
    }
exit:
    lua_pushinteger( L, newsockfd );
    lua_pushinteger( L, cli_addr.sin_addr.s_addr );
    lua_pushinteger( L, ret );

    return 3;
}

// Lua: sock = socket( type )
static int net_socket( lua_State *L ) {
    int type = ( int )luaL_checkinteger( L, 1 );

    int socket_fd = 0;

    switch (type) {
        case NET_SOCK_DGRAM:
            socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
            break;

        case NET_SOCK_STREAM:
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            break;
    }

    lua_pushinteger( L, socket_fd );
    return 1;
}

// Lua: res = close( socket )
static int net_close(lua_State* L) {
  int sock = ( int )luaL_checkinteger( L, 1 );
  int ret = NET_ERR_OK;
  
  if (closesocket(sock) < 0) {
      ret = errno_to_err();
  }
  
  lua_pushinteger( L, ret );
  return 1;
}

// Lua: res, err = send( sock, str )
static int net_send(lua_State* L) {
    int sock = ( int )luaL_checkinteger( L, 1 );
    const char *buf;
    size_t siz,sended = 0;
    int ret = NET_ERR_OK;

    if (lua_type( L, 2) == LUA_TSTRING) {
        buf = luaL_checklstring( L, 2, &siz );
    } else {
        return luaL_error( L, "Unsupported data type for send" );    
    }
    
    // Get socket type
    int type;
    int length = sizeof( int );
    
    if ((ret = getsockopt(sock, SOL_SOCKET, SO_TYPE, &type, &length)) < 0) {
        ret = errno_to_err();
    } else {
        if (type == SOCK_STREAM) {
            // TPC
            if ((sended = send(sock, buf, siz, 0)) < 0) {
                ret = errno_to_err();
                sended = 0;
            }       
        } else {
            // UDP
            struct sockaddr addr;
            size_t len;
            
            // GET ip of client
            len = sizeof(struct sockaddr);
            getpeername(sock, &addr, &len);

            // Send
            if ((sended = sendto(sock, buf, siz, 0, &addr, len)) < 0) {
                ret = errno_to_err();
                sended = 0;
            }       
        }
    }
    
    lua_pushinteger( L, sended );
    lua_pushinteger( L, ret );
    
    return 2;  
}

// Lua: err = connect( sock, iptype, port )
// "iptype" is actually an int returned by "net.packip"
static int net_connect( lua_State *L ) { 
    unsigned int ip;
    int rc = 0;
    struct sockaddr_in address;
    int ret = NET_ERR_OK;
  
    int sock = ( int )luaL_checkinteger( L, 1 );
    u16 port = ( int )luaL_checkinteger( L, 3 );
    ip = ( u32 )luaL_checkinteger( L, 2 );
 
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = ip;
        
    // Get socket type
    int type;
    int length = sizeof( int );
    
    if ((ret = getsockopt(sock, SOL_SOCKET, SO_TYPE, &type, &length)) < 0) {
        ret = errno_to_err();
    } else {
        if (type == SOCK_STREAM) {
            rc = connect(sock, (struct sockaddr*)&address, sizeof(address));
            if (rc < 0) {
                ret = errno_to_err();
            }            
        } else {
            ret = NET_ERR_OK;
        }
    }   
 
    lua_pushinteger( L, ret );
    
    return 1;  
}

// Lua: data = packip( ip0, ip1, ip2, ip3 ), or
// Lua: data = packip( "ip" )
// Returns an internal representation for the given IP address
static int net_packip( lua_State *L )
{
  net_ip ip;
  unsigned i, temp;
  
  if( lua_isnumber( L, 1 ) )
    for( i = 0; i < 4; i ++ )
    {
      temp = luaL_checkinteger( L, i + 1 );
      if( temp < 0 || temp > 255 )
        return luaL_error( L, "invalid IP adddress" );
      ip.ipbytes[ i ] = temp;
    }
  else
  {
    const char* pip = luaL_checkstring( L, 1 );
    unsigned len, temp[ 4 ];
    
    if( sscanf( pip, "%u.%u.%u.%u%n", temp, temp + 1, temp + 2, temp + 3, &len ) != 4 || len != strlen( pip ) )
      return luaL_error( L, "invalid IP adddress" );    
    for( i = 0; i < 4; i ++ )
    {
      if( temp[ i ] < 0 || temp[ i ] > 255 )
        return luaL_error( L, "invalid IP address" );
      ip.ipbytes[ i ] = ( u8 )temp[ i ];
    }
  }
  lua_pushinteger( L, ip.ipaddr );
  return 1;
}

// Lua: ip0, ip1, ip2, ip3 = unpackip( iptype, "*n" ), or
//               string_ip = unpackip( iptype, "*s" )
static int net_unpackip( lua_State *L )
{
  net_ip ip;
  unsigned i;  
  const char* fmt;
  
  ip.ipaddr = ( u32 )luaL_checkinteger( L, 1 );
  fmt = luaL_checkstring( L, 2 );
  if( !strcmp( fmt, "*n" ) )
  {
    for( i = 0; i < 4; i ++ ) 
      lua_pushinteger( L, ip.ipbytes[ i ] );
    return 4;
  }
  else if( !strcmp( fmt, "*s" ) )
  {
    lua_pushfstring( L, "%d.%d.%d.%d", ( int )ip.ipbytes[ 0 ], ( int )ip.ipbytes[ 1 ], 
                     ( int )ip.ipbytes[ 2 ], ( int )ip.ipbytes[ 3 ] );
    return 1;
  }
  else
    return luaL_error( L, "invalid format" );                                      
}

// Lua: res, err = recv( sock, maxsize, [timer_id, timeout] ) or
//      res, err = recv( sock, "*l", [timer_id, timeout] )
static int net_recv( lua_State *L ) {
    int sock = ( int )luaL_checkinteger( L, 1 );
    int maxsize;
    int ret = NET_ERR_OK;
    luaL_Buffer net_recv_buff;
    int received = 0;
    int string = 0;
    int n;
    char c;
    
    if( lua_isnumber( L, 2 ) ) {
        maxsize = ( net_size )luaL_checkinteger( L, 2 );
    } else {
        if( strcmp( luaL_checkstring( L, 2 ), "*l" ) )
            return luaL_error( L, "invalid second argument to recv" );
      
        string = 1;
        maxsize = BUFSIZ;
    }
    
    luaL_buffinit( L, &net_recv_buff );

    // Get socket type
    int type;
    int length = sizeof( int );
    
    if ((ret = getsockopt(sock, SOL_SOCKET, SO_TYPE, &type, &length)) < 0) {
        ret = errno_to_err();
    } else {
        if (type == SOCK_STREAM) {
            // TPC
            if (string){
                received = 0;
                while (((n = recv(sock, &c, 1, 0)) > 0) && (received < maxsize)) {
                    *((char *)net_recv_buff.b) = c;
                    net_recv_buff.b++;
                    net_recv_buff.n++;
                    received++;
                    
                    if (c == '\n') break;
                }
                
                net_recv_buff.b = &net_recv_buff.initb[0];
            } else {
                if ((received = recv(sock, net_recv_buff.b, maxsize, 0)) < 0) {
                    ret = errno_to_err();
                    received = 0;
                }                       
            }
        } else {
            // UDP
        }
    }
    
    luaL_pushresult( &net_recv_buff );
    lua_pushinteger( L, ret );

    return 2;
}

// Lua: iptype = lookup( "name" )
static int net_lookup(lua_State* L) {
  const char* name = luaL_checkstring( L, 1 );
    int port = 0;
 	struct sockaddr_in address;
	int rc = 0;
	sa_family_t family = AF_INET;
	struct addrinfo *result = NULL;
	struct addrinfo hints = {0, AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};
          
	if ((rc = getaddrinfo(name, NULL, &hints, &result)) == 0) {
		struct addrinfo *res = result;
		while (res) {
			if (res->ai_family == AF_INET) {
				result = res;
				break;
			}
			res = res->ai_next;
		}

		if (result->ai_family == AF_INET) {
			address.sin_port = htons(port);
			address.sin_family = family = AF_INET;
            address.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
		} else {
            rc = -1;
        }
        
        freeaddrinfo(result);
	}
    
  if (rc == 0) {
      lua_pushinteger( L, address.sin_addr.s_addr );
      return 1;
  } else {
      return 0;
  }
}

static int net_setup(lua_State* L) {
    const char *interface = luaL_checkstring( L, 1 );

    if (!platform_net_exists(interface)) {
        return luaL_error(L, "unknown interface");    
    }

    if (strcmp(interface, "en") == 0) {
        
    }
    
    if (strcmp(interface, "gprs") == 0) {
        const char *apn = luaL_checkstring( L, 2 );
        const char *pin = luaL_checkstring( L, 3 );
        
        platform_net_setup_gprs(apn, pin);
    }
    
    return 0;
}

static int net_start(lua_State* L) {
    const char *interface = luaL_checkstring( L, 1 );

    if (!platform_net_exists(interface)) {
        return luaL_error(L, "unknown interface");    
    }
    
    int res = platform_net_start(interface);
    if (res) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, platform_net_error(res));   
        
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;      
}

static int net_stop(lua_State* L) {
    const char *interface = luaL_checkstring( L, 1 );
    
    if (!platform_net_exists(interface)) {
        return luaL_error(L, "interface not supported in this release");    
    }

    int res = platform_net_stop(interface);
    if (res) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, platform_net_error(res));   
        
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;      
}

static int net_stat(lua_State* L) {
    platform_net_stat_iface("en");printf("\n");
    platform_net_stat_iface("gprs");printf("\n");
    platform_net_stat_iface("wf");printf("\n");
    
    return 0;    
}   

static int net_sntp(lua_State* L) {
    int res;
    
    res = platform_net_sntp_start();
    if (res) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, platform_net_error(res));   
        return 2;
    }
    
    time_t start;
    time_t now;

    time(&start);

    if (start < 1420070400) {
        // Clock not set
        // Wait for set, or things may be wrong
        time(&now);
        while ((now < 1420070400) && (now - start < 30)) {
            time(&now);                    
        }

        if (now < 1420070400) {
            lua_pushboolean(L, 0);
            lua_pushstring(L, "Network timeout");   
            
            return 2;
        }
    }

    lua_pushboolean(L, 1);
    return 1;
}

const luaL_Reg net_map[] = {
    {"setup", net_setup},
    {"sntp", net_sntp},
    {"start", net_start},
    {"stop", net_stop},
    {"stat", net_stat},
    {"accept", net_accept},
    {"packip", net_packip},
    {"unpackip", net_unpackip},
    {"connect", net_connect},
    {"socket", net_socket},
    {"close", net_close},
    {"send", net_send},
    {"recv", net_recv},
    {"lookup", net_lookup},
    {NULL, NULL}
};

int luaopen_net( lua_State *L ) {
    luaL_newlib(L, net_map);

    // Module constants
    lua_pushinteger(L, NET_SOCK_STREAM);
    lua_setfield(L, -2, "SOCK_STREAM");

    lua_pushinteger(L, NET_SOCK_DGRAM);
    lua_setfield(L, -2, "SOCK_DGRAM");

    lua_pushinteger(L, NET_DHCP);
    lua_setfield(L, -2, "DHCP");

    lua_pushinteger(L, NET_STATIC);
    lua_setfield(L, -2, "STATIC");

    lua_pushinteger(L, NET_ERR_OK);
    lua_setfield(L, -2, "ERR_OK");

    lua_pushinteger(L, NET_ERR_TIMEDOUT);
    lua_setfield(L, -2, "ERR_TIMEDOUT");

    lua_pushinteger(L, NET_ERR_CLOSED);
    lua_setfield(L, -2, "ERR_CLOSED");

    lua_pushinteger(L, NET_ERR_ABORTED);
    lua_setfield(L, -2, "ERR_ABORTED");

    lua_pushinteger(L, NET_ERR_OVERFLOW);
    lua_setfield(L, -2, "ERR_OVERFLOW");

    lua_pushinteger(L, NET_ERR_OTHER);
    lua_setfield(L, -2, "ERR_OTHER");

    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "NO_TIMEOUT");

    return 1;
}

#endif