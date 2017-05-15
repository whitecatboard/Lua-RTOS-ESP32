/* Copyright (c) 2014 Alexander Lamaison <alexander.lamaison@gmail.com>
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 *   Redistributions of source code must retain the above
 *   copyright notice, this list of conditions and the
 *   following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials
 *   provided with the distribution.
 *
 *   Neither the name of the copyright holder nor the names
 *   of any other contributors may be used to endorse or
 *   promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

/* Headers */
#define LIBSSH2_MBEDTLS
#define HAVE_O_NONBLOCK
#define HAVE_UNISTD_H
#define HAVE_INTTYPES_H
#define HAVE_STDLIB_H
#define HAVE_SYS_SELECT_H
#define HAVE_SYS_SOCKET_H
#define HAVE_SYS_TIME_H
#define HAVE_ARPA_INET_H
#define HAVE_NETINET_IN_H

/* Functions */
#define HAVE_STRCASECMP
#define HAVE__STRICMP
#define HAVE_SNPRINTF
#define HAVE__SNPRINTF

/* Workaround for platforms without POSIX strcasecmp (e.g. Windows) */
#ifndef HAVE_STRCASECMP
# ifdef HAVE__STRICMP
# define strcasecmp _stricmp
# define HAVE_STRCASECMP
# endif
#endif

/* Symbols */
#define HAVE___FUNC__
#define HAVE___FUNCTION__

/* Workaround for platforms without C90 __func__ */
#ifndef HAVE___FUNC__
# ifdef HAVE___FUNCTION__
# define __func__ __FUNCTION__
# define HAVE___FUNC__
# endif
#endif
