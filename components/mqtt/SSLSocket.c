/*******************************************************************************
 * Copyright (c) 2009, 2017 IBM Corp.
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
 *    Ian Craggs, Allan Stockdill-Mander - initial implementation
 *    Ian Craggs - fix for bug #409702
 *    Ian Craggs - allow compilation for OpenSSL < 1.0
 *    Ian Craggs - fix for bug #453883
 *    Ian Craggs - fix for bug #480363, issue 13
 *    Ian Craggs - SNI support
 *    Ian Craggs - fix for issues #155, #160
 *******************************************************************************/

/**
 * @file
 * \brief SSL  related functions
 *
 */

#if defined(OPENSSL)

#include "SocketBuffer.h"
#include "MQTTClient.h"
#include "SSLSocket.h"
#include "Log.h"
#include "StackTrace.h"
#include "Socket.h"

//#include "Heap.h"

#include <openssl/ssl.h>
#include <string.h>
#include <sys/status.h>

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"


extern Sockets s;

int SSLSocket_error(char* aString, SSL* ssl, int sock, int rc);
char* SSL_get_verify_result_string(int rc);
void SSL_CTX_info_callback(const SSL* ssl, int where, int ret);
char* SSLSocket_get_version_string(int version);
void SSL_CTX_msg_callback(
		int write_p,
		int version,
		int content_type,
		const void* buf, size_t len,
		SSL* ssl, void* arg);
int pem_passwd_cb(char* buf, int size, int rwflag, void* userdata);
int SSL_create_mutex(ssl_mutex_type* mutex);
int SSL_lock_mutex(ssl_mutex_type* mutex);
int SSL_unlock_mutex(ssl_mutex_type* mutex);
void SSL_destroy_mutex(ssl_mutex_type* mutex);
#if (OPENSSL_VERSION_NUMBER >= 0x010000000)
extern void SSLThread_id(CRYPTO_THREADID *id);
#else
extern unsigned long SSLThread_id(void);
#endif
extern void SSLLocks_callback(int mode, int n, const char *file, int line);
int SSLSocket_createContext(networkHandles* net, MQTTClient_SSLOptions* opts);
void SSLSocket_destroyContext(networkHandles* net);
void SSLSocket_addPendingRead(int sock);
void ssl_pm_set_hostname(SSL *ssl, const char *hostname);

//static ssl_mutex_type* sslLocks = NULL;
static ssl_mutex_type sslCoreMutex = PTHREAD_MUTEX_INITIALIZER;

#if defined(WIN32) || defined(WIN64)
#define iov_len len
#define iov_base buf
#endif

/**
 * Gets the specific error corresponding to SOCKET_ERROR
 * @param aString the function that was being used when the error occurred
 * @param sock the socket on which the error occurred
 * @return the specific TCP error code
 */
int SSLSocket_error(char* aString, SSL* ssl, int sock, int rc)
{
    int error = -1;

    FUNC_ENTRY;
    if (ssl)
        error = SSL_get_error(ssl, rc);
	//else
    //    error = ERR_get_error();

    if (error == SSL_ERROR_WANT_READ)
			Log(TRACE_MIN, -1, "SSLSocket WANT_READ in %s for socket %d", aString, sock);
    else if (error == SSL_ERROR_WANT_WRITE)
			Log(TRACE_MIN, -1, "SSLSocket WANT_WRITE in %s for socket %d", aString, sock);
    else
    {
        if (strcmp(aString, "shutdown") != 0)
        	Log(TRACE_MIN, -1, "SSLSocket error %d in %s for socket %d rc %d errno %d %s\n", error, aString, sock, rc, errno, strerror(errno));
		if (error == SSL_ERROR_SSL || error == SSL_ERROR_SYSCALL)
			error = SSL_FATAL;
    }
    FUNC_EXIT_RC(error);
    return error;
}

int SSL_create_mutex(ssl_mutex_type* mutex)
{
	int rc = 0;

	FUNC_ENTRY;
#if defined(WIN32) || defined(WIN64)
	*mutex = CreateMutex(NULL, 0, NULL);
#else
	rc = pthread_mutex_init(mutex, NULL);
#endif
	FUNC_EXIT_RC(rc);
	return rc;
}

