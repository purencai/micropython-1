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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"

#include "rtos/include/task.h"

#include "chip/wm_mem.h"
#include "wifi/wm_wifi.h"
#include "chip/wm_netif.h"
#include "chip/tls_wireless.h"
#include "lwip/netif.h"

#include "mods/modwlan.h"
#include "mods/modnetwork.h"

#include "lib/netutils/netutils.h"


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC wlan_obj_t wlan_obj = {
        .mode = -1,
        .status = 0,
        .ip = 0,
        .auth = 0,
        .channel = 0,
        .ssid = {0},
        .key = {0},
        .mac = {0},
    #if (MICROPY_PORT_HAS_TELNET || MICROPY_PORT_HAS_FTP)
        .servers_enabled = false,
    #endif
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
static volatile bool wifi_scan_done = false;
static void wifi_scan_callback(void)
{
    wifi_scan_done = true;
}


static void netif_status_changed(uint8_t status)
{
    switch(status)
    {
        case NETIF_WIFI_JOIN_SUCCESS:
            printf("NETIF_WIFI_JOIN_SUCCESS\n");
            break;

        case NETIF_WIFI_JOIN_FAILED:
            printf("NETIF_WIFI_JOIN_FAILED\n");
            break;

        case NETIF_WIFI_DISCONNECTED:
            printf("NETIF_WIFI_DISCONNECTED\n");
            break;

        case NETIF_IP_NET_UP:
            print_ipaddr(&tls_netif_get_ethif()->ip_addr);
            break;
    }
}



/******************************************************************************/
// MicroPython bindings; WLAN class

/// \class WLAN - WiFi driver

STATIC void wlan_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    wlan_obj_t *self = self_in;

    mp_printf(print, "WLAN(mode=%s)", self->mode == STATION_MODE ? "STA" : "AP");
}


STATIC const mp_arg_t wlan_init_args[] = {
    { MP_QSTR_mode,  MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = STATION_MODE} },
    { MP_QSTR_ssid,  MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_auth,  MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
};
STATIC mp_obj_t wlan_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(wlan_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(wlan_init_args), wlan_init_args, args);

    if(args[0].u_int != STATION_MODE)
    {
        mp_raise_OSError(MP_ENODEV);
    }

    // setup the object
    wlan_obj_t *self = &wlan_obj;
    self->base.type = (mp_obj_t)&mod_network_nic_type_wlan;

    self->mode = args[0].u_int;

    tls_wifi_scan_result_cb_register(wifi_scan_callback);

    tls_netif_add_status_event(netif_status_changed);

    return self;
}


