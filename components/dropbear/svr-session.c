/*
 * Dropbear - a SSH2 server
 * 
 * Copyright (c) 2002,2003 Matt Johnston
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "includes.h"
#include "session.h"
#include "dbutil.h"
#include "packet.h"
#include "algo.h"
#include "buffer.h"
#include "dss.h"
#include "ssh.h"
#include "dbrandom.h"
#include "kex.h"
#include "channel.h"
#include "chansession.h"
#include "atomicio.h"
#include "tcpfwd.h"
#include "service.h"
#include "auth.h"
#include "runopts.h"
#include "crypto_desc.h"

static void svr_remoteclosed(void);
void dropbear_server_resstart( void * pvParameter1, uint32_t ulParameter2 );
extern void dropbear_server_main_clean();
extern int __select_cancelled;

struct serversession svr_ses; /* GLOBAL */

static const packettype svr_packettypes[] = {
	{SSH_MSG_CHANNEL_DATA, recv_msg_channel_data},
	{SSH_MSG_CHANNEL_WINDOW_ADJUST, recv_msg_channel_window_adjust},
	{SSH_MSG_USERAUTH_REQUEST, recv_msg_userauth_request}, /* server */
	{SSH_MSG_SERVICE_REQUEST, recv_msg_service_request}, /* server */
	{SSH_MSG_KEXINIT, recv_msg_kexinit},
	{SSH_MSG_KEXDH_INIT, recv_msg_kexdh_init}, /* server */
	{SSH_MSG_NEWKEYS, recv_msg_newkeys},
	{SSH_MSG_GLOBAL_REQUEST, recv_msg_global_request_remotetcp},
	{SSH_MSG_CHANNEL_REQUEST, recv_msg_channel_request},
	{SSH_MSG_CHANNEL_OPEN, recv_msg_channel_open},
	{SSH_MSG_CHANNEL_EOF, recv_msg_channel_eof},
	{SSH_MSG_CHANNEL_CLOSE, recv_msg_channel_close},
	{SSH_MSG_CHANNEL_SUCCESS, ignore_recv_response},
	{SSH_MSG_CHANNEL_FAILURE, ignore_recv_response},
	{SSH_MSG_REQUEST_FAILURE, ignore_recv_response}, /* for keepalive */
	{SSH_MSG_REQUEST_SUCCESS, ignore_recv_response}, /* client */
#ifdef USING_LISTENERS
	{SSH_MSG_CHANNEL_OPEN_CONFIRMATION, recv_msg_channel_open_confirmation},
	{SSH_MSG_CHANNEL_OPEN_FAILURE, recv_msg_channel_open_failure},
#endif
	{0, 0} /* End */
};

static const struct ChanType *svr_chantypes[] = {
	&svrchansess,
#ifdef ENABLE_SVR_LOCALTCPFWD
	&svr_chan_tcpdirect,
#endif
	NULL /* Null termination is mandatory. */
};

static void
svr_session_cleanup(void) {
	/* free potential public key options */
	svr_pubkey_options_cleanup();

	m_free(svr_ses.addrstring);
	m_free(svr_ses.remotehost);
	m_free(svr_ses.childpids);
	svr_ses.childpidsize = 0;
}

void svr_session(int sock) {
	char *host, *port;
	size_t len;

	common_session_init(sock, sock);

	svr_authinitialise();
	chaninitialise(svr_chantypes);
	svr_chansessinitialise();

	/* for logging the remote address */
	get_socket_address(ses.sock_in, NULL, NULL, &host, &port, 0);
	len = strlen(host) + strlen(port) + 2;
	svr_ses.addrstring = m_malloc(len);
	snprintf(svr_ses.addrstring, len, "%s:%s", host, port);
	m_free(host);
	m_free(port);

	get_socket_address(ses.sock_in, NULL, NULL, 
			&svr_ses.remotehost, NULL, 1);

	/* set up messages etc */
	ses.remoteclosed = svr_remoteclosed;
	ses.extra_session_cleanup = svr_session_cleanup;

	/* packet handlers */
	ses.packettypes = svr_packettypes;

	ses.isserver = 1;

	/* We're ready to go now */
	sessinitdone = 1;

	/* exchange identification, version etc */
	send_session_identification();
	
	kexfirstinitialise(); /* initialise the kex state */

	/* start off with key exchange */
	send_msg_kexinit();

	/* Run the main for loop. NULL is for the dispatcher - only the client
	 * code makes use of it */
	session_loop(NULL);

	session_cleanup();

	/* Not reached */
}

