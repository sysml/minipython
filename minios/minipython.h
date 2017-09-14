/*
 * Minipython, a Xen-based Unikernel.
 *
 * Authors:  Felipe Huici <felipe.huici@neclab.eu>
 *           Simon Kuenzer <simon.kuenzer@neclab.eu>
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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "console.h"
#include "py/mpstate.h"
#include "py/nlr.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "genhdr/mpversion.h"
#include "input.h"
#if SHFS_ENABLE
#include "shfs/shfs.h"
#include "shfs/shfs_fio.h"
#endif
#if MICROPY_VFS_FAT
#include "lib/fatfs/ff.h"
#include "extmod/fsusermount.h"
extern mp_lexer_t *fat_vfs_lexer_new_from_file(const char *filename);
extern mp_import_stat_t fat_vfs_import_stat(const char *path);
#endif

int handle_uncaught_exception(mp_obj_base_t *exc);
int execute_from_lexer(mp_lexer_t *lex, mp_parse_input_kind_t input_kind, bool is_repl);

#if SHFS_ENABLE
STATIC mp_uint_t shfs_file_buf_next_byte(mp_lexer_file_buf_t *fb);
STATIC void shfs_file_buf_close(mp_lexer_file_buf_t *fb);
mp_lexer_t *mp_lexer_new_from_file(const char *filename);
#endif

#if MICROPY_VFS_FAT
mp_lexer_t *mp_lexer_new_from_file(const char *filename);
mp_obj_t vfs_proxy_call(qstr method_name, mp_uint_t n_args, const mp_obj_t *args);
#endif

int do_str(const char *str);
int do_file(const char *file);
void print_banner();
uint mp_import_stat(const char *path);
void nlr_jump_fail(void *val);
void pythonpath_append(char *path);
int main(int argc, char **argv);
