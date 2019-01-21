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
#ifndef MICROPY_INCLUDED_W600_MODS_MODWLAN_H
#define MICROPY_INCLUDED_W600_MODS_MODWLAN_H

#define MODWLAN_SSID_LEN_MAX	32

/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    MODWLAN_OK = 0,
    MODWLAN_ERROR_INVALID_PARAMS = -1,
    MODWLAN_ERROR_TIMEOUT = -2,
    MODWLAN_ERROR_UNKNOWN = -3,
} modwlan_Status_t;

typedef struct _wlan_obj_t {
    mp_obj_base_t       base;
    mp_obj_t            irq_obj;
    uint32_t            status;

    uint32_t            ip;

    int8_t              mode;
    uint8_t             auth;
    uint8_t             channel;
    uint8_t             antenna;

    // my own ssid, key and mac
    uint8_t             ssid[(MODWLAN_SSID_LEN_MAX + 1)];
    uint8_t             key[65];
    uint8_t             mac[ETH_ALEN];

    // the sssid (or name) and mac of the other device
    uint8_t             ssid_o[33];
    uint8_t             bssid[6];
    uint8_t             irq_flags;
    bool                irq_enabled;

#if (MICROPY_PORT_HAS_TELNET || MICROPY_PORT_HAS_FTP)
    bool                servers_enabled;
#endif
} wlan_obj_t;


#endif // MICROPY_INCLUDED_W600_MODS_MODWLAN_H
