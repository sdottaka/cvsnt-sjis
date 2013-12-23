/* CVS auth common routines

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#ifndef __COMMON__H
#define __COMMON__H

#define PROTOCOL_DLL
#ifdef _WIN32
#include "../windows-NT/config.h"
#else
#include "../config.h"
#endif

#ifndef HAVE_GETADDRINFO
#include "socket.h"
#include "getnameinfo.h"
#include "getaddrinfo.h"
#endif

#ifndef HAVE_INET_ATON
#include "inet_aton.h"
#endif

extern const struct server_interface *current_server;

int get_user_local_config_data(const char *key, const char *value, char *buffer, int buffer_len);
int set_user_local_config_data(const char *key, const char *value, const char *buffer);
int server_error(int fatal, char *fmt, ...);
int server_getc(const struct protocol_interface *protocol);
int server_getline(const struct protocol_interface *protocol, char** buffer, int buffer_max);
const char *get_default_port(const cvsroot_t *root);
int set_encrypted_channel(int encrypt);

/* TCP/IP helper functions */
int get_tcp_fd();
int tcp_printf(char *fmt, ...);
int tcp_connect(const cvsroot_t *cvsroot);
int tcp_connect_bind(const char *servername, const char *remote_port, int min_local_port, int max_local_port);
const struct addrinfo *get_addrinfo(int get_canonical_name);
int tcp_disconnect();
int tcp_read(void *data, int length);
int tcp_write(const void *data, int length);
int tcp_shutdown();
int tcp_readline(char* buffer, int buffer_len);

int run_command(const char *cmd, int* in_fd, int* out_fd, int* err_fd);
const char* get_username(const cvsroot_t* current_root);

#endif