STATIC mp_obj_t wlan_scan(mp_obj_t self_in) {
    STATIC const qstr wlan_scan_info_fields[] = {
        MP_QSTR_ssid, MP_QSTR_bssid, MP_QSTR_sec, MP_QSTR_channel, MP_QSTR_rssi
    };

    wlan_obj_t *self = self_in;

    // check for correct wlan mode
    if(self->mode != STATION_MODE) {
        mp_raise_OSError(MP_EPERM);
    }

    tls_wifi_scan();

    for(uint i = 0; i < 500; i++)
    {
        vTaskDelay(10 / portTICK_RATE_MS);

        if(wifi_scan_done)
        {
            wifi_scan_done = false;
            break;
        }

        if(i == 400) goto error;
    }

    uint buflen = sizeof(struct tls_scan_bss_t) + sizeof(struct tls_bss_info_t) * 16;
    char *buf = tls_mem_alloc(buflen);
    if(!buf)
    {
        goto error;
    }

    uint res = tls_wifi_get_scan_rslt((u8 *)buf, buflen);
    if(res != WM_SUCCESS)
    {
        tls_mem_free(buf);

        goto error;
    }

    mp_obj_t nets = mp_obj_new_list(0, NULL);
    struct tls_scan_bss_t *wsr = (struct tls_scan_bss_t *)buf;
    struct tls_bss_info_t *bss_info = (struct tls_bss_info_t *)(buf + 8);

    for(uint i = 0; i < wsr->count; i++)
    {
        char bssid_str[16] = {0};
        sprintf(bssid_str, "%02X%02X%02X%02X%02X%02X", bss_info->bssid[0], bss_info->bssid[1],
                bss_info->bssid[2], bss_info->bssid[3], bss_info->bssid[4], bss_info->bssid[5]);

        mp_obj_t tuple[5];
        tuple[0] = mp_obj_new_str((const char *)bss_info->ssid, bss_info->ssid_len);
        tuple[1] = mp_obj_new_bytes((const byte *)bssid_str, strlen(bssid_str));
        tuple[2] = mp_obj_new_int(bss_info->privacy);
        tuple[3] = mp_const_none;
        tuple[4] = mp_obj_new_int(bss_info->rssi);

        mp_obj_list_append(nets, mp_obj_new_attrtuple(wlan_scan_info_fields, 5, tuple));

        bss_info++;
    }
    tls_mem_free(buf);

    return nets;

error:
    return mp_obj_new_list(0, NULL);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(wlan_scan_obj, wlan_scan);


STATIC const mp_arg_t wlan_connect_args[] = {
    { MP_QSTR_ssid,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_auth,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
};
STATIC mp_obj_t wlan_connect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    wlan_obj_t *self = pos_args[0];

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(wlan_connect_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(wlan_connect_args), wlan_connect_args, args);

    if(self->mode != STATION_MODE) {
        mp_raise_OSError(MP_EPERM);
    }

    // get the ssid
    size_t ssid_len;
    const char *ssid = mp_obj_str_get_data(args[0].u_obj, &ssid_len);
    memcpy(self->ssid, ssid, ssid_len);

    // get the key
    size_t key_len = 0;
    const char *key = mp_obj_str_get_data(args[1].u_obj, &key_len);
    memcpy(self->key, key, key_len);

    tls_wifi_connect((uint8_t *)ssid, ssid_len, (uint8_t *)key, key_len);

    for(uint i = 0; i < 200; i++)
    {
        if(tls_netif_get_ethif()->status == 0) vTaskDelay(10/portTICK_RATE_MS);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(wlan_connect_obj, 1, wlan_connect);


STATIC mp_obj_t wlan_disconnect(mp_obj_t self_in) {
    tls_wifi_disconnect();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(wlan_disconnect_obj, wlan_disconnect);


STATIC mp_obj_t wlan_isconnected(mp_obj_t self_in) {
//    return tls_wifi_get_state() == WM_WIFI_JOINED ? mp_const_true : mp_const_false;
    return MP_OBJ_NEW_SMALL_INT(tls_wifi_get_state());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(wlan_isconnected_obj, wlan_isconnected);


STATIC const mp_arg_t wlan_ifconfig_args[] = {
    { MP_QSTR_config,  MP_ARG_OBJ,  {.u_obj = mp_const_none} },
};
STATIC mp_obj_t wlan_ifconfig(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    wlan_obj_t *self = pos_args[0];

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(wlan_ifconfig_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(wlan_ifconfig_args), wlan_ifconfig_args, args);

    // get the configuration
    if (args[0].u_obj == mp_const_none) {
        // get
        struct netif *netif = tls_get_netif();

        mp_obj_t ifconfig[4] = {
                netutils_format_ipv4_addr((uint8_t *)&netif->ip_addr, NETUTILS_LITTLE),
                netutils_format_ipv4_addr((uint8_t *)&netif->netmask, NETUTILS_LITTLE),
                netutils_format_ipv4_addr((uint8_t *)&netif->gw,      NETUTILS_LITTLE),
                netutils_format_ipv4_addr((uint8_t *)&netif->gw,      NETUTILS_LITTLE)  //ipV4.ipV4DnsServer
        };

        return mp_obj_new_tuple(4, ifconfig);
    } else {
        // set
        mp_obj_t *items;
        mp_obj_get_array_fixed_n(args[0].u_obj, 4, &items);

//        SlNetCfgIpV4Args_t ipV4;
//        netutils_parse_ipv4_addr(items[0], (uint8_t *)&ipV4.ipV4, NETUTILS_LITTLE);
//        netutils_parse_ipv4_addr(items[1], (uint8_t *)&ipV4.ipV4Mask, NETUTILS_LITTLE);
//        netutils_parse_ipv4_addr(items[2], (uint8_t *)&ipV4.ipV4Gateway, NETUTILS_LITTLE);
//        netutils_parse_ipv4_addr(items[3], (uint8_t *)&ipV4.ipV4DnsServer, NETUTILS_LITTLE);

        if (self->mode == STATION_MODE) {
        } else {
        }

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(wlan_ifconfig_obj, 1, wlan_ifconfig);


STATIC mp_obj_t wlan_ssid(size_t n_args, const mp_obj_t *args) {
    wlan_obj_t *self = args[0];

    if (n_args == 1) {
        return mp_obj_new_str((const char *)self->ssid, strlen((const char *)self->ssid));
    } else {
        size_t ssid_len;
        const char *ssid = mp_obj_str_get_data(args[1], &ssid_len);
        memcpy(self->ssid, ssid, ssid_len);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(wlan_ssid_obj, 1, 2, wlan_ssid);


STATIC mp_obj_t wlan_mac(size_t n_args, const mp_obj_t *args) {
    wlan_obj_t *self = args[0];

    if (n_args == 1) {
        return mp_obj_new_bytes((const byte *)self->mac, ETH_ALEN);
    } else {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
        if (bufinfo.len != 6) {
            mp_raise_ValueError("invalid argument");
        }
        memcpy(self->mac, bufinfo.buf, ETH_ALEN);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(wlan_mac_obj, 1, 2, wlan_mac);


STATIC const mp_rom_map_elem_t wlan_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_scan),                MP_ROM_PTR(&wlan_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect),             MP_ROM_PTR(&wlan_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect),          MP_ROM_PTR(&wlan_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_isconnected),         MP_ROM_PTR(&wlan_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_ifconfig),            MP_ROM_PTR(&wlan_ifconfig_obj) },
    { MP_ROM_QSTR(MP_QSTR_ssid),                MP_ROM_PTR(&wlan_ssid_obj) },
    { MP_ROM_QSTR(MP_QSTR_mac),                 MP_ROM_PTR(&wlan_mac_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_STA),                 MP_ROM_INT(STATION_MODE) },
    { MP_ROM_QSTR(MP_QSTR_AP),                  MP_ROM_INT(SOFTAP_MODE) },
    { MP_ROM_QSTR(MP_QSTR_WEP),                 MP_ROM_INT(IEEE80211_ENCRYT_NONE) },
    { MP_ROM_QSTR(MP_QSTR_WPA),                 MP_ROM_INT(IEEE80211_ENCRYT_TKIP_WPA) },
    { MP_ROM_QSTR(MP_QSTR_WPA2),                MP_ROM_INT(IEEE80211_ENCRYT_TKIP_WPA2) },
};
STATIC MP_DEFINE_CONST_DICT(wlan_locals_dict, wlan_locals_dict_table);


const mod_network_nic_type_t mod_network_nic_type_wlan = {
    .base = {
        { &mp_type_type },
        .name = MP_QSTR_WLAN,
        .print = wlan_print,
        .make_new = wlan_make_new,
        .locals_dict = (mp_obj_t)&wlan_locals_dict,
    },
};
