/*
 * Simon's HashFS (SHFS) for Mini-OS
 *
 * Copyright(C) 2013-2014 NEC Laboratories Europe. All rights reserved.
 *                        Simon Kuenzer <simon.kuenzer@neclab.eu>
 */
#ifndef _SHFS_CHECK_H_
#define _SHFS_CHECK_H_

/**
 * Can be used to detect an compatible SHFS filesystem
 * header. It returns a value >= 0 if SHFS was detected.
 * Note: chk0 buffer has to be at least 4096 bytes long
 */
int shfs_detect_hdr0(void *chk0);

#endif /* _SHFS_CHECK_H_ */
