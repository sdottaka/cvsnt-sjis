/* CVS info library interface

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */


/* info DLLs need to export one function:

	library_callback *GetCvsInfo()

*/

#ifndef INFOLIB__H
#define INFOLIB__H

#ifdef __cplusplus
extern "C" {
#endif

/* Structure passed into loginfo function */
typedef struct change_info_t
{
	const char *filename;
	const char *rev_new;
	const char *rev_old;
	enum classify_type type;
	const char *tag;
} change_info;

/* Main callback structure.  This should be returned as an intitialised structure
   by the GetCvsInfo structure. */
typedef struct library_callback_t
{
	int (*init)(struct library_callback_t* cb, const char *repository, const char *username, const char *prefix, const char *sessionid, const char *hostname);
	int (*close)(struct library_callback_t* cb);
	int (*pretag)(struct library_callback_t* cb, const char *tag, const char *action, const char *repository, int pretag_list_count, const char **pretag_list);
	int (*verifymsg)(struct library_callback_t* cb, const char *filename);
	int (*loginfo)(struct library_callback_t* cb, const char *repository, const char *hostname, const char *directory, const char *message, const char *status, int change_list_count, change_info *change_list);
	int (*history)(struct library_callback_t* cb, const char *repository, const char *history_line);
	int (*notify)(struct library_callback_t* cb, const char *short_repository, const char *file, const char *type, const char *repository, const char *who);
	int (*precommit)(struct library_callback_t* cb, const char *repository, int precommit_list_count, const char **precommit_list);
	int (*postcommit)(struct library_callback_t* cb, const char *repository);

	void *_reserved;
} library_callback;

typedef library_callback *(*CVSINFOPROC)();

#ifdef __cplusplus
}
#endif

#endif
