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

#ifndef _HEXDUMP_H_
#define _HEXDUMP_H_

enum hd_addr_type {
  HDAT_NONE = 0,
  HDAT_RELATIVE,
  HDAT_ABSOLUTE
};

/**
 * hexdump - print a text hexdump for a binary blob of data
 * @buf: data blob to dump
 * @len: number of bytes in the @buf
 * @prefix_str: null-terminated string that is prefixed to each line
 * @addr_type: controls whether the offset address, full address, or none
 *  is printed in front of each line (%HDAT_RELATIVE, %HDAT_ABSOLUTE,
 *  %HDAT_NONE)
 * @addr_offset: An offset that is added to each printed address if %addr_tpe is
 *  set to %HDAT_RELATIVE
 * @rowlen: number of buffer bytes to print per line
 * @groupsize: number of bytes to print as group together
 * @show_ascii_comlumn: include ASCII after the hex output
 *
 * Example output using %HDAT_RELATIVE and 4-byte mode:
 * 0009ab42: 40 41 42 43  44 45 46 47 @ABCD EFGH
 * Example output using %HDAT_ABSOLUTE and 2-byte mode:
 * ffffffff88089af0: 73 72  71 70  77 76  75 74 pq rs tu vw
 */
void hexdump(FILE *cout, const void *buf, size_t len,
             const char *prefix_str, enum hd_addr_type addr_type,
             off_t addr_offset ,size_t rowsize, size_t groupsize,
             int show_ascii_column);

/**
 * printh - shorthand form of print_hex_dump() with default params
 * @buf: data blob to dump
 * @len: number of bytes in the @buf
 *
 * Calls hexdump(), with rowsize of 16, groupsize of 4,
 * and enabled ASCII column output.
 */
#define printh(buf, len)	  \
	hexdump((stdout), (buf), (len), "", HDAT_RELATIVE, 0, 16, 4, 1)

#endif /* _HEXDUMP_H_ */
