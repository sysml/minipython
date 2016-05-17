/*
 * Simon's HashFS (SHFS) for Mini-OS
 *
 * Copyright(C) 2013-2014 NEC Laboratories Europe. All rights reserved.
 *                        Simon Kuenzer <simon.kuenzer@neclab.eu>
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
