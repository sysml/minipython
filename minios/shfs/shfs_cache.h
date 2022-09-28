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

#ifndef _SHFS_CACHE_
#define _SHFS_CACHE_

#include "shfs_cache.h"
#include "shfs_defs.h"
#include "shfs.h"

#include "dlist.h"
#include "mempool.h"

#ifndef SHFS_CACHE_HTABLE_AVG_LIST_LENGTH_PER_ENTRY
#define SHFS_CACHE_HTABLE_AVG_LIST_LENGTH_PER_ENTRY 2 /* defines in the end roughly the average maximum number of comparisons
						       * per table entry (Note: the real lenght might be higher) */
#endif

#ifndef SHFS_CACHE_READAHEAD
#define SHFS_CACHE_READAHEAD 2 /* how many chunks shall be read ahead (0 = disabled) */
#endif

#ifndef SHFS_CACHE_POOL_NB_BUFFERS
#ifdef  __MINIOS__
#define SHFS_CACHE_POOL_NB_BUFFERS 64 /* defines minimum cache size,
                                       * if 0, CACHE_GROW has to be enabled */
#else
#define SHFS_CACHE_POOL_NB_BUFFERS 2048 /* defines minimum cache size,
					 * if 0, CACHE_GROW has to be enabled */
#endif
#endif

#ifdef SHFS_CACHE_POOL_MAXALLOC /* if enabled, cache allocates a pool by covering the left space completely */
#ifndef SHFS_CACHE_POOL_MAXALLOC_THRESHOLD
#define SHFS_CACHE_POOL_MAXALLOC_THRESHOLD (2 * 1024 * 1024) /* keep 2MB space left (note: don't put this value too small,
							      * any successor allocataion might fail when we run out of memory.
							      * Also note that on remount this pool is reallocated, which might fail
							      * when the free memory is too fragmented
							      * (this number shall compensate this as well)) */
#endif
#endif

/*#define SHFS_CACHE_GROW*/ /* uncomment this line to allow the cache to grow in size by
			     * allocating more buffers on demand (via malloc()). When
			     * SHFS_GROW_THRESHOLD is defined, left system memory 
			     * is checked before the allocation */

#if defined SHFS_CACHE_GROW && defined __MINIOS__ && defined HAVE_LIBC
#ifdef CONFIG_ARM
#define SHFS_CACHE_GROW_THRESHOLD (1 * 1024 * 1024) /* 1MB on ARM */
#else
#define SHFS_CACHE_GROW_THRESHOLD (256 * 1024) /* 256KB */
#endif
#endif /* __MINIOS__ &6 HAVE_LIBC */

struct shfs_cache_entry {
	struct mempool_obj *pobj;

	chk_t addr;
	uint32_t refcount;

	dlist_el(alist); /* when part of the avaliable list */
	dlist_el(clist); /* when part of a collision list */

	void *buffer;
	int invalid; /* I/O didn't succeed on this buffer
		      * or buffer is a blank buffer when addr == 0 */

	SHFS_AIO_TOKEN *t; /* private I/O token */
	struct {
		/* tokens for callers */
		SHFS_AIO_TOKEN *first;
		SHFS_AIO_TOKEN *last;
	} aio_chain;
};

struct shfs_cache_htel {
	struct dlist_head clist; /* collision list */
};

struct shfs_cache {
	struct mempool *pool;
	uint32_t htlen;
	uint32_t htmask;
	uint64_t nb_ref_entries;
	uint64_t nb_entries;

#ifdef SHFS_CACHE_STATS
	struct {
		uint32_t hit;
		uint32_t hitwait;
		uint32_t rdahead;
		uint32_t miss;
		uint32_t blank;
		uint32_t evict;
		uint32_t memerr;
		uint32_t iosuc;
		uint32_t ioerr;
	} stats;
#endif /* SHFS_CACHE_STATS */

	struct dlist_head alist; /* list of available (loaded) but unreferenced entries */
	struct shfs_cache_htel htable[]; /* hash table (all loaded entries (incl. referenced)) */
};

#ifdef SHFS_CACHE_STATS
#define shfs_cache_stat_inc(name) \
  do { \
      ++shfs_vol.chunkcache->stats.name; \
  } while (0)
#define shfs_cache_stat_get(name) \
  (shfs_vol.chunkcache->stats.name)
#define shfs_cache_stats_reset(name) \
  do { \
    memset(&(shfs_vol.chunkcache->stats), 0, sizeof(shfs_vol.chunkcache->stats)); \
  } while (0)
#else /* SHFS_CACHE_STATS */
#define shfs_cache_stat_inc(name) \
  do {} while (0)
#define shfs_cache_stat_get(name) \
  (0)
#define shfs_cache_stats_reset(name) \
  do {} while (0)
#endif /* SHFS_CACHE_STATS */

