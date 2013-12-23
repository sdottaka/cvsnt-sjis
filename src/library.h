/* CVS auth library interface

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#ifndef __LIBRARY__H
#define __LIBRARY__H

#include "../protocols/protocol_interface.h"

struct server_interface *get_server_interface();
void setup_server_interface(cvsroot_t *root);
const struct protocol_interface *load_protocol(const char *protocol);
int unload_protocol(const struct protocol_interface *protocol);
const struct protocol_interface *find_authentication_mechanism(const char *tagline);
const char *enumerate_protocols(int *context);

int get_global_config_data(const char *key, const char *value, char *buffer, int buffer_len);
int set_global_config_data(const char *key, const char *value, const char *buffer);
int enum_global_config_data(const char *key, int value_num, char *value, int value_len, char *buffer, int buffer_len);

#endif
