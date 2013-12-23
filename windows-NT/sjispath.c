/*
 *  Author : Atsuo ISHIMOTO <ishimoto@axissoft.co.jp>, June 2000
 */
#ifdef SJIS
#include <string.h>
#include <mbstring.h>

char sjis_getlastchar(char *s)
{
	if (!s || !*s) {
		return 0;
	}
	if (!_ismbstrail((const unsigned char *)s, s + strlen(s) - 1)) {
		return s[strlen(s) - 1];
	}
	return 0;
}

#endif
