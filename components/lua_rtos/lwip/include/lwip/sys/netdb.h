/*
 * netdb.h
 *
 *  Created on: Jan 12, 2018
 *      Author: jolive
 */

#ifndef COMPONENTS_LUA_RTOS_LWIP_INCLUDE_SYS_NETDB_H_
#define COMPONENTS_LUA_RTOS_LWIP_INCLUDE_SYS_NETDB_H_

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define EAI_AGAIN 206
#define EAI_BADFLAGS 207
#define EAI_OVERFLOW 208

 struct servent {
	   char  *s_name;       /* official service name */
	   char **s_aliases;    /* alias list */
	   int    s_port;       /* port number */
	   char  *s_proto;      /* protocol to use */
   };

/*
 * Constants for getnameinfo()
 */
#define	NI_MAXHOST	1025
#define	NI_MAXSERV	32

/*
 * Flag values for getnameinfo()
 */
#define	NI_NOFQDN	0x00000001
#define	NI_NUMERICHOST	0x00000002
#define	NI_NAMEREQD	0x00000004
#define	NI_NUMERICSERV	0x00000008
#define	NI_DGRAM	0x00000010
#define NI_WITHSCOPEID	0x00000020

int
 getnameinfo(const struct sockaddr *sa, socklen_t salen,
 		    char *host, size_t hostlen,
 			char *serv, size_t servlen, int flags);

const char *gai_strerror(int ecode);

struct servent *getservbyname(const char *name, const char *proto);

#endif /* COMPONENTS_LUA_RTOS_LWIP_INCLUDE_SYS_NETDB_H_ */
