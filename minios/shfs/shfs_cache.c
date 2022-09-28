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

#include <mini-os/os.h>
#include <mini-os/types.h>
#include <mini-os/xmalloc.h>
#include <mini-os/lib.h>
#include <mini-os/kernel.h>
#include "shfs_cache.h"
#include "likely.h"

#if (defined SHFS_CACHE_DEBUG || defined SHFS_DEBUG)
#define ENABLE_DEBUG
#endif
#include "debug.h"

#define MIN_ALIGN 8

#ifdef __MINIOS__
#if defined HAVE_LIBC && !defined CONFIG_ARM
#define shfs_cache_free_mem() \
	({ struct mallinfo minfo = mallinfo(); \
	  ((((size_t) mm_free_pages()) << PAGE_SHIFT) + /* free pages in page allocator (used for heap increase) */ \
	   ((((size_t) mm_heap_pages()) << PAGE_SHIFT) - minfo.arena) + /* pages reserved for heap (but not allocated to it yet) */ \
	   (minfo.fordblks / minfo.ordblks)); }) /* minimum possible allocation on current heap size */
#else
#define shfs_cache_free_mem() \
	(((size_t) mm_free_pages()) << PAGE_SHIFT) /* free pages in page allocator */
#endif
#else /* __MINIOS__ */
#define shfs_cache_free_mem() \
	((size_t) 0)
#endif /* __MINIOS__ */

static void _cce_pobj_init(struct mempool_obj *pobj, void *unused)
{
    struct shfs_cache_entry *cce = pobj->private;

    cce->pobj = pobj;
    cce->refcount = 0;
    cce->buffer = pobj->data;
    cce->invalid = 1; /* buffer is not ready yet */

    cce->t = NULL;
    cce->aio_chain.first = NULL;
    cce->aio_chain.last = NULL;
}

static inline uint32_t log2(uint32_t v)
{
  uint32_t i = 0;

  while (v) {
	v >>= 1;
	i++;
  }
  return (i - 1);
}

static inline uint32_t shfs_htcollison_order(void)
{
    uint32_t htlen;

    /* quickly calculate an "optimal" collision table size (heuristical):
     * n per entry: nb_entries = max_nb_bffrs / n
     * Note: the resulting number will be shrinked down the
     * closest power-of-2 value */
#ifdef SHFS_CACHE_GROW
#ifdef SHFS_CACHE_GROW_THRESHOLD
    htlen   = (((mm_total_pages() << PAGE_SHIFT) - SHFS_CACHE_GROW_THRESHOLD) /
              shfs_vol.chunksize) / SHFS_CACHE_HTABLE_AVG_LIST_LENGTH_PER_ENTRY;
#else
    htlen   = ((mm_total_pages() << PAGE_SHIFT) / shfs_vol.chunksize) /
              SHFS_CACHE_HTABLE_AVG_LIST_LENGTH_PER_ENTRY;
#endif
#else /* SHFS_CACHE_GROW */
    htlen   = SHFS_CACHE_POOL_NB_BUFFERS /
              SHFS_CACHE_HTABLE_AVG_LIST_LENGTH_PER_ENTRY;
#endif /* SHFS_CACHE_GROW */
    return log2(htlen);
}

