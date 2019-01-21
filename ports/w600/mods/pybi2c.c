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

#include "chip/driver/wm_gpio_afsel.h"
#include "chip/driver/wm_i2c.h"
#include "pybi2c.h"

#include "misc/bufhelper.h"


/// \moduleref pyb
/// \class I2C - a two-wire serial protocol

typedef enum {
    PYB_I2C_0      =  0,
    PYB_NUM_I2CS
} pyb_i2c_id_t;

typedef struct _pyb_i2c_obj_t {
    mp_obj_base_t base;
    pyb_i2c_id_t i2c_id;
    uint baudrate;
} pyb_i2c_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_i2c_obj_t pyb_i2c_obj[PYB_NUM_I2CS] = { { .i2c_id = PYB_I2C_0} };


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void pyb_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_i2c_obj_t *self = self_in;

    mp_printf(print, "I2C(%u, baudrate=%u)", self->i2c_id, self->baudrate);
}


STATIC const mp_arg_t pyb_i2c_init_args[] = {
    { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1} },
    { MP_QSTR_baudrate, MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 10000} }
};
STATIC mp_obj_t pyb_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(pyb_i2c_init_args), pyb_i2c_init_args, args);

    uint i2c_id = args[0].u_int;
    if (i2c_id >= PYB_NUM_I2CS) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_i2c_obj_t *self = &pyb_i2c_obj[i2c_id];
    self->base.type = &pyb_i2c_type;

    self->baudrate = args[1].u_int;

    switch (self->i2c_id) {
    case PYB_I2C_0:
        tls_gpio_cfg(WM_IO_PB_13, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_PULLHIGH);
        tls_gpio_cfg(WM_IO_PB_14, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_PULLHIGH);
        wm_i2c_scl_config(WM_IO_PB_13);
        wm_i2c_sda_config(WM_IO_PB_14);
        break;

    default:
        break;
    }

    tls_i2c_init(self->baudrate);

    return self;
}


STATIC mp_obj_t pyb_i2c_scan(mp_obj_t self_in) {
    mp_obj_t list = mp_obj_new_list(0, NULL);

    for(uint addr = 0x00; addr < 0x80; addr++) {
        tls_i2c_write_byte((addr << 1) | 0, 1);
        if(tls_i2c_wait_ack() == WM_SUCCESS)
        {
            mp_obj_list_append(list, mp_obj_new_int(addr));
        }
        tls_i2c_stop();
    }

    return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_scan_obj, pyb_i2c_scan);


STATIC const mp_arg_t pyb_i2c_writeto_args[] = {
    { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0x10} },
    { MP_QSTR_buf,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
};
STATIC mp_obj_t pyb_i2c_writeto(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_writeto_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_i2c_writeto_args), pyb_i2c_writeto_args, args);

    uint addr = args[0].u_int;

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[1].u_obj, &bufinfo, data);

    tls_i2c_write_byte((addr << 1) | 0, 1);
    if(tls_i2c_wait_ack() != WM_SUCCESS)
    {
        tls_i2c_stop();
        mp_raise_OSError(MP_EIO);
    }

    for(uint i = 0; i < bufinfo.len; i++)
    {
        tls_i2c_write_byte(((uint8_t *)bufinfo.buf)[i], 0);
        if(tls_i2c_wait_ack() != WM_SUCCESS)
        {
            tls_i2c_stop();
            return MP_OBJ_NEW_SMALL_INT(i);
        }
    }
    tls_i2c_stop();

    return MP_OBJ_NEW_SMALL_INT(bufinfo.len);     // return the number of bytes written
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_writeto_obj, 1, pyb_i2c_writeto);


STATIC const mp_arg_t pyb_i2c_readfrom_args[] = {
    { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0x10} },
    { MP_QSTR_nbytes,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
};
STATIC mp_obj_t pyb_i2c_readfrom(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_readfrom_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_i2c_readfrom_args), pyb_i2c_readfrom_args, args);

    uint addr = args[0].u_int;

    vstr_t vstr;
    pyb_buf_get_for_recv(args[1].u_obj, &vstr);

    tls_i2c_write_byte((addr << 1) | 1, 1);
    if(tls_i2c_wait_ack() != WM_SUCCESS)
    {
        tls_i2c_stop();
        mp_raise_OSError(MP_EIO);
    }

    for(uint i = 0; i < vstr.len; i++)
    {
        vstr.buf[i] = (char)tls_i2c_read_byte(1, 0);
    }
    tls_i2c_stop();

    // return the received data
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_readfrom_obj, 3, pyb_i2c_readfrom);


STATIC mp_obj_t pyb_i2c_readfrom_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_readfrom_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_i2c_readfrom_args), pyb_i2c_readfrom_args, args);

    uint addr = args[0].u_int;

    vstr_t vstr;
    pyb_buf_get_for_recv(args[1].u_obj, &vstr);

    tls_i2c_write_byte((addr << 1) | 1, 1);
    if(tls_i2c_wait_ack() != WM_SUCCESS)
    {
        tls_i2c_stop();
        mp_raise_OSError(MP_EIO);
    }

    for(uint i = 0; i < vstr.len; i++)
    {
        vstr.buf[i] = (char)tls_i2c_read_byte(1, 0);
    }
    tls_i2c_stop();

    return MP_OBJ_NEW_SMALL_INT(vstr.len);    // return the number of bytes received
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_readfrom_into_obj, 1, pyb_i2c_readfrom_into);


