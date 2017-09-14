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
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#ifndef _SHFS_ADMIN_
#define _SHFS_ADMIN_

#include "tools_common.h"
#include "shfs_defs.h"

#define STR_VERSION "Simple Hash FS (SHFS) Tools: Admin"

#define MAX_NB_TRY_BLKDEVS SHFS_MAX_NB_MEMBERS

enum action {
	NONE = 0,
	ADDOBJ,
	ADDLNK,
	RMOBJ,
	CATOBJ,
	SETDEFOBJ,
	CLEARDEFOBJ,
	LSOBJS,
	SHOWINFO
};

enum ltype {
	LREDIRECT = 0,
	LRAW,
	LAUTO
};

struct token {
	struct token *next;

	enum action action;
	char *path;
	char *optstr0;
	char *optstr1;
	char *optstr2;
	enum ltype optltype;
};

struct args {
	char **devpath;
	unsigned int nb_devs;

	struct token *tokens; /* list of tokens */
};

struct vol_info {
	uuid_t uuid;
	char volname[17];
	uint32_t chunksize;
	chk_t volsize;

	struct storage s;

	/* hash table */
	struct htable *bt;
	void **htable_chunk_cache;
	int *htable_chunk_cache_state;
	chk_t htable_ref;
	chk_t htable_bak_ref;
	chk_t htable_len;
	uint32_t htable_nb_buckets;
	uint32_t htable_nb_entries;
	uint32_t htable_nb_entries_per_bucket;
	uint32_t htable_nb_entries_per_chunk;
	uint8_t hfunc;
	uint8_t hlen;

	struct shfs_bentry *def_bentry;

	/* allocator */
	uint8_t allocator;
	struct shfs_alist *al;
};

/* chunk_cache_states */
#define CCS_MODIFIED 0x02

#endif /* _SHFS_ADMIN_ */
