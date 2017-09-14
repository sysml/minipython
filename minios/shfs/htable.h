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
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef _HTABLE_H_
#define _HTABLE_H_

#include <stdint.h>
#include <errno.h>
#include "likely.h"
#include "hash.h"

/*
 * HASH TABLE ELEMENT: MEMORY LAYOUT
 *
 *           ++--------------------++
 *           || struct htable_el   ||
 *           ||                    ||
 *           || - -private area- - ||
 *           ++--------------------++
 *           |   USER PRIVATE AREA  |
 *           ++--------------------++
 */
struct htable_el {
	hash512_t *h;
	struct htable_el *prev;
	struct htable_el *next;
	void *private; /* ptr to user private data area (do not change) */
};

/*
 * HASH TABLE BUCKET: MEMORY LAYOUT
 *
 *           ++--------------------++
 *           || struct htable_bkt  ||
 *           ||                    ||
 *           || - -private area- - ||
 *    h[0] ->++--------------------++
 *           |         HASH         |
 *    h[1] ->+----------------------+
 *           |         HASH         |
 *    h[2] ->+----------------------+
 *           |         HASH         |
 *    h[3] ->+----------------------+
 *           |         ...          |
 *           ~                      ~
 *           |     // padding //    |
 *   el[0] ->+----------------------+
 *           |       ELEMENT        |
 *           |     // padding //    |
 *   el[1] ->+----------------------+
 *           |       ELEMENT        |
 *           |     // padding //    |
 *   el[2] ->+----------------------+
 *           |       ELEMENT        |
 *           |     // padding //    |
 *   el[3] ->+----------------------+
 *           |         ...          |
 *           v                      v
 *
 * Because of locality reasons during a bucket search, the hash values of the
 * elements are separated from the element data area
 */
struct htable_bkt {
	size_t el_size; /* size of an element */
	size_t el_private_len;
	void *el; /* element list reference */
	hash512_t h[0]; /* hash value list */
};

#define _htable_bkt_el(b, i) ((struct htable_el *) ((uint8_t *) (b)->el + ((b)->el_size * (i))))


/*
 * HASH TABLE: MEMORY LAYOUT
 *
 *           ++--------------------++
 *           || struct htable      ||   +-->++--------------------++
 *           ||                    ||   |   || HASH TABLE BUCKET  ||
 *           || - -private area- - ||   |   ||                    ||
 *    b[0] ->++--------------------++   |   ++--------------------++
 *           |      bucket ref    --|---+
 *    b[1] ->+----------------------+
 *           |      bucket ref    --|------>++--------------------++
 *    b[2] ->+----------------------+       || HASH TABLE BUCKET  ||
 *           |         ...          |       ||                    ||
 *           v                      v       ++--------------------++
 */
struct htable {
	uint32_t nb_bkts; /* number of buckets */
	uint32_t el_per_bkt; /* elements per bucket (bucket size) */
	uint8_t hlen; /* length of hash value */

	struct htable_el *head;
	struct htable_el *tail;

	struct htable_bkt *b[0];
};

/*
 * Retrieve bucket number from hash value
 */
static inline unsigned int _htable_bkt_no(const hash512_t h, uint8_t hlen, uint32_t nb_bkts)
{
	register uint16_t h16;
	register uint32_t h32;
	register uint32_t h64;

	switch (hlen) {
	case 0:
		return 0;
	case 1:
		return (h[0] % nb_bkts); /* 1 byte */
	case 2:
		h16 = *((uint16_t *) &h[0]);
		return (h16 % nb_bkts); /* 2 bytes */
	case 3:
		h32 = *((uint32_t *) &h[0]);
		h32 &= 0x00FFFFFF;
		return (h32 % nb_bkts); /* 3 bytes */
	case 4:
		h32 = *((uint32_t *) &h[0]);
		return (h32 % nb_bkts); /* 4 bytes */
	case 5:
		h64 = *((uint64_t *) &h[0]);
		h64 &= 0x000000FFFFFFFFFF;
		return (h64 % nb_bkts); /* 5 bytes */
	case 6:
		h64 = *((uint64_t *) &h[0]);
		h64 &= 0x0000FFFFFFFFFFFF;
		return (h64 % nb_bkts); /* 6 bytes */
	case 7:
		h64 = *((uint64_t *) &h[0]);
		h64 &= 0x00FFFFFFFFFFFFFF;
		return (h64 % nb_bkts); /* 7 bytes */
	default:
		break;
	}

	/* just take 8 bytes from hash */
	h64 = *((uint64_t *) &h[0]);
	return (h64 % nb_bkts); /* 8 bytes */
}

/*
 *
 */
struct htable *alloc_htable(uint32_t nb_bkts, uint32_t el_per_bkt, uint8_t hlen, size_t el_private_len, size_t align);
void free_htable(struct htable *ht);

/*
 * Picks an element by its total index
 *  Returns NULL if element does not exist
 */
static inline struct htable_el *htable_pick(struct htable *ht, uint64_t el_idx)
{
	register uint32_t bkt_idx;
	register uint32_t el_idx_bkt;
	struct htable_bkt *b;

	/* TODO: Check for overflows */
	bkt_idx = (uint32_t) (el_idx / (uint64_t) ht->el_per_bkt);
	el_idx_bkt = (uint32_t) (el_idx % (uint64_t) ht->el_per_bkt);

	if (unlikely(bkt_idx > ht->nb_bkts)) {
		errno = ERANGE;
		return NULL;
	}

	b = ht->b[bkt_idx];
	if (unlikely(hash_is_zero(b->h[el_idx_bkt], ht->hlen))) {
		errno = ENOENT;
		return NULL;
	}

	return _htable_bkt_el(b, el_idx_bkt);
}

