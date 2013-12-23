#ifndef __LOCKSERVICE__H
#define __LOCKSERVICE__H

bool OpenLockClient(SOCKET s, const struct sockaddr_storage *sin, int sinlen);
bool CloseLockClient(SOCKET s);
bool ParseLockCommand(SOCKET s, const char *command);

void run_server(int port, int seq, int local_only);

#endif

