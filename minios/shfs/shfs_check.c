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
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include "shfs_check.h"
#include "shfs_defs.h"

int shfs_detect_hdr0(void *chk0) {
	struct shfs_hdr_common *hdr_common;

	hdr_common = (void *)((uint8_t *) chk0 + BOOT_AREA_LENGTH);

	/* Check for SHFS magic */
	if (hdr_common->magic[0] != SHFS_MAGIC0)
		return -1;
	if (hdr_common->magic[1] != SHFS_MAGIC1)
		return -1;
	if (hdr_common->magic[2] != SHFS_MAGIC2)
		return -1;
	if (hdr_common->magic[3] != SHFS_MAGIC3)
		return -1;

	/* Check for compatible version */
	if (hdr_common->version[0] != SHFS_MAJOR)
		return -2;
	if (hdr_common->version[1] != SHFS_MINOR)
		return -2;

	/* Check Endianess */
#if __BYTE_ORDER == __LITTLE_ENDIAN
	if (hdr_common->vol_byteorder != SBO_LITTLEENDIAN)
#elif __BYTE_ORDER == __BIG_ENDIAN
	if (hdr_common->vol_byteorder != SBO_BIGENDIAN)
#else
#warning "Could not detect byte-order"
#endif
		return -3;

	/* Briefly check member count */
	if (hdr_common->member_count == 0 || hdr_common->member_count > SHFS_MAX_NB_MEMBERS)
		return -4;

	return 1; /* SHFSv1 detected */
}