STATIC const mp_arg_t pyb_i2c_writeto_mem_args[] = {
    { MP_QSTR_addr,     MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0x10} },
    { MP_QSTR_memaddr,  MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0x00} },
    { MP_QSTR_buf,      MP_ARG_REQUIRED  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_addrsize, MP_ARG_KW_ONLY   | MP_ARG_INT, {.u_int = 8} },
};
STATIC mp_obj_t pyb_i2c_writeto_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_writeto_mem_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_i2c_writeto_mem_args), pyb_i2c_writeto_mem_args, args);

    mp_uint_t addr = args[0].u_int;

    mp_uint_t mem_addr = args[1].u_int;

    // get the buffer to write from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[2].u_obj, &bufinfo, data);

    tls_i2c_write_byte((addr << 1) | 0, 1);
    if(tls_i2c_wait_ack() != WM_SUCCESS)
    {
        tls_i2c_stop();
        mp_raise_OSError(MP_EIO);
    }

    tls_i2c_write_byte(mem_addr, 0);
    if(tls_i2c_wait_ack() != WM_SUCCESS)
    {
        tls_i2c_stop();
        mp_raise_OSError(MP_EIO);
    }

    for(uint i = 0; i < bufinfo.len; i++)
    {
        tls_i2c_write_byte(((uint8_t *)bufinfo.buf)[i], 0);
        if(tls_i2c_wait_ack() != WM_SUCCESS)
        {
            tls_i2c_stop();
            return MP_OBJ_NEW_SMALL_INT(i);
        }
    }
    tls_i2c_stop();

    return MP_OBJ_NEW_SMALL_INT(bufinfo.len);     // return the number of bytes written
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_writeto_mem_obj, 1, pyb_i2c_writeto_mem);


STATIC const mp_arg_t pyb_i2c_readfrom_mem_args[] = {
    { MP_QSTR_addr,     MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0x10} },
    { MP_QSTR_memaddr,  MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0x00} },
    { MP_QSTR_nbytes,   MP_ARG_REQUIRED  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_addrsize, MP_ARG_KW_ONLY   | MP_ARG_INT, {.u_int = 8} },
};
STATIC mp_obj_t pyb_i2c_readfrom_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_readfrom_mem_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(args), pyb_i2c_readfrom_mem_args, args);

    mp_uint_t addr = args[0].u_int;

    mp_uint_t mem_addr = args[1].u_int;

    vstr_t vstr;
    pyb_buf_get_for_recv(args[2].u_obj, &vstr);

    tls_i2c_write_byte((addr << 1) | 0, 1);
    if(tls_i2c_wait_ack() != WM_SUCCESS)
    {
        tls_i2c_stop();
        mp_raise_OSError(MP_EIO);
    }

    tls_i2c_write_byte(mem_addr, 0);
    if(tls_i2c_wait_ack() != WM_SUCCESS)
    {
        tls_i2c_stop();
        mp_raise_OSError(MP_EIO);
    }

    tls_i2c_write_byte((addr << 1) | 1, 1);
    if(tls_i2c_wait_ack() != WM_SUCCESS)
    {
        tls_i2c_stop();
        mp_raise_OSError(MP_EIO);
    }

    for(uint i = 0; i < vstr.len; i++)
    {
        vstr.buf[i] = (char)tls_i2c_read_byte(1, 0);
    }
    tls_i2c_stop();

    // return the received data
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_readfrom_mem_obj, 1, pyb_i2c_readfrom_mem);


STATIC mp_obj_t pyb_i2c_readfrom_mem_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_readfrom_mem_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_i2c_readfrom_mem_args), pyb_i2c_readfrom_mem_args, args);

    mp_uint_t addr = args[0].u_int;

    mp_uint_t mem_addr = args[1].u_int;

    vstr_t vstr;
    pyb_buf_get_for_recv(args[2].u_obj, &vstr);

    tls_i2c_write_byte((addr << 1) | 0, 1);
    if(tls_i2c_wait_ack() != WM_SUCCESS)
    {
        tls_i2c_stop();
        mp_raise_OSError(MP_EIO);
    }

    tls_i2c_write_byte(mem_addr, 0);
    if(tls_i2c_wait_ack() != WM_SUCCESS)
    {
        tls_i2c_stop();
        mp_raise_OSError(MP_EIO);
    }

    tls_i2c_write_byte((addr << 1) | 1, 1);
    if(tls_i2c_wait_ack() != WM_SUCCESS)
    {
        tls_i2c_stop();
        mp_raise_OSError(MP_EIO);
    }

    for(uint i = 0; i < vstr.len; i++)
    {
        vstr.buf[i] = (char)tls_i2c_read_byte(1, 0);
    }
    tls_i2c_stop();

    return mp_obj_new_int(vstr.len);    // return the number of bytes received
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_readfrom_mem_into_obj, 1, pyb_i2c_readfrom_mem_into);


STATIC const mp_rom_map_elem_t pyb_i2c_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_scan),                MP_ROM_PTR(&pyb_i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto),             MP_ROM_PTR(&pyb_i2c_writeto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom),            MP_ROM_PTR(&pyb_i2c_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_into),       MP_ROM_PTR(&pyb_i2c_readfrom_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto_mem),         MP_ROM_PTR(&pyb_i2c_writeto_mem_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem),        MP_ROM_PTR(&pyb_i2c_readfrom_mem_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem_into),   MP_ROM_PTR(&pyb_i2c_readfrom_mem_into_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_i2c_locals_dict, pyb_i2c_locals_dict_table);


const mp_obj_type_t pyb_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = pyb_i2c_print,
    .make_new = pyb_i2c_make_new,
    .locals_dict = (mp_obj_t)&pyb_i2c_locals_dict,
};
