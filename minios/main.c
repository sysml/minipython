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

#include "shfs/shfs.h"
#include "shfs/shfs_fio.h"

// Command line options, with their defaults
STATIC bool compile_only = false;
STATIC uint emit_opt = MP_EMIT_OPT_NONE;

#if MICROPY_ENABLE_GC
// Heap size of GC heap (if enabled)
// Make it larger on a 64 bit machine, because pointers are larger.
long heap_size = 1024*1024 * (sizeof(mp_uint_t) / 4);
#endif

STATIC void stderr_print_strn(void *env, const char *str, size_t len) {
    (void)env;
    ssize_t dummy = write(STDERR_FILENO, str, len);
    mp_uos_dupterm_tx_strn(str, len);
    (void)dummy;
}

const mp_print_t mp_stderr_print = {NULL, stderr_print_strn};

#define FORCED_EXIT (0x100)
// If exc is SystemExit, return value where FORCED_EXIT bit set,
// and lower 8 bits are SystemExit value. For all other exceptions,
// return 1.
STATIC int handle_uncaught_exception(mp_obj_base_t *exc) {
    // check for SystemExit
    if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(exc->type), MP_OBJ_FROM_PTR(&mp_type_SystemExit))) {
        // None is an exit value of 0; an int is its value; anything else is 1
        mp_obj_t exit_val = mp_obj_exception_get_value(MP_OBJ_FROM_PTR(exc));
        mp_int_t val = 0;
        if (exit_val != mp_const_none && !mp_obj_get_int_maybe(exit_val, &val)) {
            val = 1;
        }
        return FORCED_EXIT | (val & 255);
    }

    // Report all other exceptions
    mp_obj_print_exception(&mp_stderr_print, MP_OBJ_FROM_PTR(exc));
    return 1;
}

typedef struct _mp_lexer_file_buf_t {
#if SHFS_ENABLE
  SHFS_FD f;
#endif
  byte buf[20];
  uint16_t len;
  uint16_t pos;
  uint64_t fpos;
  uint64_t fsize;
} mp_lexer_file_buf_t;

// Returns standard error codes: 0 for success, 1 for all other errors,
// except if FORCED_EXIT bit is set then script raised SystemExit and the
// value of the exit is in the lower 8 bits of the return value
STATIC int execute_from_lexer(mp_lexer_t *lex, mp_parse_input_kind_t input_kind, bool is_repl) {
    if (lex == NULL) {
        printf("MemoryError: lexer could not allocate memory\n");
        return 1;
    }

    mp_hal_set_interrupt_char(CHAR_CTRL_C);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        qstr source_name = lex->source_name;

        #if MICROPY_PY___FILE__
        if (input_kind == MP_PARSE_FILE_INPUT) {
            mp_store_global(MP_QSTR___file__, MP_OBJ_NEW_QSTR(source_name));
        }
        #endif

        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);

        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, emit_opt, is_repl);

        if (!compile_only) {
            // execute it
            mp_call_function_0(module_fun);
            // check for pending exception
            if (MP_STATE_VM(mp_pending_exception) != MP_OBJ_NULL) {
                mp_obj_t obj = MP_STATE_VM(mp_pending_exception);
                MP_STATE_VM(mp_pending_exception) = MP_OBJ_NULL;
                nlr_raise(obj);
            }
        }

        mp_hal_set_interrupt_char(-1);
        nlr_pop();
        return 0;

    } else {
        // uncaught exception
        mp_hal_set_interrupt_char(-1);
        return handle_uncaught_exception(nlr.ret_val);
    }
}

#if MICROPY_USE_READLINE == 1
#include "lib/mp-readline/readline.h"
#else
#endif

#if SHFS_ENABLE
STATIC mp_uint_t shfs_file_buf_next_byte(mp_lexer_file_buf_t *fb) {
  uint64_t rlen;
  int ret;

  if (fb->pos >= fb->len) {
    rlen = sizeof(fb->buf) + fb->fpos > fb->fsize ? fb->fsize - fb->fpos : sizeof(fb->buf);
    if (rlen == 0) return MP_LEXER_EOF;
    
    ret = shfs_fio_read(fb->f, fb->fpos, &fb->buf, rlen);
    fb->len = rlen;
    fb->pos = 0;
    fb->fpos += rlen;
  }
  return fb->buf[fb->pos++];  
}

