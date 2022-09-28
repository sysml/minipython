/*
 * Simple HashFS (SHFS) for Mini-OS
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

#ifndef _SHFS_BTABLE_H_
#define _SHFS_BTABLE_H_

#ifdef __SHFS_TOOLS__
#include <semaphore.h>
#include <assert.h>
#define ASSERT assert
#else
//#include <target/sys.h>
#include <mini-os/os.h>
#include <mini-os/types.h>
#include <mini-os/xmalloc.h>
#include <mini-os/lib.h>
#include <mini-os/kernel.h>
#include <mini-os/semaphore.h>
typedef struct semaphore sem_t;
#endif

#include "shfs_defs.h"
#include "htable.h"

#ifdef SHFS_STATS
#include "shfs_stats_data.h"
#endif

#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 64
#endif

/*
 * Bucket entry that points to
 * the depending hentry (SHFS Hash Table Entry)
 */
struct shfs_bentry {
	chk_t hentry_htchunk;       /* relative chunk:offfset addres to entry in SHFS htable */
	off_t hentry_htoffset;

#ifndef __SHFS_TOOLS__
	struct shfs_hentry *hentry; /* reference to buffered entry in cache */
	uint32_t refcount;
	sem_t updatelock; /* lock is helt as long the file is opened */
	int update; /* is set when a entry update is ongoing */

#ifdef SHFS_STATS
	struct shfs_el_stats hstats;
#endif /* SHFS_STATS */

	void *cookie; /* shfs_fio: upper layer software can attach cookies to open files */
#endif
};

#define shfs_alloc_btable(nb_bkts, ent_per_bkt, hlen) \
	alloc_htable((nb_bkts), (ent_per_bkt), (hlen), sizeof(struct shfs_bentry), CACHELINE_SIZE);
#define shfs_free_btable(bt) \
	free_htable((bt))

/**
 * Does a lookup for a bucket entry by its hash value
 */
static inline struct shfs_bentry *shfs_btable_lookup(struct htable *bt, hash512_t h) {
	struct htable_el *el;

	el = htable_lookup(bt, h);
	if (el)
		return (struct shfs_bentry *) el->private;
	return NULL;
}

/**
 * Searches and allocates an according bucket entry for a given hash value
 */
static inline struct shfs_bentry *shfs_btable_addentry(struct htable *bt, hash512_t h) {
	struct htable_el *el;

	el = htable_add(bt, h);
	if (el)
		return (struct shfs_bentry *) el->private;
	return NULL;
}

#ifndef __MINIOS__
/**
 * Deletes an entry from table
 */
static void shfs_btable_rmentry(struct htable *bt, hash512_t h) {
	struct htable_el *el;

	el = htable_lookup(bt, h);
	if (el)
		htable_rm(bt, el);
}
#endif

/**
 * This function is intended to be used during (re-)mount time.
 * It is intended to load a hash table from a device:
 * It picks a bucket entry by its total index of the hash table,
 * replaces its hash value and (re-)links the element to the end of the table list.
 * The functions returns the according shfs_bentry so that this data structure
 * can be filled-in/updated with further meta data
 */
static inline struct shfs_bentry *shfs_btable_feed(struct htable *bt, uint64_t ent_idx, hash512_t h) {
	uint32_t bkt_idx;
	uint32_t el_idx_bkt;
	struct htable_bkt *b;
	struct htable_el *el;

	/* TODO: Check for overflows */
	bkt_idx = (uint32_t) (ent_idx / (uint64_t) bt->el_per_bkt);
	el_idx_bkt = (uint32_t) (ent_idx % (uint64_t) bt->el_per_bkt);
	ASSERT(bkt_idx < bt->nb_bkts);

	/* entry found */
	b = bt->b[bkt_idx];
	el = _htable_bkt_el(b, el_idx_bkt);

	/* check if a previous entry was there -> if yes, unlink it */
	if (!hash_is_zero(b->h[el_idx_bkt], bt->hlen)) {
		if (el->prev)
			el->prev->next = el->next;
		else
			bt->head = el->next;

		if (el->next)
			el->next->prev = el->prev;
		else
			bt->tail = el->prev;
	}

	/* replace hash value */
	hash_copy(b->h[el_idx_bkt], h, bt->hlen);

	/* link the new element to the list, (if it is not empty) */
	if (!hash_is_zero(h, bt->hlen)) {
		if (!bt->head) {
			bt->head = el;
			bt->tail = el;
			el->prev = NULL;
			el->next = NULL;
		} else {
			bt->tail->next = el;
			el->prev = bt->tail;
			el->next = NULL;
			bt->tail = el;
		}
	}

	return (struct shfs_bentry *) el->private;
}

#endif /* _SHFS_BTABLE_H_ */
