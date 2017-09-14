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

#ifndef _TOOLS_COMMON_
#define _TOOLS_COMMON_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>

#include "shfs_defs.h"

/*
 * Print helpers
 */
extern unsigned int verbosity;
extern int force;

#define eprintf(...)		fprintf(stderr, __VA_ARGS__)
#define fatal()			eprintf("%s\n", strerror(errno))
#define dief(...)		do { eprintf(__VA_ARGS__); exit(EXIT_FAILURE); } while(0)
#define die()			do { fatal(); exit(EXIT_FAILURE); } while(0)
#define dprintf(LEVEL, ...)	do { if (verbosity >= (LEVEL)) fprintf(stderr, __VA_ARGS__); } while(0)
#define printvar(VAR, FMT)	do { if (verbosity >= (D_MAX)) fprintf(stderr, #VAR ": "FMT"\n", (VAR)); } while(0)

#define D_L0		1
#define D_L1		2
#define D_MAX		D_L1


/*
 * Argument parsing helper
 */
static inline int parse_args_setval_str(char** out, const char* buf)
{
	if (*out)
		free(*out);
	*out = strdup(buf);
	if (!*out) {
		*out = NULL;
		return -ENOMEM;
	}

	return 0;
}

static inline int parse_args_setval_int(int* out, const char* buf)
{
	if (sscanf(optarg, "%d", out) != 1)
		return -EINVAL;
	return 0;
}

/*
 * Disk I/O
 */
struct disk {
	int fd;
	char *path;
	uint64_t size;
	uint32_t blksize;
	int discard;
};

struct disk *open_disk(const char *path, int mode);
void close_disk(struct disk *d);

struct vol_member {
	struct disk *d;
	uuid_t uuid;
};

struct storage {
	struct vol_member member[SHFS_MAX_NB_MEMBERS];
	uint8_t nb_members;
	uint32_t stripesize;
	uint8_t stripemode;
};

int sync_io_chunk(struct storage *s, chk_t start, chk_t len, int owrite, void *buffer);
#define sync_read_chunk(s, start, len, buffer)	  \
	sync_io_chunk((s), (start), (len), 0, (buffer))
#define sync_write_chunk(s, start, len, buffer)	  \
	sync_io_chunk((s), (start), (len), 1, (buffer))
int sync_erase_chunk(struct storage *s, chk_t start, chk_t len);


/*
 * Misc
 */
void print_shfs_hdr_summary(struct shfs_hdr_common *hdr_common,
                            struct shfs_hdr_config *hdr_config);
chk_t metadata_size(struct shfs_hdr_common *hdr_common,
                    struct shfs_hdr_config *hdr_config);
chk_t avail_space(struct shfs_hdr_common *hdr_common,
                  struct shfs_hdr_config *hdr_config);
static inline void hash_unparse(hash512_t h, uint8_t hlen, char *out)
{
	uint8_t i;

	for (i = 0; i < hlen; i++)
		snprintf(out + (2*i), 3, "%02x", h[i]);
}

static inline size_t strftimestamp_s(char *s, size_t slen, const char *fmt, uint64_t ts_sec)
{
	struct tm *tm;
	time_t *tsec = (time_t *) &ts_sec;
	tm = localtime(tsec);
	return strftime(s, slen, fmt, tm);
}

size_t strshfshost(char *s, size_t slen, struct shfs_host *h);

#endif /* _TOOLS_COMMON_ */