STATIC void shfs_file_buf_close(mp_lexer_file_buf_t *fb) {
  shfs_fio_close(fb->f);
  m_del_obj(mp_lexer_file_buf_t, fb);
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
  uint64_t rlen;
  int ret = 0;
    
  mp_lexer_file_buf_t *fb = m_new_obj_maybe(mp_lexer_file_buf_t);

  if (fb == NULL)  return NULL;

  fb->f = shfs_fio_open(filename);
  if (!fb->f) {
    printk("%s: Could not open: %s\n", filename, strerror(errno));
    return NULL;
  }

  shfs_fio_size(fb->f, &fb->fsize);
  rlen = sizeof(fb->buf) > fb->fsize ? fb->fsize : sizeof(fb->buf);
  ret = shfs_fio_read(fb->f, 0, &fb->buf, rlen);
  
  fb->len = rlen;
  fb->fpos = rlen;
  fb->pos = 0;
  
  return mp_lexer_new(qstr_from_str(filename), fb, (mp_lexer_stream_next_byte_t)shfs_file_buf_next_byte, (mp_lexer_stream_close_t)shfs_file_buf_close);
}
#endif

STATIC int do_str(const char *str) {
    mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, str, strlen(str), false);
    return execute_from_lexer(lex, MP_PARSE_FILE_INPUT, false);
}

STATIC int do_file(const char *file) {
  mp_lexer_t *lex = mp_lexer_new_from_file(file);
  return execute_from_lexer(lex, MP_PARSE_FILE_INPUT, false);
}

void print_banner() {
  printk("\n");  
  printk(" __  __ _       _        _____       _   _                      \n");
  printk("|  \\/  (_)     (_)      |  __ \\     | | | |                   \n");
  printk("| \\  / |_ _ __  _ ______| |__) |   _| |_| |__   ___  _ __      \n");
  printk("| |\\/| | | '_ \\| |______|  ___/ | | | __| '_ \\ / _ \\| '_ \\ \n");
  printk("| |  | | | | | | |      | |   | |_| | |_| | | | (_) | | | |     \n");
  printk("|_|  |_|_|_| |_|_|      |_|    \\__, |\\__|_| |_|\\___/|_| |_|  \n");
  printk("                                __/ |                           \n");
  printk("                               |___/                            \n");
  printk("\n");
  printk("Copyright(C) 2013-2016 NEC Europe Ltd.                          \n");
  printk("Authors: Felipe Huici  <felipe.huici@neclab.eu>                 \n");  
  printk("         Simon Kuenzer <simon.kuenzer@neclab.eu>                \n");  
  printk("         Filipe Manco  <filipe.manco@neclab.eu>                 \n");
  printk("\n");
}

uint mp_import_stat(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    if (S_ISDIR(st.st_mode)) {
      return MP_IMPORT_STAT_DIR;
    } else if (S_ISREG(st.st_mode)) {
      return MP_IMPORT_STAT_FILE;
    }
  }
  return MP_IMPORT_STAT_NO_EXIST;
}

void nlr_jump_fail(void *val) {
  printf("FATAL: uncaught NLR %p\n", val);
  exit(1);
}

int main(int argc, char **argv) {
#if SHFS_ENABLE      
    int ret = 0;
    int id = 51712;
#endif
    
    print_banner();  

    mp_stack_ctrl_init();
    mp_stack_set_limit(40000 * (BYTES_PER_WORD / 4));

#if MICROPY_ENABLE_GC
    char *heap = malloc(heap_size);
    gc_init(heap, heap + heap_size);
#endif

    mp_init();

#if SHFS_ENABLE    
    init_shfs();
    ret = mount_shfs(&id, 1);   
    if (ret < 0) return 0;
    //    do_file("");
#endif
    //do_str("import lwip\nlwip.reset()\n");    
    do_str("import lwip\nlwip.reset()\neth = lwip.ether('172.64.0.100', '255.255.255.0', '0.0.0.0')\nwhile 1: eth.poll()\n");
    
    mp_deinit();

#if MICROPY_ENABLE_GC && !defined(NDEBUG)
    // We don't really need to free memory since we are about to exit the
    // process, but doing so helps to find memory leaks.
    free(heap);
#endif

    return 0;
}
