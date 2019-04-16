/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2015 Daniel Campora
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

#include <stdint.h>

// board specific definitions
#include "mpconfigboard.h"

// options to control how MicroPython is built

#define MICROPY_ALLOC_PATH_MAX                      (128)
#define MICROPY_PERSISTENT_CODE_LOAD                (1)
#define MICROPY_EMIT_THUMB                          (0)
#define MICROPY_EMIT_INLINE_THUMB                   (0)
#define MICROPY_COMP_MODULE_CONST                   (1)
#define MICROPY_ENABLE_GC                           (1)
#define MICROPY_ENABLE_FINALISER                    (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN            (0)
#define MICROPY_STACK_CHECK                         (0)
#define MICROPY_HELPER_REPL                         (1)
#define MICROPY_ENABLE_SOURCE_LINE                  (1)
#define MICROPY_ENABLE_DOC_STRING                   (0)
#define MICROPY_REPL_AUTO_INDENT                    (1)
#define MICROPY_ERROR_REPORTING                     (MICROPY_ERROR_REPORTING_TERSE)
#define MICROPY_LONGINT_IMPL                        (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_FLOAT_IMPL                          (MICROPY_FLOAT_IMPL_NONE)
#define MICROPY_OPT_COMPUTED_GOTO                   (0)
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE    (0)
#define MICROPY_READER_VFS                          (1)
#define MICROPY_CPYTHON_COMPAT                      (1)
#define MICROPY_QSTR_BYTES_IN_HASH                  (1)

// fatfs configuration used in ffconf.h
#define MICROPY_FATFS_ENABLE_LFN                    (2)
#define MICROPY_FATFS_MAX_LFN                       (MICROPY_ALLOC_PATH_MAX)
#define MICROPY_FATFS_LFN_CODE_PAGE                 (437) // 1=SFN/ANSI 437=LFN/U.S.(OEM)
#define MICROPY_FATFS_RPATH                         (2)
/*
#define MICROPY_FATFS_REENTRANT                     (1)
#define MICROPY_FATFS_SYNC_T                        SemaphoreHandle_t
*/

#define MICROPY_STREAMS_NON_BLOCK                   (1)
#define MICROPY_MODULE_WEAK_LINKS                   (1)
#define MICROPY_CAN_OVERRIDE_BUILTINS               (1)
#define MICROPY_USE_INTERNAL_ERRNO                  (1)
#define MICROPY_VFS                                 (1)
#define MICROPY_VFS_FAT                             (1)
#define MICROPY_PY_ASYNC_AWAIT                      (0)
#define MICROPY_PY_ALL_SPECIAL_METHODS              (1)
#define MICROPY_PY_BUILTINS_INPUT                   (1)
#define MICROPY_PY_BUILTINS_HELP                    (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT				mp_help_default_text
#define MICROPY_PY_BUILTINS_STR_UNICODE             (1)
#define MICROPY_PY_BUILTINS_STR_SPLITLINES          (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW              (1)
#define MICROPY_PY_BUILTINS_FROZENSET               (1)
#define MICROPY_PY_BUILTINS_EXECFILE                (1)
#define MICROPY_PY_ARRAY_SLICE_ASSIGN               (1)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT          (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO             (0)
#define MICROPY_PY_SYS_MAXSIZE                      (1)
#define MICROPY_PY_SYS_EXIT                         (1)
#define MICROPY_PY_SYS_STDFILES                     (1)
#define MICROPY_PY_CMATH                            (0)
#define MICROPY_PY_IO                               (1)
#define MICROPY_PY_IO_FILEIO                        (1)
#define MICROPY_PY_UERRNO                           (1)
#define MICROPY_PY_UERRNO_ERRORCODE                 (0)
#define MICROPY_PY_THREAD                           (0)
#define MICROPY_PY_THREAD_GIL                       (0)
#define MICROPY_PY_UBINASCII                        (1)
#define MICROPY_PY_UCTYPES                          (0)
#define MICROPY_PY_UZLIB                            (0)
#define MICROPY_PY_UJSON                            (1)
#define MICROPY_PY_URE                              (1)
#define MICROPY_PY_UHEAPQ                           (0)
#define MICROPY_PY_UHASHLIB                         (0)
#define MICROPY_PY_USELECT                          (1)
#define MICROPY_PY_UTIME_MP_HAL                     (1)

#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF      (1)
#define MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE        (0)
#define MICROPY_KBD_EXCEPTION                       (1)

// We define our own list of errno constants to include in uerrno module
#define MICROPY_PY_UERRNO_LIST \
    X(EPERM) \
    X(EIO) \
    X(ENODEV) \
    X(EINVAL) \
    X(ETIMEDOUT) \

// TODO these should be generic, not bound to fatfs
#define mp_type_fileio mp_type_vfs_fat_fileio
#define mp_type_textio mp_type_vfs_fat_textio

// use vfs's functions for import stat and builtin open
#define mp_import_stat mp_vfs_import_stat
#define mp_builtin_open mp_vfs_open
#define mp_builtin_open_obj mp_vfs_open_obj

// extra built in names to add to the global namespace
#define MICROPY_PORT_BUILTINS \
    { MP_ROM_QSTR(MP_QSTR_open),  MP_ROM_PTR(&mp_builtin_open_obj) },   \

