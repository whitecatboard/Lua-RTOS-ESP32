/*******************************************************************************
 * Copyright (c) 2009, 2018 IBM Corp.
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
char* MQTTProtocol_addressPort(const char* uri, int* port);

#include "Heap.h"

#if !__XTENSA__
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/x509v3.h>
#else
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
#endif
#if __XTENSA__
#include <platform/ssl_port.h>
#endif

extern Sockets s;

int SSLSocket_error(char* aString, SSL* ssl, int sock, int rc);
char* SSL_get_verify_result_string(int rc);
#if !__XTENSA__
void SSL_CTX_info_callback(const SSL* ssl, int where, int ret);
char* SSLSocket_get_version_string(int version);
void SSL_CTX_msg_callback(
		int write_p,
		int version,
		int content_type,
		const void* buf, size_t len,
		SSL* ssl, void* arg);
#endif
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

#if !__XTENSA__
/* 1 ~ we are responsible for initializing openssl; 0 ~ openssl init is done externally */
static int handle_openssl_init = 1;
static ssl_mutex_type* sslLocks = NULL;
#endif

#if !__XTENSA__
static ssl_mutex_type sslCoreMutex;
#else
static ssl_mutex_type sslCoreMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#if defined(WIN32) || defined(WIN64)
#define iov_len len
#define iov_base buf
#endif

#if defined(OPENSSL) && __XTENSA__
SSL_SESSION *SSL_get1_session(SSL *ssl)
{
    return ssl->session;
}

int SSL_set_session(SSL *ssl, SSL_SESSION *session)
{
    ssl->session = session;
    return 1;
}

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
    // MUST NOT !!! FREE THIS BUFFER: mbedtls_free( buf );

    //SSL_CTX_add_client_CA(ctx, server_CA) <- will load a cert into ctx->client_CA
    //SSL_CTX_use_certificate(ctx, server_CA) <- will load a cert into ctx->cert->x509

    //ssl_pm_reload_crt will hand over ctx->client_CA to mbedtls_ssl_conf_ca_chain
    //so despite it might look weired we need to call SSL_CTX_add_client_CA here...
    return SSL_CTX_add_client_CA(ctx, server_CA);
}
#endif

/**
 * Gets the specific error corresponding to SOCKET_ERROR
 * @param aString the function that was being used when the error occurred
 * @param sock the socket on which the error occurred
 * @return the specific TCP error code
 */
int SSLSocket_error(char* aString, SSL* ssl, int sock, int rc)
{
#if !__XTENSA__       
    int error;
#else
    int error = -1;
#endif
    

    FUNC_ENTRY;
    if (ssl)
        error = SSL_get_error(ssl, rc);
#if !__XTENSA__        
    else
        error = ERR_get_error();
#else
    else {
        Log(TRACE_MIN, -1, "SSLSocket error in %s\n", aString);
    }
#endif
    if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
    {
		Log(TRACE_MIN, -1, "SSLSocket error WANT_READ/WANT_WRITE");
    }
    else
    {

#if !__XTENSA__
        static char buf[120];

        if (strcmp(aString, "shutdown") != 0)
        	Log(TRACE_MIN, -1, "SSLSocket error %s(%d) in %s for socket %d rc %d errno %d %s\n", buf, error, aString, sock, rc, errno, strerror(errno));
        ERR_print_errors_fp(stderr);
#else
        if (strcmp(aString, "shutdown") != 0)
        	Log(TRACE_MIN, -1, "SSLSocket error %d in %s for socket %d rc %d errno %d %s\n", error, aString, sock, rc, errno, strerror(errno));
#endif
		if (error == SSL_ERROR_SSL || error == SSL_ERROR_SYSCALL)
			error = SSL_FATAL;
    }
    FUNC_EXIT_RC(error);
    return error;
}

