/*******************************************************************************
 * Copyright (c) 2009, 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial implementation and documentation
 *    Ian Craggs - async client updates
 *******************************************************************************/

#if !defined(SOCKET_H)
#define SOCKET_H

#if !__XTENSA__
#include <sys/types.h>
#endif

#if defined(WIN32) || defined(WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#define MAXHOSTNAMELEN 256
#if !defined(SSLSOCKET_H)
#undef EAGAIN
#define EAGAIN WSAEWOULDBLOCK
#undef EINTR
#define EINTR WSAEINTR
#undef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#undef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef ENOTCONN
#define ENOTCONN WSAENOTCONN
#undef ECONNRESET
#define ECONNRESET WSAECONNRESET
#undef ETIMEDOUT
#define ETIMEDOUT WAIT_TIMEOUT
#endif
#define ioctl ioctlsocket
#define socklen_t int
#else
#define INVALID_SOCKET SOCKET_ERROR
#include <sys/socket.h>
#if !defined(_WRS_KERNEL)
#include <sys/param.h>
#include <sys/time.h>
#if !__XTENSA__
#include <sys/select.h>
#include <sys/uio.h>
#endif
#else
#if !__XTENSA__
#include <selectLib.h>
#endif
#endif
#if !__XTENSA__
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <limits.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#define ULONG size_t
#endif

#include "mutex_type.h" /* Needed for mutex_type */

/** socket operation completed successfully */
#define TCPSOCKET_COMPLETE 0
#if !defined(SOCKET_ERROR)
	/** error in socket operation */
	#define SOCKET_ERROR -1
#endif
/** must be the same as SOCKETBUFFER_INTERRUPTED */
#define TCPSOCKET_INTERRUPTED -22
#define SSL_FATAL -3

#if !defined(INET6_ADDRSTRLEN)
#define INET6_ADDRSTRLEN 46 /** only needed for gcc/cygwin on windows */
#endif


#if !defined(max)
#define max(A,B) ( (A) > (B) ? (A):(B))
#endif

#include "LinkedList.h"

/*BE
def FD_SET
{
   128 n8 "data"
}

def SOCKETS
{
	FD_SET "rset"
	FD_SET "rset_saved"
	n32 dec "maxfdp1"
	n32 ptr INTList "clientsds"
	n32 ptr INTItem "cur_clientsds"
	n32 ptr INTList "connect_pending"
	n32 ptr INTList "write_pending"
	FD_SET "pending_wset"
}
BE*/


/**
 * Structure to hold all socket data for the module
 */
typedef struct
{
	fd_set rset, /**< socket read set (see select doc) */
		rset_saved; /**< saved socket read set */
	int maxfdp1; /**< max descriptor used +1 (again see select doc) */
	List* clientsds; /**< list of client socket descriptors */
	ListElement* cur_clientsds; /**< current client socket descriptor (iterator) */
	List* connect_pending; /**< list of sockets for which a connect is pending */
	List* write_pending; /**< list of sockets for which a write is pending */
	fd_set pending_wset; /**< socket pending write set for select */
} Sockets;


void Socket_outInitialize(void);
void Socket_outTerminate(void);
int Socket_getReadySocket(int more_work, struct timeval *tp, mutex_type mutex);
int Socket_getch(int socket, char* c);
char *Socket_getdata(int socket, size_t bytes, size_t* actual_len);
int Socket_putdatas(int socket, char* buf0, size_t buf0len, int count, char** buffers, size_t* buflens, int* frees);
void Socket_close(int socket);
int Socket_new(char* addr, int port, int* socket);

int Socket_noPendingWrites(int socket);
#if !__XTENSA__
char* Socket_getpeer(int sock);
#endif

void Socket_addPendingWrite(int socket);
void Socket_clearPendingWrite(int socket);

typedef void Socket_writeComplete(int socket, int rc);
void Socket_setWriteCompleteCallback(Socket_writeComplete*);

#endif /* SOCKET_H */
