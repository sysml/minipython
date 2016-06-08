/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2014 Paul Sokolovsky
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

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "py/mpconfig.h"

#include "py/nlr.h"
#include "py/runtime.h"
#include "py/objtuple.h"
#include "extmod/misc.h"
#include "lib/fatfs/ff.h"

extern const mp_obj_type_t mp_fat_vfs_type;

#define RAISE_ERRNO(err_flag, error_val) \
    { if (err_flag != FR_OK) \
        { nlr_raise(mp_obj_new_exception_arg1(&mp_type_OSError, MP_OBJ_NEW_SMALL_INT(error_val))); } }

STATIC mp_obj_t mod_os_unlink(mp_obj_t path_in) {
    mp_uint_t len;
    const char *path = mp_obj_str_get_data(path_in, &len);
    FRESULT r = f_unlink(path);
    RAISE_ERRNO(r, errno);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_os_unlink_obj, mod_os_unlink);

STATIC mp_obj_t mod_os_mkdir(mp_obj_t path_in) {
    const char *path = mp_obj_str_get_str(path_in);
    FRESULT res = f_mkdir(path);
    RAISE_ERRNO(res, errno);
    
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_os_mkdir_obj, mod_os_mkdir);

STATIC mp_obj_t mod_os_errno(size_t n_args, const mp_obj_t *args) {
    if (n_args == 0) {
        return MP_OBJ_NEW_SMALL_INT(errno);
    }

    errno = mp_obj_get_int(args[0]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_os_errno_obj, 0, 1, mod_os_errno);

STATIC const mp_rom_map_elem_t mp_module_os_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_uos) },
    { MP_ROM_QSTR(MP_QSTR_errno), MP_ROM_PTR(&mod_os_errno_obj) },
    { MP_ROM_QSTR(MP_QSTR_unlink), MP_ROM_PTR(&mod_os_unlink_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&mod_os_mkdir_obj) },
    #if MICROPY_VFS_FAT
    { MP_ROM_QSTR(MP_QSTR_VfsFat), MP_ROM_PTR(&mp_fat_vfs_type) },
    #endif
};

STATIC MP_DEFINE_CONST_DICT(mp_module_os_globals, mp_module_os_globals_table);

const mp_obj_module_t mp_module_os = {
    .base = { &mp_type_module },
    .name = MP_QSTR_uos,
    .globals = (mp_obj_dict_t*)&mp_module_os_globals,
};
