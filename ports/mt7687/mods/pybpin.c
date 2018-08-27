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
#include "py/gc.h"
#include "pybpin.h"
#include "mperror.h"


/// \moduleref pyb
/// \class Pin - control I/O pins
///


/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/

// C API used to convert a user-supplied pin name into an ordinal pin number.
pin_obj_t *pin_find(mp_obj_t user_obj) {
    pin_obj_t *pin_obj;

    // if a pin was provided, use it
    if (MP_OBJ_IS_TYPE(user_obj, &pyb_pin_type)) {
        pin_obj = user_obj;
        return pin_obj;
    }

    // otherwise see if the pin name matches a cpu pin
    pin_obj = pin_find_named_pin(&pin_board_pins_locals_dict, user_obj);
    if (pin_obj) {
        return pin_obj;
    }

    mp_raise_ValueError(mpexception_value_invalid_arguments);
}

void pin_config (pin_obj_t *self, int af, uint dir, uint pull, int value) {
    self->dir  = dir;
    self->pull = pull;

    // if af is -1, then we want to keep it as it is
    if (af != -1) {
        self->af = af;
    }

    // if value is -1, then we want to keep it as it is
    if (value != -1) {
        self->value = value;
    }

    // mark the pin as used
    self->used = true;
    pin_obj_configure ((const pin_obj_t *)self);
}


/******************************************************************************
DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
STATIC pin_obj_t *pin_find_named_pin(const mp_obj_dict_t *named_pins, mp_obj_t name) {
    mp_map_t *named_map = mp_obj_dict_get_map((mp_obj_t)named_pins);
    mp_map_elem_t *named_elem = mp_map_lookup(named_map, name, MP_MAP_LOOKUP);
    if (named_elem != NULL && named_elem->value != NULL) {
        return named_elem->value;
    }
    return NULL;
}

STATIC pin_obj_t *pin_find_pin_by_port_bit (const mp_obj_dict_t *named_pins, uint port, uint bit) {
    mp_map_t *named_map = mp_obj_dict_get_map((mp_obj_t)named_pins);
    for (uint i = 0; i < named_map->used; i++) {
        if ((((pin_obj_t *)named_map->table[i].value)->port == port) &&
                (((pin_obj_t *)named_map->table[i].value)->bit == bit)) {
            return named_map->table[i].value;
        }
    }
    return NULL;
}

STATIC int8_t pin_obj_find_af (const pin_obj_t* pin, uint8_t fn, uint8_t unit, uint8_t type) {
    for (int i = 0; i < pin->num_afs; i++) {
        if (pin->af_list[i].fn == fn && pin->af_list[i].unit == unit && pin->af_list[i].type == type) {
            return pin->af_list[i].idx;
        }
    }
    return -1;
}



STATIC void pin_obj_configure (const pin_obj_t *self) {
    uint32_t type;
    if (self->mode == PIN_TYPE_ANALOG) {
        type = PIN_TYPE_ANALOG;
    } else {
        type = self->pull;
        uint32_t direction = self->mode;
        if (direction == PIN_TYPE_OD || direction == GPIO_DIR_MODE_ALT_OD) {
            direction = GPIO_DIR_MODE_OUT;
            type |= PIN_TYPE_OD;
        }
        if (self->mode != GPIO_DIR_MODE_ALT && self->mode != GPIO_DIR_MODE_ALT_OD) {
            // enable the peripheral clock for the GPIO port of this pin
            switch (self->port) {
            case PORT_A0:
                MAP_PRCMPeripheralClkEnable(PRCM_GPIOA0, PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);
                break;
            case PORT_A1:
                MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);
                break;
            case PORT_A2:
                MAP_PRCMPeripheralClkEnable(PRCM_GPIOA2, PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);
                break;
            case PORT_A3:
                MAP_PRCMPeripheralClkEnable(PRCM_GPIOA3, PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);
                break;
            default:
                break;
            }
            // configure the direction
            MAP_GPIODirModeSet(self->port, self->bit, direction);
            // set the pin value
            if (self->value) {
                MAP_GPIOPinWrite(self->port, self->bit, self->bit);
            } else {
                MAP_GPIOPinWrite(self->port, self->bit, 0);
            }
        }
        // now set the alternate function
        MAP_PinModeSet (self->pin_num, self->af);
    }
    MAP_PinConfigSet(self->pin_num, self->strength, type);
}

/******************************************************************************/
// MicroPython bindings

