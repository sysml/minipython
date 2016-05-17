/*
 * Simon's HashFS (SHFS) for Mini-OS
 *
 * Copyright(C) 2013-2014 NEC Laboratories Europe. All rights reserved.
 *                        Simon Kuenzer <simon.kuenzer@neclab.eu>
 */
#ifndef _SHFS_TOOLS_H_
#define _SHFS_TOOLS_H_

#include "shfs_defs.h"
#ifdef HAVE_CTLDIR
#include <target/ctldir.h>
#endif

/**
 * Registers shfs tools to micro shell + ctldir (if *cd is not NULL)
 */
#ifdef HAVE_CTLDIR
int register_shfs_tools(struct ctldir *cd);
#else
int register_shfs_tools(void);
#endif

/**
 * Prints an uuid/hash number to a buffer
 * Note: The target buffer for the UUID has to be at least 37 bytes long
 * Note: The target buffer for the hash has to be at least ((2 * hlen) + 1) bytes long
 */
#ifdef __MINIOS__
void uuid_unparse(const uuid_t uu, char *out);
#endif
void hash_unparse(const hash512_t h, uint8_t hlen, char *out);

size_t strftimestamp_s(char *s, size_t slen, const char *fmt, uint64_t ts_sec);

size_t strshfshost(char *s, size_t slen, struct shfs_host *h);

#ifdef HAVE_LWIP
#include <lwip/err.h>
#include <lwip/ip_addr.h>
#include <lwip/dns.h>

#if LWIP_DNS
static inline int shfshost2ipaddr(const struct shfs_host *h, ip_addr_t *out, dns_found_callback dns_cb, void *dns_cb_argp)
#else
static inline int shfshost2ipaddr(const struct shfs_host *h, ip_addr_t *out)
#endif
{
	err_t err;
	char hname[sizeof(h->name) + 1];

	switch(h->type) {
#if !(defined LWIP_IPV4) || LWIP_IPV4
	case SHFS_HOST_TYPE_IPV4:
	  IP4_ADDR(out, h->addr[0], h->addr[1], h->addr[2], h->addr[3]);
	  return 0;
#endif
#if LWIP_IPV6
	case SHFS_HOST_TYPE_IPV6:
	  IP6_ADDR(out, h->addr[0], h->addr[1], h->addr[2],  h->addr[3],
		        h->addr[4], h->addr[5], h->addr[6],  h->addr[7]);
	  return 0;
#endif
#if LWIP_DNS
	case SHFS_HOST_TYPE_NAME:
	  /* FIXME: remove this workaround for null-terminating hostname in SHFS field */
	  strncpy(hname, h->name, sizeof(h->name));
	  hname[sizeof(hname) - 1] = '\0';
	  err = dns_gethostbyname(h->name, out, dns_cb, dns_cb_argp);
	  if (err == ERR_OK)
	    return 0; /* hostname found in local table */
	  if (err == ERR_INPROGRESS)
	    return 1; /* query sent, callback will be called on answer */
	  return -EINVAL; /* general error */
#endif
	default:
	  return -ENOTSUP;
	}
}
#endif /* HAVE_LWIP */

#endif /* _SHFS_TOOLS_H_ */
