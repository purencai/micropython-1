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

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "chip/wm_regs.h"
#include "chip/driver/wm_pwm.h"
#include "chip/driver/wm_gpio_afsel.h"

#include "mods/pybpwm.h"


/// \moduleref pyb
/// \class PWM - Pulse-Width Modulation

typedef enum {
    PYB_PWM_0      =  0,
    PYB_PWM_1      =  1,
    PYB_PWM_2      =  2,
    PYB_PWM_3      =  3,
    PYB_PWM_4      =  4,
    PYB_NUM_PWMS
} pyb_pwm_id_t;

typedef struct _pyb_pwm_obj_t {
    mp_obj_base_t base;
    pyb_pwm_id_t pwm_id;
    uint clkdiv;
    uint period;
    uint duty;
} pyb_pwm_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_pwm_obj_t pyb_pwm_obj[PYB_NUM_PWMS] = { { .pwm_id = PYB_PWM_0 },
                                                   { .pwm_id = PYB_PWM_1 },
                                                   { .pwm_id = PYB_PWM_2 },
                                                   { .pwm_id = PYB_PWM_3 },
                                                   { .pwm_id = PYB_PWM_4 }};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void pyb_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_pwm_obj_t *self = self_in;

    mp_printf(print, "PWM(%d, clkdiv=%u, period=%u, duty=%u)", self->pwm_id, self->clkdiv, self->period, self->duty);
}

STATIC const mp_arg_t pyb_pwm_init_args[] = {
    { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1} },
    { MP_QSTR_clkdiv,   MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 40000} },
    { MP_QSTR_period,   MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 100} },
    { MP_QSTR_duty,     MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int =  30} },
};
STATIC mp_obj_t pyb_pwm_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_pwm_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), pyb_pwm_init_args, args);

    uint pwm_id = args[0].u_int;
    if(pwm_id >= PYB_NUM_PWMS) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_pwm_obj_t *self = &pyb_pwm_obj[pwm_id];
    self->base.type = &pyb_pwm_type;

    self->clkdiv = args[1].u_int;
    self->period = args[2].u_int;
    self->duty   = args[3].u_int;
    if((self->clkdiv > 65535) || (self->period > 255) || (self->duty > self->period))
    {
        goto invalid_args;
    }

    switch(self->pwm_id) {
    case PYB_PWM_0:
        wm_pwm1_config(WM_IO_PB_18);
        break;

    case PYB_PWM_1:
        wm_pwm2_config(WM_IO_PB_17);
        break;

    case PYB_PWM_2:
        wm_pwm3_config(WM_IO_PB_16);
        break;

    case PYB_PWM_3:
        wm_pwm4_config(WM_IO_PB_15);
        break;

    case PYB_PWM_4:
        wm_pwm5_config(WM_IO_PB_14);
        break;

    default:
        break;
    }

    pwm_init_param param;
    memset(&param, 0, sizeof(pwm_init_param));
    param.mode = WM_PWM_OUT_MODE_INDPT;
    param.channel = self->pwm_id;
    param.clkdiv = self->clkdiv;
    param.period = self->period;
    param.duty = self->duty;
    param.dten = 0;
    param.cnt_type = WM_PWM_CNT_TYPE_EDGE_ALIGN_OUT;
    param.loop_type = WM_PWM_LOOP_TYPE_LOOP;
    param.inverse_en = false;
    param.pnum_int = 0;
    tls_pwm_out_init(param);

    return self;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}

STATIC mp_obj_t pyb_pwm_start(mp_obj_t self_in) {
    pyb_pwm_obj_t *self = self_in;

    tls_pwm_start(self->pwm_id);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_pwm_start_obj, pyb_pwm_start);

STATIC mp_obj_t pyb_pwm_stop(mp_obj_t self_in) {
    pyb_pwm_obj_t *self = self_in;

    tls_pwm_stop(self->pwm_id);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_pwm_stop_obj, pyb_pwm_stop);

STATIC mp_obj_t pyb_pwm_clkdiv(size_t n_args, const mp_obj_t *args) {
    pyb_pwm_obj_t *self = args[0];
    if (n_args == 1) {
        return MP_OBJ_NEW_SMALL_INT(self->clkdiv);
    } else {
        self->clkdiv = mp_obj_get_int(args[1]);
        tls_pwm_freq_config(self->pwm_id, self->clkdiv, self->period);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_clkdiv_obj, 1, 2, pyb_pwm_clkdiv);

STATIC mp_obj_t pyb_pwm_period(size_t n_args, const mp_obj_t *args) {
    pyb_pwm_obj_t *self = args[0];
    if (n_args == 1) {
        return MP_OBJ_NEW_SMALL_INT(self->period);
    } else {
        self->period = mp_obj_get_int(args[1]);
        tls_pwm_freq_config(self->pwm_id, self->clkdiv, self->period);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_period_obj, 1, 2, pyb_pwm_period);

STATIC mp_obj_t pyb_pwm_duty(size_t n_args, const mp_obj_t *args) {
    pyb_pwm_obj_t *self = args[0];
    if (n_args == 1) {
        return MP_OBJ_NEW_SMALL_INT(self->duty);
    } else {
        self->duty = mp_obj_get_int(args[1]);
        tls_pwm_duty_config(self->pwm_id, self->duty);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_duty_obj, 1, 2, pyb_pwm_duty);


STATIC const mp_rom_map_elem_t pyb_pwm_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_start),               MP_ROM_PTR(&pyb_pwm_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),                MP_ROM_PTR(&pyb_pwm_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_clkdiv),              MP_ROM_PTR(&pyb_pwm_clkdiv_obj) },
    { MP_ROM_QSTR(MP_QSTR_period),              MP_ROM_PTR(&pyb_pwm_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty),                MP_ROM_PTR(&pyb_pwm_duty_obj) },

    // class constants
};

STATIC MP_DEFINE_CONST_DICT(pyb_pwm_locals_dict, pyb_pwm_locals_dict_table);

const mp_obj_type_t pyb_pwm_type = {
    { &mp_type_type },
    .name = MP_QSTR_PWM,
    .print = pyb_pwm_print,
    .make_new = pyb_pwm_make_new,
    .locals_dict = (mp_obj_t)&pyb_pwm_locals_dict,
};
