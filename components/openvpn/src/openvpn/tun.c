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

/*
 * Support routines for configuring and accessing TUN/TAP
 * virtual network adapters.
 *
 * This file is based on the TUN/TAP driver interface routines
 * from VTun by Maxim Krasnyansky <max_mk@yahoo.com>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#elif defined(_MSC_VER)
#include "config-msvc.h"
#endif

#include "syshead.h"

#include "tun.h"
#include "fdmisc.h"
#include "common.h"
#include "misc.h"
#include "socket.h"
#include "manage.h"
#include "route.h"
#include "win32.h"

#include "memdbg.h"

#ifdef _WIN32
#include "openvpn-msg.h"
#endif

#include <string.h>

#ifdef _WIN32
#endif /* ifdef _WIN32 */

static void clear_tuntap(struct tuntap *tuntap);

bool
is_dev_type(const char *dev, const char *dev_type, const char *match_type)
{
    ASSERT(match_type);
    if (!dev)
    {
        return false;
    }
    if (dev_type)
    {
        return !strcmp(dev_type, match_type);
    }
    else
    {
        return !strncmp(dev, match_type, strlen(match_type));
    }
}

int
dev_type_enum(const char *dev, const char *dev_type)
{
    if (is_dev_type(dev, dev_type, "tun"))
    {
        return DEV_TYPE_TUN;
    }
    else if (is_dev_type(dev, dev_type, "tap"))
    {
        return DEV_TYPE_TAP;
    }
    else if (is_dev_type(dev, dev_type, "null"))
    {
        return DEV_TYPE_NULL;
    }
    else
    {
        return DEV_TYPE_UNDEF;
    }
}

const char *
dev_type_string(const char *dev, const char *dev_type)
{
    switch (dev_type_enum(dev, dev_type))
    {
        case DEV_TYPE_TUN:
            return "tun";

        case DEV_TYPE_TAP:
            return "tap";

        case DEV_TYPE_NULL:
            return "null";

        default:
            return "[unknown-dev-type]";
    }
}


/* --ifconfig-nowarn disables some options sanity checking */
static const char ifconfig_warn_how_to_silence[] = "(silence this warning with --ifconfig-nowarn)";

/*
 * If !tun, make sure ifconfig_remote_netmask looks
 *  like a netmask.
 *
 * If tun, make sure ifconfig_remote_netmask looks
 *  like an IPv4 address.
 */
static void
ifconfig_sanity_check(bool tun, in_addr_t addr, int topology)
{
    struct gc_arena gc = gc_new();
    const bool looks_like_netmask = ((addr & 0xFF000000) == 0xFF000000);
    if (tun)
    {
        if (looks_like_netmask && (topology == TOP_NET30 || topology == TOP_P2P))
        {
            msg(M_WARN, "WARNING: Since you are using --dev tun with a point-to-point topology, the second argument to --ifconfig must be an IP address.  You are using something (%s) that looks more like a netmask. %s",
                print_in_addr_t(addr, 0, &gc),
                ifconfig_warn_how_to_silence);
        }
    }
    else /* tap */
    {
        if (!looks_like_netmask)
        {
            msg(M_WARN, "WARNING: Since you are using --dev tap, the second argument to --ifconfig must be a netmask, for example something like 255.255.255.0. %s",
                ifconfig_warn_how_to_silence);
        }
    }
    gc_free(&gc);
}

/*
 * For TAP-style devices, generate a broadcast address.
 */
static in_addr_t
generate_ifconfig_broadcast_addr(in_addr_t local,
                                 in_addr_t netmask)
{
    return local | ~netmask;
}

/*
 * Check that --local and --remote addresses do not
 * clash with ifconfig addresses or subnet.
 */
