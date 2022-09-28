/*
 * HashFS (SHFS) for Mini-OS
 *
 * Authors: Simon Kuenzer <simon.kuenzer@neclab.eu>
 *
 *
 * Copyright (c) 2013-2017, NEC Europe Ltd., NEC Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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