int shfs_alloc_cache(void)
{
    struct shfs_cache *cc;
    uint32_t htlen, i;
    size_t cc_size;
#ifdef SHFS_CACHE_POOL_MAXALLOC
    size_t pool_size;
#endif
    int ret;

    ASSERT(shfs_vol.chunkcache == NULL);

    htlen   = 1 << shfs_htcollison_order();

    cc_size = sizeof(*cc) + (htlen * sizeof(struct shfs_cache_htel));
    cc = _xmalloc(cc_size, MIN_ALIGN);
    if (!cc) {
	    ret = -ENOMEM;
	    goto err_out;
    }
#if defined SHFS_CACHE_GROW && !defined SHFS_CACHE_POOL_MAXALLOC
    if (SHFS_CACHE_POOL_NB_BUFFERS) {
#endif
#ifdef SHFS_CACHE_POOL_MAXALLOC
#if defined HAVE_LIBC && !defined CONFIG_ARM
      pool_size = (SHFS_CACHE_POOL_MAXALLOC_THRESHOLD >= shfs_cache_free_mem()) ? 0 : (shfs_cache_free_mem() - SHFS_CACHE_POOL_MAXALLOC_THRESHOLD);
#else
      pool_size = (1 << (log2(mm_free_pages() - (SHFS_CACHE_POOL_MAXALLOC_THRESHOLD >> PAGE_SHIFT)) - 1)) << PAGE_SHIFT; /* FIXME: -1 is a workaround!!!!,
															  * it seems that the page allocator on arm still returns 
															  * memory even if the allocation failed! -> crash on pool access */
#endif
      cc->pool = alloc_enhanced_mempool2(pool_size,
					 shfs_vol.chunksize,
					 shfs_vol.ioalign,
					 0,
					 0,
					 sizeof(struct shfs_cache_entry),
					 1,
					 NULL, NULL,
					 _cce_pobj_init, NULL,
					 NULL, NULL);
#else
    cc->pool = alloc_enhanced_mempool(SHFS_CACHE_POOL_NB_BUFFERS,
				      shfs_vol.chunksize,
				      shfs_vol.ioalign,
				      0,
				      0,
				      sizeof(struct shfs_cache_entry),
				      1,
				      NULL, NULL,
				      _cce_pobj_init, NULL,
				      NULL, NULL);
#endif /* SHFS_CACHE_POOL_MAXALLOC */
    if (!cc->pool) {
	    printd("Could not allocate cache pool\n");
	    ret = -ENOMEM;
	    goto err_free_cc;
    }
#if defined SHFS_CACHE_GROW && !defined SHFS_CACHE_POOL_MAXALLOC
    } else {
	    cc->pool = NULL;
    }
#endif
    dlist_init_head(cc->alist);
    for (i = 0; i < htlen; ++i)
	    dlist_init_head(cc->htable[i].clist);
    cc->htlen = htlen;
    cc->htmask = htlen - 1;
    cc->nb_entries = 0;
    cc->nb_ref_entries = 0;

    shfs_vol.chunkcache = cc;
    shfs_cache_stats_reset();
    return 0;

 err_free_cc:
    xfree(cc);
 err_out:
    return ret;
}

#define shfs_cache_htindex(addr) \
	(((uint32_t) (addr)) & (shfs_vol.chunkcache->htmask))

static inline struct shfs_cache_entry *shfs_cache_pick_cce(void) {
    struct mempool_obj *cce_obj;
#ifdef SHFS_CACHE_GROW
    struct shfs_cache_entry *cce;
    void *buf;

    if (shfs_vol.chunkcache->pool) {
#endif
    cce_obj = mempool_pick(shfs_vol.chunkcache->pool);
    if (cce_obj) {
	/* got a new buffer */
	++shfs_vol.chunkcache->nb_entries;
	return (struct shfs_cache_entry *) cce_obj->private;
    }
#ifdef SHFS_CACHE_GROW
    }

#if (defined SHFS_CACHE_GROW) && (defined SHFS_CACHE_GROW_THRESHOLD)
    if (shfs_cache_free_mem() < SHFS_CACHE_GROW_THRESHOLD)
	return NULL;
#endif
    /* try to malloc a buffer from heap */
    buf = _xmalloc(shfs_vol.chunksize, shfs_vol.ioalign);
    if (!buf) {
	return NULL;
    }
    cce = _xmalloc(sizeof(*cce), MIN_ALIGN);
    if (!cce) {
        xfree(buf);
	return NULL;
    }
    cce->pobj = NULL;
    cce->refcount = 0;
    cce->buffer = buf;
    cce->invalid = 1; /* buffer is not ready yet */
    cce->t = NULL;
    cce->aio_chain.first = NULL;
    cce->aio_chain.last = NULL;
    ++shfs_vol.chunkcache->nb_entries;
    return cce;
#else
    return NULL;
#endif
}

#ifdef SHFS_CACHE_GROW
static inline void shfs_cache_put_cce(struct shfs_cache_entry *cce) {
	if (!cce->pobj) {
		xfree(cce->buffer);
		xfree(cce);
	} else {
		mempool_put(cce->pobj);
	}
	--shfs_vol.chunkcache->nb_entries;
}
#else
#define shfs_cache_put_cce(cce) \
	do { \
		mempool_put((cce)->pobj); \
		--shfs_vol.chunkcache->nb_entries; \
	} while(0)
