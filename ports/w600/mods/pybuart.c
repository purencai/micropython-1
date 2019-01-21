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
#include "py/objlist.h"
#include "py/stream.h"
#include "py/mphal.h"

#include "chip/wm_regs.h"
#include "chip/driver/wm_uart.h"
#include "chip/driver/wm_gpio_afsel.h"

#include "mods/pybuart.h"
#include "lib/utils/interrupt_char.h"

/// \moduleref pyb
/// \class UART - duplex serial communication bus

/******************************************************************************
 DEFINE PRIVATE TYPES
 ******************************************************************************/
struct _pyb_uart_obj_t {
    mp_obj_base_t base;
    pyb_uart_id_t uart_id;
    struct tls_uart_port *UARTx;
    uint baudrate;
    byte databits;
    byte parity;
    byte stopbits;
};

/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
extern struct tls_uart_port uart_port[3];
STATIC pyb_uart_obj_t pyb_uart_obj[PYB_NUM_UARTS] = {
    {.uart_id = PYB_UART_0, .UARTx = &uart_port[0], .baudrate = 0},
    {.uart_id = PYB_UART_1, .UARTx = &uart_port[1], .baudrate = 0},
    {.uart_id = PYB_UART_2, .UARTx = &uart_port[2], .baudrate = 0}
};


/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void uart_init0(void) {
    // save references of the UART objects, to prevent the read buffers from being trashed by the gc
    MP_STATE_PORT(pyb_uart_objs)[0] = &pyb_uart_obj[0];
    MP_STATE_PORT(pyb_uart_objs)[1] = &pyb_uart_obj[1];
    MP_STATE_PORT(pyb_uart_objs)[2] = &pyb_uart_obj[2];
}

uint32_t uart_rx_any(pyb_uart_obj_t *self) {
    if(self->UARTx->recv.head >= self->UARTx->recv.tail)
        return self->UARTx->recv.head - self->UARTx->recv.tail;
    else
        return TLS_UART_RX_BUF_SIZE + self->UARTx->recv.tail - self->UARTx->recv.head;
}

int uart_rx_char(pyb_uart_obj_t *self) {
    uint8_t ch;
    tls_uart_read(self->uart_id, &ch, 1);

    return ch;
}

bool uart_tx_char(pyb_uart_obj_t *self, char ch) {
    tls_uart_write(self->uart_id, &ch, 1);

    return true;
}

bool uart_tx_strn(pyb_uart_obj_t *self, const char *str, uint len) {
    tls_uart_write(self->uart_id, str, len);

    return true;
}


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
// Waits at most timeout microseconds for at least 1 char to become ready for reading
// Returns true if something available, false if not.
STATIC bool uart_rx_wait (pyb_uart_obj_t *self) {
    int timeout = ((11 * 1000000) / self->baudrate) * 10;   // 10 charaters time
    while(1) {
        if(uart_rx_any(self)) {
            return true;   // we have at least 1 char ready for reading
        }

        if(timeout > 0) {
            mp_hal_delay_us(1);
            timeout--;
        }
        else {
            return false;
        }
    }
}


/******************************************************************************/
/* MicroPython bindings                                                      */

STATIC void pyb_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_uart_obj_t *self = self_in;

    mp_printf(print, "UART(%u, baudrate=%u, bits=%u", self->uart_id, self->baudrate, self->databits);

    switch(self->parity) {
    case TLS_UART_PMODE_DISABLED: mp_print_str(print, ", parity=None");  break;
    case TLS_UART_PMODE_ODD:      mp_print_str(print, ", parity=Odd");   break;
    case TLS_UART_PMODE_EVEN:     mp_print_str(print, ", parity=Even");  break;
    case TLS_UART_PMODE_MARK:     mp_print_str(print, ", parity=Mask");  break;
    case TLS_UART_PMODE_SPACE:    mp_print_str(print, ", parity=Space"); break;
    default: break;
    }

    mp_printf(print, ", stop=%u)", self->stopbits);
}

