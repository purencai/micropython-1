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

#include <time.h>

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include "chip/wm_regs.h"
#include "chip/driver/wm_rtc.h"

#include "mods/pybrtc.h"


/// \moduleref pyb
/// \class RTC - real time clock

/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
typedef struct _pyb_rtc_obj_t {
    mp_obj_base_t base;
} pyb_rtc_obj_t;


STATIC pyb_rtc_obj_t pyb_rtc_obj;


/******************************************************************************
 DECLARE PUBLIC FUNCTIONS
 ******************************************************************************/
uint32_t pyb_rtc_get_seconds(void) {
    struct tm tm;

    tls_get_rtc(&tm);

    return mktime(&tm);
}


/******************************************************************************
 DECLARE PRIVATE FUNCTIONS
 ******************************************************************************/
STATIC mp_obj_t pyb_rtc_set(mp_obj_t self_in, mp_obj_t datetime);


/******************************************************************************/
// MicroPython bindings

STATIC const mp_arg_t pyb_rtc_init_args[] = {
    { MP_QSTR_datetime,  MP_ARG_OBJ,  {.u_obj = mp_const_none} },
};
STATIC mp_obj_t pyb_rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_rtc_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(pyb_rtc_init_args), pyb_rtc_init_args, args);

    // setup the object
    pyb_rtc_obj_t *self = &pyb_rtc_obj;
    self->base.type = &pyb_rtc_type;

    if(args[0].u_obj != mp_const_none)
    {
        pyb_rtc_set((mp_obj_t)self, args[0].u_obj);
    }

    return self;
}


STATIC mp_obj_t pyb_rtc_now(mp_obj_t self_in) {
    struct tm tm;

    tls_get_rtc(&tm);
    mp_obj_t tuple[8] = {
        MP_OBJ_NEW_SMALL_INT(tm.tm_year+1970),
        MP_OBJ_NEW_SMALL_INT(tm.tm_mon),
        MP_OBJ_NEW_SMALL_INT(tm.tm_mday),
        MP_OBJ_NEW_SMALL_INT(tm.tm_hour),
        MP_OBJ_NEW_SMALL_INT(tm.tm_min),
        MP_OBJ_NEW_SMALL_INT(tm.tm_sec),
        MP_OBJ_NEW_SMALL_INT(0),
        MP_OBJ_NEW_SMALL_INT(0),
    };

    return mp_obj_new_tuple(8, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_rtc_now_obj, pyb_rtc_now);


/// The 8-tuple has the same format as CPython's datetime object:
///
///     (year, month, day, hours, minutes, seconds, milliseconds, tzinfo=None)
///
STATIC mp_obj_t pyb_rtc_set(mp_obj_t self_in, mp_obj_t datetime) {
    size_t len;
    mp_obj_t *items;
    mp_obj_get_array(datetime, &len, &items);

    struct tm tm;
    tm.tm_year = mp_obj_get_int(items[0])-1970;
    tm.tm_mon  = mp_obj_get_int(items[1]);
    tm.tm_mday = mp_obj_get_int(items[2]);
    tm.tm_hour = mp_obj_get_int(items[3]);
    tm.tm_min  = mp_obj_get_int(items[4]);
    tm.tm_sec  = mp_obj_get_int(items[5]);

    tls_set_rtc(&tm);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_rtc_set_obj, pyb_rtc_set);


STATIC const mp_rom_map_elem_t pyb_rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_now),      MP_ROM_PTR(&pyb_rtc_now_obj) },
    { MP_ROM_QSTR(MP_QSTR_set),      MP_ROM_PTR(&pyb_rtc_set_obj) },

    // class constants
};
STATIC MP_DEFINE_CONST_DICT(pyb_rtc_locals_dict, pyb_rtc_locals_dict_table);


const mp_obj_type_t pyb_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .make_new = pyb_rtc_make_new,
    .locals_dict = (mp_obj_t)&pyb_rtc_locals_dict,
};