#endif

static inline struct shfs_cache_entry *shfs_cache_find(chk_t addr)
{
    struct shfs_cache_entry *cce;
    register uint32_t i;

    i = shfs_cache_htindex(addr);
    dlist_foreach(cce, shfs_vol.chunkcache->htable[i].clist, clist) {
        if (cce->addr == addr)
            return cce;
    }
    return NULL; /* not found */
}

/* removes a cache entry from the cache
 * Note: never call this function on custom buffers that do not appear in any lists */
static inline void shfs_cache_unlink(struct shfs_cache_entry *cce)
{
#ifndef SHFS_CACHE_DISABLE
    register uint32_t i;
#endif /* SHFS_CACHE_DISABLE */

    ASSERT(cce->refcount == 0);

#ifndef SHFS_CACHE_DISABLE
    /* unlink element from hash table collision list */
    i = shfs_cache_htindex(cce->addr);
    dlist_unlink(cce, shfs_vol.chunkcache->htable[i].clist, clist);
#endif /* SHFS_CACHE_DISABLE */

    /* unlink element from available list */
    dlist_unlink(cce, shfs_vol.chunkcache->alist, alist);
}

/* put unreferenced buffers back to the pool */
static inline void shfs_cache_flush_alist(void)
{
    struct shfs_cache_entry *cce;

    printd("Flushing cache...\n");
    while ((cce = dlist_first_el(shfs_vol.chunkcache->alist, struct shfs_cache_entry)) != NULL) {
	    if (cce->t) {
		    printd("I/O of chunk buffer %llu is not done yet, "
		            "waiting for completion...\n", cce->addr);
		    /* set refcount to 1 in order to avoid freeing of an invalid
		     * buffer by aiocb */
		    cce->refcount = 1;

		     /* wait for I/O without having thread switching
		      * because otherwise, the state of alist might change */
		    while (cce->t)
			    shfs_poll_blkdevs(); /* requires shfs_mounted = 1 */

		    cce->refcount = 0; /* retore refcount */
	    }

	    printd("Releasing chunk buffer %llu...\n", cce->addr);
	    shfs_cache_unlink(cce); /* unlinks element from alist and clist */
	    shfs_cache_put_cce(cce);
    }
}

void shfs_flush_cache(void)
{
    shfs_cache_flush_alist();
}

void shfs_free_cache(void)
{
    shfs_cache_flush_alist();
    free_mempool(shfs_vol.chunkcache->pool); /* will fail with an assertion
                                              * if objects were not put back to the pool already */
    xfree(shfs_vol.chunkcache);
    shfs_vol.chunkcache = NULL;
}

static void _cce_aiocb(SHFS_AIO_TOKEN *t, void *cookie, void *argp)
{
    struct shfs_cache_entry *cce = (struct shfs_cache_entry *) cookie;
    SHFS_AIO_TOKEN *t_cur, *t_next;
    int ret;

    BUG_ON(cce->refcount == 0 && cce->aio_chain.first);
    BUG_ON(t != cce->t);

    ret = shfs_aio_finalize(t);
    cce->t = NULL;
    cce->invalid = (ret < 0) ? 1 : 0;
    printd("Cache I/O at chunk %"PRIchk" returned: %d\n", cce->addr, ret);

    if (cce->invalid)
	shfs_cache_stat_inc(ioerr);
    else
	shfs_cache_stat_inc(iosuc);

    /* I/O failed and no references? (in case of read-ahead) */
    if (unlikely(cce->refcount == 0
#ifndef SHFS_CACHE_DISABLE
		 && cce->invalid)) {
        printd("Destroy failed cache I/O at chunk %"PRIchk"\n", cce->addr);
#else /* SHFS_CACHE_DISABLE */
		 )) {
        printd("Releasing unreferenced cached chunk %"PRIchk"\n", cce->addr);
#endif /* SHFS_CACHE_DISABLE */
	shfs_cache_unlink(cce);
	shfs_cache_put_cce(cce);
        return;
    }

    /* clear chain */
    t_cur = cce->aio_chain.first;
    cce->aio_chain.first = NULL;
    cce->aio_chain.last = NULL;

    /* call registered callbacks (AIO_TOKEN emulation) */
    while (t_cur) {
	printd("Notify child token (chunk %llu): %p\n", cce->addr, t_cur);
	t_next = t_cur->_next;
	t_cur->ret = ret;
	t_cur->infly = 0;
	if (t_cur->cb) {
	    /* Call child callback */
	    t_cur->cb(t_cur, t_cur->cb_cookie, t_cur->cb_argp);
	}
	t_cur = t_next;
    }
}

