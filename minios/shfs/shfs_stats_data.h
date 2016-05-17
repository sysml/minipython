/*
 * Statistics extension for SHFS
 */
#ifndef _SHFS_STATS_DATA_H_
#define _SHFS_STATS_DATA_H_

#include "htable.h"

#ifdef SHFS_STATS_HTTP
 #ifdef SHFS_STATS_HTTP_DPC
  #ifndef SHFS_STATS_HTTP_DPCR
   #define SHFS_STATS_HTTP_DPCR 3 /* enabler download progress counters
                                    per default with 0%, 50%, 100% */
  #else
   #if SHFS_STATS_HTTP_DPCR < 2
    #warn "DPCR value has to be >= 2, disabling download progress counters"
    #undef SHFS_STATS_HTTP_DPC /* disable download progress counters */
   #endif
  #endif
 #endif

 #ifdef SHFS_STATS_HTTP_DPC
  /* threshold calculation */
  #define SHFS_STATS_HTTP_DPC_THRESHOLD(r, x) \
          (((x) * (r)) / ((SHFS_STATS_HTTP_DPCR) - 1))
  #define SHFS_STATS_HTTP_DPC_THRESHOLD_PERCENTAGE(x) \
          SHFS_STATS_HTTP_DPC_THRESHOLD(100, (x))
 #endif
#endif

struct shfs_mstats {
	uint32_t i; /* invalid requests */
	uint32_t e; /* errors */
	struct htable *el_ht; /* hash table of elements that are not in cache */
};

struct shfs_el_stats {
	uint32_t laccess; /* last access timestamp */
	uint32_t h; /* element hit */
	uint32_t m; /* element miss */
#ifdef SHFS_STATS_HTTP
	uint32_t c; /* successfully completed file transfers (even partial) */

	/* download progress counters */
#ifdef SHFS_STATS_HTTP_DPC
	uint32_t p[SHFS_STATS_HTTP_DPCR];
#endif
#endif
};

int shfs_init_mstats(uint32_t nb_bkts, uint32_t ent_per_bkt, uint8_t hlen);
void shfs_free_mstats(void);

#endif
