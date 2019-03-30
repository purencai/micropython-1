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
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "chip/wm_regs.h"
#include "chip/wm_type_def.h"
#include "chip/driver/wm_timer.h"

#include "pybtimer.h"


/// \moduleref pyb
/// \class Timer - generate periodic events
///

/******************************************************************************
 DEFINE PRIVATE TYPES
 ******************************************************************************/
typedef enum {
    PYB_TIMER_0      =  0,
    PYB_TIMER_1      =  1,
    PYB_TIMER_2      =  2,
    PYB_TIMER_3      =  3,
    PYB_TIMER_4      =  4,
    PYB_TIMER_5      =  5,
    PYB_NUM_TIMERS
} pyb_timer_id_t;

typedef struct _pyb_timer_obj_t {
    mp_obj_base_t base;
    pyb_timer_id_t timer_id;
    uint32_t period;        // 单位us
    mp_obj_t callback;      // 中断处理函数
} pyb_timer_obj_t;


/******************************************************************************
 DEFINE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_timer_obj_t pyb_timer_obj[PYB_NUM_TIMERS] = {{.timer_id = PYB_TIMER_0},
                                                        {.timer_id = PYB_TIMER_1},
                                                        {.timer_id = PYB_TIMER_2},
                                                        {.timer_id = PYB_TIMER_3},
                                                        {.timer_id = PYB_TIMER_4},
                                                        {.timer_id = PYB_TIMER_5}};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
static void TIMR_Handler(void *arg) {
    pyb_timer_obj_t *self = (pyb_timer_obj_t *)arg;

    if(self->callback != mp_const_none)
    {
        gc_lock();
        nlr_buf_t nlr;
        if(nlr_push(&nlr) == 0)
        {
            mp_call_function_1(self->callback, self);
            nlr_pop();
        }
        else
        {
            self->callback = mp_const_none;

            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
        gc_unlock();
    }
}

/******************************************************************************/
/* MicroPython bindings                                                      */

STATIC void pyb_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_timer_obj_t *self = self_in;

    mp_printf(print, "Timer(%d, period=%uuS)", self->timer_id, self->period);
}

STATIC const mp_arg_t pyb_timer_init_args[] = {
    { MP_QSTR_period,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 1000000} },
    { MP_QSTR_callback,   MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} }
};
STATIC mp_obj_t pyb_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_timer_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), pyb_timer_init_args, args);

    struct tls_timer_cfg cfg;
    cfg.unit = TLS_TIMER_UNIT_US;
    cfg.timeout = args[0].u_int;
    cfg.is_repeat = true;
    cfg.callback = TIMR_Handler;
    cfg.arg = (void *)0;
    uint timer_id = tls_timer_create(&cfg);
    if(timer_id == WM_TIMER_ID_INVALID)
    {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_timer_obj_t *self = &pyb_timer_obj[timer_id];
    self->base.type = &pyb_timer_type;

    self->period = args[0].u_int;

    self->callback = args[1].u_obj;

    tls_timer_set_arg(timer_id, (void *)self);

    return self;
}

STATIC mp_obj_t pyb_timer_period(size_t n_args, const mp_obj_t *args) {
    pyb_timer_obj_t *self = args[0];
    if (n_args == 1) {  // get
        return mp_obj_new_int(self->period);
    } else {            // set
        self->period = mp_obj_get_int(args[1]);
        tls_timer_change(self->timer_id, self->period);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_period_obj, 1, 2, pyb_timer_period);

STATIC mp_obj_t pyb_timer_start(mp_obj_t self_in) {
    pyb_timer_obj_t *self = self_in;

    tls_timer_start(self->timer_id);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_timer_start_obj, pyb_timer_start);

STATIC mp_obj_t pyb_timer_stop(mp_obj_t self_in) {
    pyb_timer_obj_t *self = self_in;

    tls_timer_stop(self->timer_id);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_timer_stop_obj, pyb_timer_stop);

STATIC mp_obj_t pyb_timer_destroy(mp_obj_t self_in) {
    pyb_timer_obj_t *self = self_in;

    tls_timer_destroy(self->timer_id);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_timer_destroy_obj, pyb_timer_destroy);


STATIC const mp_rom_map_elem_t pyb_timer_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_period),                  MP_ROM_PTR(&pyb_timer_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_start),                   MP_ROM_PTR(&pyb_timer_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),                    MP_ROM_PTR(&pyb_timer_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_destroy),                 MP_ROM_PTR(&pyb_timer_destroy_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_timer_locals_dict, pyb_timer_locals_dict_table);


const mp_obj_type_t pyb_timer_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    .print = pyb_timer_print,
    .make_new = pyb_timer_make_new,
    .locals_dict = (mp_obj_t)&pyb_timer_locals_dict,
};