/*
 * Does a lookup for an element by its hash value
 *  Returns NULL on failure
 */
static inline struct htable_el *htable_lookup(struct htable *ht, const hash512_t h)
{
	register uint32_t i;
	register uint32_t bkt_idx;
	struct htable_bkt *b;

	if (unlikely(hash_is_zero(h, ht->hlen))) {
		errno = EINVAL;
		goto err_out;
	}

	bkt_idx = _htable_bkt_no(h, ht->hlen, ht->nb_bkts);
	b = ht->b[bkt_idx];
	for (i = 0; i < ht->el_per_bkt; ++i) {
		if (hash_compare(b->h[i], h, ht->hlen) == 0)
			return _htable_bkt_el(b, i);
	}

	/* no entry found */
	errno = ENOENT;
 err_out:
	return NULL;
}

/*
 * Tries to add an element for a given hash value
 *  and returns it on success (NULL on failure (e.g., bucket is full))
 *
 * Note: User data area might not be initialized
 */
static inline struct htable_el *htable_add(struct htable *ht, const hash512_t h)
{
	register uint32_t i;
	register uint32_t bkt_idx;
	struct htable_bkt *b;
	struct htable_el *el;

	if (unlikely(hash_is_zero(h, ht->hlen))) {
		errno = EINVAL;
		goto err_out;
	}

	bkt_idx = _htable_bkt_no(h, ht->hlen, ht->nb_bkts);
	b = ht->b[bkt_idx];
	for (i = 0; i < ht->el_per_bkt; ++i) {
		/* TODO: Check for already existence (preserve unique entries) */
		if (hash_is_zero(b->h[i], ht->hlen)) {
			/* found */
			el = _htable_bkt_el(b, i);
			hash_copy(b->h[i], h, ht->hlen);

			/* update linked list of elements */
			if (!ht->head) {
				ht->head = el;
				el->prev = NULL;
			} else {
				ht->tail->next = el;
				el->prev = ht->tail;
			}
			el->next = NULL;
			ht->tail = el;

			return el;
		}
	}

	/* bucket is full, cannot store hash */
	errno = ENOBUFS;
 err_out:
	return NULL;
}

/*
 * Tries to lookup an element for a given hash value and adds a new
 * one if it does not exist.
 *  If it exists, the element is returned. Also, is_new is set to 0.
 *  If it does not exist, it creates a new entry and returns this one
 *  instead (is_new is set to 1).
 *  On failure, NULL is returned (e.g., bucket is full)
 *
 * Note: User data area might not be initialized
 */
static inline struct htable_el *htable_lookup_add(struct htable *ht, const hash512_t h, int *is_new)
{
	register int empty_slot_found = 0;
	register uint32_t i, e = 0;
	register uint32_t bkt_idx;
	struct htable_bkt *b;
	struct htable_el *el;

	if (unlikely(hash_is_zero(h, ht->hlen))) {
		errno = EINVAL;
		return NULL;
	}

	bkt_idx = _htable_bkt_no(h, ht->hlen, ht->nb_bkts);
	b = ht->b[bkt_idx];
	for (i = 0; i < ht->el_per_bkt; ++i) {
		if (hash_compare(b->h[i], h, ht->hlen) == 0) {
			if (is_new)
				*is_new = 0;
			return _htable_bkt_el(b, i);
		}
		if (!empty_slot_found) {
			if (hash_is_zero(b->h[i], ht->hlen)) {
				e = i;
				empty_slot_found = 1;
			}
		}
	}

	if (unlikely(!empty_slot_found)) {
		/* bucket is full */
		errno = ENOBUFS;
		return NULL;
	}

	/* insert new element */
	hash_copy(b->h[e], h, ht->hlen);
	el = _htable_bkt_el(b, e);
	if (!ht->head) {
		ht->head = el;
		el->prev = NULL;
	} else {
		ht->tail->next = el;
		el->prev = ht->tail;
	}
	el->next = NULL;
	ht->tail = el;

	if (is_new)
		*is_new = 1;
	return el;
}

/*
 * Removes an element of a given hash value
 *
 * Note: Ensure that el belongs to ht
 * Note: el has to be non-NULL
 */
static inline void htable_rm(struct htable *ht, struct htable_el *el)
{
	/* update linked list of elements */
	if (el->prev)
		el->prev->next = el->next;
	else
		ht->head = el->next;

	if (el->next)
		el->next->prev = el->prev;
	else
		ht->tail = el->prev;

	/* clear hash value */
	hash_clear(*el->h, ht->hlen);
}

/*
 * Returns the head of list of added elements
 *
 * Note: List might change while schedule() is called during iterating
 */
#define htable_lhead(ht) ((ht)->head)

#define foreach_htable_el(ht, el) for((el) = (ht)->head; (el) != NULL; (el) = (el)->next)

/*
 * Quickly erases all elements of the hash table
 */
static inline void htable_clear(struct htable *ht)
{
	struct htable_el *el;

	foreach_htable_el(ht, el)
		hash_clear(*el->h, ht->hlen);
	ht->head = NULL;
	ht->tail = NULL;
}

#endif /* _HTABLE_H_ */
