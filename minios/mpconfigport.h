/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// options to control how Micro Python is built
#define MP_SSIZE_MAX (0x7fffffff)

#define MICROPY_HW_BOARD_NAME "minimal"
#define MICROPY_HW_MCU_NAME "unknown-cpu"

#define MICROPY_ALLOC_PATH_MAX      (PATH_MAX)
#define MICROPY_PERSISTENT_CODE_LOAD (1)
#if !defined(MICROPY_EMIT_X64) && defined(__x86_64__)
    #define MICROPY_EMIT_X64        (1)
#endif
#if !defined(MICROPY_EMIT_X86) && defined(__i386__)
    #define MICROPY_EMIT_X86        (1)
#endif
#if !defined(MICROPY_EMIT_THUMB) && defined(__thumb2__)
    #define MICROPY_EMIT_THUMB      (1)
    #define MICROPY_MAKE_POINTER_CALLABLE(p) ((void*)((mp_uint_t)(p) | 1))
#endif
// Some compilers define __thumb2__ and __arm__ at the same time, let
// autodetected thumb2 emitter have priority.
#if !defined(MICROPY_EMIT_ARM) && defined(__arm__) && !defined(__thumb2__)
    #define MICROPY_EMIT_ARM        (1)
#endif
#define MICROPY_COMP_MODULE_CONST   (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (1)
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_ENABLE_FINALISER    (1)
#define MICROPY_STACK_CHECK         (1)
#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)
#define MICROPY_MEM_STATS           (1)
#define MICROPY_DEBUG_PRINTERS      (0) // changed
// Printing debug to stderr may give tests which
// check stdout a chance to pass, etc.
//#define MICROPY_DEBUG_PRINTER_DEST  mp_stderr_print // changed
#define MICROPY_USE_READLINE_HISTORY (0)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_REPL_EMACS_KEYS     (1)
#define MICROPY_REPL_AUTO_INDENT    (1)
#define MICROPY_HELPER_LEXER_UNIX   (1)
#define MICROPY_ENABLE_SOURCE_LINE  (1)
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_STREAMS_NON_BLOCK   (1)
#define MICROPY_OPT_COMPUTED_GOTO   (1)
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (1)
#define MICROPY_CAN_OVERRIDE_BUILTINS (0)
#define MICROPY_PY_FUNCTION_ATTRS   (1)
#define MICROPY_PY_DESCRIPTORS      (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE (1)
#define MICROPY_PY_BUILTINS_STR_SPLITLINES (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW (1)
#define MICROPY_PY_BUILTINS_FROZENSET (1)
#define MICROPY_PY_BUILTINS_COMPILE (1)
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO (1)
#define MICROPY_PY_ALL_SPECIAL_METHODS (1)
#define MICROPY_PY_ARRAY_SLICE_ASSIGN (1)
#define MICROPY_PY_BUILTINS_SLICE_ATTRS (1)
#define MICROPY_PY_SYS_EXIT         (0)
#if defined(__APPLE__) && defined(__MACH__)
    #define MICROPY_PY_SYS_PLATFORM  "darwin"
#else
    #define MICROPY_PY_SYS_PLATFORM  "linux"
#endif
#define MICROPY_PY_SYS_MAXSIZE      (0)
#define MICROPY_PY_SYS              (1)
#define MICROPY_PY_SYS_STDFILES     (0)
#define MICROPY_PY_SYS_EXC_INFO     (0)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT (1)
#ifndef MICROPY_PY_MATH_SPECIAL_FUNCTIONS
#define MICROPY_PY_MATH_SPECIAL_FUNCTIONS (1)
#endif
#define MICROPY_PY_CMATH            (1)
#define MICROPY_PY_IO               (0)
#define MICROPY_PY_IO_FILEIO        (0)
#define MICROPY_PY_GC_COLLECT_RETVAL (1)
#define MICROPY_MODULE_FROZEN_STR   (0)
#define MICROPY_PY_LWIP             (1)
#define MICROPY_STACKLESS           (0)
#define MICROPY_STACKLESS_STRICT    (0)

#define MICROPY_PY_USSL (0)
#define MICROPY_PY_WEBSOCKET (1)

#define MICROPY_PY_OS_STATVFS       (1)
#define MICROPY_PY_UCTYPES          (1)
#define MICROPY_PY_UZLIB            (1)
#define MICROPY_PY_UJSON            (1)
#define MICROPY_PY_URE              (1)
#define MICROPY_PY_UHEAPQ           (1)
#define MICROPY_PY_UHASHLIB         (1)
#if MICROPY_PY_USSL
#define MICROPY_PY_UHASHLIB_SHA1    (1)
#endif
#define MICROPY_PY_UBINASCII        (1)
#define MICROPY_PY_URANDOM          (1)
#ifndef MICROPY_PY_USELECT
#define MICROPY_PY_USELECT          (1)
#endif
#define MICROPY_PY_MACHINE          (1)
//#define MICROPY_MACHINE_MEM_GET_READ_ADDR   mod_machine_mem_get_addr
//#define MICROPY_MACHINE_MEM_GET_WRITE_ADDR  mod_machine_mem_get_addr