static void
check_addr_clash(const char *name,
                 int type,
                 in_addr_t public,
                 in_addr_t local,
                 in_addr_t remote_netmask)
{
    struct gc_arena gc = gc_new();
#if 0
    msg(M_INFO, "CHECK_ADDR_CLASH type=%d public=%s local=%s, remote_netmask=%s",
        type,
        print_in_addr_t(public, 0, &gc),
        print_in_addr_t(local, 0, &gc),
        print_in_addr_t(remote_netmask, 0, &gc));
#endif

    if (public)
    {
        if (type == DEV_TYPE_TUN)
        {
            const in_addr_t test_netmask = 0xFFFFFF00;
            const in_addr_t public_net = public & test_netmask;
            const in_addr_t local_net = local & test_netmask;
            const in_addr_t remote_net = remote_netmask & test_netmask;

            if (public == local || public == remote_netmask)
            {
                msg(M_WARN,
                    "WARNING: --%s address [%s] conflicts with --ifconfig address pair [%s, %s]. %s",
                    name,
                    print_in_addr_t(public, 0, &gc),
                    print_in_addr_t(local, 0, &gc),
                    print_in_addr_t(remote_netmask, 0, &gc),
                    ifconfig_warn_how_to_silence);
            }

            if (public_net == local_net || public_net == remote_net)
            {
                msg(M_WARN,
                    "WARNING: potential conflict between --%s address [%s] and --ifconfig address pair [%s, %s] -- this is a warning only that is triggered when local/remote addresses exist within the same /24 subnet as --ifconfig endpoints. %s",
                    name,
                    print_in_addr_t(public, 0, &gc),
                    print_in_addr_t(local, 0, &gc),
                    print_in_addr_t(remote_netmask, 0, &gc),
                    ifconfig_warn_how_to_silence);
            }
        }
        else if (type == DEV_TYPE_TAP)
        {
            const in_addr_t public_network = public & remote_netmask;
            const in_addr_t virtual_network = local & remote_netmask;
            if (public_network == virtual_network)
            {
                msg(M_WARN,
                    "WARNING: --%s address [%s] conflicts with --ifconfig subnet [%s, %s] -- local and remote addresses cannot be inside of the --ifconfig subnet. %s",
                    name,
                    print_in_addr_t(public, 0, &gc),
                    print_in_addr_t(local, 0, &gc),
                    print_in_addr_t(remote_netmask, 0, &gc),
                    ifconfig_warn_how_to_silence);
            }
        }
    }
    gc_free(&gc);
}

/*
 * Issue a warning if ip/netmask (on the virtual IP network) conflicts with
 * the settings on the local LAN.  This is designed to flag issues where
 * (for example) the OpenVPN server LAN is running on 192.168.1.x, but then
 * an OpenVPN client tries to connect from a public location that is also running
 * off of a router set to 192.168.1.x.
 */
void
check_subnet_conflict(const in_addr_t ip,
                      const in_addr_t netmask,
                      const char *prefix)
{
#if 0 /* too many false positives */
    struct gc_arena gc = gc_new();
    in_addr_t lan_gw = 0;
    in_addr_t lan_netmask = 0;

    if (get_default_gateway(&lan_gw, &lan_netmask) && lan_netmask)
    {
        const in_addr_t lan_network = lan_gw & lan_netmask;
        const in_addr_t network = ip & netmask;

        /* do the two subnets defined by network/netmask and lan_network/lan_netmask intersect? */
        if ((network & lan_netmask) == lan_network
            || (lan_network & netmask) == network)
        {
            msg(M_WARN, "WARNING: potential %s subnet conflict between local LAN [%s/%s] and remote VPN [%s/%s]",
                prefix,
                print_in_addr_t(lan_network, 0, &gc),
                print_in_addr_t(lan_netmask, 0, &gc),
                print_in_addr_t(network, 0, &gc),
                print_in_addr_t(netmask, 0, &gc));
        }
    }
    gc_free(&gc);
#endif /* if 0 */
}