STATIC const mp_arg_t pin_init_args[] = {
    { MP_QSTR_dir,                         MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_alt,      MP_ARG_KW_ONLY  |  MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_pull,     MP_ARG_KW_ONLY  |  MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_value,    MP_ARG_KW_ONLY  |  MP_ARG_INT, {.u_int = 0} },
};
#define pin_INIT_NUM_ARGS MP_ARRAY_SIZE(pin_init_args)

STATIC mp_obj_t pin_obj_init_helper(pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    mp_arg_val_t args[pin_INIT_NUM_ARGS];
    mp_arg_parse_all(n_args, pos_args, kw_args, pin_INIT_NUM_ARGS, pin_init_args, args);

    // get the io dir
    uint dir;
    if (args[0].u_obj == MP_OBJ_NULL) {
        dir = GPIO_DIR_IN;
    } else {
        mp_obj_get_s
        dir = mp_obj_get_int(args[0].u_obj);
    }

    // get the pull type
    uint pull;
    if (args[1].u_obj == mp_const_none) {
        pull = PIN_TYPE_STD;
    } else {
        pull = mp_obj_get_int(args[1].u_obj);
        pin_validate_pull (pull);
    }

    // get the value
    int value = -1;
    if (args[2].u_obj != MP_OBJ_NULL) {
        if (mp_obj_is_true(args[2].u_obj)) {
            value = 1;
        } else {
            value = 0;
        }
    }

    // get the alternate function
    int af = args[4].u_int;
    if (mode != GPIO_DIR_MODE_ALT && mode != GPIO_DIR_MODE_ALT_OD) {
        if (af == -1) {
            af = 0;
        } else {
            goto invalid_args;
        }
    } else if (af < -1 || af > 15) {
        goto invalid_args;
    }

    // check for a valid af and then free it from any other pins
    if (af > PIN_MODE_0) {
        uint8_t fn, unit, type;
        pin_validate_af (self, af, &fn, &unit, &type);
        pin_free_af_from_pins(fn, unit, type);
    }
    pin_config (self, af, dir, pull, value);

    return mp_const_none;

invalid_args:
    mp_raise_ValueError(mpexception_value_invalid_arguments);
}

STATIC void pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pin_obj_t *self = self_in;

    // pin name
    mp_printf(print, "Pin('%q'", self->name);

    if(self->af == 8)
    {
        mp_printf(print, ", dir=%s)", (dir == GPIO_DIR_IN) ? "in" : "out");
    }
    else
    {
        mp_printf(print, ", alf=%q)", self->af_list[self->af].name);
    }
}

STATIC mp_obj_t pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    pin_obj_t *pin = (pin_obj_t *)pin_find(args[0]);

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    pin_obj_init_helper(pin, n_args - 1, args + 1, &kw_args);

    return (mp_obj_t)pin;
}

STATIC mp_obj_t pin_value(size_t n_args, const mp_obj_t *args) {
    pin_obj_t *self = args[0];
    if (n_args == 1) {
        // get the pin value
        return MP_OBJ_NEW_SMALL_INT(GPIO_GetBit(self->index));
    } else {
        // set the pin value
        if (mp_obj_is_true(args[1])) {
            self->value = 1;
            GPIO_SetBit(self->index);
        } else {
            self->value = 0;
            GPIO_ClrBit(self->index);
        }
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pin_value_obj, 1, 2, pin_value);


STATIC const mp_rom_map_elem_t pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_value),                   MP_ROM_PTR(&pin_value_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_IN),                      MP_ROM_INT(GPIO_DIR_MODE_IN) },
    { MP_ROM_QSTR(MP_QSTR_OUT),                     MP_ROM_INT(GPIO_DIR_MODE_OUT) },
};

STATIC MP_DEFINE_CONST_DICT(pin_locals_dict, pin_locals_dict_table);


const mp_obj_type_t pyb_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pin,
    .print = pin_print,
    .make_new = pin_make_new,
    .locals_dict = (mp_obj_t)&pin_locals_dict,
};
