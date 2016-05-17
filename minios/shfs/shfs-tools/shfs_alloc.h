/*
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