int SSL_lock_mutex(ssl_mutex_type* mutex)
{
	int rc = -1;

	/* don't add entry/exit trace points, as trace gets lock too, and it might happen quite frequently  */
#if defined(WIN32) || defined(WIN64)
	if (WaitForSingleObject(*mutex, INFINITE) != WAIT_FAILED)
#else
	if ((rc = pthread_mutex_lock(mutex)) == 0)
#endif
	rc = 0;

	return rc;
}

int SSL_unlock_mutex(ssl_mutex_type* mutex)
{
	int rc = -1;

	/* don't add entry/exit trace points, as trace gets lock too, and it might happen quite frequently  */
#if defined(WIN32) || defined(WIN64)
	if (ReleaseMutex(*mutex) != 0)
#else
	if ((rc = pthread_mutex_unlock(mutex)) == 0)
#endif
	rc = 0;

	return rc;
}

void SSL_destroy_mutex(ssl_mutex_type* mutex)
{
	int rc = 0;

	FUNC_ENTRY;
#if defined(WIN32) || defined(WIN64)
	rc = CloseHandle(*mutex);
#else
	rc = pthread_mutex_destroy(mutex);
#endif
	FUNC_EXIT_RC(rc);
	(void)rc;
}



#if (OPENSSL_VERSION_NUMBER >= 0x010000000)
extern void SSLThread_id(CRYPTO_THREADID *id)
{
#if defined(WIN32) || defined(WIN64)
	CRYPTO_THREADID_set_numeric(id, (unsigned long)GetCurrentThreadId());
#else
	CRYPTO_THREADID_set_numeric(id, (unsigned long)pthread_self());
#endif
}
#else
extern unsigned long SSLThread_id(void)
{
#if defined(WIN32) || defined(WIN64)
	return (unsigned long)GetCurrentThreadId();
#else
	return (unsigned long)pthread_self();
#endif
}
#endif

/*
extern void SSLLocks_callback(int mode, int n, const char *file, int line)
{
	if (sslLocks)
	{
		if (mode & CRYPTO_LOCK)
			SSL_lock_mutex(&sslLocks[n]);
		else
			SSL_unlock_mutex(&sslLocks[n]);
	}
}


void SSLSocket_handleOpensslInit(int bool_value)
{
	handle_openssl_init = bool_value;
}
*/

int SSLSocket_initialize(void)
{
	int rc = 0;
	/*int prc;*/
	/*
	int i;
	int lockMemSize;
	
	FUNC_ENTRY;

	if (handle_openssl_init)
	{
		if ((rc = SSL_library_init()) != 1)
			rc = -1;
			
		ERR_load_crypto_strings();
		SSL_load_error_strings();
		
		/ * OpenSSL 0.9.8o and 1.0.0a and later added SHA2 algorithms to SSL_library_init(). 
		Applications which need to use SHA2 in earlier versions of OpenSSL should call 
		OpenSSL_add_all_algorithms() as well. * /
		
		OpenSSL_add_all_algorithms();
		
		lockMemSize = CRYPTO_num_locks() * sizeof(ssl_mutex_type);

		sslLocks = malloc(lockMemSize);
		if (!sslLocks)
		{
			rc = -1;
			goto exit;
		}
		else
			memset(sslLocks, 0, lockMemSize);

		for (i = 0; i < CRYPTO_num_locks(); i++)
		{
			/ * prc = * /SSL_create_mutex(&sslLocks[i]);
		}

#if (OPENSSL_VERSION_NUMBER >= 0x010000000)
		CRYPTO_THREADID_set_callback(SSLThread_id);
#else
		CRYPTO_set_id_callback(SSLThread_id);
#endif
		CRYPTO_set_locking_callback(SSLLocks_callback);
		
	}
	*/
	SSL_create_mutex(&sslCoreMutex);

//exit:
	FUNC_EXIT_RC(rc);
	return rc;
}

void SSLSocket_terminate(void)
{
	FUNC_ENTRY;
	/*
	if (handle_openssl_init)
	{
		EVP_cleanup();
		ERR_free_strings();
		CRYPTO_set_locking_callback(NULL);
		if (sslLocks)
		{
			int i = 0;

			for (i = 0; i < CRYPTO_num_locks(); i++)
			{
				SSL_destroy_mutex(&sslLocks[i]);
			}
			free(sslLocks);
		}
	}
	*/
	if(sslCoreMutex != PTHREAD_MUTEX_INITIALIZER) {
		SSL_destroy_mutex(&sslCoreMutex);
		sslCoreMutex = PTHREAD_MUTEX_INITIALIZER;
	}
	FUNC_EXIT;
}

