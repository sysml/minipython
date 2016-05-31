/*
 * Copyright 2013 NEC Laboratories Europe
 *                Simon Kuenzer
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