#if !__XTENSA__
static struct
{
	int code;
	char* string;
}
X509_message_table[] =
{
	{ X509_V_OK, "X509_V_OK" },
	{ X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT, "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT" },
	{ X509_V_ERR_UNABLE_TO_GET_CRL, "X509_V_ERR_UNABLE_TO_GET_CRL" },
	{ X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE, "X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE" },
	{ X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE, "X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE" },
	{ X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY, "X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY" },
	{ X509_V_ERR_CERT_SIGNATURE_FAILURE, "X509_V_ERR_CERT_SIGNATURE_FAILURE" },
	{ X509_V_ERR_CRL_SIGNATURE_FAILURE, "X509_V_ERR_CRL_SIGNATURE_FAILURE" },
	{ X509_V_ERR_CERT_NOT_YET_VALID, "X509_V_ERR_CERT_NOT_YET_VALID" },
	{ X509_V_ERR_CERT_HAS_EXPIRED, "X509_V_ERR_CERT_HAS_EXPIRED" },
	{ X509_V_ERR_CRL_NOT_YET_VALID, "X509_V_ERR_CRL_NOT_YET_VALID" },
	{ X509_V_ERR_CRL_HAS_EXPIRED, "X509_V_ERR_CRL_HAS_EXPIRED" },
	{ X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD, "X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD" },
	{ X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD, "X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD" },
	{ X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD, "X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD" },
	{ X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD, "X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD" },
	{ X509_V_ERR_OUT_OF_MEM, "X509_V_ERR_OUT_OF_MEM" },
	{ X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT, "X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT" },
	{ X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN, "X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN" },
	{ X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY, "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY" },
	{ X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE, "X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE" },
	{ X509_V_ERR_CERT_CHAIN_TOO_LONG, "X509_V_ERR_CERT_CHAIN_TOO_LONG" },
	{ X509_V_ERR_CERT_REVOKED, "X509_V_ERR_CERT_REVOKED" },
	{ X509_V_ERR_INVALID_CA, "X509_V_ERR_INVALID_CA" },
	{ X509_V_ERR_PATH_LENGTH_EXCEEDED, "X509_V_ERR_PATH_LENGTH_EXCEEDED" },
	{ X509_V_ERR_INVALID_PURPOSE, "X509_V_ERR_INVALID_PURPOSE" },
	{ X509_V_ERR_CERT_UNTRUSTED, "X509_V_ERR_CERT_UNTRUSTED" },
	{ X509_V_ERR_CERT_REJECTED, "X509_V_ERR_CERT_REJECTED" },
	{ X509_V_ERR_SUBJECT_ISSUER_MISMATCH, "X509_V_ERR_SUBJECT_ISSUER_MISMATCH" },
	{ X509_V_ERR_AKID_SKID_MISMATCH, "X509_V_ERR_AKID_SKID_MISMATCH" },
	{ X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH, "X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH" },
	{ X509_V_ERR_KEYUSAGE_NO_CERTSIGN, "X509_V_ERR_KEYUSAGE_NO_CERTSIGN" },
	{ X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER, "X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER" },
	{ X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION, "X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION" },
	{ X509_V_ERR_KEYUSAGE_NO_CRL_SIGN, "X509_V_ERR_KEYUSAGE_NO_CRL_SIGN" },
	{ X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION, "X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION" },
	{ X509_V_ERR_INVALID_NON_CA, "X509_V_ERR_INVALID_NON_CA" },
	{ X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED, "X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED" },
	{ X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE, "X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE" },
	{ X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED, "X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED" },
	{ X509_V_ERR_INVALID_EXTENSION, "X509_V_ERR_INVALID_EXTENSION" },
	{ X509_V_ERR_INVALID_POLICY_EXTENSION, "X509_V_ERR_INVALID_POLICY_EXTENSION" },
	{ X509_V_ERR_NO_EXPLICIT_POLICY, "X509_V_ERR_NO_EXPLICIT_POLICY" },
	{ X509_V_ERR_UNNESTED_RESOURCE, "X509_V_ERR_UNNESTED_RESOURCE" },
#if defined(X509_V_ERR_DIFFERENT_CRL_SCOPE)
	{ X509_V_ERR_DIFFERENT_CRL_SCOPE, "X509_V_ERR_DIFFERENT_CRL_SCOPE" },
	{ X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE, "X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE" },
	{ X509_V_ERR_PERMITTED_VIOLATION, "X509_V_ERR_PERMITTED_VIOLATION" },
	{ X509_V_ERR_EXCLUDED_VIOLATION, "X509_V_ERR_EXCLUDED_VIOLATION" },
	{ X509_V_ERR_SUBTREE_MINMAX, "X509_V_ERR_SUBTREE_MINMAX" },
	{ X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE, "X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE" },
	{ X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX, "X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX" },
	{ X509_V_ERR_UNSUPPORTED_NAME_SYNTAX, "X509_V_ERR_UNSUPPORTED_NAME_SYNTAX" },
#endif
};

