/*******************************************************************************
 * Copyright (c) 2009, 2013 IBM Corp.
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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Ian Craggs - updates for the async client
 *******************************************************************************/

/*
 * This file has adapted to Lua RTOS syslog
 *
 */

#if !defined(LOG_H)
#define LOG_H

#include <sys/syslog.h>
#include "Messages.h"

#define TRACE_MAX      8
#define TRACE_MINIMUM  8
#define TRACE_PROTOCOL 10
#define LOG_PROTOCOL   10

#define TRACE_MIN      LOG_INFO
#define LOG_ERROR	   LOG_ERR
#define LOG_SEVERE     LOG_CRIT
#define LOG_FATAL	   LOG_CRIT

#define Log_initialize()
#define Log_terminate()

#define Log(level, msgno, fmt, ...) \
	do { \
		if (msgno < 0) { \
			syslog(level, fmt, ##__VA_ARGS__); \
		} else { \
			if ((level == TRACE_PROTOCOL) || (level == LOG_PROTOCOL)) { \
				syslog(LOG_DEBUG, (const char *)Messages_get(msgno, level), ##__VA_ARGS__); \
			} \
		} \
	} while (0)

#endif
