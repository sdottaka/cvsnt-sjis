#ifndef CVSROOTT__H
#define CVSROOTT__H

typedef struct cvsroot_s {
    char *original;		/* the complete source CVSroot string */
    char *method;		/* protocol name */
    char *username;		/* the username or NULL if method == local */
    char *password;		/* the username or NULL if method == local */
    char *hostname;		/* the hostname or NULL if method == local */
    char *port;			/* the port or zero if method == local */
    char *directory;		/* the directory name */
	char *proxyport; /* Port number of proxy */
	char *proxyprotocol; /* Protocol type of proxy (http, SOAP, etc.) */
	char *proxy; /* Proxy server */
	char *proxyuser; /* Username for proxy */
	char *proxypassword; /* Password for proxy */
	char *unparsed_directory; /* unparsed directory name */
	char *mapped_directory; /* Original directory, mapped to filesystem */
	char *optional_1; /* Protocol defined keyword - also text between {...} */
	char *optional_2; /* Protocol defined keyword */
	char *optional_3; /* Protocol defined keyword */
	char *optional_4; /* Protocol defined keyword */
	char *optional_5; /* Protocol defined keyword */
	char *optional_6; /* Protocol defined keyword */
	char *optional_7; /* Protocol defined keyword */
	char *optional_8; /* Protocol defined keyword */
#ifdef CLIENT_SUPPORT
    unsigned char isremote;	/* nonzero if we are doing remote access */
#endif /* CLIENT_SUPPORT */
} cvsroot_t;

#endif
