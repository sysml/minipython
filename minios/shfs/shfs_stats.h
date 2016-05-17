/*
 * Statistics extension for SHFS
 */
#ifndef _SHFS_STATS_H_
#define _SHFS_STATS_H_

#include "shfs_stats_data.h"
#include "shfs_btable.h"
#include "shfs_fio.h"
#include "shfs.h"
#include "likely.h"
#ifdef HAVE_CTLDIR
#include <target/ctldir.h>
#endif

/*
 * Retrieve stats structure from SHFS btable entry
 */
#define shfs_stats_from_bentry(bentry) \
	(&((bentry)->hstats))

/*
 * Retrieves stats structure from an SHFS_FD
 * NOTE: No NULL check is made since it is assumed that
 * f is provided by the currently mounted shfs hash table
 * -> bentry does exist
 */
static inline struct shfs_el_stats *shfs_stats_from_fd(SHFS_FD f) {
	struct shfs_bentry *bentry = (struct shfs_bentry *) f;

	return shfs_stats_from_bentry(bentry);
}

/*
 * Retrieves stats element from miss stats table
 * NOTE: A new entry is created automatically, if it does not
 * exist yet
 */
static inline struct shfs_el_stats *shfs_stats_from_mstats(hash512_t h) {
	int is_new;
	struct htable_el *el;
	struct shfs_el_stats *el_stats;

	el = htable_lookup_add(shfs_vol.mstats.el_ht, h, &is_new);
	if (unlikely(!el))
		return NULL;

	el_stats = (struct shfs_el_stats *) el->private;
	if (is_new)
		memset(el_stats, 0, sizeof(*el_stats));
	return el_stats;
}

/*
 * Deletes an entry from mstat table
 */
static inline void shfs_stats_mstats_drop(hash512_t h) {
	struct htable_el *el;

	el = htable_lookup(shfs_vol.mstats.el_ht, h);
	if (!el)
		return;
	htable_rm(shfs_vol.mstats.el_ht, el);
}

/*
 * Resetting statistics
 */
static inline void shfs_reset_mstats(void) {
	htable_clear(shfs_vol.mstats.el_ht);
	shfs_vol.mstats.i = 0;
	shfs_vol.mstats.e = 0;
}

static inline void shfs_reset_hstats(void) {
	struct htable_el *el;
	struct shfs_el_stats *el_stats;

	foreach_htable_el(shfs_vol.bt, el) {
		el_stats = shfs_stats_from_bentry((struct shfs_bentry *) el->private);
		memset(el_stats, 0, sizeof(*el_stats));
	}
}

#define shfs_reset_stats() \
	do { \
		shfs_reset_hstats(); \
		shfs_reset_mstats(); \
	} while (0)

/*
 * Dumps statistics of element entries
 */
typedef int (*shfs_dump_el_stats_t)(void *argp, hash512_t h, int loaded, struct shfs_el_stats *stats);

int shfs_dump_mstats(shfs_dump_el_stats_t dump_el, void *dump_el_argp);
int shfs_dump_hstats(shfs_dump_el_stats_t dump_el, void *dump_el_argp);

static inline int shfs_dump_stats(shfs_dump_el_stats_t dump_el, void *dump_el_argp)
{
	int ret;
	ret = shfs_dump_hstats(dump_el, dump_el_argp);
	if (unlikely(ret < 0))
		return ret;
	ret = shfs_dump_mstats(dump_el, dump_el_argp);
	if (unlikely(ret < 0))
		return ret;
	return 0;
}

/*
 * Tools to display/export stats via uSh/ctldir
 */
int init_shfs_stats_export(blkdev_id_t bd_id);
#ifdef HAVE_CTLDIR
int register_shfs_stats_tools(struct ctldir *cd);
#else
int register_shfs_stats_tools(void);
#endif
void exit_shfs_stats_export(void);

#endif /* _SHFS_STATS_H_ */
