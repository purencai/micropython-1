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
#include <string.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include "chip/driver/wm_gpio_afsel.h"
#include "chip/driver/wm_hostspi.h"

#include "mods/pybspi.h"

#include "misc/bufhelper.h"


/// \moduleref pyb
/// \class SPI - a master-driven serial protocol

/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_SPI_0      =  0,
    PYB_NUM_SPIS
} pyb_spi_id_t;

typedef struct _pyb_spi_obj_t {
    mp_obj_base_t base;
    pyb_spi_id_t spi_id;
    uint baudrate;
    byte polarity;
    byte phase;
    bool slave;
    byte bits;
} pyb_spi_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_spi_obj_t pyb_spi_obj[PYB_NUM_SPIS] = { {.spi_id = PYB_SPI_0} };


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void pyb_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_spi_obj_t *self = self_in;

    mp_printf(print, "SPI(%u, baudrate=%u, bits=%u, polarity=%u, phase=%u, mode=%s)",
              self->spi_id, self->baudrate, self->bits, self->polarity, self->phase, self->slave ? "slave" : "master");
}

static const mp_arg_t pyb_spi_init_args[] = {
    { MP_QSTR_id,        MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 0} },
    { MP_QSTR_baudrate,  MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1000000} },    // 1MHz
    { MP_QSTR_polarity,  MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 0} },
    { MP_QSTR_phase,     MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 0} },
    { MP_QSTR_slave,     MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool= false} },
    { MP_QSTR_bits,      MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 8} },
};
STATIC mp_obj_t pyb_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_spi_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(pyb_spi_init_args), pyb_spi_init_args, args);

    // check the peripheral id
    uint32_t spi_id = args[0].u_int;
    if (spi_id >= PYB_NUM_SPIS) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_spi_obj_t *self = &pyb_spi_obj[spi_id];
    self->base.type = &pyb_spi_type;

    self->baudrate = args[1].u_int;

    self->polarity = args[2].u_int;
    self->phase    = args[3].u_int;

    self->slave    = args[4].u_bool;
    if(self->slave == true)
    {
        goto invalid_arg;
    }

    self->bits     = args[5].u_int;
    if(self->bits != 8)
    {
        goto invalid_arg;
    }

    switch(self->spi_id) {
    case PYB_SPI_0:
        wm_spi_ck_config(WM_IO_PB_16);
        wm_spi_cs_config(WM_IO_PB_15);
        wm_spi_di_config(WM_IO_PB_17);
        wm_spi_do_config(WM_IO_PB_18);
        break;

    default:
        break;
    }

    tls_spi_init();
    tls_spi_setup(self->phase | (self->polarity << 1), TLS_SPI_CS_LOW, self->baudrate);

    return self;

invalid_arg:
    mp_raise_ValueError("invalid argument(s) value");
}


STATIC mp_obj_t pyb_spi_write (mp_obj_t self_in, mp_obj_t buf)
{
    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(buf, &bufinfo, data);

    tls_spi_write((const uint8_t *)bufinfo.buf, bufinfo.len);

    return mp_obj_new_int(bufinfo.len);     // return the number of bytes written
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_spi_write_obj, pyb_spi_write);


static const mp_arg_t pyb_spi_read_args[] = {
    { MP_QSTR_nbytes,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_write,                     MP_ARG_INT, {.u_int = 0xFFFF} },
};
STATIC mp_obj_t pyb_spi_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_spi_read_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_spi_read_args), pyb_spi_read_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[0].u_obj, &vstr);

    tls_spi_read((uint8_t *)vstr.buf, vstr.len);

    // return the received data
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_spi_read_obj, 1, pyb_spi_read);


static const mp_arg_t pyb_spi_readinto_args[] = {
    { MP_QSTR_buf,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_write,                     MP_ARG_INT, {.u_int = 0x00} },
};
STATIC mp_obj_t pyb_spi_readinto(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_spi_readinto_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_spi_readinto_args), pyb_spi_readinto_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[0].u_obj, &vstr);

    tls_spi_read((uint8_t *)vstr.buf, vstr.len);

    // return the number of bytes received
    return mp_obj_new_int(vstr.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_spi_readinto_obj, 1, pyb_spi_readinto);


STATIC mp_obj_t pyb_spi_write_readinto (mp_obj_t self, mp_obj_t writebuf, mp_obj_t readbuf) {
    // get buffers to write from/read to
    mp_buffer_info_t bufinfo_write;
    uint8_t data_send[1];
    mp_buffer_info_t bufinfo_read;

    if (writebuf == readbuf) {
        // same object for writing and reading, it must be a r/w buffer
        mp_get_buffer_raise(writebuf, &bufinfo_write, MP_BUFFER_RW);
        bufinfo_read = bufinfo_write;
    } else {
        // get the buffer to write from
        pyb_buf_get_for_send(writebuf, &bufinfo_write, data_send);

        // get the read buffer
        mp_get_buffer_raise(readbuf, &bufinfo_read, MP_BUFFER_WRITE);
        if (bufinfo_read.len != bufinfo_write.len) {
            mp_raise_ValueError("invalid argument(s) value");
        }
    }

    tls_spi_write((const uint8_t *)bufinfo_write.buf, bufinfo_write.len);

    tls_spi_read((uint8_t *)bufinfo_read.buf, bufinfo_read.len);

    // return the number of transferred bytes
    return mp_obj_new_int(bufinfo_write.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_spi_write_readinto_obj, pyb_spi_write_readinto);


STATIC const mp_rom_map_elem_t pyb_spi_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_write),               MP_ROM_PTR(&pyb_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),                MP_ROM_PTR(&pyb_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto),            MP_ROM_PTR(&pyb_spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto),      MP_ROM_PTR(&pyb_spi_write_readinto_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_CPHA_0),              MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_CPHA_1),              MP_ROM_INT(SPI_CPHA) },
    { MP_ROM_QSTR(MP_QSTR_CPOL_0),              MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_CPOL_1),              MP_ROM_INT(SPI_CPOL) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_spi_locals_dict, pyb_spi_locals_dict_table);


const mp_obj_type_t pyb_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = pyb_spi_print,
    .make_new = pyb_spi_make_new,
    .locals_dict = (mp_obj_t)&pyb_spi_locals_dict,
};
