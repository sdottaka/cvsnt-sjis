#ifndef _getline_h_
#define _getline_h_ 1

#include <stdio.h>

#if defined (__GNUC__) || (defined (__STDC__) && __STDC__) || defined(_WIN32)
#define __PROTO(args) args
#else
#define __PROTO(args) ()
#endif  /* GCC.  */

#define GETLINE_NO_LIMIT -1

int
  getline __PROTO ((char **_lineptr, size_t *_n, FILE *_stream));
int
  getstr __PROTO ((char **_lineptr, size_t *_n, FILE *_stream,
		   char _terminator, int _offset, int limit));

int io_getline(int fd, char** buffer, int buffer_max);

#endif /* _getline_h_ */
