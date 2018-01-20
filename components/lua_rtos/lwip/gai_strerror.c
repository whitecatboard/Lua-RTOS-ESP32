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
