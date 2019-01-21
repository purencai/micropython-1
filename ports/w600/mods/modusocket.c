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

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "rtos/include/semphr.h"

#include "chip/wm_sockets.h"
#include "lwip/sockets.h"

#include "modnetwork.h"
#include "modusocket.h"

#include "lib/netutils/netutils.h"


/******************************************************************************
 DEFINE PRIVATE CONSTANTS
 ******************************************************************************/
#define MOD_NETWORK_MAX_SOCKETS                 10

/******************************************************************************
 DEFINE PRIVATE TYPES
 ******************************************************************************/
typedef struct {
    int16_t sd;
    bool    user;
} modusocket_sock_t;

STATIC modusocket_sock_t modusocket_sockets[MOD_NETWORK_MAX_SOCKETS] = {{.sd = -1}, {.sd = -1}, {.sd = -1}, {.sd = -1}, {.sd = -1},
                                                                        {.sd = -1}, {.sd = -1}, {.sd = -1}, {.sd = -1}, {.sd = -1}};

STATIC const mp_obj_type_t socket_type;
STATIC xSemaphoreHandle modsocket_lock;

/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void modusocket_socket_add (int16_t sd, bool user) {
    xSemaphoreTake(modsocket_lock, portMAX_DELAY);
    for (int i = 0; i < MOD_NETWORK_MAX_SOCKETS; i++) {
        if (modusocket_sockets[i].sd < 0) {
            modusocket_sockets[i].sd = sd;
            modusocket_sockets[i].user = user;
            break;
        }
    }
    xSemaphoreGive(modsocket_lock);
}

void modusocket_socket_delete (int16_t sd) {
    xSemaphoreTake(modsocket_lock, portMAX_DELAY);
    for (int i = 0; i < MOD_NETWORK_MAX_SOCKETS; i++) {
        if (modusocket_sockets[i].sd == sd) {
            modusocket_sockets[i].sd = -1;
            break;
        }
    }
    xSemaphoreGive(modsocket_lock);
}

void modusocket_close_all_user_sockets (void) {
    xSemaphoreTake(modsocket_lock, portMAX_DELAY);
    for (int i = 0; i < MOD_NETWORK_MAX_SOCKETS; i++) {
        if (modusocket_sockets[i].sd >= 0 && modusocket_sockets[i].user) {
            lwip_close(modusocket_sockets[i].sd);
            modusocket_sockets[i].sd = -1;
        }
    }
    xSemaphoreGive(modsocket_lock);
}

/******************************************************************************/
// socket class

// constructor socket(family=AF_INET, type=SOCK_STREAM, proto=IPPROTO_TCP, fileno=None)
STATIC mp_obj_t socket_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 4, false);

    // create socket object
    mod_network_socket_obj_t *self = m_new_obj_with_finaliser(mod_network_socket_obj_t);
    self->base.type = (mp_obj_t)&socket_type;
    self->u_param.domain = AF_INET;
    self->u_param.type = SOCK_STREAM;
    self->u_param.proto = IPPROTO_TCP;
    self->u_param.fileno = -1;
    self->timeout_ms = 0;

    if (n_args > 0) {
        self->u_param.domain = mp_obj_get_int(args[0]);
        if (n_args > 1) {
            self->u_param.type = mp_obj_get_int(args[1]);
            if (n_args > 2) {
                self->u_param.proto = mp_obj_get_int(args[2]);
                if (n_args > 3) {
                    self->u_param.fileno = mp_obj_get_int(args[3]);
                }
            }
        }
    }

    // create the socket
    self->sd = lwip_socket(self->u_param.domain, self->u_param.type, self->u_param.proto);
    if(self->sd < 0) {
        mp_raise_OSError(self->sd);
    }

    // add the socket to the list
    modusocket_socket_add(self->sd, true);

    modsocket_lock = xSemaphoreCreateCounting(1, 0);

    return self;
}