static inline struct shfs_cache_entry *shfs_cache_add(chk_t addr)
{
    struct shfs_cache_entry *cce;
    register uint32_t i;

    cce = shfs_cache_pick_cce();
    if (cce) {
	/* got a new buffer: append it to alist */
	dlist_append(cce, shfs_vol.chunkcache->alist, alist);
    } else {
#ifndef SHFS_CACHE_DISABLE
	/* try to pick a buffer (that has completed I/O) from the available list */
	dlist_foreach(cce, shfs_vol.chunkcache->alist, alist) {
		if (cce->t == NULL)
			goto found;
	}
	/* we are out of buffers */
	errno = EAGAIN;
	return NULL;

    found:
	shfs_cache_stat_inc(evict);
	/* unlink from hash table */
	i = shfs_cache_htindex(cce->addr);
	dlist_unlink(cce, shfs_vol.chunkcache->htable[i].clist, clist);
	/* move entry to the tail of alist */
	dlist_relink_tail(cce, shfs_vol.chunkcache->alist, alist);
#else /* SHFS_CACHE_DISABLE */
	errno = EAGAIN;
	return NULL;
#endif /* SHFS_CACHE_DISABLE */
    }

    cce->addr = addr;
    cce->t = shfs_aread_chunk(addr, 1, cce->buffer,
                              _cce_aiocb, cce, NULL);
    if (unlikely(!cce->t)) {
	    dlist_unlink(cce, shfs_vol.chunkcache->alist, alist);
	    shfs_cache_put_cce(cce);
	    printd("Could not initiate I/O request for chunk %"PRIchk": %d\n", addr, errno);
	    return NULL;
    }

#ifndef SHFS_CACHE_DISABLE
    /* link element to hash table */
    i = shfs_cache_htindex(addr);
    dlist_append(cce, shfs_vol.chunkcache->htable[i].clist, clist);
#endif /* SHFS_CACHE_DISABLE */

    return cce;
}

#if (SHFS_CACHE_READAHEAD > 0)
static inline void shfs_cache_readahead(chk_t addr)
{
	struct shfs_cache_entry *cce;
	register chk_t i;

	for (i = 1; i <= SHFS_CACHE_READAHEAD; ++i) {
		register chk_t addri = addr + i;

		if (unlikely((addri) >= shfs_vol.volsize))
			return; /* end of volume */
		cce = shfs_cache_find(addri);
		if (!cce) {
			cce = shfs_cache_add(addri);
			if (!cce) {
				printd("Read-ahead chunk %"PRIchk" (%u/%u): Failed: Out of buffers\n", (addri), i, SHFS_CACHE_READAHEAD);
				shfs_cache_stat_inc(memerr);
				return; /* out of buffers */
			} else {
				printd("Read-ahead chunk %"PRIchk" (%u/%u): Requested\n", (addri), i, SHFS_CACHE_READAHEAD);
				shfs_cache_stat_inc(rdahead);
			}
		} else {
			printd("Read-ahead chunk %"PRIchk" (%u/%u): Already in cache\n", (addri), i, SHFS_CACHE_READAHEAD);
			if (shfs_aio_is_done(cce->t))
				shfs_cache_stat_inc(hit);
			else
				shfs_cache_stat_inc(hitwait);
		}
	}
}
#endif

