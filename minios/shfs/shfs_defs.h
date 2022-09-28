/*
 * HashFS (SHFS) for Mini-OS
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

#ifndef _SHFS_DEFS_H_
#define _SHFS_DEFS_H_

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>
#include "hash.h"

#ifdef __SHFS_TOOLS__
#include <uuid/uuid.h>
#include <mhash.h>
#else
typedef uint8_t uuid_t[16];
#endif /* __SHFS_TOOLS__ */

typedef uint64_t chk_t;
typedef uint64_t strp_t;

#define PRIchk PRIu64
#define PRIstrp PRIu64

#define SHFS_MAX_NB_MEMBERS 16

/* vol_byteorder */
#define SBO_LITTLEENDIAN 0
#define SBO_BIGENDIAN    1

/* vol_encoding */
#define SENC_UNSPECIFIED 0

/* allocator */
#define SALLOC_FIRSTFIT  0
#define SALLOC_BESTFIT   1

/* hash function */
#define SHFUNC_MANUAL    1
#define SHFUNC_SHA       2
#define SHFUNC_CRC       3
#define SHFUNC_MD5       4
#define SHFUNC_HAVAL     5

/*
 * Helper
 */
#ifndef ALIGN_UP
/* Note: align has to be a power of 2 */
#define ALIGN_UP(size, align)  (((size) + (align) - 1) & ~((align) - 1))
#endif
#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(num, div) (((num) + (div) - 1) / (div))
#endif
#ifndef POWER_OF_2
#define POWER_OF_2(x)          ((0 != x) && (0 == (x & (x-1))))
#endif
#ifndef min
#define min(a, b) \
    ({ __typeof__ (a) __a = (a); \
       __typeof__ (b) __b = (b); \
       __a < __b ? __a : __b; })
#endif
#ifndef max
#define max(a, b) \
    ({ __typeof__ (a) __a = (a); \
       __typeof__ (b) __b = (b); \
       __a > __b ? __a : __b; })
#endif

struct shfs_host {
	uint8_t type; /* name or address */
	union {
		char      name[32]; /* hostname */
		uint8_t   addr[32]; /* address (e.g., IPv4, IPv6) */
	};
} __attribute__((packed));

#define SHFS_HOST_TYPE_NAME 0x00
#define SHFS_HOST_TYPE_IPV4 0x01
#define SHFS_HOST_TYPE_IPV6 0x02 /* not supported yet */

/**
 * Common SHFS header
 * (on chunk no. 0)
 * Note: character strings fields are not necessarily null-terminated
 */
#define BOOT_AREA_LENGTH 1024
#define SHFS_MAGIC0 'S'
#define SHFS_MAGIC1 'H'
#define SHFS_MAGIC2 'F'
#define SHFS_MAGIC3 'S'
#define SHFS_MAJOR 0x02
#define SHFS_MINOR 0x01

/* member_stripemode */
#define SHFS_SM_INDEPENDENT 0x0
#define SHFS_SM_COMBINED    0x1

struct shfs_hdr_common {
	uint8_t            magic[4];
	uint8_t            version[2]; /* little endian */
	uuid_t             vol_uuid;
	char               vol_name[16];
	uint8_t            vol_byteorder;
	uint8_t            vol_encoding;
	chk_t              vol_size;
	uint64_t           vol_ts_creation;
	uint8_t            member_stripemode;
	uint32_t           member_stripesize; /* at least 4 KiB (because of first chunk), blkfront can handle at most 32 KiB */
	uuid_t             member_uuid; /* this disk */
	uint8_t            member_count;
	struct {           /* uuid's of all members */
		uuid_t     uuid;
	}                  member[SHFS_MAX_NB_MEMBERS];
} __attribute__((packed));


/**
 * SHFS configuration header
 * (on chunk no. 1)
 */
struct shfs_hdr_config {
	chk_t              htable_ref;
	chk_t              htable_bak_ref; /* if 0 => no backup */
	uint8_t            hfunc;
	uint8_t            hlen; /* num bytes of hash digest, max is 64 (= 512 bits) */
	uint32_t           htable_bucket_count;
	uint32_t           htable_entries_per_bucket;
	uint8_t            allocator;
} __attribute__((packed));

/**
 * SHFS entry (container description)
 * Note: character strings fields are not necessarily null-terminated
 */
/* hentry flags */
#define SHFS_EFLAG_HIDDEN    0x1
#define SHFS_EFLAG_DEFAULT   0x8
#define SHFS_EFLAG_LINK      0x4

/* l_attr.type */
#define SHFS_LTYPE_REDIRECT  0x0
#define SHFS_LTYPE_RAW       0x1
#define SHFS_LTYPE_AUTO      0x2

struct shfs_hentry {
	hash512_t          hash; /* hash digest */

	union {
		/* SHFS_EFLAG_LINK not set */
		struct {
			chk_t     chunk;
			uint64_t  offset; /* byte offset, usually 0 */
			uint64_t  len;    /* length (bytes) */

			char      mime[32]; /* internet media type */
			char      encoding[16]; /* set on pre-encoded content */
		} f_attr __attribute__((packed));

