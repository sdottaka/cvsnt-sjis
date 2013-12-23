#ifndef _FAKE_INET_ATON_H
#define _FAKE_INET_ATON_H

#include <netinet/in.h>

int inet_aton(const char *cp, struct in_addr *addr);

#endif /* _BSD_INET_ATON_H */