int shfs_cache_aread(chk_t addr, shfs_aiocb_t *cb, void *cb_cookie, void *cb_argp, struct shfs_cache_entry **cce_out, SHFS_AIO_TOKEN **t_out)
{
    struct shfs_cache_entry *cce;
    SHFS_AIO_TOKEN *t;
    int ret;

    ASSERT(cce_out != NULL);
    ASSERT(t_out != NULL);

    /* sanity checks */
    if (unlikely(!shfs_mounted)) {
        ret = -ENODEV;
        goto err_out;
    }
    if (unlikely(addr == 0 || addr > shfs_vol.volsize)) {
        ret = -EINVAL;
        goto err_out;
    }

    /* check if we cached already this request */
#ifndef SHFS_CACHE_DISABLE
    cce = shfs_cache_find(addr);
    if (!cce) {
        shfs_cache_stat_inc(miss);
#endif /* SHFS_CACHE_DISABLE */
        /* no -> initiate a new I/O request */
        printd("Try to add chunk %"PRIchk" to cache\n", addr);
	cce = shfs_cache_add(addr);
	if (!cce) {
	    ret = -errno;
	    goto err_out;
	}
#ifndef SHFS_CACHE_DISABLE
    }
#endif /* SHFS_CACHE_DISABLE */

    /* increase refcount */
    if (cce->refcount == 0) {
	dlist_unlink(cce, shfs_vol.chunkcache->alist, alist);
	++shfs_vol.chunkcache->nb_ref_entries;
    }
    ++cce->refcount;

#ifndef SHFS_CACHE_DISABLE
#if (SHFS_CACHE_READAHEAD > 0)
    /* try to read ahead next addresses */
    shfs_cache_readahead(addr);
#endif
#endif /* SHFS_CACHE_DISABLE */
    shfs_aio_submit();
#ifndef SHFS_CACHE_DISABLE

    /* I/O of element done already? */
    if (likely(shfs_aio_is_done(cce->t))) {
        printd("Chunk %"PRIchk" found in cache and it is ready\n", addr);
        *t_out = NULL;
        *cce_out = cce;
        shfs_cache_stat_inc(hit);
        return 0;
    }
#endif /* SHFS_CACHE_DISABLE */

    /* chain a new AIO token for caller (multiplexes async I/O) */
    printd("Chunk %"PRIchk" found in cache but it is not ready yet: Appending AIO token\n", addr);
    t = shfs_aio_pick_token();
    if (unlikely(!t)) {
	printd("Failed to append AIO token: Out of token\n");
	ret = -EAGAIN;
	goto err_dec_refcount;
    }
    t->cb = cb;
    t->cb_cookie = cb_cookie;
    t->cb_argp = cb_argp;
    t->infly = 1; /* mark token as "busy" */

    /* append token to cce's token list */
    if (cce->aio_chain.last) {
	    cce->aio_chain.last->_next = t;
	    t->_prev = cce->aio_chain.last;
    } else {
	    cce->aio_chain.first = t;
	    t->_prev = NULL;
    }
    t->_next = NULL;
    cce->aio_chain.last = t;

    *t_out = t;
    *cce_out = cce;
    shfs_cache_stat_inc(hitwait);
    return 1;

 err_dec_refcount:
#ifndef SHFS_CACHE_DISABLE
    --cce->refcount;
    if (cce->refcount == 0) {
	--shfs_vol.chunkcache->nb_ref_entries;
	dlist_append(cce, shfs_vol.chunkcache->alist, alist);
    }
#else /* SHFS_CACHE_DISABLE */
    shfs_cache_put_cce(cce);
#endif /* SHFS_CACHE_DISABLE */
 err_out:
    *t_out = NULL;
    *cce_out = NULL;
    shfs_cache_stat_inc(memerr);
    return ret;
}

int shfs_cache_eblank(struct shfs_cache_entry **cce_out)
{
    struct shfs_cache_entry *cce;
    int ret;

    ASSERT(cce_out != NULL);

    /* sanity checks */
    if (unlikely(!shfs_mounted)) {
        ret = -ENODEV;
        goto err_out;
    }

    cce = shfs_cache_pick_cce();
    if (!cce) {
	/* try to pick a buffer (that has completed I/O) from the available list */
	dlist_foreach(cce, shfs_vol.chunkcache->alist, alist) {
		if (cce->t == NULL)
			goto found;
	}
	/* we are out of buffers */
	ret = -EAGAIN;
	shfs_cache_stat_inc(memerr);
	goto err_out;

    found:
	shfs_cache_stat_inc(evict);

	/* unlink from hash collision table and available list */
	shfs_cache_unlink(cce);
    }

    /* set refcount */
    cce->refcount = 1;
    ++shfs_vol.chunkcache->nb_ref_entries;

    /* initialize fields */
    /* TODO: These fields let a blank cce buffer to be released (put_cce()),
     *       because they are not part of any collision lists.
     *       As optimization, such buffers could be prepended to the alist instead...,
     *       thus, such a released buffer would be prefered for new I/O requests */
    cce->t = NULL;
    cce->addr = 0;
    cce->invalid = 1;

    *cce_out = cce;
    shfs_cache_stat_inc(blank);
    return 0;

 err_out:
    *cce_out = NULL;
    return ret;
}

