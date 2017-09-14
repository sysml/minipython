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
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef _SHFS_H_
#define _SHFS_H_

//#include <target/sys.h>
#include <mini-os/os.h>
#include <mini-os/types.h>
#include <mini-os/xmalloc.h>
#include <mini-os/lib.h>
#include <mini-os/kernel.h>
#include <mini-os/semaphore.h>
typedef struct semaphore sem_t;
#include "blkdev.h"
#include <stdint.h>

#include "mempool.h"
#include "likely.h"

#include "shfs_defs.h"
#ifdef SHFS_STATS
#include "shfs_stats_data.h"
#endif

#if defined __MINIOS__ && !defined CONFIG_ARM && !defined DEBUG_BUILD
#include <rte_memcpy.h>
#define shfs_memcpy(dst, src, len) \
  rte_memcpy((dst), (src), (len))
#warning "rte_memcpy is used for SHFS"
#else
#define shfs_memcpy(dst, src, len) \
  memcpy((dst), (src), (len))
#endif

#define MAX_NB_TRY_BLKDEVS 64
#define NB_AIOTOKEN 750 /* should be at least MAX_REQUESTS */

struct shfs_cache;

struct vol_member {
	struct blkdev *bd;
	uuid_t uuid;
	sector_t sfactor;
};

struct vol_info {
	uuid_t uuid;
	char volname[17];
	uint64_t ts_creation;
	uint32_t chunksize;
	chk_t volsize;

	uint8_t nb_members;
	struct vol_member member[SHFS_MAX_NB_MEMBERS];
	uint32_t stripesize;
	uint8_t stripemode;
	uint32_t ioalign;
#if defined CONFIG_SELECT_POLL && defined CAN_POLL_BLKDEV
	int members_maxfd; /* biggest fd number of mounted members (required for select()) */
#endif

	struct htable *bt; /* SHFS bucket entry table */
	void **htable_chunk_cache;
	void *remount_chunk_buffer;
	chk_t htable_ref;
	chk_t htable_bak_ref;
	chk_t htable_len;
	uint32_t htable_nb_buckets;
	uint32_t htable_nb_entries;
	uint32_t htable_nb_entries_per_bucket;
	uint32_t htable_nb_entries_per_chunk;
	uint8_t hlen;

	struct shfs_bentry *def_bentry;

	struct mempool *aiotoken_pool; /* token for async I/O */
	struct shfs_cache *chunkcache; /* chunkcache */

#ifdef SHFS_STATS
	struct shfs_mstats mstats;
#endif
};

extern struct vol_info shfs_vol;
extern sem_t shfs_mount_lock;
extern int shfs_mounted;
extern unsigned int shfs_nb_open;

int init_shfs(void);
int mount_shfs(blkdev_id_t bd_id[], unsigned int count);
int remount_shfs(void);
int umount_shfs(int force);
void exit_shfs(void);

#define shfs_blkdevs_count() \
	((shfs_mounted) ? shfs_vol.nb_members : 0)

static inline void shfs_poll_blkdevs(void) {
	register unsigned int i;
	register uint8_t m = shfs_blkdevs_count();

	for(i = 0; i < m; ++i)
		blkdev_poll_req(shfs_vol.member[i].bd);
}

#ifdef CAN_POLL_BLKDEV
#include <sys/select.h>

static inline void shfs_blkdevs_fds(int *fds) {
	register unsigned int i;
	register uint8_t m = shfs_blkdevs_count();

	for(i = 0; i < m; ++i)
		fds[i] = blkdev_get_fd(shfs_vol.member[i].bd);
}

static inline void shfs_blkdevs_fdset(fd_set *fdset) {
	register unsigned int i;
	register uint8_t m = shfs_blkdevs_count();

	for(i = 0; i < m; ++i)
		FD_SET(blkdev_get_fd(shfs_vol.member[i].bd), fdset);
}
#endif /* CAN_POLL_BLKDEV */

/**
 * Fast I/O: asynchronous I/O for volume chunks
 * A request is done via shfs_aio_chunk(). This function returns immediately
 * after the I/O request was set up.
 * Afterwards, the caller has to wait for the I/O completion via
 * tests on shfs_aio_is_done() or by calling shfs_aio_wait() or using a
 * function callback registration on shfs_aio_chunk().
 * The result (return code) of the I/O operation is retrieved via
 * shfs_aio_finalize() (can be called within the user's callback).
 */
struct _shfs_aio_token;
typedef struct _shfs_aio_token SHFS_AIO_TOKEN;
typedef void (shfs_aiocb_t)(SHFS_AIO_TOKEN *t, void *cookie, void *argp);
struct _shfs_aio_token {
	/** this struct has only private data **/
	struct mempool_obj *p_obj;
	uint64_t infly;
	int ret;

	shfs_aiocb_t *cb;
	void *cb_cookie;
	void *cb_argp;