void
warn_on_use_of_common_subnets(void)
{
    struct gc_arena gc = gc_new();
    struct route_gateway_info rgi;
    const int needed = (RGI_ADDR_DEFINED|RGI_NETMASK_DEFINED);

    get_default_gateway(&rgi);
    if ((rgi.flags & needed) == needed)
    {
        const in_addr_t lan_network = rgi.gateway.addr & rgi.gateway.netmask;
        if (lan_network == 0xC0A80000 || lan_network == 0xC0A80100)
        {
            msg(M_WARN, "NOTE: your local LAN uses the extremely common subnet address 192.168.0.x or 192.168.1.x.  Be aware that this might create routing conflicts if you connect to the VPN server from public locations such as internet cafes that use the same subnet.");
        }
    }
    gc_free(&gc);
}

/*
 * Return a string to be used for options compatibility check
 * between peers.
 */
const char *
ifconfig_options_string(const struct tuntap *tt, bool remote, bool disable, struct gc_arena *gc)
{
    struct buffer out = alloc_buf_gc(256, gc);
    if (tt->did_ifconfig_setup && !disable)
    {
        if (tt->type == DEV_TYPE_TAP || (tt->type == DEV_TYPE_TUN && tt->topology == TOP_SUBNET))
        {
            buf_printf(&out, "%s %s",
                       print_in_addr_t(tt->local & tt->remote_netmask, 0, gc),
                       print_in_addr_t(tt->remote_netmask, 0, gc));
        }
        else if (tt->type == DEV_TYPE_TUN)
        {
            const char *l, *r;
            if (remote)
            {
                r = print_in_addr_t(tt->local, 0, gc);
                l = print_in_addr_t(tt->remote_netmask, 0, gc);
            }
            else
            {
                l = print_in_addr_t(tt->local, 0, gc);
                r = print_in_addr_t(tt->remote_netmask, 0, gc);
            }
            buf_printf(&out, "%s %s", r, l);
        }
        else
        {
            buf_printf(&out, "[undef]");
        }
    }
    return BSTR(&out);
}

/*
 * Return a status string describing wait state.
 */
const char *
tun_stat(const struct tuntap *tt, unsigned int rwflags, struct gc_arena *gc)
{
    struct buffer out = alloc_buf_gc(64, gc);
    if (tt)
    {
        if (rwflags & EVENT_READ)
        {
            buf_printf(&out, "T%s",
                       (tt->rwflags_debug & EVENT_READ) ? "R" : "r");
#ifdef _WIN32
            buf_printf(&out, "%s",
                       overlapped_io_state_ascii(&tt->reads));
#endif
        }
        if (rwflags & EVENT_WRITE)
        {
            buf_printf(&out, "T%s",
                       (tt->rwflags_debug & EVENT_WRITE) ? "W" : "w");
#ifdef _WIN32
            buf_printf(&out, "%s",
                       overlapped_io_state_ascii(&tt->writes));
#endif
        }
    }
    else
    {
        buf_printf(&out, "T?");
    }
    return BSTR(&out);
}

/*
 * Return true for point-to-point topology, false for subnet topology
 */
bool
is_tun_p2p(const struct tuntap *tt)
{
    bool tun = false;

    if (tt->type == DEV_TYPE_TAP
          || (tt->type == DEV_TYPE_TUN && tt->topology == TOP_SUBNET)
          || tt->type == DEV_TYPE_NULL )
    {
        tun = false;
    }
    else if (tt->type == DEV_TYPE_TUN)
    {
        tun = true;
    }
    else
    {
        msg(M_FATAL, "Error: problem with tun vs. tap setting"); /* JYFIXME -- needs to be caught earlier, in init_tun? */

    }
    return tun;
}

/*
 * Set the ifconfig_* environment variables, both for IPv4 and IPv6
 */