void *ssl_mem_zalloc(size_t size);
static void mbedtls_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

int SSL_CTX_load_verify_locations(SSL_CTX *ctx, const char *CAfile,
                                  const char *CApath) {
  /* Path is not supported */
  if (CApath != NULL) {
    return 0;
  }

  size_t n;
  unsigned char *buf;

	if( mbedtls_pk_load_file( CAfile, &buf, &n ) != 0 ) return 0;
	// no need to free ctx->client_CA before calling SSL_CTX_add_client_CA
	X509* server_CA = d2i_X509(NULL, buf, n);
	mbedtls_zeroize( buf, n );
	mbedtls_free( buf );

	//SSL_CTX_add_client_CA(ctx, server_CA) <- will load a cert into ctx->client_CA
	//SSL_CTX_use_certificate(ctx, server_CA) <- will load a cert into ctx->cert->x509

	//ssl_pm_reload_crt will hand over ctx->client_CA to mbedtls_ssl_conf_ca_chain
	//so despite it might look weired we need to call SSL_CTX_add_client_CA here...
	return SSL_CTX_add_client_CA(ctx, server_CA);
}

int SSLSocket_createContext(networkHandles* net, MQTTClient_SSLOptions* opts)
{
	int rc = 1;

	FUNC_ENTRY;

	if (net->ctx == NULL) {
		if ((net->ctx = SSL_CTX_new(TLS_client_method())) == NULL)	/* TLS_client_method for compatibility with TLSv1 */
		{
			rc = SSLSocket_error("SSL_CTX_new", NULL, net->socket, rc);
			goto exit;
		}
	}

	if (opts->keyStore)
	{
		rc = SSLSocket_error("keyStore not supported!", NULL, net->socket, rc);
		goto free_ctx;
	}

	if (opts->trustStore)
	{
		if ((rc = SSL_CTX_load_verify_locations(net->ctx, opts->trustStore, NULL)) != 1)
		{
			SSLSocket_error("SSL_CTX_load_verify_locations", NULL, net->socket, rc);
			goto free_ctx;
		}
	}

	if (opts->enabledCipherSuites != NULL)
	{
		rc = SSLSocket_error("enabledCipherSuites not supported!", NULL, net->socket, rc);
		goto free_ctx;
	}

	/*
	 * SSL_CTX_set_mode(net->ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER) is not supported
	 *
   * mbedTLS requires to recall ssl_write with the SAME parameters
   * in case of WANT_WRITE and SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER
   * - doesn't support directly -
   * but since mbedTLS stores sent data as OFFSET (not pointer)
   * it is not a problem to move buffer (if data remains the same)
   */

	goto exit;
free_ctx:
	SSL_CTX_free(net->ctx);
	net->ctx = NULL;

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


int SSLSocket_setSocketForSSL(networkHandles* net, MQTTClient_SSLOptions* opts, char* hostname)
{
	int rc = 1;

	FUNC_ENTRY;

	if (net->ctx != NULL || (rc = SSLSocket_createContext(net, opts)) == 1)
	{

		if (opts->enableServerCertAuth) {
			SSL_CTX_set_verify(net->ctx, SSL_VERIFY_PEER, NULL);
		}

		net->ssl = SSL_new(net->ctx);

		if ((rc = SSL_set_fd(net->ssl, net->socket)) != 1)
			SSLSocket_error("SSL_set_fd", net->ssl, net->socket, rc);

		//SNI - server name indication
		ssl_pm_set_hostname(net->ssl, hostname);
	}

	FUNC_EXIT_RC(rc);
	return rc;
}


int SSLSocket_connect(SSL* ssl, int sock)
{
	int rc = 0;

	FUNC_ENTRY;

retry:
	rc = SSL_connect(ssl);
	if (rc != 1)
	{
		int error;
		if (ssl->verify_mode!=SSL_VERIFY_NONE && (error = SSL_get_verify_result(ssl)) != X509_V_OK) {
			Log(TRACE_MIN, -1, "SSLSocket error %d in %s for socket %d\n", error, "SSL_get_verify_result", sock);
			error = SSL_get_error(ssl, rc);
			rc = error;
		}
		else {
			error = SSL_get_error(ssl, rc);
			if (error == SSL_FATAL) {
				SSLSocket_error("SSL_connect", ssl, sock, rc);
				rc = error;
			}
			if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
				if (NETWORK_AVAILABLE()) {
					goto retry;
				}

				SSLSocket_error("SSL_connect", ssl, sock, rc);
				rc = TCPSOCKET_INTERRUPTED;
			}
		}
	}

	FUNC_EXIT_RC(rc);
	return rc;
}



