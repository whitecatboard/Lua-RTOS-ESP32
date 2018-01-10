/*
 * in6.h
 *
 *  Created on: Jan 2, 2018
 *      Author: jolive
 */

#ifndef COMPONENTS_LINUX_INCLUDE_IN6_H_
#define COMPONENTS_LINUX_INCLUDE_IN6_H_

#define IN6_IS_ADDR_UNSPECIFIED(a) \
	(((__const uint32_t *) (a))[0] == 0				      \
	 && ((__const uint32_t *) (a))[1] == 0				      \
	 && ((__const uint32_t *) (a))[2] == 0				      \
	 && ((__const uint32_t *) (a))[3] == 0)
#define IN6_IS_ADDR_LOOPBACK(a) \
	(((__const uint32_t *) (a))[0] == 0				      \
	 && ((__const uint32_t *) (a))[1] == 0				      \
	 && ((__const uint32_t *) (a))[2] == 0				      \
	 && ((__const uint32_t *) (a))[3] == htonl (1))
#define IN6_IS_ADDR_MULTICAST(a) (((__const uint8_t *) (a))[0] == 0xff)
#define IN6_IS_ADDR_LINKLOCAL(a) \
	((((__const uint32_t *) (a))[0] & htonl (0xffc00000))		      \
	 == htonl (0xfe800000))
#define IN6_IS_ADDR_SITELOCAL(a) \
	((((__const uint32_t *) (a))[0] & htonl (0xffc00000))		      \
	 == htonl (0xfec00000))
#define IN6_IS_ADDR_V4MAPPED(a) \
	((((__const uint32_t *) (a))[0] == 0)				      \
	 && (((__const uint32_t *) (a))[1] == 0)			      \
	 && (((__const uint32_t *) (a))[2] == htonl (0xffff)))
#define IN6_IS_ADDR_V4COMPAT(a) \
	((((__const uint32_t *) (a))[0] == 0)				      \
	 && (((__const uint32_t *) (a))[1] == 0)			      \
	 && (((__const uint32_t *) (a))[2] == 0)			      \
	 && (ntohl (((__const uint32_t *) (a))[3]) > 1))
#define IN6_ARE_ADDR_EQUAL(a,b) \
	((((__const uint32_t *) (a))[0] == ((__const uint32_t *) (b))[0])     \
	 && (((__const uint32_t *) (a))[1] == ((__const uint32_t *) (b))[1])  \
	 && (((__const uint32_t *) (a))[2] == ((__const uint32_t *) (b))[2])  \
	 && (((__const uint32_t *) (a))[3] == ((__const uint32_t *) (b))[3]))

#define IN6_IS_ADDR_MC_NODELOCAL(a) \
	(IN6_IS_ADDR_MULTICAST(a)					      \
	 && ((((__const uint8_t *) (a))[1] & 0xf) == 0x1))
#define IN6_IS_ADDR_MC_LINKLOCAL(a) \
	(IN6_IS_ADDR_MULTICAST(a)					      \
	 && ((((__const uint8_t *) (a))[1] & 0xf) == 0x2))
#define IN6_IS_ADDR_MC_SITELOCAL(a) \
	(IN6_IS_ADDR_MULTICAST(a)					      \
	 && ((((__const uint8_t *) (a))[1] & 0xf) == 0x5))
#define IN6_IS_ADDR_MC_ORGLOCAL(a) \
	(IN6_IS_ADDR_MULTICAST(a)					      \
	 && ((((__const uint8_t *) (a))[1] & 0xf) == 0x8))
#define IN6_IS_ADDR_MC_GLOBAL(a) \
	(IN6_IS_ADDR_MULTICAST(a)					      \
	 && ((((__const uint8_t *) (a))[1] & 0xf) == 0xe))

int res_init(void);

#endif /* COMPONENTS_LINUX_INCLUDE_IN6_H_ */
