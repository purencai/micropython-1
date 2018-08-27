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
#ifndef MICROPY_INCLUDED_MT7687_MODS_PYBPIN_H
#define MICROPY_INCLUDED_MT7687_MODS_PYBPIN_H


typedef struct {
    qstr name;
    uint8_t index;
} pin_af_t;

typedef struct {
    const mp_obj_base_t base;
    const qstr          name;
    const uint8_t       index;
    const pin_af_t      *af_list;
    const uint8_t       af_count;
    uint8_t             af;
    uint8_t             dir;
    uint8_t             pull;
    uint8_t             value;
    uint8_t             used;
    uint8_t             irq_trigger;
    uint8_t             irq_flags;
} pin_obj_t;

extern const mp_obj_type_t pyb_pin_type;



pin_obj_t *pin_find(mp_obj_t user_obj);
void pin_config(pin_obj_t *self, int af, uint mode, uint type, int value, uint strength);


#endif // MICROPY_INCLUDED_MT7687_MODS_PYBPIN_H
