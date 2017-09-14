/*
 * Hexdump-like routines
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

#include <mini-os/os.h>
#include <mini-os/types.h>
#include <mini-os/xmalloc.h>
#include <mini-os/lib.h>
#include <mini-os/kernel.h>
#include <mini-os/semaphore.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <hexdump.h>

#ifndef min
#define min(a, b) \
    ({ __typeof__ (a) __a = (a); \
       __typeof__ (b) __b = (b); \
       __a < __b ? __a : __b; })
#endif

/**
 * @buf: data blob to dump
 * @len: number of bytes in the @buf
 * @rowsize: number of buffer bytes to print per line; must be 16, 32,
 * 64, or 128
 * @groupsize: number of bytes to print as group together (1, 2, 4, 8)
 * @linebuf: where to put the hexdump string
 * @linebuflen: total size of @linebuf, including terminating \0 character
 * @show_ascii_comlumn: include ASCII after the hex output
 *
 * _hdline2buf() prints a single "line" of output at a time, i.e.,
 * 16 or 32 bytes of input data converted to hex + ASCII output.
 *
 * Given a buffer of uint8_t data, _hdline2buf() converts the input data
 * to a hex + ASCII dump at the supplied memory location.
 * The converted output (string) is always NUL-terminated (\0).
 */
static inline size_t _hdline2buf(const void *buf, size_t len,
                                 size_t rowlen, size_t groupsize,
                                 int show_ascii_column,
                                 char *trglinebuf, size_t trglinebuflen)
{
    const uint8_t *ptr = buf;
    uint8_t ch;
    size_t nb_groups, group, byte;
    size_t trglinebufpos = 0;

    if (!len)
        goto eol;
    if (len > rowlen) /* limit input to rowlen */
        len = rowlen;
    nb_groups = rowlen / groupsize;

    if (show_ascii_column)
        ASSERT(trglinebuflen >= (((3 * groupsize) * nb_groups) - 1) + (nb_groups) + 1);
    else
        ASSERT(trglinebuflen >= (((3 * groupsize) * nb_groups) - 1) + (2 * nb_groups) + 1 + ((groupsize * nb_groups) - 1) + 1);

    for (group = 0; group < nb_groups; group++) {
        if (group) /* leading space for group */
            trglinebufpos += snprintf(trglinebuf + trglinebufpos, trglinebuflen - trglinebufpos, " ");

        for (byte = ( group     * groupsize);
             byte < ((group + 1)* groupsize);
             byte++) {
            if (byte < len) { /* print hex number */
                ptr = ((uint8_t *) buf) + byte;
                trglinebufpos += snprintf(trglinebuf + trglinebufpos, trglinebuflen - trglinebufpos,
                                          "%s%02x", (group || byte) ? " " : "", *(ptr));
            } else { /* print white space */
                if (!show_ascii_column)
                    goto eol;
                trglinebufpos += snprintf(trglinebuf + trglinebufpos, trglinebuflen - trglinebufpos,
                                          "%s  ", (group || byte) ? " " : "");
            }
        }
    }

    if (show_ascii_column) {
        trglinebufpos += snprintf(trglinebuf + trglinebufpos, trglinebuflen - trglinebufpos, " ");

        for (group = 0; group < nb_groups; group++) {
            if (group) /* leading space for group */
                trglinebufpos += snprintf(trglinebuf + trglinebufpos, trglinebuflen - trglinebufpos, " ");

            for (byte = ( group     * groupsize);
                 byte < ((group + 1)* groupsize);
                 byte++) {
                if (byte < len) { /* print ascii character */
                    ptr = ((uint8_t *) buf) + byte;
                    ch  = (char) *(ptr);
                    trglinebufpos += snprintf(trglinebuf + trglinebufpos, trglinebuflen - trglinebufpos,
                                              "%c", (ch >= 32 && ch <= 126) ? ch : '.');
                } else {
                    break;
                }
            }
        }
    }
eol:
    trglinebuf[trglinebufpos++] = '\0';
    return trglinebufpos;
}

void hexdump(FILE *cout, const void *buf, size_t len,
             const char *prefix_str, enum hd_addr_type addr_type,
             off_t addr_offset, size_t rowlen, size_t groupsize,
             int show_ascii_column)
{
    const uint8_t *ptr;
    size_t i, linelen, remaining;
    char linebuf[512];

    ASSERT(rowlen == 16 || rowlen == 32 || rowlen == 64 || rowlen == 128);
    ASSERT(groupsize == 1 || groupsize == 2 || groupsize == 4 || groupsize == 8);

    ptr = buf;
    remaining = len;

    for (i = 0; i < len; i += rowlen) {
        linelen = min(remaining, rowlen);
        remaining -= linelen;

        _hdline2buf(ptr + i, linelen, rowlen, groupsize,
                    show_ascii_column, linebuf, sizeof(linebuf));

        switch (addr_type) {
        case HDAT_ABSOLUTE:
	    fprintf(cout, "%s%p: %s\n",
                   prefix_str, ptr + i, linebuf);
            break;
        case HDAT_RELATIVE:
            fprintf(cout, "%s%.8x: %s\n", prefix_str, i + addr_offset, linebuf);
            break;
        default: /* HDAT_NONE */
            fprintf(cout, "%s%s\n", prefix_str, linebuf);
            break;
        }
    }
}
