#ifndef _getdelim_h
#define _getdelim_h_ 1

#include <stdio.h>

#ifndef HAVE_GETDELIM

int getdelim(char **_lineptr, size_t *_n, int delim, FILE *_stream);

#endif

#endif /* _getline_h_ */