/**
 *  Reads one byte from a socket
 *  @param socket the socket to read from
 *  @param c the character read, returned
 *  @return completion code
 */
int SSLSocket_getch(SSL* ssl, int socket, char* c)
{
	int rc = SOCKET_ERROR;

	FUNC_ENTRY;
	if ((rc = SocketBuffer_getQueuedChar(socket, c)) != SOCKETBUFFER_INTERRUPTED)
		goto exit;

retry:
	if ((rc = SSL_read(ssl, c, (size_t)1)) < 0)
	{
		int err = SSL_get_error(ssl, rc);
		if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
		{
			if (NETWORK_AVAILABLE()) {
				goto retry;
			} else {
				SSLSocket_error("SSL_read - getch", ssl, socket, rc);

				//we should discard 'half-received' data when the socket (wifi) is gone...
				//so we basically call connectionLost here...
				rc = TCPSOCKET_INTERRUPTED_FINAL;
			}
		} else {
			SSLSocket_error("SSL_read - getch", ssl, socket, rc);
		}
	}
	else if (rc == 0)
		rc = SOCKET_ERROR; 	/* The return value from recv is 0 when the peer has performed an orderly shutdown. */
	else if (rc == 1)
	{
		SocketBuffer_queueChar(socket, *c);
		rc = TCPSOCKET_COMPLETE;
	}
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}



/**
 *  Attempts to read a number of bytes from a socket, non-blocking. If a previous read did not
 *  finish, then retrieve that data.
 *  @param socket the socket to read from
 *  @param bytes the number of bytes to read
 *  @param actual_len the actual number of bytes read
 *  @return completion code
 */
char *SSLSocket_getdata(SSL* ssl, int socket, size_t bytes, size_t* actual_len)
{
	int rc;
	char* buf;

	FUNC_ENTRY;
	if (bytes == 0)
	{
		buf = SocketBuffer_complete(socket);
		goto exit;
	}

	buf = SocketBuffer_getQueuedData(socket, bytes, actual_len);

retry:
	if ((rc = SSL_read(ssl, buf + (*actual_len), (int)(bytes - (*actual_len)))) < 0)
	{
		rc = SSL_get_error(ssl, rc);
		if (rc != SSL_ERROR_WANT_READ && rc != SSL_ERROR_WANT_WRITE)
		{
			SSLSocket_error("SSL_read - getdata", ssl, socket, rc);
			buf = NULL;
			goto exit;
		} else {
			if (NETWORK_AVAILABLE()) {
				goto retry;
			}

			SSLSocket_error("SSL_read - getdata", ssl, socket, rc);
		}
	}
	else if (rc == 0) /* rc 0 means the other end closed the socket */
	{
		buf = NULL;
		goto exit;
	}
	else
		*actual_len += rc;

	if (*actual_len == bytes)
	{
		SocketBuffer_complete(socket);
		/* if we read the whole packet, there might still be data waiting in the SSL buffer, which
		isn't picked up by select.  So here we should check for any data remaining in the SSL buffer, and
		if so, add this socket to a new "pending SSL reads" list.
		*/
		if (SSL_pending(ssl) > 0) /* return no of bytes pending */
			SSLSocket_addPendingRead(socket);
	}
	else /* we didn't read the whole packet */
	{
		SocketBuffer_interrupted(socket, *actual_len);
		Log(TRACE_MAX, -1, "SSL_read: %d bytes expected but %d bytes now received", bytes, *actual_len);
	}
exit:
	FUNC_EXIT;
	return buf;
}

void SSLSocket_destroyContext(networkHandles* net)
{
	FUNC_ENTRY;
	if (net->ctx)
		SSL_CTX_free(net->ctx);
	net->ctx = NULL;
	FUNC_EXIT;
}