/*
 * Fast version to release a cache buffer,
 * however, be sure that the I/O is finalized on the buffer
 */
void shfs_cache_release(struct shfs_cache_entry *cce)
{
    printd("Release cache of chunk %llu (refcount=%u, caller=%p)\n", cce->addr, cce->refcount, get_caller());
    BUG_ON(cce->refcount == 0);
    BUG_ON(!shfs_aio_is_done(cce->t));

    --cce->refcount;
    if (cce->refcount == 0) {
	--shfs_vol.chunkcache->nb_ref_entries;
#ifndef SHFS_CACHE_DISABLE
	if (likely(!cce->invalid)) {
	    dlist_append(cce, shfs_vol.chunkcache->alist, alist);
	} else {
#endif /* SHFS_CACHE_DISABLE */
            printd("Destroy invalid cache of chunk %llu\n", cce->addr);
	    if (!cce->addr == 0) /* note: blank buffers are not linked to any lists */
	        shfs_cache_unlink(cce);
	    shfs_cache_put_cce(cce);
#ifndef SHFS_CACHE_DISABLE
	}
#endif /* SHFS_CACHE_DISABLE */
    }
}

/*
 * Release a cache buffer (like shfs_cache_release)
 * but also cancels an incomplete I/O request
 * The corresponding AIO token is released
 */
void shfs_cache_release_ioabort(struct shfs_cache_entry *cce, SHFS_AIO_TOKEN *t)
{
    printd("Release cache of chunk %llu (refcount=%u, caller=%p)\n", cce->addr, cce->refcount, get_caller());
    BUG_ON(cce->refcount == 0);
    BUG_ON(!shfs_aio_is_done(cce->t) && t == NULL);
    BUG_ON(shfs_aio_is_done(cce->t) && !shfs_aio_is_done(t));

    if (!shfs_aio_is_done(t)) {
	    /* unlink token from AIO notification chain */
	    printd(" \\_ Abort AIO token %p\n", t);
	    if (t->_prev)
		t->_prev->_next = t->_next;
	    else
		cce->aio_chain.first = t->_next;
	    if (t->_next)
		t->_next->_prev = t->_prev;
	    else
		cce->aio_chain.last = t->_prev;
    }
    if (t) {
	    /* release token */
	    shfs_aio_put_token(t);
    }

    /* decrease refcount */
    --cce->refcount;
    if (cce->refcount == 0) {
	--shfs_vol.chunkcache->nb_ref_entries;
	if (shfs_aio_is_done(cce->t)
#ifndef SHFS_CACHE_DISABLE
	    && cce->invalid) {
#else /* SHFS_CACHE_DISABLE */
	    ) {
#endif /* SHFS_CACHE_DISABLE */
	    printd("Destroy invalid cache of chunk %llu\n", cce->addr);
	    if (!cce->addr == 0) /* note: blank buffers are not linked to any lists */
	        shfs_cache_unlink(cce);
	    shfs_cache_put_cce(cce);
	} else {
	    dlist_append(cce, shfs_vol.chunkcache->alist, alist);
	}
    }
}