#if !defined(ARRAY_SIZE)
/**
 * Macro to calculate the number of entries in an array
 */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

char* SSL_get_verify_result_string(int rc)
{
	int i;
	char* retstring = "undef";

	for (i = 0; i < ARRAY_SIZE(X509_message_table); ++i)
	{
		if (X509_message_table[i].code == rc)
		{
			retstring = X509_message_table[i].string;
			break;
		}
	}
	return retstring;
}

#if !__XENSA__
void SSL_CTX_info_callback(const SSL* ssl, int where, int ret)
{
	if (where & SSL_CB_LOOP)
	{
		Log(TRACE_PROTOCOL, 1, "SSL state %s:%s:%s",
                  (where & SSL_ST_CONNECT) ? "connect" : (where & SSL_ST_ACCEPT) ? "accept" : "undef",
                    SSL_state_string_long(ssl), SSL_get_cipher_name(ssl));
	}
	else if (where & SSL_CB_EXIT)
	{
		Log(TRACE_PROTOCOL, 1, "SSL %s:%s",
                  (where & SSL_ST_CONNECT) ? "connect" : (where & SSL_ST_ACCEPT) ? "accept" : "undef",
                    SSL_state_string_long(ssl));
	}
	else if (where & SSL_CB_ALERT)
	{
		Log(TRACE_PROTOCOL, 1, "SSL alert %s:%s:%s",
                  (where & SSL_CB_READ) ? "read" : "write",
                    SSL_alert_type_string_long(ret), SSL_alert_desc_string_long(ret));
	}
	else if (where & SSL_CB_HANDSHAKE_START)
	{
		Log(TRACE_PROTOCOL, 1, "SSL handshake started %s:%s:%s",
                  (where & SSL_CB_READ) ? "read" : "write",
                    SSL_alert_type_string_long(ret), SSL_alert_desc_string_long(ret));
	}
	else if (where & SSL_CB_HANDSHAKE_DONE)
	{
		Log(TRACE_PROTOCOL, 1, "SSL handshake done %s:%s:%s",
                  (where & SSL_CB_READ) ? "read" : "write",
                    SSL_alert_type_string_long(ret), SSL_alert_desc_string_long(ret));
		Log(TRACE_PROTOCOL, 1, "SSL certificate verification: %s",
                    SSL_get_verify_result_string(SSL_get_verify_result(ssl)));
	}
	else
	{
		Log(TRACE_PROTOCOL, 1, "SSL state %s:%s:%s", SSL_state_string_long(ssl),
                   SSL_alert_type_string_long(ret), SSL_alert_desc_string_long(ret));
	}
}

char* SSLSocket_get_version_string(int version)
{
	int i;
	static char buf[20];
	char* retstring = NULL;
	static struct
	{
		int code;
		char* string;
	}
	version_string_table[] =
	{
		{ SSL2_VERSION, "SSL 2.0" },
		{ SSL3_VERSION, "SSL 3.0" },
		{ TLS1_VERSION, "TLS 1.0" },
#if defined(TLS2_VERSION)
		{ TLS2_VERSION, "TLS 1.1" },
#endif
#if defined(TLS3_VERSION)
		{ TLS3_VERSION, "TLS 1.2" },
#endif
	};

	for (i = 0; i < ARRAY_SIZE(version_string_table); ++i)
	{
		if (version_string_table[i].code == version)
		{
			retstring = version_string_table[i].string;
			break;
		}
	}

	if (retstring == NULL)
	{
		sprintf(buf, "%i", version);
		retstring = buf;
	}
	return retstring;
}