int SSLSocket_close(networkHandles* net)
{
	int rc = 1;
	FUNC_ENTRY;
	if (net->ssl) {
		rc = SSL_shutdown(net->ssl);

		//nasty bug... must NOT free original net->ssl->session or else the reconnect won't work
		net->ssl->session = ssl_mem_zalloc(sizeof(SSL_SESSION));

		SSL_free(net->ssl);
		net->ssl = NULL;
	}
	SSLSocket_destroyContext(net);
	FUNC_EXIT_RC(rc);
	return rc;
}


/* No SSL_writev() provided by OpenSSL. Boo. */
int SSLSocket_putdatas(SSL* ssl, int socket, char* buf0, size_t buf0len, int count, char** buffers, size_t* buflens, int* frees)
{
	int rc = 0;
	int i;
	char *ptr;
	iobuf iovec;
	int sslerror;

	FUNC_ENTRY;
	iovec.iov_len = (ULONG)buf0len;
	for (i = 0; i < count; i++)
		iovec.iov_len += (ULONG)buflens[i];

	ptr = iovec.iov_base = (char *)malloc(iovec.iov_len);
	memcpy(ptr, buf0, buf0len);
	ptr += buf0len;
	for (i = 0; i < count; i++)
	{
		memcpy(ptr, buffers[i], buflens[i]);
		ptr += buflens[i];
	}

	SSL_lock_mutex(&sslCoreMutex);
	if ((rc = SSL_write(ssl, iovec.iov_base, iovec.iov_len)) == iovec.iov_len)
		rc = TCPSOCKET_COMPLETE;
	else
	{
		sslerror = SSLSocket_error("SSL_write", ssl, socket, rc);

		if (sslerror == SSL_ERROR_WANT_WRITE)
		{
			if (NETWORK_AVAILABLE()) {
				int* sockmem = (int*)malloc(sizeof(int));
				int free = 1;

				Log(TRACE_MIN, -1, "Partial write: incomplete write of %d bytes on SSL socket %d",
					iovec.iov_len, socket);
				SocketBuffer_pendingWrite(socket, ssl, 1, &iovec, &free, iovec.iov_len, 0);
				*sockmem = socket;
				ListAppend(s.write_pending, sockmem, sizeof(int));
				FD_SET(socket, &(s.pending_wset));
				rc = TCPSOCKET_INTERRUPTED;
			} else {
				rc = SOCKET_ERROR;
			}
		}
		else
			rc = SOCKET_ERROR;
	}
	SSL_unlock_mutex(&sslCoreMutex);

	if (rc != TCPSOCKET_INTERRUPTED)
		free(iovec.iov_base);
	else
	{
		int i;
		free(buf0);
		for (i = 0; i < count; ++i)
		{
			if (frees[i])
				free(buffers[i]);
		}
	}
	FUNC_EXIT_RC(rc);
	return rc;
}

static List pending_reads = {NULL, NULL, NULL, 0, 0};

void SSLSocket_addPendingRead(int sock)
{
	FUNC_ENTRY;
	if (ListFindItem(&pending_reads, &sock, intcompare) == NULL) /* make sure we don't add the same socket twice */
	{
		int* psock = (int*)malloc(sizeof(sock));
		*psock = sock;
		ListAppend(&pending_reads, psock, sizeof(sock));
	}
	else
		Log(TRACE_MIN, -1, "SSLSocket_addPendingRead: socket %d already in the list", sock);

	FUNC_EXIT;
}


int SSLSocket_getPendingRead()
{
	int sock = -1;

	if (pending_reads.count > 0)
	{
		sock = *(int*)(pending_reads.first->content);
		ListRemoveHead(&pending_reads);
	}
	return sock;
}


int SSLSocket_continueWrite(pending_writes* pw)
{
	int rc = 0;

	FUNC_ENTRY;
	if ((rc = SSL_write(pw->ssl, pw->iovecs[0].iov_base, pw->iovecs[0].iov_len)) == pw->iovecs[0].iov_len)
	{
		/* topic and payload buffers are freed elsewhere, when all references to them have been removed */
		free(pw->iovecs[0].iov_base);
		Log(TRACE_MIN, -1, "SSL continueWrite: partial write now complete for socket %d", pw->socket);
		rc = 1;
	}
	else
	{
		int sslerror = SSLSocket_error("SSL_write", pw->ssl, pw->socket, rc);
		if (sslerror == SSL_ERROR_WANT_WRITE)
			rc = 0; /* indicate we haven't finished writing the payload yet */
	}
	FUNC_EXIT_RC(rc);
	return rc;
}
#endif