int shfs_alloc_cache(void);
void shfs_flush_cache(void); /* releases unreferenced buffers */
void shfs_free_cache(void);
#define shfs_cache_ref_count() \
	(shfs_vol.chunkcache->nb_ref_entries)

/*
 * Function to read one chunk from the SHFS volume through the cache
 *
 * There are two cases of success
 *  (1) the cache can serve a request directly
 *  (2) the cache initiated an AIO request
 *  Like the direct AIO interfaces, a callback function can be passed that gets
 *  called when the I/O operation has completed or the SHFS_AIO_TOKEN can be polled.
 *
 * If the cache could serve the request directly,
 *  0 is returned and *cce_out points to the corresponding cache entry that holds
 *    the chunk data on its buffer
 *
 * If an AIO operation was initiated
 *  1 is returned and *t_out points to the corresponding SHFS_AIO_TOKEN that can be checked.
 *    *cce_out points to a newly created cache entry that will hold the data after the
 *    I/O operation completed
 *
 * a negative value is returned when there was an error:
 *  -EINVAL: Invalid chunk address
 *  -EAGAIN: Cannot perform operation currently, all cache buffers in use and could
 *           not create a new one or volume cannot handle a new request currently
 *  -ENODEV: No SHFS volume mounted currently
 *
 * A cache buffer is reserved until it is released back to the cache. That's
 * why shfs_cache_release() needs to be called after the buffer is not required
 * anymore.
 *
 * Note: This cache implementation can only be used for read-only operation
 *       because buffers can be shared.
 */
int shfs_cache_aread(chk_t addr, shfs_aiocb_t *cb, void *cb_cookie, void *cb_argp, struct shfs_cache_entry **cce_out, SHFS_AIO_TOKEN **t_out);

/*
 * Function to retrieve a blank SHFS buffer from the cache for custom I/O
 * The returned buffer on *cce_out has no address (= 0) associated with it and does not initiate
 * an I/O request. It can be used for custom I/O and will never be shared with other callers.
 * Data size is equal to chunk size of the current and alignemnet is according ioalignment.
 *
 * a negative value is returned when there was an error:
 *  0:       cce_out points to the blank cache buffer
 *  -EAGAIN: No buffers free currently that could be used as blank buffer
 *  -ENODEV: No SHFS volume mounted currently
 */
int shfs_cache_eblank(struct shfs_cache_entry **cce_out);

/* Release a shfs cache buffer */
void shfs_cache_release(struct shfs_cache_entry *cce); /* Note: I/O needs to be done! */
void shfs_cache_release_ioabort(struct shfs_cache_entry *cce, SHFS_AIO_TOKEN *t); /* I/O can be still in progress */

/* synchronous I/O read using the cache */
static inline struct shfs_cache_entry *shfs_cache_read(chk_t addr)
{
	struct shfs_cache_entry *cce;
	SHFS_AIO_TOKEN *t;
	int ret;

	do {
		ret = shfs_cache_aread(addr, NULL, NULL, NULL, &cce, &t);
		if (ret == -EAGAIN) {
			schedule();
			shfs_poll_blkdevs();
		}
	} while (ret == -EAGAIN);
	if (ret < 0) {
		errno = -ret;
		return NULL;
	}
	if (ret == 1) {
		/* wait for completion */
		shfs_aio_wait(t);
		ret = shfs_aio_finalize(t);
		if (ret < 0) {
			/* I/O failed */
			shfs_cache_release(cce);
			errno = -ret;
			return NULL;
		}
	} else if (unlikely(cce->invalid)) {
		/* cache buffer is broken */
		shfs_cache_release(cce);
		errno = EIO;
		return NULL;
	}
	return cce;
}

/* synchronous read version that does not call schedule() */
static inline struct shfs_cache_entry *shfs_cache_read_nosched(chk_t addr)
{
	struct shfs_cache_entry *cce;
	SHFS_AIO_TOKEN *t;
	int ret;

	do {
		ret = shfs_cache_aread(addr, NULL, NULL, NULL, &cce, &t);
		if (ret == -EAGAIN)
			shfs_poll_blkdevs();
	} while (ret == -EAGAIN);
	if (ret < 0) {
		errno = -ret;
		return NULL;
	}
	if (ret == 1) {
		/* wait for completion */
		shfs_aio_wait_nosched(t);
		ret = shfs_aio_finalize(t);
		if (ret < 0) {
			/* I/O failed */
			shfs_cache_release(cce);
			errno = -ret;
			return NULL;
		}
	} else if (unlikely(cce->invalid)) {
		/* cache buffer is broken */
		shfs_cache_release(cce);
		errno = EIO;
		return NULL;
	}
	return cce;
}

#ifdef SHFS_CACHE_INFO
#include "shell.h"
int shcmd_shfs_cache_info(FILE *cio, int argc, char *argv[]);
#endif

#endif /* _SHFS_CACHE_ */