// extra built in modules to add to the list of known ones
extern const struct _mp_obj_module_t machine_module;
extern const struct _mp_obj_module_t network_module;
extern const struct _mp_obj_module_t socket_module;
extern const struct _mp_obj_module_t mp_module_ure;
extern const struct _mp_obj_module_t mp_module_ujson;
extern const struct _mp_obj_module_t mp_module_uos;
extern const struct _mp_obj_module_t mp_module_utime;
extern const struct _mp_obj_module_t mp_module_uselect;
extern const struct _mp_obj_module_t mp_module_ubinascii;

#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_ROM_QSTR(MP_QSTR_uos),         MP_ROM_PTR(&mp_module_uos) },       \
    { MP_ROM_QSTR(MP_QSTR_umachine),    MP_ROM_PTR(&machine_module) },      \
	{ MP_ROM_QSTR(MP_QSTR_unetwork),    MP_ROM_PTR(&network_module) },      \
	{ MP_ROM_QSTR(MP_QSTR_usocket),     MP_ROM_PTR(&socket_module) },       \
    { MP_ROM_QSTR(MP_QSTR_utime),       MP_ROM_PTR(&mp_module_utime) },     \
    { MP_ROM_QSTR(MP_QSTR_uselect),     MP_ROM_PTR(&mp_module_uselect) },   \
    { MP_ROM_QSTR(MP_QSTR_ubinascii),   MP_ROM_PTR(&mp_module_ubinascii) }, \

#define MICROPY_PORT_BUILTIN_MODULE_WEAK_LINKS \
    { MP_ROM_QSTR(MP_QSTR_os),          MP_ROM_PTR(&mp_module_uos) },       \
    { MP_ROM_QSTR(MP_QSTR_machine),     MP_ROM_PTR(&machine_module) },      \
	{ MP_ROM_QSTR(MP_QSTR_network),     MP_ROM_PTR(&network_module) },      \
	{ MP_ROM_QSTR(MP_QSTR_socket),      MP_ROM_PTR(&socket_module) },       \
    { MP_ROM_QSTR(MP_QSTR_time),        MP_ROM_PTR(&mp_module_utime) },     \
    { MP_ROM_QSTR(MP_QSTR_select),      MP_ROM_PTR(&mp_module_uselect) },   \
    { MP_ROM_QSTR(MP_QSTR_binascii),    MP_ROM_PTR(&mp_module_ubinascii) }, \
    { MP_ROM_QSTR(MP_QSTR_re),          MP_ROM_PTR(&mp_module_ure) },       \
    { MP_ROM_QSTR(MP_QSTR_json),        MP_ROM_PTR(&mp_module_ujson) },     \
    { MP_ROM_QSTR(MP_QSTR_errno),       MP_ROM_PTR(&mp_module_uerrno) },    \
    { MP_ROM_QSTR(MP_QSTR_struct),      MP_ROM_PTR(&mp_module_ustruct) },   \

// extra constants
#define MICROPY_PORT_CONSTANTS \
   { MP_ROM_QSTR(MP_QSTR_umachine),     MP_ROM_PTR(&machine_module) },


// vm state and root pointers for the gc
#define MP_STATE_PORT MP_STATE_VM
#define MICROPY_PORT_ROOT_POINTERS                    \
	const char *readline_hist[8];                     \
	mp_obj_t machine_config_main;                     \
	struct _pyb_uart_obj_t *pyb_uart_objs[6];         \
	struct _os_term_dup_obj_t *os_term_dup_obj;       \
	struct _nic_obj_t *modnetwork_nic;                \


// type definitions for the specific machine
#define MICROPY_MAKE_POINTER_CALLABLE(p)            ((void*)((mp_uint_t)(p) | 1))
#define MP_SSIZE_MAX                                (0x7FFFFFFF)

#define UINT_FMT                                    "%u"
#define INT_FMT                                     "%d"

typedef int32_t         mp_int_t;                   // must be pointer size
typedef unsigned int    mp_uint_t;                  // must be pointer size
typedef long            mp_off_t;


/* 包含M480.h会导致宏定义“PC”冲突，无法使用__set_PRIMASK、__get_PRIMASK、__disable_irq三函数
 * 因此直接展开其内容使用
 */
static inline void enable_irq(mp_uint_t state) {
    //__set_PRIMASK(state);
    __asm volatile ("MSR primask, %0" : : "r" (state) : "memory");
}

static inline mp_uint_t disable_irq(void) {
    //mp_uint_t state = __get_PRIMASK();
    mp_uint_t state;
    __asm volatile ("MRS %0, primask" : "=r" (state) );

    //__disable_irq();
    __asm volatile ("cpsid i" : : : "memory");

    return state;
}

#define MICROPY_EVENT_POLL_HOOK             {}
#define MICROPY_BEGIN_ATOMIC_SECTION()      disable_irq()
#define MICROPY_END_ATOMIC_SECTION(state)   enable_irq(state)

#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)


// There is no classical C heap in bare-metal ports, only Python garbage-collected heap.
// For completeness, emulate C heap via GC heap. Note that MicroPython core never uses malloc()
// and friends, so these defines are mostly to help extension module writers.
#define malloc(n) m_malloc(n)
#define free(p) m_free(p)
#define realloc(p, n) m_realloc(p, n)

// We need to provide a declaration/definition of alloca()
#include <alloca.h>		// alloca()在栈中分配空间，离开此函数时自动释放
