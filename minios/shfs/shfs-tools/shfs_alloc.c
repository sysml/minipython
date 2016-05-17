/*
 *
 */
#include <stdlib.h>
#include <errno.h>

#include "shfs_alloc.h"

struct shfs_alist *shfs_alloc_alist(chk_t area_size, uint8_t allocator)
{
	struct shfs_alist *alist;

	if (allocator != SALLOC_FIRSTFIT &&
	    allocator != SALLOC_BESTFIT) {
		errno = ENOTSUP;
		return NULL;
	}

	alist = malloc(sizeof(*alist));
	if (!alist)
		return NULL;

	alist->head = NULL;
	alist->tail = NULL;
	alist->count = 0;
	alist->end = area_size;
	alist->allocator = allocator;
	return alist;
}

void shfs_free_alist(struct shfs_alist *al)
{
	struct shfs_aentry *cur;
	struct shfs_aentry *next;

	if (al) {
		cur = al->head;
		while (cur) {
			next = cur->next;
			free(cur);
			cur = next;
		}

		free(al);
	}
}

int shfs_alist_register(struct shfs_alist *al, chk_t start, chk_t len)
{
	struct shfs_aentry *e;
	struct shfs_aentry *prev;
	struct shfs_aentry *next;
	struct shfs_aentry *new;

	new = malloc(sizeof(*new));
	if (!new)
		return -ENOMEM;
	new->start = start;
	new->end = (start + len);

	if (!al->head) {
		/* list is empty */
		al->head = new;
		al->tail = new;
		new->prev = NULL;
		new->next = NULL;
	} else {
		/* search for predecessor which has start <= new->start  */
		prev = NULL;
		next = NULL;
		for (e = al->head; e != NULL; e = e->next) {
			if (e->start <= start) {
				prev = e;
				continue;
			} else {
				next = e;
				break;
			}
		}

		new->prev = prev;
		new->next = next;
		if (prev)
			prev->next = new;
		else
			al->head = new;
		if (next)
			next->prev = new;
		else
			al->tail = new;
	}

	al->count++;
	return 0;
}

int shfs_alist_unregister(struct shfs_alist *al, chk_t start, chk_t len)
{
	struct shfs_aentry *e;
	chk_t end = (start + len);

	/* search for element in the list and remove it if found */
	for (e = al->head; e != NULL; e = e->next) {
		if (e->start == start &&
		    e->end == end) {
			if (e->prev)
				e->prev->next = e->next;
			else
				al->head = e->next;
			if (e->next)
				e->next->prev = e->prev;
			else
				al->tail = e->prev;
			free(e);
			al->count--;
			return 0;
		}
	}

	return -ENOENT;
}

static chk_t _shfs_alist_find_ff(struct shfs_alist *al, chk_t len)
{
	struct shfs_aentry *e;
	chk_t free_start, free_end;

	/* list all segments */
	for (e = al->head; e != NULL; e = e->next) {
		/* find actual start and end of a free space segment */
		free_start = e->end;
		while (e->next &&
		       e->next->start <= e->end) {
			if (e->next->end > free_start)
				free_start = e->next->end;
			e = e->next;
		}

		if (e->next)
			free_end = e->next->start;
		else {
			free_end = al->end;
			if (free_end == free_start)
				break; /* nothing else found -> cancel */
		}

		/* free_start and free_end points now to a free space segment */
		if (free_end - free_start >= len)
			return free_start;
	}

	return 0;
}

static chk_t _shfs_alist_find_bf(struct shfs_alist *al, chk_t len)
{
	struct shfs_aentry *e;
	chk_t free_start, free_end;

	/* list all segments */
	for (e = al->head; e != NULL; e = e->next) {
		/* find actual start and end of a free space segment */
		free_start = e->end;
		while (e->next &&
		       e->next->start <= e->end) {
			if (e->next->end > free_start)
				free_start = e->next->end;
			e = e->next;
		}

		if (e->next)
			free_end = e->next->start;
		else {
			free_end = al->end;
			if (free_end == free_start)
				break; /* nothing else found -> cancel */
		}

		/* free_start and free_end points now to a free space segment */
		printf("[FREE]  %15"PRIchk" - %15"PRIchk"\n", free_start, free_end);
	}

	return 0;
}

chk_t shfs_alist_find_free(struct shfs_alist *al, chk_t len)
{
	switch (al->allocator) {
	case SALLOC_FIRSTFIT:
		return _shfs_alist_find_ff(al, len);
	case SALLOC_BESTFIT:
		return _shfs_alist_find_bf(al, len);
	default:
		break;
	}
	return 0;
}
