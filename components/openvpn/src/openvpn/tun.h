/*
 *  OpenVPN -- An application to securely tunnel IP networks
 *             over a single TCP/UDP port, with support for SSL/TLS-based
 *             session authentication and key exchange,
 *             packet encryption, packet authentication, and
 *             packet compression.
 *
 *  Copyright (C) 2002-2017 OpenVPN Technologies, Inc. <sales@openvpn.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef TUN_H
#define TUN_H

#ifdef _WIN32
#include <winioctl.h>
#include <tap-windows.h>
#endif

#include "buffer.h"
#include "error.h"
#include "mtu.h"
#include "win32.h"
#include "event.h"
#include "proto.h"
#include "misc.h"

struct tuntap_options {
    int dummy; /* not used */
};

/*
 * Define a TUN/TAP dev.
 */

struct tuntap
{
#define TUNNEL_TYPE(tt) ((tt) ? ((tt)->type) : DEV_TYPE_UNDEF)
    int type; /* DEV_TYPE_x as defined in proto.h */

#define TUNNEL_TOPOLOGY(tt) ((tt) ? ((tt)->topology) : TOP_UNDEF)
    int topology; /* one of the TOP_x values */

    bool did_ifconfig_setup;
    bool did_ifconfig_ipv6_setup;
    bool did_ifconfig;

    bool persistent_if;         /* if existed before, keep on program end */

    struct tuntap_options options; /* options set on command line */

    char *actual_name; /* actual name of TUN/TAP dev, usually including unit number */

    /* number of TX buffers */
    int txqueuelen;

    /* ifconfig parameters */
    in_addr_t local;
    in_addr_t remote_netmask;
    in_addr_t broadcast;

    struct in6_addr local_ipv6;
    struct in6_addr remote_ipv6;
    int netbits_ipv6;

    int fd; /* file descriptor for TUN/TAP dev */

    /* used for printing status info only */
    unsigned int rwflags_debug;

    /* Some TUN/TAP drivers like to be ioctled for mtu
     * after open */
    int post_open_mtu;
};

static inline bool
tuntap_defined(const struct tuntap *tt)
{
    return tt && tt->fd >= 0;
}

/*
 * Function prototypes
 */

void open_tun(struct tuntap *tt);

void close_tun(struct tuntap *tt);

int write_tun(struct tuntap *tt, uint8_t *buf, int len);

int read_tun(struct tuntap *tt, uint8_t *buf, int len);

void tuncfg(const char *dev, const char *dev_type, const char *dev_node,
            int persist_mode, const char *username,
            const char *groupname, const struct tuntap_options *options);

const char *guess_tuntap_dev(const char *dev,
                             const char *dev_type,
                             const char *dev_node,
                             struct gc_arena *gc);

struct tuntap *init_tun(const char *dev,        /* --dev option */
                        const char *dev_type,   /* --dev-type option */
                        int topology,           /* one of the TOP_x values */
                        const char *ifconfig_local_parm,           /* --ifconfig parm 1 */
                        const char *ifconfig_remote_netmask_parm,  /* --ifconfig parm 2 */
                        const char *ifconfig_ipv6_local_parm,      /* --ifconfig parm 1 / IPv6 */
                        int ifconfig_ipv6_netbits_parm,            /* --ifconfig parm 1 / bits */
                        const char *ifconfig_ipv6_remote_parm,     /* --ifconfig parm 2 / IPv6 */
                        struct addrinfo *local_public,
                        struct addrinfo *remote_public,
                        const bool strict_warn,
                        struct env_set *es);

void init_tun_post(struct tuntap *tt,
                   const struct frame *frame,
                   const struct tuntap_options *options);

void do_ifconfig_setenv(const struct tuntap *tt,
                        struct env_set *es);

void do_ifconfig(struct tuntap *tt,
                 const char *actual,     /* actual device name */
                 int tun_mtu,
                 const struct env_set *es);

bool is_dev_type(const char *dev, const char *dev_type, const char *match_type);

int dev_type_enum(const char *dev, const char *dev_type);

const char *dev_type_string(const char *dev, const char *dev_type);

const char *ifconfig_options_string(const struct tuntap *tt, bool remote, bool disable, struct gc_arena *gc);

bool is_tun_p2p(const struct tuntap *tt);

void check_subnet_conflict(const in_addr_t ip,
                           const in_addr_t netmask,
                           const char *prefix);

void warn_on_use_of_common_subnets(void);

/*
 * Inline functions
 */

static inline void
tun_adjust_frame_parameters(struct frame *frame, int size)
{
    frame_add_to_extra_tun(frame, size);
}

/*
 * Should ifconfig be called before or after
 * tun dev open?
 */

#define IFCONFIG_BEFORE_TUN_OPEN 0
#define IFCONFIG_AFTER_TUN_OPEN  1

#define IFCONFIG_DEFAULT         IFCONFIG_AFTER_TUN_OPEN

static inline int
ifconfig_order(void)
{
    return IFCONFIG_DEFAULT;
}

#define ROUTE_BEFORE_TUN 0
#define ROUTE_AFTER_TUN 1
#define ROUTE_ORDER_DEFAULT ROUTE_AFTER_TUN

static inline int
route_order(void)
{
    return ROUTE_ORDER_DEFAULT;
}


static inline bool
tuntap_stop(int status)
{
    return false;
}

static inline bool
tuntap_abort(int status)
{
    return false;
}

static inline void
tun_standby_init(struct tuntap *tt)
{
}

static inline bool
tun_standby(struct tuntap *tt)
{
    return true;
}

/*
 * TUN/TAP I/O wait functions
 */

static inline event_t
tun_event_handle(const struct tuntap *tt)
{
    return tt->fd;
}

static inline unsigned int
tun_set(struct tuntap *tt,
        struct event_set *es,
        unsigned int rwflags,
        void *arg,
        unsigned int *persistent)
{
    if (tuntap_defined(tt))
    {
        /* if persistent is defined, call event_ctl only if rwflags has changed since last call */
        if (!persistent || *persistent != rwflags)
        {
            event_ctl(es, tun_event_handle(tt), rwflags, arg);
            if (persistent)
            {
                *persistent = rwflags;
            }
        }

        tt->rwflags_debug = rwflags;
    }
    return rwflags;
}

const char *tun_stat(const struct tuntap *tt, unsigned int rwflags, struct gc_arena *gc);

#endif /* TUN_H */
