/*
 * Statistics extensions for SHFS
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