#if defined(FATFS_ENABLE)
#define MICROPY_FATFS_ENABLE_LFN       (1)
#define MICROPY_FATFS_RPATH            (2)
// Can't have less than 3 values because diskio.h uses volume numbers
// as volume types and PD_USER == 2.
#define MICROPY_FATFS_VOLUMES          (3)
#define MICROPY_FATFS_MAX_SS           (4096)
#define MICROPY_FATFS_LFN_CODE_PAGE    (932) /* 1=SFN/ANSI 437=LFN/U.S.(OEM) */
#define MICROPY_FSUSERMOUNT            (1) 
#define MICROPY_VFS_FAT                (1)
#endif

// Define to MICROPY_ERROR_REPORTING_DETAILED to get function, etc.
// names in exception messages (may require more RAM).
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_WARNINGS            (1)

// Define to 1 to use undertested inefficient GC helper implementation
// (if more efficient arch-specific one is not available).
#ifndef MICROPY_GCREGS_SETJMP
    #ifdef __mips__
        #define MICROPY_GCREGS_SETJMP (1)
    #else
        #define MICROPY_GCREGS_SETJMP (0)
    #endif
#endif

#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF   (1)
#define MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE  (256)
#define MICROPY_ASYNC_KBD_INTR      (1)

//extra built in names to add to the global namespace
extern const struct _mp_obj_fun_builtin_t mp_builtin_open_obj;
#define MICROPY_PORT_BUILTINS					\
  { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) }, \

// extra built in modules to add to the list of known ones
extern const struct _mp_obj_module_t mp_module_usocket;
#define MICROPY_PORT_BUILTIN_MODULES \
  { MP_OBJ_NEW_QSTR(MP_QSTR_usocket), (mp_obj_t)&mp_module_usocket }, \

// type definitions for the specific machine
// assume that if we already defined the obj repr then we also defined types
#ifndef MICROPY_OBJ_REPR
#ifdef __LP64__
typedef long mp_int_t; // must be pointer size
typedef unsigned long mp_uint_t; // must be pointer size
#else
// These are definitions for machines where sizeof(int) == sizeof(void*),
// regardless of actual size.
typedef int mp_int_t; // must be pointer size
typedef unsigned int mp_uint_t; // must be pointer size
#endif
#endif

#define BYTES_PER_WORD sizeof(mp_int_t)

// Cannot include <sys/types.h>, as it may lead to symbol name clashes
#if _FILE_OFFSET_BITS == 64 && !defined(__LP64__)
typedef long long mp_off_t;
#else
typedef long mp_off_t;
#endif

typedef void *machine_ptr_t; // must be of pointer size
typedef const void *machine_const_ptr_t; // must be of pointer size

#ifndef MP_NOINLINE
#define MP_NOINLINE __attribute__((noinline))
#endif

#if MICROPY_PY_OS_DUPTERM
#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)
#else
#include <unistd.h>
#define MP_PLAT_PRINT_STRN(str, len) do { ssize_t ret = write(1, str, len); (void)ret; } while (0)
#endif

#ifdef __linux__
// Can access physical memory using /dev/mem
#define MICROPY_PLAT_DEV_MEM  (1)
#endif

// Assume that select() call, interrupted with a signal, and erroring
// with EINTR, updates remaining timeout value.
#define MICROPY_SELECT_REMAINING_TIME (1)

#ifdef __ANDROID__
#include <android/api-level.h>
#if __ANDROID_API__ < 4
// Bionic libc in Android 1.5 misses these 2 functions
#define MP_NEED_LOG2 (1)
#define nan(x) NAN
#endif
#endif

#define MP_STATE_PORT MP_STATE_VM

#define MICROPY_PORT_ROOT_POINTERS \
    const char *readline_hist[50]; \
    mp_obj_t keyboard_interrupt_obj; \
    void *mmap_region_head; \

// We need to provide a declaration/definition of alloca()
// unless support for it is disabled.
#if !defined(MICROPY_NO_ALLOCA) || MICROPY_NO_ALLOCA == 0
#ifdef __FreeBSD__
#include <stdlib.h>
#else
#include <alloca.h>
#endif
#endif

// From "man readdir": "Under glibc, programs can check for the availability
// of the fields [in struct dirent] not defined in POSIX.1 by testing whether
// the macros [...], _DIRENT_HAVE_D_TYPE are defined."
// Other libc's don't define it, but proactively assume that dirent->d_type
// is available on a modern *nix system.
#ifndef _DIRENT_HAVE_D_TYPE
#define _DIRENT_HAVE_D_TYPE (1)
#endif
// This macro is not provided by glibc but we need it so ports that don't have
// dirent->d_ino can disable the use of this field.
#ifndef _DIRENT_HAVE_D_INO
#define _DIRENT_HAVE_D_INO (1)
#endif