void
do_ifconfig_setenv(const struct tuntap *tt, struct env_set *es)
{
    struct gc_arena gc = gc_new();
    const char *ifconfig_local = print_in_addr_t(tt->local, 0, &gc);
    const char *ifconfig_remote_netmask = print_in_addr_t(tt->remote_netmask, 0, &gc);

    /*
     * Set environmental variables with ifconfig parameters.
     */
    if (tt->did_ifconfig_setup)
    {
        bool tun = is_tun_p2p(tt);

        setenv_str(es, "ifconfig_local", ifconfig_local);
        if (tun)
        {
            setenv_str(es, "ifconfig_remote", ifconfig_remote_netmask);
        }
        else
        {
            const char *ifconfig_broadcast = print_in_addr_t(tt->broadcast, 0, &gc);
            setenv_str(es, "ifconfig_netmask", ifconfig_remote_netmask);
            setenv_str(es, "ifconfig_broadcast", ifconfig_broadcast);
        }
    }

    if (tt->did_ifconfig_ipv6_setup)
    {
        const char *ifconfig_ipv6_local = print_in6_addr(tt->local_ipv6, 0, &gc);
        const char *ifconfig_ipv6_remote = print_in6_addr(tt->remote_ipv6, 0, &gc);

        setenv_str(es, "ifconfig_ipv6_local", ifconfig_ipv6_local);
        setenv_int(es, "ifconfig_ipv6_netbits", tt->netbits_ipv6);
        setenv_str(es, "ifconfig_ipv6_remote", ifconfig_ipv6_remote);
    }

    gc_free(&gc);
}

/*
 * Init tun/tap object.
 *
 * Set up tuntap structure for ifconfig,
 * but don't execute yet.
 */
struct tuntap *
init_tun(const char *dev,        /* --dev option */
         const char *dev_type,   /* --dev-type option */
         int topology,           /* one of the TOP_x values */
         const char *ifconfig_local_parm,           /* --ifconfig parm 1 */
         const char *ifconfig_remote_netmask_parm,  /* --ifconfig parm 2 */
         const char *ifconfig_ipv6_local_parm,      /* --ifconfig parm 1 IPv6 */
         int ifconfig_ipv6_netbits_parm,
         const char *ifconfig_ipv6_remote_parm,     /* --ifconfig parm 2 IPv6 */
         struct addrinfo *local_public,
         struct addrinfo *remote_public,
         const bool strict_warn,
         struct env_set *es)
{
    struct gc_arena gc = gc_new();
    struct tuntap *tt;

    ALLOC_OBJ(tt, struct tuntap);
    clear_tuntap(tt);

    tt->type = DEV_TYPE_TUN;
    tt->topology = topology;

    if (ifconfig_local_parm && ifconfig_remote_netmask_parm)
    {
        bool tun = false;

        /*
         * We only handle TUN/TAP devices here, not --dev null devices.
         */
        tun = is_tun_p2p(tt);

        /*
         * Convert arguments to binary IPv4 addresses.
         */

        tt->local = getaddr(
            GETADDR_RESOLVE
            | GETADDR_HOST_ORDER
            | GETADDR_FATAL_ON_SIGNAL
            | GETADDR_FATAL,
            ifconfig_local_parm,
            0,
            NULL,
            NULL);

        tt->remote_netmask = getaddr(
            (tun ? GETADDR_RESOLVE : 0)
            | GETADDR_HOST_ORDER
            | GETADDR_FATAL_ON_SIGNAL
            | GETADDR_FATAL,
            ifconfig_remote_netmask_parm,
            0,
            NULL,
            NULL);

        /*
         * Look for common errors in --ifconfig parms
         */
        if (strict_warn)
        {
            struct addrinfo *curele;
            ifconfig_sanity_check(tt->type == DEV_TYPE_TUN, tt->remote_netmask, tt->topology);

            /*
             * If local_public or remote_public addresses are defined,
             * make sure they do not clash with our virtual subnet.
             */

            for (curele = local_public; curele; curele = curele->ai_next)
            {
                if (curele->ai_family == AF_INET)
                {
                    check_addr_clash("local",
                                     tt->type,
                                     ((struct sockaddr_in *)curele->ai_addr)->sin_addr.s_addr,
                                     tt->local,
                                     tt->remote_netmask);
                }
            }

            for (curele = remote_public; curele; curele = curele->ai_next)
            {
                if (curele->ai_family == AF_INET)
                {
                    check_addr_clash("remote",
                                     tt->type,
                                     ((struct sockaddr_in *)curele->ai_addr)->sin_addr.s_addr,
                                     tt->local,
                                     tt->remote_netmask);
                }
            }

            if (tt->type == DEV_TYPE_TAP || (tt->type == DEV_TYPE_TUN && tt->topology == TOP_SUBNET))
            {
                check_subnet_conflict(tt->local, tt->remote_netmask, "TUN/TAP adapter");
            }
            else if (tt->type == DEV_TYPE_TUN)
            {
                check_subnet_conflict(tt->local, IPV4_NETMASK_HOST, "TUN/TAP adapter");
            }
        }

        /*
         * If TAP-style interface, generate broadcast address.
         */
        if (!tun)
        {
            tt->broadcast = generate_ifconfig_broadcast_addr(tt->local, tt->remote_netmask);
        }

        tt->did_ifconfig_setup = true;
    }