// method socket.close()
STATIC mp_obj_t socket_close(mp_obj_t self_in) {
    mod_network_socket_obj_t *self = self_in;

    if(self->sd >= 0) {
        modusocket_socket_delete(self->sd);
        lwip_close(self->sd);
        self->sd = -1;
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(socket_close_obj, socket_close);

// method socket.bind(address)
STATIC mp_obj_t socket_bind(mp_obj_t self_in, mp_obj_t addr_in) {
    mod_network_socket_obj_t *self = self_in;

    // get address
    uint8_t ip[MOD_NETWORK_IPV4ADDR_BUF_SIZE];
    mp_uint_t port = netutils_parse_inet_addr(addr_in, ip, NETUTILS_LITTLE);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = ((u32) 0x00000000UL); // automatically fill with my IP
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));


    uint err = lwip_bind(self->sd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(err == -1)
    {
        mp_raise_OSError(err);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_bind_obj, socket_bind);

// method socket.listen([backlog])
STATIC mp_obj_t socket_listen(size_t n_args, const mp_obj_t *args) {
    mod_network_socket_obj_t *self = args[0];

    int32_t backlog = 0;
    if (n_args > 1) {
        backlog = mp_obj_get_int(args[1]);
        backlog = (backlog < 0) ? 0 : backlog;
    }

    uint err = lwip_listen(self->sd, backlog);
    if(err == -1)
    {
        mp_raise_OSError(err);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socket_listen_obj, 1, 2, socket_listen);

// method socket.accept()
STATIC mp_obj_t socket_accept(mp_obj_t self_in) {
    mod_network_socket_obj_t *self = self_in;

    // create new socket object
    mod_network_socket_obj_t *clien_sock = m_new_obj_with_finaliser(mod_network_socket_obj_t);
    // the new socket inherits all properties from its parent
    memcpy(clien_sock, self, sizeof(mod_network_socket_obj_t));

    // accept the incoming connection
    uint8_t ip[MOD_NETWORK_IPV4ADDR_BUF_SIZE];
    mp_uint_t port = 0;

    int16_t sd;
    struct sockaddr_in client_addr;
    socklen_t sin_size = sizeof(client_addr);

    uint32_t timeout_ms = self->timeout_ms;
    for (;;) {
        sd = lwip_accept(self->sd, (struct sockaddr *) &client_addr, &sin_size);
        if (sd >= 0) {
            // save the socket descriptor
            clien_sock->sd = sd;
            // add the socket to the list
            modusocket_socket_add(clien_sock->sd, true);

            // make the return value
            mp_obj_tuple_t *client = mp_obj_new_tuple(2, NULL);
            client->items[0] = clien_sock;
            client->items[1] = netutils_format_inet_addr(ip, port, NETUTILS_LITTLE);

            return client;
        }

        if((self->timeout_ms == 0) || (timeout_ms > 0))
        {
            mp_hal_delay_ms(2);

            if(self->timeout_ms != 0)
            {
                if(--timeout_ms == 0)
                {
                    mp_raise_OSError(-1);
                }
            }
        }
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(socket_accept_obj, socket_accept);

// method socket.connect(address)
STATIC mp_obj_t socket_connect(mp_obj_t self_in, mp_obj_t addr_in) {
    mod_network_socket_obj_t *self = self_in;

    // get address
    uint8_t ip[MOD_NETWORK_IPV4ADDR_BUF_SIZE];
    mp_uint_t port = netutils_parse_inet_addr(addr_in, ip, NETUTILS_LITTLE);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;  // IPv4
    addr.sin_addr.s_addr = ip[0] | (ip[1] << 8) | (ip[2] << 16) | (ip[3] << 24);
    addr.sin_port = htons(port);

    int err = lwip_connect(self->sd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if(err == -1)
    {
        mp_raise_OSError(err);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_connect_obj, socket_connect);

// method socket.send(bytes)
STATIC mp_obj_t socket_send(mp_obj_t self_in, mp_obj_t buf_in) {
    mod_network_socket_obj_t *self = self_in;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    int res = lwip_send(self->sd, bufinfo.buf, bufinfo.len, 0);
    if(res < 0)
    {
        mp_raise_OSError(res);
    }

    return mp_obj_new_int_from_uint(res);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_send_obj, socket_send);

// method socket.recv(bufsize)
STATIC mp_obj_t socket_recv(mp_obj_t self_in, mp_obj_t len_in) {
    mod_network_socket_obj_t *self = self_in;

    mp_int_t len = mp_obj_get_int(len_in);
    vstr_t vstr;
    vstr_init_len(&vstr, len);

    int res = lwip_recv(self->sd, vstr.buf, vstr.len, 0);
    if(res < 0)
    {
        mp_raise_OSError(res);
    }
    else if(res == 0) {
        return mp_const_empty_bytes;
    }

    vstr.len = res;
    vstr.buf[vstr.len] = '\0';
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_recv_obj, socket_recv);

// method socket.sendto(bytes, address)
STATIC mp_obj_t socket_sendto(mp_obj_t self_in, mp_obj_t data_in, mp_obj_t addr_in) {
    mod_network_socket_obj_t *self = self_in;

    // get the data
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data_in, &bufinfo, MP_BUFFER_READ);

    // get address
    uint8_t ip[MOD_NETWORK_IPV4ADDR_BUF_SIZE];
    mp_uint_t port = netutils_parse_inet_addr(addr_in, ip, NETUTILS_LITTLE);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;  // IPv4
    addr.sin_addr.s_addr = ip[0] | (ip[1] << 8) | (ip[2] << 16) | (ip[3] << 24);
    addr.sin_port = htons(port);

    int res = lwip_sendto(self->sd, bufinfo.buf, bufinfo.len, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
    if(res < 0)
    {
        mp_raise_OSError(res);
    }

    return mp_obj_new_int(res);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(socket_sendto_obj, socket_sendto);

// method socket.recvfrom(bufsize)
STATIC mp_obj_t socket_recvfrom(mp_obj_t self_in, mp_obj_t len_in) {
    mod_network_socket_obj_t *self = self_in;

    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(len_in));

    struct sockaddr_in addr;
    uint32_t addr_len;

    int res = lwip_recvfrom(self->sd, vstr.buf, vstr.len, 0, (struct sockaddr*)&addr, &addr_len);
    if(res < 0)
    {
        mp_raise_OSError(res);
    }

    mp_obj_t tuple[2];
    if(res == 0) {
        tuple[0] = mp_const_empty_bytes;
    } else {
        vstr.len = res;
        vstr.buf[vstr.len] = '\0';
        tuple[0] = mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
    }
    tuple[1] = netutils_format_inet_addr((uint8_t *)&addr.sin_addr.s_addr, htons(addr.sin_port), NETUTILS_LITTLE);
    return mp_obj_new_tuple(2, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_recvfrom_obj, socket_recvfrom);

// method socket.setsockopt(level, optname, value)
STATIC mp_obj_t socket_setsockopt(size_t n_args, const mp_obj_t *args) {
    mod_network_socket_obj_t *self = args[0];

    mp_int_t level = mp_obj_get_int(args[1]);
    mp_int_t opt = mp_obj_get_int(args[2]);

    const void *optval;
    mp_uint_t optlen;
    mp_int_t val;
    if (mp_obj_is_integer(args[3])) {
        val = mp_obj_get_int_truncated(args[3]);
        optval = &val;
        optlen = sizeof(val);
    } else {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[3], &bufinfo, MP_BUFFER_READ);
        optval = bufinfo.buf;
        optlen = bufinfo.len;
    }

    int res = lwip_setsockopt(self->sd, level, opt, optval, optlen);
    if(res < 0)
    {
        mp_raise_OSError(res);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socket_setsockopt_obj, 4, 4, socket_setsockopt);

STATIC mp_obj_t socket_makefile(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    return args[0];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socket_makefile_obj, 1, 6, socket_makefile);

STATIC const mp_rom_map_elem_t socket_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__),         MP_ROM_PTR(&socket_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_close),           MP_ROM_PTR(&socket_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_bind),            MP_ROM_PTR(&socket_bind_obj) },
    { MP_ROM_QSTR(MP_QSTR_listen),          MP_ROM_PTR(&socket_listen_obj) },
    { MP_ROM_QSTR(MP_QSTR_accept),          MP_ROM_PTR(&socket_accept_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect),         MP_ROM_PTR(&socket_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_send),            MP_ROM_PTR(&socket_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendall),         MP_ROM_PTR(&socket_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv),            MP_ROM_PTR(&socket_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendto),          MP_ROM_PTR(&socket_sendto_obj) },
    { MP_ROM_QSTR(MP_QSTR_recvfrom),        MP_ROM_PTR(&socket_recvfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_setsockopt),      MP_ROM_PTR(&socket_setsockopt_obj) },
    { MP_ROM_QSTR(MP_QSTR_makefile),        MP_ROM_PTR(&socket_makefile_obj) },

    // stream methods
    { MP_ROM_QSTR(MP_QSTR_read),            MP_ROM_PTR(&mp_stream_read1_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto),        MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline),        MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),           MP_ROM_PTR(&mp_stream_write_obj) },
};

MP_DEFINE_CONST_DICT(socket_locals_dict, socket_locals_dict_table);


STATIC mp_uint_t socket_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    mod_network_socket_obj_t *self = self_in;

    mp_int_t ret = lwip_recv(self->sd, buf, size, 0);
    if(ret < 0) {
        *errcode = ret;
        ret = MP_STREAM_ERROR;
    }

    return ret;
}

STATIC mp_uint_t socket_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    mod_network_socket_obj_t *self = self_in;

    mp_int_t ret = lwip_send(self->sd, buf, size, 0);
    if (ret < 0) {
        *errcode = ret;
        ret = MP_STREAM_ERROR;
    }

    return ret;
}

STATIC mp_uint_t socket_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    mod_network_socket_obj_t *self = self_in;

//    return wlan_socket_ioctl(self, request, arg, errcode);
    return 0;
}

