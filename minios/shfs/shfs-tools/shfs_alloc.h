/*
 * Simple hash filesystem (SHFS) tools
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

#ifndef _SHFS_ALLOC_
#define _SHFS_ALLOC_

#include "shfs_defs.h"

struct shfs_aentry {
	chk_t start;
	chk_t end;

	struct shfs_aentry *next;
	struct shfs_aentry *prev;
};

struct shfs_alist {
	chk_t end;
	unsigned int count;
	uint8_t allocator;

	struct shfs_aentry *head;
	struct shfs_aentry *tail;
};

struct shfs_alist *shfs_alloc_alist(chk_t area_size, uint8_t allocator);
void shfs_free_alist(struct shfs_alist *al);

int shfs_alist_register(struct shfs_alist *al, chk_t start, chk_t len);
int shfs_alist_unregister(struct shfs_alist *al, chk_t start, chk_t len);
chk_t shfs_alist_find_free(struct shfs_alist *al, chk_t len);


/******* DEBUG ********/
#include <stdio.h>

static inline print_alist(struct shfs_alist *al)
{
	struct shfs_aentry *e;
	unsigned int i = 0;

	for (e = al->head; e != NULL; e = e->next)
		printf("[entry%5u] %15"PRIchk" - %15"PRIchk" (len: %15"PRIchk")\n", i++, e->start, e->end, e->end - e->start);
}


#endif /* _SHFS_ALLOC_ */