void SSL_CTX_msg_callback(int write_p, int version, int content_type, const void* buf, size_t len,
        SSL* ssl, void* arg)
{

/*
called by the SSL/TLS library for a protocol message, the function arguments have the following meaning:

write_p
This flag is 0 when a protocol message has been received and 1 when a protocol message has been sent.

version
The protocol version according to which the protocol message is interpreted by the library. Currently, this is one of SSL2_VERSION, SSL3_VERSION and TLS1_VERSION (for SSL 2.0, SSL 3.0 and TLS 1.0, respectively).

content_type
In the case of SSL 2.0, this is always 0. In the case of SSL 3.0 or TLS 1.0, this is one of the ContentType values defined in the protocol specification (change_cipher_spec(20), alert(21), handshake(22); but never application_data(23) because the callback will only be called for protocol messages).

buf, len
buf points to a buffer containing the protocol message, which consists of len bytes. The buffer is no longer valid after the callback function has returned.

ssl
The SSL object that received or sent the message.

arg
The user-defined argument optionally defined by SSL_CTX_set_msg_callback_arg() or SSL_set_msg_callback_arg().

*/

	Log(TRACE_PROTOCOL, -1, "%s %s %d buflen %d", (write_p ? "sent" : "received"),
		SSLSocket_get_version_string(version),
		content_type, (int)len);
}
#endif

int pem_passwd_cb(char* buf, int size, int rwflag, void* userdata)
{
	int rc = 0;

	FUNC_ENTRY;
	if (!rwflag)
	{
		strncpy(buf, (char*)(userdata), size);
		buf[size-1] = '\0';
		rc = (int)strlen(buf);
	}
	FUNC_EXIT_RC(rc);
	return rc;
}
#endif

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
#if __XTENSA__
	(void)rc;
#endif
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

#if !__XTENSA__
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
#endif


int SSLSocket_initialize(void)
{
	int rc = 0;
	/*int prc;*/
#if !__XTENSA__
	int i;
	int lockMemSize;

	FUNC_ENTRY;

	if (handle_openssl_init)
	{
		if ((rc = SSL_library_init()) != 1)
			rc = -1;

		ERR_load_crypto_strings();
		SSL_load_error_strings();

		/* OpenSSL 0.9.8o and 1.0.0a and later added SHA2 algorithms to SSL_library_init().
		Applications which need to use SHA2 in earlier versions of OpenSSL should call
		OpenSSL_add_all_algorithms() as well. */

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
			/* prc = */SSL_create_mutex(&sslLocks[i]);
		}

#if (OPENSSL_VERSION_NUMBER >= 0x010000000)
		CRYPTO_THREADID_set_callback(SSLThread_id);
#else
		CRYPTO_set_id_callback(SSLThread_id);
#endif
		CRYPTO_set_locking_callback(SSLLocks_callback);

	}
#endif

	SSL_create_mutex(&sslCoreMutex);

#if !__XTENSA__
exit:
#endif
	FUNC_EXIT_RC(rc);
	return rc;
}

void SSLSocket_terminate(void)
{
	FUNC_ENTRY;
#if !__XTENSA__
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

	SSL_destroy_mutex(&sslCoreMutex);
#else
	if(sslCoreMutex != PTHREAD_MUTEX_INITIALIZER) {
		SSL_destroy_mutex(&sslCoreMutex);
		sslCoreMutex = PTHREAD_MUTEX_INITIALIZER;
	}
#endif

	FUNC_EXIT;
}