	struct _shfs_aio_token *_prev; /* token chains (used by shfs_cache) */
	struct _shfs_aio_token *_next;
};

/*
 * Setups a asynchronous I/O operation and returns a token
 * NULL is returned if the async I/O operation could not be set up
 * The callback registration is optional and can be seen as an alternative way
 * to wait for the I/O completation compared to using shfs_aio_is_done()
 * or shfs_aio_wait()
 * cb_cookie and cb_argp are user definable values that get passed
 * to the user defined callback.
 */
SHFS_AIO_TOKEN *shfs_aio_chunk(chk_t start, chk_t len, int write, void *buffer,
                               shfs_aiocb_t *cb, void *cb_cookie, void *cb_argp);
#define shfs_aread_chunk(start, len, buffer, cb, cb_cookie, cb_argp)	  \
	shfs_aio_chunk((start), (len), 0, (buffer), (cb), (cb_cookie), (cb_argp))
#define shfs_awrite_chunk(start, len, buffer, cb, cb_cookie, cb_argp) \
	shfs_aio_chunk((start), (len), 1, (buffer), (cb), (cb_cookie), (cb_argp))

static inline void shfs_aio_submit(void) {
	register unsigned int i;
	register uint8_t m = shfs_blkdevs_count();

	for(i = 0; i < m; ++i)
		blkdev_async_io_submit(shfs_vol.member[i].bd);
}

static inline void shfs_aio_wait_slot(void) {
	register unsigned int i;
	register uint8_t m = shfs_blkdevs_count();

	for(i = 0; i < m; ++i)
		blkdev_async_io_wait_slot(shfs_vol.member[i].bd);
}

/*
 * Internal AIO token management (do not use this functions directly!)
 */
static inline SHFS_AIO_TOKEN *shfs_aio_pick_token(void)
{
	struct mempool_obj *t_obj;
	t_obj = mempool_pick(shfs_vol.aiotoken_pool);
	if (!t_obj)
		return NULL;
	return (SHFS_AIO_TOKEN *) t_obj->data;
}
#define shfs_aio_put_token(t) \
	mempool_put(t->p_obj)

/*
 * Returns 1 if the I/O operation has finished, 0 otherwise
 */
#define shfs_aio_is_done(t)	  \
	(!(t) || (t)->infly == 0)

/*
 * Busy-waiting until the async I/O operation is completed
 *
 * Note: This function will end up in a deadlock when there is no
 * SHFS volume mounted
 */
#define shfs_aio_wait(t) \
	while (!shfs_aio_is_done((t))) { \
		shfs_poll_blkdevs(); \
		if (!shfs_aio_is_done((t)))	\
			schedule();	     \
	}

#define shfs_aio_wait_nosched(t) \
	while (!shfs_aio_is_done((t))) { \
		shfs_poll_blkdevs(); \
	}

/*
 * Destroys an asynchronous I/O token after the I/O completed
 * This function returns the return code of the IO operation
 *
 * Note: This function has and can only be called after an I/O is done!
 */
static inline int shfs_aio_finalize(SHFS_AIO_TOKEN *t)
{
	int ret;

	BUG_ON(t->infly != 0);
	ret = t->ret;
	shfs_aio_put_token(t);

	return ret;
}

/**
 * Slow I/O: sequential sync I/O for volume chunks
 * These functions are intended to be used during mount/umount time
 */
static inline int shfs_io_chunk(chk_t start, chk_t len, int write, void *buffer) {
	SHFS_AIO_TOKEN *t;

 retry:
	t = shfs_aio_chunk(start, len, write, buffer, NULL, NULL, NULL);
	shfs_aio_submit();
	if (unlikely(!t && errno == EBUSY)) {
		shfs_aio_wait_slot(); /* yield CPU */
		goto retry;
	}
	if (unlikely(!t))
		return -errno;

	shfs_aio_wait(t);
	return shfs_aio_finalize(t);
}
#define shfs_read_chunk(start, len, buffer) \
	shfs_io_chunk((start), (len), 0, (buffer))
#define shfs_write_chunk(start, len, buffer) \
	shfs_io_chunk((start), (len), 1, (buffer))

static inline int shfs_io_chunk_nosched(chk_t start, chk_t len, int write, void *buffer) {
	SHFS_AIO_TOKEN *t;

 retry:
	t = shfs_aio_chunk(start, len, write, buffer, NULL, NULL, NULL);
	shfs_aio_submit();
	if (unlikely(!t && errno == EBUSY))
		goto retry;
	if (unlikely(!t))
		return -errno;

	shfs_aio_wait_nosched(t);
	return shfs_aio_finalize(t);
}
#define shfs_read_chunk_nosched(start, len, buffer) \
	shfs_io_chunk_nosched((start), (len), 0, (buffer))
#define shfs_write_chunk_nosched(start, len, buffer) \
	shfs_io_chunk_nosched((start), (len), 1, (buffer))

#endif /* _SHFS_H_ */
