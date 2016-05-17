/*
 *
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