STATIC mp_obj_t pyb_uart_init_helper(pyb_uart_obj_t *self, const mp_arg_val_t *args) {
    self->baudrate = args[0].u_int;

    self->databits = args[1].u_int;
    if((self->databits < 5) || (self->databits > 8)) goto invalid_arg;

    self->parity   = args[2].u_int;

    self->stopbits = args[3].u_int;
    if((self->stopbits < 1) || (self->stopbits > 2)) goto invalid_arg;

    switch(self->uart_id) {
    case PYB_UART_0:
        wm_uart0_tx_config(WM_IO_PA_04);
        wm_uart0_rx_config(WM_IO_PA_05);
        break;

    case PYB_UART_1:
        wm_uart1_rx_config(WM_IO_PB_11);
        wm_uart1_tx_config(WM_IO_PB_12);
        break;

    case PYB_UART_2:
        wm_uart2_rx_config(WM_IO_PA_00);
        wm_uart2_tx_scio_config(WM_IO_PA_01);
        break;

    default:
        break;
    }

    tls_uart_options_t option;
    option.baudrate = self->baudrate;
    switch(self->databits) {
    case 8:  option.charlength = TLS_UART_CHSIZE_8BIT; break;
    case 7:  option.charlength = TLS_UART_CHSIZE_7BIT; break;
    case 6:  option.charlength = TLS_UART_CHSIZE_6BIT; break;
    case 5:  option.charlength = TLS_UART_CHSIZE_5BIT; break;
    default: option.charlength = TLS_UART_CHSIZE_8BIT; break;
    }
    option.paritytype = self->parity;
    switch(self->stopbits) {
    case 1:  option.stopbits = TLS_UART_ONE_STOPBITS;  break;
    case 2:  option.stopbits = TLS_UART_TWO_STOPBITS;  break;
    default: option.stopbits = TLS_UART_ONE_STOPBITS;  break;
    }
    option.flow_ctrl  = TLS_UART_FLOW_CTRL_NONE;
    tls_uart_port_init(self->uart_id, &option, 0);

    return mp_const_none;

invalid_arg:
    mp_raise_ValueError("invalid arguments");
}

STATIC const mp_arg_t pyb_uart_init_args[] = {
    { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1} },
    { MP_QSTR_baudrate, MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 57600} },
    { MP_QSTR_bits,     MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 8} },
    { MP_QSTR_parity,   MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = TLS_UART_PMODE_DISABLED} },
    { MP_QSTR_stop,     MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 1} },
};
STATIC mp_obj_t pyb_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_uart_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), pyb_uart_init_args, args);

    // get the correct uart instance
    if(args[0].u_int >= PYB_NUM_UARTS) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_uart_obj_t *self = &pyb_uart_obj[args[0].u_int];
    self->base.type = &pyb_uart_type;

    pyb_uart_init_helper(self, &args[1]);

    return self;
}

STATIC mp_obj_t pyb_uart_any(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;

    return mp_obj_new_int(uart_rx_any(self));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_any_obj, pyb_uart_any);

STATIC mp_uint_t pyb_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    byte *buf = buf_in;

    // wait for first char to become available
    if (!uart_rx_wait(self)) {
        // return MP_EAGAIN error to indicate non-blocking (then read() method returns None)
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    return tls_uart_read(self->uart_id, buf, size);
}

STATIC mp_uint_t pyb_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    const char *buf = buf_in;

    if (!uart_tx_strn(self, buf, size)) {
        mp_raise_OSError(MP_EIO);
    }

    return size;
}

STATIC mp_uint_t pyb_uart_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    mp_uint_t ret;

    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && uart_rx_any(self)) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR)) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

STATIC const mp_stream_p_t uart_stream_p = {
    .read = pyb_uart_read,
    .write = pyb_uart_write,
    .ioctl = pyb_uart_ioctl,
    .is_text = false,
};


STATIC const mp_rom_map_elem_t pyb_uart_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_any),         MP_ROM_PTR(&pyb_uart_any_obj) },

    /// \method read([nbytes])
    { MP_ROM_QSTR(MP_QSTR_read),        MP_ROM_PTR(&mp_stream_read_obj) },
    /// \method readline()
    { MP_ROM_QSTR(MP_QSTR_readline),    MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    /// \method readinto(buf[, nbytes])
    { MP_ROM_QSTR(MP_QSTR_readinto),    MP_ROM_PTR(&mp_stream_readinto_obj) },
    /// \method write(buf)
    { MP_ROM_QSTR(MP_QSTR_write),       MP_ROM_PTR(&mp_stream_write_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_NONE),        MP_ROM_INT(TLS_UART_PMODE_DISABLED) },
    { MP_ROM_QSTR(MP_QSTR_ODD),         MP_ROM_INT(TLS_UART_PMODE_ODD) },
    { MP_ROM_QSTR(MP_QSTR_EVEN),        MP_ROM_INT(TLS_UART_PMODE_EVEN) },
    { MP_ROM_QSTR(MP_QSTR_MASK),        MP_ROM_INT(TLS_UART_PMODE_MARK) },
    { MP_ROM_QSTR(MP_QSTR_SPACE),       MP_ROM_INT(TLS_UART_PMODE_SPACE) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_uart_locals_dict, pyb_uart_locals_dict_table);

const mp_obj_type_t pyb_uart_type = {
    { &mp_type_type },
    .name = MP_QSTR_UART,
    .print = pyb_uart_print,
    .make_new = pyb_uart_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &uart_stream_p,
    .locals_dict = (mp_obj_t)&pyb_uart_locals_dict,
};
