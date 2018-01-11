#include "linux/netdb.h"
#include "lwip/ip_addr.h"

int
getnameinfo(const struct sockaddr *sa, socklen_t salen,
		    char *host, size_t hostlen,
			char *serv, size_t servlen, int flags)
{
	 if (flags & ~(NI_NUMERICHOST | NI_NUMERICSERV)) {
		return EAI_BADFLAGS;
	}

	const struct sockaddr_in *sinp = (const struct sockaddr_in *) sa;

	switch (sa->sa_family) {
	case AF_INET:
		if (flags & NI_NUMERICHOST) {
			if (inet_ntop (AF_INET, &sinp->sin_addr, host, hostlen) == NULL) {
				return EAI_OVERFLOW;
			}
		}

		if (flags & NI_NUMERICSERV) {
			if (snprintf(serv, servlen, "%d", ntohs (sinp->sin_port)) < 0) {
				return EAI_OVERFLOW;
			}
		}

		break;
	}


	return 0;
}
