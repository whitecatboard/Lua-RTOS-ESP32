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
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Ian Craggs - updates for the async client
 *    Ian Craggs - fix for bug #427028
 *******************************************************************************/

/**
 * @file
 * \brief Logging and tracing module
 *
 * 
 */

#include "Log.h"
#include "Messages.h"

#include <unistd.h>

#include <sys/syslog.h>

static enum LOG_LEVELS output_level = TRACE_MINIMUM;
static Log_traceCallback* trace_callback = NULL;

int Log_initialize(Log_nameValue* info)
{
    output_level = TRACE_MINIMUM;
    return 0;
}


void Log_setTraceCallback(Log_traceCallback* callback)
{
	trace_callback = callback;
}


void Log_setTraceLevel(enum LOG_LEVELS level)
{
	output_level = level;
}


void Log_terminate(void)
{
	output_level = INVALID_LEVEL;
}

/**
 * Log a message.  If possible, all messages should be indexed by message number, and
 * the use of the format string should be minimized or negated altogether.  If format is
 * provided, the message number is only used as a message label.
 * @param log_level the log level of the message
 * @param msgno the id of the message to use if the format string is NULL
 * @param aFormat the printf format string to be used if the message id does not exist
 * @param ... the printf inserts
 */
void Log(enum LOG_LEVELS log_level, int msgno, const char *format, ...)
{
	if (log_level >= output_level)
	{
        const char *temp = NULL;
        va_list ap;

        if (format == NULL && (temp = Messages_get(msgno, log_level)) != NULL)
            format = temp;

	    va_start(ap, format);
        vsyslog(LOG_DEBUG, format, ap);
        va_end(ap);
	}
}
