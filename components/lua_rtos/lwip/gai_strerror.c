/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS gai_strerror implementation
 *
 */

#include <netdb.h>
#include "lwip/sys/netdb.h"

const char *
gai_strerror(int errnum)
{
	switch (errnum) {
	case 0:
		return "no error";
	case EAI_BADFLAGS:
		return "invalid value for ai_flags";
	case EAI_NONAME:
		return "name or service is not known";
	case EAI_AGAIN:
		return "temporary failure in name resolution";
	case EAI_FAIL:
		return "non-recoverable failure in name resolution";
	//case EAI_NODATA:
	//	return "no address associated with name";
	case EAI_FAMILY:
		return "ai_family not supported";
	//case EAI_SOCKTYPE:
	//	return "ai_socktype not supported";
	case EAI_SERVICE:
		return "service not supported for ai_socktype";
	//case EAI_ADDRFAMILY:
	//	return "address family for name not supported";
	case EAI_MEMORY:
		return "memory allocation failure";
	//case EAI_SYSTEM:
	//	return "system error";
	//case EAI_BADHINTS:
	//	return "invalid value for hints";
	//case EAI_PROTOCOL:
	//	return "resolved protocol is unknown";
	case EAI_OVERFLOW:
		return "argument buffer overflow";
	default:
		return "unknown/invalid error";
	}
}
