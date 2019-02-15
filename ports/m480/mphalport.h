/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
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

#ifndef MICROPY_INCLUDED_M480_MPHALPORT_H
#define MICROPY_INCLUDED_M480_MPHALPORT_H

#include <stdint.h>
#include <stdbool.h>


void mp_hal_ticks_cpu_enable(void);
static inline mp_uint_t mp_hal_ticks_cpu(void)
{
#if __CORTEX_M == 0
    return 0;
#else
    if(!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk))
    {
        mp_hal_ticks_cpu_enable();
    }

    return DWT->CYCCNT;
#endif
}


mp_uint_t mp_hal_ticks_ms(void);
mp_uint_t mp_hal_ticks_us(void);

void mp_hal_delay_ms(mp_uint_t ms);
void mp_hal_delay_us(mp_uint_t us);

void mp_hal_stdout_tx_str(const char *str);
void mp_hal_stdout_tx_strn(const char *str, size_t len);
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len);

int mp_hal_stdin_rx_chr(void);
void mp_hal_set_interrupt_char(int c);

#endif //MICROPY_INCLUDED_M480_MPHALPORT_H
