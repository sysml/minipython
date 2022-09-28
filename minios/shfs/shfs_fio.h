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

#ifndef _SHFS_FIO_
#define _SHFS_FIO_

#include "shfs_defs.h"
#include "shfs.h"
#include "shfs_btable.h"
#include "shfs_cache.h"
#include "likely.h"

#define SHFS_HASH_INDICATOR_PREFIX '?' /* has to be the same as HTTPURL_ARGS_INDICATOR_PREFIX in http.c */

typedef struct shfs_bentry *SHFS_FD;

/**
 * Opens a file/object via hash string or name depending on
 * the first character of path:
 *
 * Hash: "?024a5bec"
 * Name: "index.html"
 */
SHFS_FD shfs_fio_open(const char *path);
/**
 * Opens a file/object via a hash digest
 */
SHFS_FD shfs_fio_openh(hash512_t h);
/**
 * Creates a file descriptor clone
 */
SHFS_FD shfs_fio_openf(SHFS_FD f);
/**
 * Closes a file descriptor
 */
void shfs_fio_close(SHFS_FD f);

void shfs_fio_name(SHFS_FD f, char *out, size_t outlen); /* null-termination is ensured */
void shfs_fio_hash(SHFS_FD f, hash512_t out);
#define shfs_fio_islink(f) \
	(SHFS_HENTRY_ISLINK((f)->hentry))
void shfs_fio_size(SHFS_FD f, uint64_t *out); /* returns 0 on links */

/**
 * Link object attributes
 * The following interfaces can only be used on link objects
 */
#define shfs_fio_link_type(f) \
	(SHFS_HENTRY_LINK_TYPE((f)->hentry))
#define shfs_fio_link_rport(f) \
	(SHFS_HENTRY_LINKATTR((f)->hentry).rport)
#define shfs_fio_link_rhost(f) \
	(&(SHFS_HENTRY_LINKATTR((f)->hentry).rhost))
void shfs_fio_link_rpath(SHFS_FD f, char *out, size_t outlen); /* null-termination is ensured */

/**
 * File object attributes
 * The following interfaces can only be used to non-link objects
 */
void shfs_fio_mime(SHFS_FD f, char *out, size_t outlen); /* null-termination is ensured */

/* file container size in chunks */
#define shfs_fio_size_chks(f) \
	(DIV_ROUND_UP(((f)->hentry->f_attr.offset + (f)->hentry->f_attr.len), shfs_vol.chunksize))

/* volume chunk address of file chunk address */
#define shfs_volchk_fchk(f, fchk) \
	((f)->hentry->f_attr.chunk + (fchk))

/* volume chunk address of file byte offset */
#define shfs_volchk_foff(f, foff) \
	(((f)->hentry->f_attr.offset + (foff)) / shfs_vol.chunksize + (f)->hentry->f_attr.chunk)
/* byte offset in volume chunk of file byte offset */
#define shfs_volchkoff_foff(f, foff) \
	(((f)->hentry->f_attr.offset + (foff)) % shfs_vol.chunksize)

/* Check macros to test if a address is within file bounds */
#define shfs_is_fchk_in_bound(f, fchk) \
	(shfs_fio_size_chks((f)) > (fchk))
#define shfs_is_foff_in_bound(f, foff) \
	((f)->hentry->f_attr.len > (foff))

/**
 * File cookies
 */
#define shfs_fio_get_cookie(f) \
	((f)->cookie)
static inline int shfs_fio_set_cookie(SHFS_FD f, void *cookie) {
  if (f->cookie)
    return -EBUSY;
  f->cookie = cookie;
  return 0;
}
#define shfs_fio_clear_cookie(f) \
  do { (f)->cookie = NULL; } while (0)

/*
 * Simple but synchronous file read
 * Note: Busy-waiting is used
 */
/* direct read */
int shfs_fio_read(SHFS_FD f, uint64_t offset, void *buf, uint64_t len);
int shfs_fio_read_nosched(SHFS_FD f, uint64_t offset, void *buf, uint64_t len);
/* read is using cache */
int shfs_fio_cache_read(SHFS_FD f, uint64_t offset, void *buf, uint64_t len);
int shfs_fio_cache_read_nosched(SHFS_FD f, uint64_t offset, void *buf, uint64_t len);

/*
 * Async file read
 */
static inline int shfs_fio_cache_aread(SHFS_FD f, chk_t offset, shfs_aiocb_t *cb, void *cb_cookie, void *cb_argp, struct shfs_cache_entry **cce_out, SHFS_AIO_TOKEN **t_out)
{
    register chk_t addr;

    if (unlikely(!(shfs_is_fchk_in_bound(f, offset))))
	return -EINVAL;
    addr = shfs_volchk_fchk(f, offset);
    return shfs_cache_aread(addr, cb, cb_cookie, cb_argp, cce_out, t_out);
}

#endif /* _SHFS_FIO_ */