#if !__XTENSA__
int SSLSocket_createContext(networkHandles* net, MQTTClient_SSLOptions* opts)
{
	int rc = 1;
	const char* ciphers = NULL;

	FUNC_ENTRY;
	if (net->ctx == NULL)
	{
		int sslVersion = MQTT_SSL_VERSION_DEFAULT;
		if (opts->struct_version >= 1) sslVersion = opts->sslVersion;
/* SSL_OP_NO_TLSv1_1 is defined in ssl.h if the library version supports TLSv1.1.
 * OPENSSL_NO_TLS1 is defined in opensslconf.h or on the compiler command line
 * if TLS1.x was removed at OpenSSL library build time via Configure options.
 */
		switch (sslVersion)
		{
		case MQTT_SSL_VERSION_DEFAULT:
			net->ctx = SSL_CTX_new(SSLv23_client_method()); /* SSLv23 for compatibility with SSLv2, SSLv3 and TLSv1 */
			break;
#if defined(SSL_OP_NO_TLSv1) && !defined(OPENSSL_NO_TLS1)
		case MQTT_SSL_VERSION_TLS_1_0:
			net->ctx = SSL_CTX_new(TLSv1_client_method());
			break;
#endif
#if defined(SSL_OP_NO_TLSv1_1) && !defined(OPENSSL_NO_TLS1)
		case MQTT_SSL_VERSION_TLS_1_1:
			net->ctx = SSL_CTX_new(TLSv1_1_client_method());
			break;
#endif
#if defined(SSL_OP_NO_TLSv1_2) && !defined(OPENSSL_NO_TLS1)
		case MQTT_SSL_VERSION_TLS_1_2:
			net->ctx = SSL_CTX_new(TLSv1_2_client_method());
			break;
#endif
		default:
			break;
		}
		if (net->ctx == NULL)
		{
			SSLSocket_error("SSL_CTX_new", NULL, net->socket, rc);
			goto exit;
		}
	}

	if (opts->keyStore)
	{
		if ((rc = SSL_CTX_use_certificate_chain_file(net->ctx, opts->keyStore)) != 1)
		{
			SSLSocket_error("SSL_CTX_use_certificate_chain_file", NULL, net->socket, rc);
			goto free_ctx; /*If we can't load the certificate (chain) file then loading the privatekey won't work either as it needs a matching cert already loaded */
		}

		if (opts->privateKey == NULL)
			opts->privateKey = opts->keyStore;   /* the privateKey can be included in the keyStore */

		if (opts->privateKeyPassword != NULL)
		{
			SSL_CTX_set_default_passwd_cb(net->ctx, pem_passwd_cb);
			SSL_CTX_set_default_passwd_cb_userdata(net->ctx, (void*)opts->privateKeyPassword);
		}

		/* support for ASN.1 == DER format? DER can contain only one certificate? */
		rc = SSL_CTX_use_PrivateKey_file(net->ctx, opts->privateKey, SSL_FILETYPE_PEM);
		if (opts->privateKey == opts->keyStore)
			opts->privateKey = NULL;
		if (rc != 1)
		{
			SSLSocket_error("SSL_CTX_use_PrivateKey_file", NULL, net->socket, rc);
			goto free_ctx;
		}
	}

	if (opts->trustStore)
	{
		if ((rc = SSL_CTX_load_verify_locations(net->ctx, opts->trustStore, NULL)) != 1)
		{
			SSLSocket_error("SSL_CTX_load_verify_locations", NULL, net->socket, rc);
			goto free_ctx;
		}
	}
	else if ((rc = SSL_CTX_set_default_verify_paths(net->ctx)) != 1)
	{
		SSLSocket_error("SSL_CTX_set_default_verify_paths", NULL, net->socket, rc);
		goto free_ctx;
	}

	if (opts->enabledCipherSuites == NULL)
		ciphers = "DEFAULT";
	else
		ciphers = opts->enabledCipherSuites;

	if ((rc = SSL_CTX_set_cipher_list(net->ctx, ciphers)) != 1)
	{
		SSLSocket_error("SSL_CTX_set_cipher_list", NULL, net->socket, rc);
		goto free_ctx;
	}

	SSL_CTX_set_mode(net->ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	goto exit;
free_ctx:
	SSL_CTX_free(net->ctx);
	net->ctx = NULL;

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
#else
int SSLSocket_createContext(networkHandles* net, MQTTClient_SSLOptions* opts)
{
    int rc = 1;

    FUNC_ENTRY;

    if (net->ctx == NULL) {
        if ((net->ctx = SSL_CTX_new(TLS_client_method())) == NULL)  /* TLS_client_method for compatibility with TLSv1 */
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
#endif

int SSLSocket_setSocketForSSL(networkHandles* net, MQTTClient_SSLOptions* opts, char* hostname)
{
	int rc = 1;

	FUNC_ENTRY;

	if (net->ctx != NULL || (rc = SSLSocket_createContext(net, opts)) == 1)
	{
#if !__XTENSA__
		int i;

		SSL_CTX_set_info_callback(net->ctx, SSL_CTX_info_callback);
		SSL_CTX_set_msg_callback(net->ctx, SSL_CTX_msg_callback);
#endif
   		if (opts->enableServerCertAuth)
			SSL_CTX_set_verify(net->ctx, SSL_VERIFY_PEER, NULL);

		net->ssl = SSL_new(net->ctx);

#if !__XTENSA__
		/* Log all ciphers available to the SSL sessions (loaded in ctx) */
		for (i = 0; ;i++)
		{
			const char* cipher = SSL_get_cipher_list(net->ssl, i);
			if (cipher == NULL)
				break;
			Log(TRACE_PROTOCOL, 1, "SSL cipher available: %d:%s", i, cipher);
	    	}
#endif

		if ((rc = SSL_set_fd(net->ssl, net->socket)) != 1)
			SSLSocket_error("SSL_set_fd", net->ssl, net->socket, rc);

		if ((rc = SSL_set_tlsext_host_name(net->ssl, hostname)) != 1)
			SSLSocket_error("SSL_set_tlsext_host_name", NULL, net->socket, rc);
	}

	FUNC_EXIT_RC(rc);
	return rc;
}

/*
 * Return value: 1 - success, TCPSOCKET_INTERRUPTED - try again, anything else is failure
 */
int SSLSocket_connect(SSL* ssl, int sock, char* hostname, int verify)
{
	int rc = 0;

	FUNC_ENTRY;

#if __XTENSA__

#endif
	rc = SSL_connect(ssl);
	if (rc != 1)
	{
		int error;
		error = SSLSocket_error("SSL_connect", ssl, sock, rc);
		if (error == SSL_FATAL)
			rc = error;
		if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
			rc = TCPSOCKET_INTERRUPTED;
	}
#if (OPENSSL_VERSION_NUMBER >= 0x010002000) /* 1.0.2 and later */
	else if (verify == 1)
	{
		char* peername = NULL;
		int port;
		char* addr = NULL;

		X509* cert = SSL_get_peer_certificate(ssl);
		addr = MQTTProtocol_addressPort(hostname, &port);

		rc = X509_check_host(cert, addr, strlen(addr), 0, &peername);
		if (rc == 0)
			rc = SOCKET_ERROR;
		Log(TRACE_MIN, -1, "rc from X509_check_host is %d", rc);
		Log(TRACE_MIN, -1, "peername from X509_check_host is %s", peername);

		if (cert)
			X509_free(cert);
		if (addr != hostname)
			free(addr);
	}
#endif

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

	if ((rc = SSL_read(ssl, c, (size_t)1)) < 0)
	{
		int err = SSLSocket_error("SSL_read - getch", ssl, socket, rc);
		if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
		{
			rc = TCPSOCKET_INTERRUPTED;
			SocketBuffer_interrupted(socket, 0);
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

	if ((rc = SSL_read(ssl, buf + (*actual_len), (int)(bytes - (*actual_len)))) < 0)
	{
		rc = SSLSocket_error("SSL_read - getdata", ssl, socket, rc);
		if (rc != SSL_ERROR_WANT_READ && rc != SSL_ERROR_WANT_WRITE)
		{
			buf = NULL;
			goto exit;
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

#if __XTENSA__
		//nasty bug... must NOT free original net->ssl->session or there will be a
		//Guru Meditation Error: Core  0 panic'ed (LoadProhibited) on reconnect
		net->ssl->session = ssl_mem_zalloc(sizeof(SSL_SESSION));
#endif

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
#if __XTENSA__
  if (ptr) {
#endif
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
			int* sockmem = (int*)malloc(sizeof(int));
			int free = 1;

			Log(TRACE_MIN, -1, "Partial write: incomplete write of %d bytes on SSL socket %d",
				iovec.iov_len, socket);
			SocketBuffer_pendingWrite(socket, ssl, 1, &iovec, &free, iovec.iov_len, 0);
			*sockmem = socket;
			ListAppend(s.write_pending, sockmem, sizeof(int));
			FD_SET(socket, &(s.pending_wset));
			rc = TCPSOCKET_INTERRUPTED;
		}
		else
			rc = SOCKET_ERROR;
	}
	SSL_unlock_mutex(&sslCoreMutex);

	if (rc != TCPSOCKET_INTERRUPTED)
		free(iovec.iov_base);
	else
	{
#if !__XTENSA__
		int i;
#endif
		free(buf0);
		for (i = 0; i < count; ++i)
		{
		    if (frees[i])
		    {
			free(buffers[i]);
			buffers[i] = NULL;
		    }
		}	
	}
#if __XTENSA__
  }
#endif
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
#if __XTENSA__
    if (psock) {
#endif
		*psock = sock;
		ListAppend(&pending_reads, psock, sizeof(sock));
#if __XTENSA__
    }
#endif
	}
	else
		Log(TRACE_MIN, -1, "SSLSocket_addPendingRead: socket %d already in the list", sock);

	FUNC_EXIT;
}


int SSLSocket_getPendingRead(void)
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
