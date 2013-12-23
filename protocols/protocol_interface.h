/* CVS auth protocol interface

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#ifndef PROTOCOL_INTERFACE__H
#define PROTOCOL_INTERFACE__H

#ifdef _WIN32
#define CVS_EXPORT __declspec(dllexport)
#else
#define CVS_EXPORT
#endif

#include "../src/cvsroott.h" // cvsroot_t

/* Allowed CVSROOT elements */
enum
{
	elemNone	 = 0x0000,
	elemUsername = 0x0001,
	elemPassword = 0x0002,
	elemHostname = 0x0004,
	elemPort	 = 0x0008,
	elemTunnel	 = 0x0010,

	flagAlwaysEncrypted = 0x8000
};

struct protocol_interface
{
	const unsigned short interface_version;

	const char *name;	/* Protocol name ('Foobar')*/
	const char *version;	/* Protocol version ('Foobar version 1.0') */
	const char *syntax;	/* Syntax (':foobar:user@server[:port][:]path') */

	/* The 'destroy' call must remain in the same place for all interface versions */
	void (*destroy)(const struct protocol_interface *protocol);

	unsigned required_elements; /* Required CVSROOT elements */
	unsigned valid_elements;	/* Valid CVSROOT elements */

	int (*validate_details)(const struct protocol_interface *protocol, cvsroot_t *newroot);
	int (*connect)(const struct protocol_interface *protocol, int verify_only);
	int (*disconnect)(const struct protocol_interface *protocol);
	int (*login)(const struct protocol_interface *protocol, char *password);
	int (*logout)(const struct protocol_interface *protocol);
	int (*wrap)(const struct protocol_interface *protocol, int unwrap, int encrypt, const void *input, int size, void *output, int *newsize);
	int (*auth_protocol_connect)(const struct protocol_interface *protocol, const char *auth_string);
	int (*read_data)(const struct protocol_interface *protocol, void *data, int length);
	int (*write_data)(const struct protocol_interface *protocol, const void *data, int length);
	int (*flush_data)(const struct protocol_interface *protocol);
	int (*shutdown)(const struct protocol_interface *protocol);
	int (*impersonate)(const struct protocol_interface *protocol, const char *username, void *user_handle);
	int (*validate_keyword)(const struct protocol_interface *protocol, cvsroot_t *root, const char *keyword, const char *value);
	const char *(*get_keyword_help)(const struct protocol_interface *protocol);
	int (*server_read_data)(const struct protocol_interface *protocol, void *data, int length);
	int (*server_write_data)(const struct protocol_interface *protocol, const void *data, int length);
	int (*server_flush_data)(const struct protocol_interface *protocol);
	int (*server_shutdown)(const struct protocol_interface *protocol);

	void* __reserved; // Used by cvs main code to store context data

	/* The following should be filled in by auth_protocol_connect before it returns SUCCESS */
	int verify_only;
	char *auth_username;
	char *auth_password;
	char *auth_repository;
};

struct server_interface
{
	cvsroot_t *current_root;
	const char *library_dir;
	const char *cvs_command;
	int in_fd; /* FD for server input */
	int out_fd; /* FD for server output */

	int (*get_config_data)(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len);
	int (*set_config_data)(const struct server_interface *server, const char *key, const char *value, const char *buffer);
	int (*get_global_config_data)(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len);
	int (*set_global_config_data)(const struct server_interface *server, const char *key, const char *value, const char *buffer);
	int (*error)(const struct server_interface *server, int fatal, const char *text);
	int (*getpass)(char *password, int max_length, const char *prompt); /* return 1 OK, 0 Cancel */
	int (*yesno)(const char *message, const char *title, int withcancel); /* Return -1 cancel, 0 No, 1 Yes */
	int (*set_encrypted_channel)(int encrypt); /* Set when plaintext I/O no longer valid */
};

enum
{
	CVSPROTO_SUCCESS			=  0,
	CVSPROTO_FAIL				= -1, /* Generic failure (errno set) */
	CVSPROTO_BADPARMS			= -2, /* (Usually) wrong parameters from cvsroot string */
	CVSPROTO_AUTHFAIL			= -3, /* Authorization or login failed */
	CVSPROTO_NOTME				= -4, /* Not for this protocol (used by protocol connect) */
	CVSPROTO_NOTIMP				= -5, /* Not implemented */
	CVSPROTO_SUCCESS_NOPROTOCOL = -6, /* Connect succeeded, don't wait for 'I LOVE YOU' response */
};

/* Exported from each shared library to get interface data */
CVS_EXPORT struct protocol_interface *get_protocol_interface(const struct server_interface *server);

typedef struct protocol_interface *(*tGPI)(const struct server_interface *server);
		
#define PROTOCOL_INTERFACE_VERSION 0x0130

#endif
