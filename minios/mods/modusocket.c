#include "extmod/modlwip.h"
#include "py/obj.h"
#include "lwip/sockets.h"

#define SEC_SOCKET           100    /* Secured Socket Layer (SSL,TLS)     */

STATIC const mp_map_elem_t socket_locals_dict_table[] = {
  { MP_OBJ_NEW_QSTR(MP_QSTR___del__),         (mp_obj_t)&lwip_socket_close },
  { MP_OBJ_NEW_QSTR(MP_QSTR_close),           (mp_obj_t)&lwip_socket_close },
  { MP_OBJ_NEW_QSTR(MP_QSTR_bind),            (mp_obj_t)&lwip_socket_bind },
  { MP_OBJ_NEW_QSTR(MP_QSTR_listen),          (mp_obj_t)&lwip_socket_listen },
  { MP_OBJ_NEW_QSTR(MP_QSTR_accept),          (mp_obj_t)&lwip_socket_accept },
  { MP_OBJ_NEW_QSTR(MP_QSTR_connect),         (mp_obj_t)&lwip_socket_connect },
  { MP_OBJ_NEW_QSTR(MP_QSTR_send),            (mp_obj_t)&lwip_socket_send },
  { MP_OBJ_NEW_QSTR(MP_QSTR_sendall),         (mp_obj_t)&lwip_socket_sendall },
  { MP_OBJ_NEW_QSTR(MP_QSTR_recv),            (mp_obj_t)&lwip_socket_recv },
  { MP_OBJ_NEW_QSTR(MP_QSTR_sendto),          (mp_obj_t)&lwip_socket_sendto },
  { MP_OBJ_NEW_QSTR(MP_QSTR_recvfrom),        (mp_obj_t)&lwip_socket_recvfrom },
  { MP_OBJ_NEW_QSTR(MP_QSTR_setsockopt),      (mp_obj_t)&lwip_socket_setsockopt },
  { MP_OBJ_NEW_QSTR(MP_QSTR_settimeout),      (mp_obj_t)&lwip_socket_settimeout },
  { MP_OBJ_NEW_QSTR(MP_QSTR_setblocking),     (mp_obj_t)&lwip_socket_setblocking },
  { MP_OBJ_NEW_QSTR(MP_QSTR_makefile),        (mp_obj_t)&lwip_socket_makefile },

  // stream methods
  { MP_OBJ_NEW_QSTR(MP_QSTR_read),            (mp_obj_t)&lwip_socket_read },
  { MP_OBJ_NEW_QSTR(MP_QSTR_write),           (mp_obj_t)&lwip_socket_write },
};

MP_DEFINE_CONST_DICT(socket_locals_dict, socket_locals_dict_table);

STATIC const mp_obj_type_t socket_type = {
  { &mp_type_type },
  .name = MP_QSTR_usocket,
  .make_new = lwip_socket_make_new,
  .stream_p = NULL,
  .locals_dict = (mp_obj_t)&socket_locals_dict,
};

STATIC MP_DEFINE_CONST_FUN_OBJ_2(lwip_getaddrinfo_obj, lwip_getaddrinfo);

STATIC const mp_map_elem_t mp_module_usocket_globals_table[] = {
  { MP_OBJ_NEW_QSTR(MP_QSTR___name__),        MP_OBJ_NEW_QSTR(MP_QSTR_usocket) },

  { MP_OBJ_NEW_QSTR(MP_QSTR_socket),          (mp_obj_t)&socket_type },
  { MP_OBJ_NEW_QSTR(MP_QSTR_getaddrinfo),     (mp_obj_t)&lwip_getaddrinfo_obj },

  // class exceptions
  { MP_OBJ_NEW_QSTR(MP_QSTR_error),           (mp_obj_t)&mp_type_OSError },
  //  { MP_OBJ_NEW_QSTR(MP_QSTR_timeout),         (mp_obj_t)&mp_type_TimeoutError },

  // class constants
  { MP_OBJ_NEW_QSTR(MP_QSTR_AF_INET),         MP_OBJ_NEW_SMALL_INT(AF_INET) },

  { MP_OBJ_NEW_QSTR(MP_QSTR_SOCK_STREAM),     MP_OBJ_NEW_SMALL_INT(SOCK_STREAM) },
  { MP_OBJ_NEW_QSTR(MP_QSTR_SOCK_DGRAM),      MP_OBJ_NEW_SMALL_INT(SOCK_DGRAM) },
  { MP_OBJ_NEW_QSTR(MP_QSTR_SO_REUSEADDR),      MP_OBJ_NEW_SMALL_INT(SO_REUSEADDR) },  

  { MP_OBJ_NEW_QSTR(MP_QSTR_IPPROTO_SEC),     MP_OBJ_NEW_SMALL_INT(SEC_SOCKET) },
  { MP_OBJ_NEW_QSTR(MP_QSTR_SOL_SOCKET),      MP_OBJ_NEW_SMALL_INT(SOL_SOCKET) },  
  { MP_OBJ_NEW_QSTR(MP_QSTR_IPPROTO_TCP),     MP_OBJ_NEW_SMALL_INT(IPPROTO_TCP) },
  { MP_OBJ_NEW_QSTR(MP_QSTR_IPPROTO_UDP),     MP_OBJ_NEW_SMALL_INT(IPPROTO_UDP) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_usocket_globals, mp_module_usocket_globals_table);


const mp_obj_module_t mp_module_usocket = {
  .base = { &mp_type_module },
  .name = MP_QSTR_usocket,
  .globals = (mp_obj_dict_t*)&mp_module_usocket_globals,
};