const mp_stream_p_t socket_stream_p = {
    .read = socket_read,
    .write = socket_write,
    .ioctl = socket_ioctl,
    .is_text = false,
};

STATIC const mp_obj_type_t socket_type = {
    { &mp_type_type },
    .name = MP_QSTR_socket,
    .make_new = socket_make_new,
    .protocol = &socket_stream_p,
    .locals_dict = (mp_obj_t)&socket_locals_dict,
};

/******************************************************************************/
// usocket module

// function usocket.getaddrinfo(host, port)
/// \function getaddrinfo(host, port)
STATIC mp_obj_t mod_usocket_getaddrinfo(mp_obj_t host_in, mp_obj_t port_in) {
    size_t hlen;
    const char *host = mp_obj_str_get_data(host_in, &hlen);
    mp_int_t port = mp_obj_get_int(port_in);

    // ipv4 only
    uint8_t out_ip[MOD_NETWORK_IPV4ADDR_BUF_SIZE];
//    int32_t result = wlan_gethostbyname(host, hlen, out_ip, SL_AF_INET);
//    if (result < 0) {
//        mp_raise_OSError(-result);
//    }
    mp_obj_tuple_t *tuple = mp_obj_new_tuple(5, NULL);
    tuple->items[0] = MP_OBJ_NEW_SMALL_INT(AF_INET);
    tuple->items[1] = MP_OBJ_NEW_SMALL_INT(SOCK_STREAM);
    tuple->items[2] = MP_OBJ_NEW_SMALL_INT(0);
    tuple->items[3] = MP_OBJ_NEW_QSTR(MP_QSTR_);
    tuple->items[4] = netutils_format_inet_addr(out_ip, port, NETUTILS_LITTLE);
    return mp_obj_new_list(1, (mp_obj_t*)&tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_usocket_getaddrinfo_obj, mod_usocket_getaddrinfo);

STATIC const mp_rom_map_elem_t mp_module_usocket_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_ROM_QSTR(MP_QSTR_usocket) },

    { MP_ROM_QSTR(MP_QSTR_socket),          MP_ROM_PTR(&socket_type) },
    { MP_ROM_QSTR(MP_QSTR_getaddrinfo),     MP_ROM_PTR(&mod_usocket_getaddrinfo_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_AF_INET),         MP_ROM_INT(AF_INET) },

    { MP_ROM_QSTR(MP_QSTR_SOCK_STREAM),     MP_ROM_INT(SOCK_STREAM) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_DGRAM),      MP_ROM_INT(SOCK_DGRAM) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_usocket_globals, mp_module_usocket_globals_table);

const mp_obj_module_t mp_module_usocket = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_usocket_globals,
};