#ifdef SHFS_CACHE_INFO
int shcmd_shfs_cache_info(FILE *cio, int argc, char *argv[])
{
	struct shfs_cache_entry *cce;
	uint32_t i;
	uint32_t chunksize;
	uint64_t nb_entries;
	uint64_t nb_ref_entries;
	uint32_t htlen;
	uint64_t depth, max_depth;
	uint32_t nb_objs = 0;
	uint64_t pool_size = 0;

	if (!shfs_mounted) {
		fprintf(cio, "Filesystem is not mounted\n");
		return -1;
	}

	max_depth = 0;
#ifdef SHFS_CACHE_DEBUG
	printk("\nBuffer states:\n");
#endif
	for (i = 0; i < shfs_vol.chunkcache->htlen; ++i) {
#ifdef SHFS_CACHE_DEBUG
		printk(" ht[%3"PRIu32"]:\n", i);
#endif
		depth = 0;
		dlist_foreach(cce, shfs_vol.chunkcache->htable[i].clist, clist) {
#ifdef SHFS_CACHE_DEBUG
			printk(" %12"PRIchk" chk: %s, refcount: %3"PRIu32"\n",
			       cce->addr,
			       cce->invalid ? "INVALID" : "valid",
			       cce->refcount);
#endif
			++depth;
		}
		max_depth = depth > max_depth ? depth : max_depth;
	}

	chunksize      = shfs_vol.chunksize;
	nb_entries     = shfs_vol.chunkcache->nb_entries;
	nb_ref_entries = shfs_vol.chunkcache->nb_ref_entries;
	htlen          = shfs_vol.chunkcache->htlen;
	if (shfs_vol.chunkcache->pool) {
		nb_objs = mempool_nb_objs(shfs_vol.chunkcache->pool);
		pool_size = mempool_size(shfs_vol.chunkcache->pool);
	}

	fprintf(cio, " Number of buffers in cache:         %12"PRIu64" (total: %"PRIu64" KiB)\n",
	        nb_entries,
	        (nb_entries * chunksize) /1024);
	fprintf(cio, " Number of used buffers in cache:    %12"PRIu32"\n",
	        nb_ref_entries);
	fprintf(cio, " Hash table size:                    %12"PRIu32"\n",
	        htlen);
	fprintf(cio, " Current max list depth:             %12"PRIu32"\n",
	        max_depth);
#if SHFS_CACHE_READAHEAD
	fprintf(cio, " Buffer read-ahead:                  %12"PRIu32"\n",
	        SHFS_CACHE_READAHEAD);
#endif
#if SHFS_CACHE_POOL_NB_BUFFERS
	fprintf(cio, " Number pre-allocated buffers:       %12"PRIu32" (pool size: %7"PRIu64" KiB)\n",
	        nb_objs, pool_size / 1024);
#endif
#ifdef SHFS_CACHE_GROW
	fprintf(cio, " Dynamic buffer allocation:               enabled");
#ifdef SHFS_CACHE_GROW_THRESHOLD
	fprintf(cio, " (limited by %"PRIu64" B left free memory)\n", (uint64_t) SHFS_CACHE_GROW_THRESHOLD);
#else
	fprintf(cio, "\n");
#endif
#else
	fprintf(cio, " Dynamic buffer allocation:              disabled\n");
#endif

#if SHFS_CACHE_STATS
	fprintf(cio, " Access statistics:\n");
	fprintf(cio, "  Hits:                              %12"PRIu32"\n", shfs_cache_stat_get(hit));
	fprintf(cio, "  Hits+Wait for I/O:                 %12"PRIu32"\n", shfs_cache_stat_get(hitwait));
	fprintf(cio, "  Read-aheads:                       %12"PRIu32"\n", shfs_cache_stat_get(rdahead));
	fprintf(cio, "  Misses:                            %12"PRIu32"\n", shfs_cache_stat_get(miss));
	fprintf(cio, "  Blanks:                            %12"PRIu32"\n", shfs_cache_stat_get(blank));
	fprintf(cio, "  Evicts:                            %12"PRIu32"\n", shfs_cache_stat_get(evict));
	fprintf(cio, "  Out of memory:                     %12"PRIu32"\n", shfs_cache_stat_get(memerr));
	fprintf(cio, "  Successful I/O:                    %12"PRIu32"\n", shfs_cache_stat_get(iosuc));
	fprintf(cio, "  Failed I/O:                        %12"PRIu32"\n", shfs_cache_stat_get(ioerr));
#endif

#ifdef SHFS_CACHE_DEBUG
	fprintf(cio, " Buffer states dumped to system output\n");
#endif
	return 0;
}
#endif
