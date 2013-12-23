/* CVS pserver password scrambling

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#ifndef SCRAMBLE__H
#define SCRAMBLE__H

#ifdef __cplusplus
extern "C" {
#endif

int pserver_decrypt_password(const char *str, char *s, int s_max);
int pserver_crypt_password (const char *str, char *s, int s_max);

#ifdef __cplusplus
}
#endif

#endif
