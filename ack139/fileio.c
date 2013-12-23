/*	Copyright 1993,94 H.Ogasawara (COR.)	*/

/* v1.00  1993 10/10	Ogasawara Hiroyuki		*/
/*			oga@dgw.yz.yamagata-u.ac.jp	*/
/* v1.10  1994 03/19	Ogasawara Hiroyuki		*/

#ifndef ONLY_CONVERT_FUNC
#include	<stdio.h>
#endif

#define	EMPTY		0
#define	STACKSIZE	256
#define	PRESIZE		(1024*16)

static int	cstack[STACKSIZE],
		*cstackptr= cstack,
		presize= 0;
unsigned char	prebuf[PRESIZE],
		*preptr= prebuf;

#ifndef ONLY_CONVERT_FUNC
FILE	*FP, *FO;
#endif

void
putcode( code )
unsigned int	code;
{
#ifndef ONLY_CONVERT_FUNC
	putc( code, FO );
#endif
}

void
putcodew( code )
unsigned int	code;
{
	unsigned int	hi= code>>8;
	if( hi )
		putcode( hi );
	putcode( code & 0xff );
}

int
getcode()
{
	if( cstackptr > cstack )
		return	*--cstackptr;
	if( presize ){
		presize--;
		return	*preptr++;
	}
#ifndef ONLY_CONVERT_FUNC
	return	getc( FP );
#else
	return -1;
#endif
}

void
ungetcode( code )
{
	*cstackptr++= code;
}

#ifndef ONLY_CONVERT_FUNC
void
preread( cp )
void	*cp;
{
	presize= fread( preptr= prebuf, 1, PRESIZE, FP );
	SjisEucCheck( prebuf, presize, cp );
}

#if HUMAN
isdir( name )
char	*name;
{
	return	CHMOD( name, -1 ) & 0x10;
}
#else
#include	<sys/types.h>
#include	<sys/stat.h>
filedate( fn )
{
	struct stat	st;
	fstat( fn, &st );
	return	st.st_mtime;
}

filesetdate( name, set )
char	*name;
{
	time_t	tim[2];
	tim[0]= tim[1]= set;
	utime( name, tim );
}
#if 0 /* liback.a */
isdir( name )
char	*name;
{
	struct stat	st;
	stat( name, &st );
	return	st.st_mode & S_IFDIR;
}
#endif /* liback.a */
#endif
#endif