/* failure exit - format must be <= 100 chars */
void svr_dropbear_exit(int exitcode, const char* format, va_list param) {
	char *exitmsg;
	char *fullmsg;
	int i;

	exitmsg = malloc(150);
	if (!exitmsg) {
		goto exit;
	}

	fullmsg = malloc(300);
	if (!fullmsg) {
		goto exit;
	}

	/* Render the formatted exit message */
	vsnprintf(exitmsg, 150, format, param);

	/* Add the prefix depending on session/auth state */
	if (!sessinitdone) {
		/* before session init */
		snprintf(fullmsg, 300, "Early exit: %s", exitmsg);
	} else if (ses.authstate.authdone) {
		/* user has authenticated */
		snprintf(fullmsg, 300,
				"Exit (%s). %s",
				ses.authstate.pw_name, exitmsg);
	} else if (ses.authstate.pw_name) {
		free(exitmsg);
		free(fullmsg);

		return;
	} else {
		free(exitmsg);
		free(fullmsg);

		return;
	}

	dropbear_log(LOG_INFO, "%s", fullmsg);

exit:
	free(exitmsg);
	free(fullmsg);

	svr_session_cleanup();

	m_close(ses.sock_in);
	m_close(ses.sock_out);

	if (svr_opts.hostkey) {
		sign_key_free(svr_opts.hostkey);
		svr_opts.hostkey = NULL;
	}
	for (i = 0; i < DROPBEAR_MAX_PORTS; i++) {
		m_free(svr_opts.addresses[i]);
		m_free(svr_opts.ports[i]);
	}

	dropbear_server_main_clean();

	exit(exitcode);
}

/* priority is priority as with syslog() */
void svr_dropbear_log(int priority, const char* format, va_list param) {
	char*printbuf;
	char *datestr;
	time_t timesec;
	int havetrace = 0;

	printbuf = malloc(1024);
	if (!printbuf) {
		vsyslog(priority, format, param);
		return;
	}

	datestr = malloc(20);
	if (!datestr) {
		free(printbuf);
		vsyslog(priority, format, param);
		return;
	}

	vsnprintf(printbuf, 1024, format, param);

	syslog(priority, "dropbear: %s", printbuf);

	/* if we are using DEBUG_TRACE, we want to print to stderr even if
	 * syslog is used, so it is included in error reports */
#ifdef DEBUG_TRACE
	havetrace = debug_trace;
#endif

	if (!opts.usingsyslog || havetrace) {
		struct tm * local_tm = NULL;
		timesec = time(NULL);
		local_tm = localtime(&timesec);
		if (local_tm == NULL
			|| strftime(datestr, 20, "%b %d %H:%M:%S",
						local_tm) == 0)
		{
			/* upon failure, just print the epoch-seconds time. */
			snprintf(datestr, 20, "%d", (int)timesec);
		}
		fprintf(stderr, "dropbear: %s %s\n", datestr, printbuf);
	}

	free(printbuf);
	free(datestr);
}

struct dropbear_progress_connection {
	struct addrinfo *res;
	struct addrinfo *res_iter;

	char *remotehost, *remoteport; /* For error reporting */

	connect_callback cb;
	void *cb_data;

	struct Queue *writequeue; /* A queue of encrypted packets to send with TCP fastopen,
								or NULL. */

	int sock;

	char* errstring;
};

/* called when the remote side closes the connection */
static void svr_remoteclosed() {
	__select_cancelled = 1;
	exitflag = 1;
}
