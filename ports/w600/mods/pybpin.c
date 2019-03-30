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
#include <stdint.h>
#include <string.h>

#include "py/runtime.h"

#include "chip/wm_regs.h"
#include "chip/wm_type_def.h"
#include "chip/driver/wm_io.h"
#include "chip/driver/wm_gpio.h"

#include "pybpin.h"

/// \moduleref pyb
/// \class Pin - control I/O pins
///

/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/

pin_obj_t *pin_find_by_name(mp_obj_t name) {
    mp_map_t *named_pins = mp_obj_dict_get_map((mp_obj_t)&pins_locals_dict);

    mp_map_elem_t *named_elem = mp_map_lookup(named_pins, name, MP_MAP_LOOKUP);
    if (named_elem != NULL && named_elem->value != NULL) {
        return (pin_obj_t *)named_elem->value;
    }

    mp_raise_ValueError("invalid argument(s) value");
}


/******************************************************************************
DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/



/******************************************************************************/
// MicroPython bindings

STATIC void pyb_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pin_obj_t *self = self_in;

    mp_printf(print, "Pin('%q', dir=%s)", self->name, (self->dir == WM_GPIO_DIR_INPUT) ? "IN" : "OUT");
}

STATIC mp_obj_t pyb_pin_init_helper(pin_obj_t *self, const mp_arg_val_t *args) {
    // get the io dir
    self->dir = args[0].u_int;

    self->alt = WM_IO_OPT5_GPIO;

    // get the pull type
    self->mode = args[1].u_int;

    // get the value
    self->value = args[2].u_int;

    tls_gpio_cfg(self->index, self->dir, self->mode);

    return mp_const_none;
}

STATIC const mp_arg_t pin_init_args[] = {
    { MP_QSTR_name,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_dir,                        MP_ARG_INT, {.u_int = WM_GPIO_DIR_INPUT} },
    { MP_QSTR_pull,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = WM_GPIO_ATTR_PULLHIGH} },
    { MP_QSTR_value,    MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
};
STATIC mp_obj_t pyb_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pin_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), pin_init_args, args);

    pin_obj_t *self = pin_find_by_name(args[0].u_obj);

    pyb_pin_init_helper(self, &args[1]);

    return self;
}

STATIC mp_obj_t pyb_pin_value(size_t n_args, const mp_obj_t *args) {
    pin_obj_t *self = args[0];
    if (n_args == 1) {
        // get the pin value
        return MP_OBJ_NEW_SMALL_INT(tls_gpio_read(self->index));
    } else {
        // set the pin value
        if (mp_obj_is_true(args[1])) {
            self->value = 1;
        } else {
            self->value = 0;
        }
        tls_gpio_write(self->index, self->value);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pin_value_obj, 1, 2, pyb_pin_value);


STATIC const mp_rom_map_elem_t pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_value),    MP_ROM_PTR(&pyb_pin_value_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_IN),          MP_ROM_INT(WM_GPIO_DIR_INPUT) },
    { MP_ROM_QSTR(MP_QSTR_OUT),         MP_ROM_INT(WM_GPIO_DIR_OUTPUT) },
    { MP_ROM_QSTR(MP_QSTR_FLOATING),    MP_ROM_INT(WM_GPIO_ATTR_FLOATING) },
    { MP_ROM_QSTR(MP_QSTR_PULLHIGH),    MP_ROM_INT(WM_GPIO_ATTR_PULLHIGH) },
    { MP_ROM_QSTR(MP_QSTR_PULLLOW),     MP_ROM_INT(WM_GPIO_ATTR_PULLLOW) },
};

STATIC MP_DEFINE_CONST_DICT(pin_locals_dict, pin_locals_dict_table);


const mp_obj_type_t pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pin,
    .print = pyb_pin_print,
    .make_new = pyb_pin_make_new,
    .locals_dict = (mp_obj_t)&pin_locals_dict,
};
