/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2015 Galen Hazelwood
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

#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "py/nlr.h"
#include "py/objlist.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"

#include "netutils.h"

#include "lwip/init.h"
#include "lwip/timers.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/dns.h"
#include "lwip/tcp_impl.h"
#include "lwip/netif.h"
#include "lwip/inet.h"
#include <mini-os/lwip-net.h>

typedef struct _lwip_socket_obj_t {
    mp_obj_base_t base;

    volatile union {
        struct tcp_pcb *tcp;
        struct udp_pcb *udp;
    } pcb;
    volatile union {
        struct pbuf *pbuf;
        struct tcp_pcb *connection;
    } incoming;
    mp_obj_t callback;
    byte peer[4];
    mp_uint_t peer_port;
    mp_uint_t timeout;
    uint16_t leftover_count;

    uint8_t domain;
    uint8_t type;

    #define STATE_NEW 0
    #define STATE_CONNECTING 1
    #define STATE_CONNECTED 2
    #define STATE_PEER_CLOSED 3
    // Negative value is lwIP error
    int8_t state;
} lwip_socket_obj_t;

struct mcargs {
  struct eth_addr mac;
  struct netif    netif;
  ip4_addr_t      ip;
  ip4_addr_t      mask;
  ip4_addr_t      gw;
  #if LWIP_DNS
  ip4_addr_t      dns0;
  ip4_addr_t      dns1;
  #endif
} args;


void lwip_socket_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
mp_obj_t lwip_socket_close(mp_obj_t self_in);
mp_obj_t lwip_socket_bind(mp_obj_t self_in, mp_obj_t addr_in);
mp_obj_t lwip_socket_listen(mp_obj_t self_in, mp_obj_t backlog_in);
mp_obj_t lwip_socket_accept(mp_obj_t self_in);
mp_obj_t lwip_socket_connect(mp_obj_t self_in, mp_obj_t addr_in);
void lwip_socket_check_connected(lwip_socket_obj_t *socket);
mp_obj_t lwip_socket_send(mp_obj_t self_in, mp_obj_t buf_in);
mp_obj_t lwip_socket_recv(mp_obj_t self_in, mp_obj_t len_in);
mp_obj_t lwip_socket_sendto(mp_obj_t self_in, mp_obj_t data_in, mp_obj_t addr_in);
mp_obj_t lwip_socket_recvfrom(mp_obj_t self_in, mp_obj_t len_in);
mp_obj_t lwip_socket_sendall(mp_obj_t self_in, mp_obj_t buf_in);
mp_obj_t lwip_socket_settimeout(mp_obj_t self_in, mp_obj_t timeout_in);
mp_obj_t lwip_socket_setblocking(mp_obj_t self_in, mp_obj_t flag_in);
mp_obj_t lwip_socket_setsockopt(mp_uint_t n_args, const mp_obj_t *args);
mp_obj_t lwip_socket_makefile(mp_uint_t n_args, const mp_obj_t *args);
mp_uint_t lwip_socket_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode);
mp_uint_t lwip_socket_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode);
mp_obj_t lwip_socket_make_new(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);
mp_obj_t lwip_getaddrinfo(mp_obj_t host_in, mp_obj_t port_in);