    if (ifconfig_ipv6_local_parm && ifconfig_ipv6_remote_parm)
    {

        /*
         * Convert arguments to binary IPv6 addresses.
         */

        if (inet_pton( AF_INET6, ifconfig_ipv6_local_parm, &tt->local_ipv6 ) != 1
            || inet_pton( AF_INET6, ifconfig_ipv6_remote_parm, &tt->remote_ipv6 ) != 1)
        {
            msg( M_FATAL, "init_tun: problem converting IPv6 ifconfig addresses %s and %s to binary", ifconfig_ipv6_local_parm, ifconfig_ipv6_remote_parm );
        }
        tt->netbits_ipv6 = ifconfig_ipv6_netbits_parm;

        tt->did_ifconfig_ipv6_setup = true;
    }

    /*
     * Set environmental variables with ifconfig parameters.
     */
    if (es)
    {
        do_ifconfig_setenv(tt, es);
    }

    gc_free(&gc);
    return tt;
}

/*
 * Platform specific tun initializations
 */
void
init_tun_post(struct tuntap *tt,
              const struct frame *frame,
              const struct tuntap_options *options)
{
    tt->options = *options;
#ifdef _WIN32
    overlapped_io_init(&tt->reads, frame, FALSE, true);
    overlapped_io_init(&tt->writes, frame, TRUE, true);
    tt->rw_handle.read = tt->reads.overlapped.hEvent;
    tt->rw_handle.write = tt->writes.overlapped.hEvent;
    tt->adapter_index = TUN_ADAPTER_INDEX_INVALID;
#endif
}

/* execute the ifconfig command through the shell */
void
do_ifconfig(struct tuntap *tt,
            const char *actual,     /* actual device name */
            int tun_mtu,
            const struct env_set *es)
{
    if (tt->did_ifconfig_setup)
    {
        msg( M_DEBUG, "do_ifconfig, tt->did_ifconfig_ipv6_setup=%d",
             tt->did_ifconfig_ipv6_setup );

        // Set ip information to tun adapter
        tcpip_adapter_ip_info_t ip_info;

        // To use tcpip_adapter_get_ip_info, first whe have to stop dhcp
		tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_TUN);

		// Get previous ip info
		tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_TUN, &ip_info);

		// Update ip info
    	ip_info.ip.addr = ntohl(tt->local);
		ip_info.netmask.addr = ntohl(tt->remote_netmask);
		ip_info.gw.addr = ntohl(tt->broadcast);

		// Update ip information to tun adapter
		tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_TUN, &ip_info);
    }
}

static void
clear_tuntap(struct tuntap *tuntap)
{
    CLEAR(*tuntap);
    tuntap->fd = -1;
}

void
open_tun(struct tuntap *tt)
{
	vfs_tun_register();

    tt->fd = open("/dev/tun", O_RDWR);
    tt->actual_name = string_alloc("tu", NULL);
}

void
close_tun(struct tuntap *tt)
{
    if (tt)
    {
    	if (tt->fd >= 0) {
    		close(tt->fd);
    	}

    	free(tt);
    }
}

int
write_tun(struct tuntap *tt, uint8_t *buf, int len)
{
    return write(tt->fd, buf, len);
}

int
read_tun(struct tuntap *tt, uint8_t *buf, int len)
{
    return read(tt->fd, buf, len);
}
