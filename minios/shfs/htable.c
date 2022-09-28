/*
 * Simple hash table implementation for MiniOS and POSIX
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

#ifdef __MINIOS__
#include <mini-os/os.h>
#include <mini-os/types.h>
#include <mini-os/xmalloc.h>
#else
#include <stdlib.h>
#include <string.h>
#endif

#ifndef MIN_ALIGN
#define MIN_ALIGN 8
#endif

#include "htable.h"

#ifndef max
#define max(a, b) \
    ({ __typeof__ (a) __a = (a); \
       __typeof__ (b) __b = (b); \
       __a > __b ? __a : __b; })
#endif

static inline size_t align_up(size_t size, size_t align)
{
  return (size + align - 1) & ~(align - 1);
}

struct htable *alloc_htable(uint32_t nb_bkts, uint32_t el_per_bkt, uint8_t hlen, size_t el_private_len, size_t align)
{
	size_t ht_size;
	size_t bkt_hdr_size;
	size_t bkt_size;
	size_t el_hdr_size;
	size_t el_size;
	struct htable *ht;
	struct htable_bkt *bkt;
	struct htable_el *el;
	uint32_t i, j;

	align = max(MIN_ALIGN, align);

	el_hdr_size  = align_up(sizeof(struct htable_el), align);
	el_size      = el_hdr_size + align_up(el_private_len, align);
	bkt_hdr_size = align_up(sizeof(struct htable_bkt)
	               + (sizeof(hash512_t) * el_per_bkt), align); /* hash list */
	bkt_size     = bkt_hdr_size
		       + (el_size * el_per_bkt) /* element list */;
	ht_size      = sizeof(struct htable)
		       + sizeof(struct htable_bkt *) * nb_bkts;

#ifdef HTABLE_DEBUG
	printf("el_size  = %lu B\n", el_size);
	printf("bkt_size = %lu B\n", bkt_size);
	printf("ht_size  = %lu B\n", ht_size);
#endif
	/* allocate main htable struct */
#ifdef __MINIOS__
	ht = _xmalloc(ht_size, align);
#else
	ht = malloc(ht_size);
#endif
	if (!ht) {
		errno = ENOMEM;
		goto err_out;
	}
	memset(ht, 0, ht_size);

#ifdef HTABLE_DEBUG
	printf("htable (%lu B) @ %p\n", ht_size);
#endif
	ht->nb_bkts = nb_bkts;
	ht->el_per_bkt = el_per_bkt;
	ht->hlen = hlen;
	ht->head = NULL;
	ht->tail = NULL;

	/* allocate buckets */
	for (i = 0; i < nb_bkts; ++i) {
#ifdef __MINIOS__
		bkt = _xmalloc(bkt_size, align);
#else
		bkt = malloc(bkt_size);
#endif
		if (!bkt) {
			errno = ENOMEM;
			goto err_free_bkts;
		}
		memset(bkt, 0, bkt_size);

#ifdef HTABLE_DEBUG
		printf(" bucket %lu (%lu B) @ %p\n", i, bkt_size, bkt);
#endif
		ht->b[i] = bkt;
		bkt->el = (void *) (((uint8_t *) ht->b[i]) + bkt_hdr_size);
		bkt->el_size = el_size;
		bkt->el_private_len = el_private_len;

		for (j = 0; j < el_per_bkt; ++j) {
			el = _htable_bkt_el(bkt, j);
			el->h = &bkt->h[j];
			el->private = (void *) (((uint8_t *) el) + el_hdr_size);

#ifdef HTABLE_DEBUG
			//printf("  entry %3lu:%02lu (%p): h = @%p; private = @%p\n",
			//        i, j, el, el->h, el->private);
#endif
		}
	}

	return ht;

 err_free_bkts:
	for (i = 0; i < nb_bkts; ++i) {
		if (ht->b[i]) {
#ifdef __MINIOS__
			xfree(ht->b[i]);
#else
			free(ht->b[i]);
#endif
		}
	}
#ifdef __MINIOS__
	xfree(ht);
#else
	free(ht);
#endif
 err_out:
	return NULL;
}

void free_htable(struct htable *ht)
{
	uint32_t i;

	for (i = 0; i < ht->nb_bkts; ++i) {
		if (ht->b[i]) {
#ifdef __MINIOS__
			xfree(ht->b[i]);
#else
			free(ht->b[i]);
#endif
		}
	}
#ifdef __MINIOS__
	xfree(ht);
#else
	free(ht);
#endif
}
