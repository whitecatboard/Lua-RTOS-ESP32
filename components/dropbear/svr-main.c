/*
 * Dropbear - a SSH2 server
 * 
 * Copyright (c) 2002-2006 Matt Johnston
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

#include "includes.h"
#include "dbutil.h"
#include "session.h"
#include "buffer.h"
#include "signkey.h"
#include "runopts.h"
#include "dbrandom.h"
#include "crypto_desc.h"
#include "gensignkey.h"

#include <pthread.h>

#include <sys/path.h>
#include <sys/driver.h>
#include <sys/delay.h>
#include <drivers/net.h>

static size_t listensockets(int *sock, size_t sockcount, int *maxfd);
static void sigintterm_handler(int fish);
static void commonsetup(void);
int dropbearkey_main(int argc, char ** argv);

static int childsock;

void dropbear_server_main_clean() {
	m_close(childsock);
}

int dropbear_server_main(int argc, char ** argv)
{
	fd_set fds;
	unsigned int i;
	int val;
	int maxsock = -1;
	size_t listensockcount = 0;
	int listensocks[MAX_LISTEN_ADDR];

	_dropbear_exit = svr_dropbear_exit;
	_dropbear_log = svr_dropbear_log;

	/* get commandline options */
	svr_getopts(argc, argv);

	/* Note: commonsetup() must happen before we daemon()ise. Otherwise
	   daemon() will chdir("/"), and we won't be able to find local-dir
	   hostkeys. */
	commonsetup();

	/* Set up the listening sockets */
	listensockcount = listensockets(listensocks, MAX_LISTEN_ADDR, &maxsock);
	if (listensockcount == 0)
	{
		dropbear_exit("No listening ports available.");
	}

	dropbear_log(LOG_INFO, "started");

	for (i = 0; i < listensockcount; i++) {
		FD_SET(listensocks[i], &fds);
	}

	/* incoming connection select loop */
	for(;;) {
		FD_ZERO(&fds);

		/* listening sockets */
		for (i = 0; i < listensockcount; i++) {
			FD_SET(listensocks[i], &fds);
		}

		/* pre-authentication clients */
		val = select(maxsock+1, &fds, NULL, NULL, NULL);

		if (val == 0) {
			/* timeout reached - shouldn't happen. eh */
			continue;
		}

		if (val < 0) {
			if (errno == EINTR) {
				continue;
			}
		}

		/* handle each socket which has something to say */
		for (i = 0; i < listensockcount; i++) {
			char *remote_host = NULL, *remote_port = NULL;
			struct sockaddr_storage remoteaddr;
			socklen_t remoteaddrlen;

			if (!FD_ISSET(listensocks[i], &fds))
				continue;

			remoteaddrlen = sizeof(remoteaddr);
			childsock = accept(listensocks[i],
					(struct sockaddr*)&remoteaddr, &remoteaddrlen);

			if (childsock < 0) {
				/* accept failed */
				continue;
			}

			getaddrstring(&remoteaddr, &remote_host, &remote_port, 0);
			dropbear_log(LOG_INFO, "Child connection from %s:%s", remote_host, remote_port);

			if (remote_host) {
				m_free(remote_host);
			}

			if (remote_port) {
				m_free(remote_port);
			}

			seedrandom();

			// Start the session
			svr_session(childsock);

			dropbear_server_main_clean();

			remote_host = NULL;
			remote_port = NULL;

			exitflag = 0;
		}
	} /* for(;;) loop */

	return -1;
}

/* catch ctrl-c or sigterm */
static void sigintterm_handler(int UNUSED(unused)) {
	exitflag = 1;
}

static void commonsetup() {
	if (signal(SIGINT, sigintterm_handler) == SIG_ERR) {
		dropbear_exit("signal() error");
	}

	crypto_init();

	/* Now we can setup the hostkeys - needs to be after logging is on,
	 * otherwise we might end up blatting error messages to the socket */
	load_all_hostkeys();

	seedrandom();
}

/* Set up listening sockets for all the requested ports */
static size_t listensockets(int *socks, size_t sockcount, int *maxfd) {

	unsigned int i, n;
	char* errstring = NULL;
	size_t sockpos = 0;
	int nsock;

	TRACE(("listensockets: %d to try", svr_opts.portcount))

	for (i = 0; i < svr_opts.portcount; i++) {

		TRACE(("listening on '%s:%s'", svr_opts.addresses[i], svr_opts.ports[i]))

		nsock = dropbear_listen(svr_opts.addresses[i], svr_opts.ports[i], &socks[sockpos], 
				sockcount - sockpos,
				&errstring, maxfd);

		if (nsock < 0) {
			dropbear_log(LOG_WARNING, "Failed listening on '%s': %s", 
							svr_opts.ports[i], errstring);
			m_free(errstring);
			continue;
		}

		for (n = 0; n < (unsigned int)nsock; n++) {
			int sock = socks[sockpos + n];
			set_sock_priority(sock, DROPBEAR_PRIO_LOWDELAY);
#ifdef DROPBEAR_SERVER_TCP_FAST_OPEN
			set_listen_fast_open(sock);
#endif
		}

		sockpos += nsock;

	}
	return sockpos;
}

static void create_server_keys() {
	syslog(LOG_INFO, "dropbear: generating %s key file, this may take a while...", RSA_PRIV_FILENAME);
	signkey_generate(DROPBEAR_SIGNKEY_RSA, 0, RSA_PRIV_FILENAME, 1);

	syslog(LOG_INFO, "dropbear: generating %s key file, this may take a while...", DSS_PRIV_FILENAME);
	signkey_generate(DROPBEAR_SIGNKEY_DSS, 0, DSS_PRIV_FILENAME, 1);
}

static void *dropbear_thread(void *arg) {
	driver_error_t *error;

	// Create dropbear directory structure if not exist
	mkpath("/etc/dropbear");

	// Create the system passwd file if not exist
	mkfile("/etc/passwd");

	// Create the server keys if not exist
	create_server_keys();

	// Wait for network
	while ((error = net_check_connectivity())) {
		free(error);
		delay(100);
	}

	const char *argv[] = {
		"dropbear"
	};

	dropbear_server_main(1, (char **)argv);

	return NULL;
}

void dropbear_server_start() {
	pthread_t thread;
	pthread_attr_t attr;

	syslog(LOG_INFO, "dropbear: starting ...");

	// Start a new thread to launch the dropbear server
	pthread_attr_init(&attr);

    pthread_attr_setstacksize(&attr, 12288);

    if (pthread_create(&thread, &attr, dropbear_thread, NULL)) {
    	return;
	}

    pthread_setname_np(thread, "ssh");
}

void dropbear_server_stop() {

}