		/* SHFS_EFLAG_LINK set */
		struct {
			struct shfs_host rhost; /* remote host */
			uint16_t         rport;
			char             rpath[64 + 7];
			uint8_t          type;
		} l_attr __attribute__((packed));
	};

	uint64_t           ts_creation;
	uint8_t            flags;
	char               name[64];
} __attribute__((packed));

#define SHFS_MIN_CHUNKSIZE 4096

#define CHUNKS_TO_BYTES(chunks, chunksize) ((uint64_t) (chunks) * (uint64_t) (chunksize))

#define SHFS_CHUNKSIZE(hdr_common) (hdr_common->member_stripemode == SHFS_SM_COMBINED ? \
		((hdr_common)->member_stripesize * (uint32_t) ((hdr_common)->member_count)) : \
		(hdr_common)->member_stripesize)
#define SHFS_HENTRY_ALIGN 64 /* has to be a power of 2 */
#define SHFS_HENTRY_SIZE ALIGN_UP(sizeof(struct shfs_hentry), SHFS_HENTRY_ALIGN)
#define SHFS_HENTRIES_PER_CHUNK(chunksize) ((chunksize) / SHFS_HENTRY_SIZE)

#define SHFS_HTABLE_NB_ENTRIES(hdr_config) \
	((hdr_config)->htable_entries_per_bucket * (hdr_config)->htable_bucket_count)
#define SHFS_HTABLE_SIZE_CHUNKS(hdr_config, chunksize) \
	DIV_ROUND_UP(SHFS_HTABLE_NB_ENTRIES((hdr_config)), SHFS_HENTRIES_PER_CHUNK((chunksize)))

#define SHFS_HTABLE_CHUNK_NO(hentry_no, hentries_per_chunk) \
	((hentry_no) / (hentries_per_chunk))
#define SHFS_HTABLE_ENTRY_OFFSET(hentry_no, hentries_per_chunk) \
	(((hentry_no) % (hentries_per_chunk)) * SHFS_HENTRY_SIZE)

#define SHFS_HENTRY_ISHIDDEN(hentry) \
	((hentry)->flags & (SHFS_EFLAG_HIDDEN))
#define SHFS_HENTRY_ISDEFAULT(hentry) \
	((hentry)->flags & (SHFS_EFLAG_DEFAULT))
#define SHFS_HENTRY_ISLINK(hentry) \
	((hentry)->flags & (SHFS_EFLAG_LINK))

#define SHFS_HENTRY_LINKATTR(hentry) \
	((hentry)->l_attr)
#define SHFS_HENTRY_FILEATTR(hentry) \
	((hentry)->f_attr)

#define SHFS_HENTRY_LINK_TYPE(hentry) \
	((SHFS_HENTRY_LINKATTR((hentry))).type)

#ifndef __SHFS_TOOLS__
static inline int uuid_compare(const uuid_t uu1, const uuid_t uu2)
{
	return memcmp(uu1, uu2, sizeof(uuid_t));
}

static inline int uuid_is_zero(const uuid_t uu)
{
	unsigned i;
	for (i = 0; i < sizeof(uuid_t); ++i)
		if (uu[i] != 0)
			return 0;
	return 1;
}

static inline int uuid_is_null(const uuid_t uu)
{
	return (uu == NULL);
}

static inline void uuid_copy(uuid_t dst, const uuid_t src)
{
	memcpy(dst, src, sizeof(uuid_t));
}
#endif /* __SHFS_TOOLS__ */

static inline int shfshost_compare(const struct shfs_host *h0, const struct shfs_host *h1)
{
	size_t l0, l1;

	if (h0->type != h1->type)
		return 1;

	switch(h0->type) {
	case SHFS_HOST_TYPE_NAME:
		l0 = strnlen(h0->name, sizeof(h0->name));
		l1 = strnlen(h1->name, sizeof(h1->name));

		if (l0 != l1)
			return 1;
		return (memcmp(h0->name, h1->name, l0) != 0);

	case SHFS_HOST_TYPE_IPV4:
		if ((h0->addr[0] != h1->addr[0]) ||
		    (h0->addr[1] != h1->addr[1]) ||
		    (h0->addr[2] != h1->addr[2]) ||
		    (h0->addr[3] != h1->addr[3]))
			return 1;
		break;
	default:
		return 2; /* unsupported type */
	}

	return 0;
}

static inline void shfshost_copy(struct shfs_host *dst, const struct shfs_host *src)
{
	dst->type = src->type;

	switch(src->type) {
	case SHFS_HOST_TYPE_NAME:
		strncpy(dst->name, src->name, sizeof(src->name));
		break;

	case SHFS_HOST_TYPE_IPV4:
		dst->addr[0] = src->addr[0];
		dst->addr[1] = src->addr[1];
		dst->addr[2] = src->addr[2];
		dst->addr[3] = src->addr[3];
		break;
	default:
		/* unsupported type; just copy */
		memcpy(dst, src, sizeof(*dst));
		break;
	}
}

static inline uint64_t gettimestamp_s(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return (uint64_t) now.tv_sec;
}

#endif /* _SHFS_DEFS_H_ */
