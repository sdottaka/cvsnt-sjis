#ifndef _getline_h_
#define _getline_h_ 1

#include <stdio.h>

#ifndef HAVE_GETLINE

int getline(char **_lineptr, size_t *_n, FILE *_stream);

#endif

#endif /* _getline_h_ */
