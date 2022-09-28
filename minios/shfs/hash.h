/*
 * Simple hash value implementation for MiniOS and POSIX
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

#ifndef _HASH_H_
#define _HASH_H_

#include <string.h>

typedef uint8_t hash512_t[64] __attribute__((aligned(8)));


static inline void hash_copy(hash512_t dst, const hash512_t src, uint8_t hlen)
{
#ifdef __x86_64
	register uint8_t nbleft = hlen & 0x07; /* = mod by 8 */
	register uint64_t mask64;
	register uint64_t *p64_dst;
	register uint64_t *p64_src;
	register uint8_t i;

	hlen -= nbleft;
	i = 0;
	for (; i < hlen; i += 8) {
		p64_dst = (uint64_t *) &dst[i];
		p64_src = (uint64_t *) &src[i];
		*p64_dst = *p64_src;
	}
	if (nbleft) {
		mask64 = ((uint64_t) 1 << (nbleft << 3)) - 1;
		p64_dst = (uint64_t *) &dst[i];
		p64_src = (uint64_t *) &src[i];
		*p64_dst = *p64_src & mask64;
	}
#else
	memcpy(dst, src, hlen);
#endif
}

static inline int hash_compare(const hash512_t h0, const hash512_t h1, uint8_t hlen)
{
#ifdef __x86_64
	register uint8_t nbleft = hlen & 0x07; /* = mod by 8 */
	register uint64_t mask64;
	register uint64_t *p64_0;
	register uint64_t *p64_1;
	register uint8_t i;

	hlen -= nbleft;
	i = 0;
	for (; i < hlen; i += 8) {
		p64_0 = (uint64_t *) &h0[i];
		p64_1 = (uint64_t *) &h1[i];
		if (*p64_0 != *p64_1)
			return 1;
	}
	if (nbleft) {
		mask64 = ((uint64_t) 1 << (nbleft << 3)) - 1;
		p64_0 = (uint64_t *) &h0[i];
		p64_1 = (uint64_t *) &h1[i];
		if ((*p64_0 & mask64) != (*p64_1 & mask64))
			return 1;
	}

	return 0;
#else
	return (memcmp(h0, h1, hlen) != 0);
#endif
}

static inline int hash_is_zero(const hash512_t h, uint8_t hlen)
{
#ifdef __x86_64
	register uint8_t nbleft = hlen & 0x07; /* = mod by 8 */
	register uint64_t mask64;
	register uint64_t *p64;
	register uint8_t i;

	hlen -= nbleft;
	i = 0;
	for (; i < hlen; i += 8) {
		p64 = (uint64_t *) &h[i];
		if (*p64 != 0)
			return 0;
	}
	if (nbleft) {
		mask64 = ((uint64_t) 1 << (nbleft << 3)) - 1;
		p64 = (uint64_t *) &h[i];
		if ((*p64 & mask64) != 0)
			return 0;
	}

	return 1;
#else
	uint8_t i;

	for (i = 0; i < hlen; ++i)
		if (h[i] != 0)
			return 0;
	return 1;
#endif
}

static inline int hash_is_max(const hash512_t h, uint8_t hlen)
{
#ifdef __x86_64
	register uint8_t nbleft = hlen & 0x07; /* = mod by 8 */
	register uint64_t mask64;
	register uint64_t *p64;
	register uint8_t i;

	hlen -= nbleft;
	i = 0;
	for (; i < hlen; i += 8) {
		p64 = (uint64_t *) &h[i];
		if (*p64 != UINT64_MAX)
			return 0;
	}
	if (nbleft) {
		mask64 = ((uint64_t) 1 << (nbleft << 3)) - 1;
		p64 = (uint64_t *) &h[i];
		if ((*p64 & mask64) != mask64)
			return 0;
	}

	return 1;
#else
	uint8_t i;

	for (i = 0; i < hlen; ++i)
		if (h[i] != 0xFF)
			return 0;
	return 1;
#endif
}

static inline void hash_clear(hash512_t h, uint8_t hlen)
{
#ifdef __x86_64
	register uint64_t *p64;
	register uint8_t i;

	for (i = 0; i < hlen; i += 8) {
		p64 = (uint64_t *) &h[i];
		*p64 = 0;
	}
#else
	memset(h, 0x00, hlen);
#endif
}

static inline int hash_parse(const char *in, hash512_t h, uint8_t hlen)
{
	uint8_t strlen = hlen * 2;
	uint8_t i, nu, nl;

	for (i = 0; i < strlen; ++i) {
		/* get upper nibble (4 bits) */
		switch (in[i]) {
		case '0' ... '9':
			nu = in[i] - '0';
			break;
		case 'a' ... 'f':
			nu = in[i] - 'a' + 10;
			break;
		case 'A' ... 'F':
			nu = in[i] - 'A' + 10;
			break;
		case '\0':
		default:
			return -1; /* unknown character or string ended unexpectedly */
		}

		/* get lower nibble (4 bits) */
		++i;
		switch (in[i]) {
		case '0' ... '9':
			nl = in[i] - '0';
			break;
		case 'a' ... 'f':
			nl = in[i] - 'a' + 10;
			break;
		case 'A' ... 'F':
			nl = in[i] - 'A' + 10;
			break;
		case '\0':
		default:
			return -1;
		}

		h[i >> 1] = (nu << 4) | nl;
	}

	if (in[i] != '\0')
		return -1;

	return 0;
}

#endif /* _HASH_H_ */
