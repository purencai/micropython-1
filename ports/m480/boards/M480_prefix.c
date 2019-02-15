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

// SWM320_prefix.c becomes the initial portion of the generated pins file.

#include <stdio.h>
#include <stdint.h>

#include "py/mpconfig.h"
#include "py/obj.h"

#include "chip/M480.h"

#include "pybpin.h"


#define AF(_name, _value, _mask, _reg) \
{ \
	.name  = MP_QSTR_ ## _name, \
	.value = (_value), \
    .mask  = (_mask), \
	.reg   = (_reg), \
}


#define PIN(_name, _port, _pbit, _preg, _afs, _IRQn) \
{ \
    { &pin_type }, \
    .name         = MP_QSTR_ ## _name, \
    .port         = (_port), \
    .pbit         = (_pbit), \
    .preg         = (_preg), \
    .af           = 0, \
    .afs          = _afs, \
    .afn          = sizeof(_afs) / sizeof(_afs[0]), \
    .dir          = 0, \
    .pull         = 0, \
    .IRQn         = _IRQn, \
    .irq_trigger  = 0, \
    .irq_priority = 0, \
    .irq_callback = 0, \
}
