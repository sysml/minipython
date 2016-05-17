/*
 *
 */
#ifndef _SHFS_MKFS_
#define _SHFS_MKFS_

#include "tools_common.h"

#define STR_VERSION "Simple Hash FS (SHFS) Tools: MakeFS"

struct args {
	char **devpath;
	uint8_t nb_devs;

	uint8_t  encoding;
	char     volname[17]; /* null-terminated */
	uint32_t stripesize;

	int fullerase;
	int combined_striping;

	uint8_t  allocator;
	uint8_t  hashfunc;
	uint8_t  hashlen;
	uint32_t bucket_count;
	uint32_t entries_per_bucket;
};

#endif /* _SHFS_MKFS_ */
