/*	Copyright 1993 H.Ogasawara (COR.)	*/

/* v1.00  1993 10/10	Ogasawara Hiroyuki		*/
/*			oga@dgw.yz.yamagata-u.ac.jp	*/

#ifndef ONLY_CONVERT_FUNC
#include	"kanjicode.h"

static char	*kinmode[]= {
		"\033(J",	/* JISX 201-1976 roma */
		"\033(B",	/* ANSI X3.4 ascii */
		"\033(I",	/* JISX 201-1976 katakana */
		"\033$B",	/* JISX 208-1983 */
		"\033$@",	/* JISX 208-1978 */
		"\017",		/* SI */
		"\016",		/* SO */
		"\033$(B",	/* JISX 208-1983 ISO */
		"\033$(@",	/* JISX 208-1978 ISO */
	};

static int	kanjimode= KANJI,
		romamode=  ROMA,
		kanamode=  KANA,
		komode= ROMA;

SetJisMode( mode )
{
	kanjimode=	  mode & 1 ? QKANJI : KANJI;
	komode= romamode= mode & 2 ? ASCII  : ROMA;
	kanamode=	  mode & 4 ? SO     : KANA;
	if( mode & 8 )
		kanjimode += KANJI2-KANJI;
}

static void
putmode( km )
int	km;
{
	if( komode != km ){
		char	*p;
		if( komode == SO && km != SI )
			putmode( SI );
		for( p= kinmode[ komode= km ]; *p ; putcode( *p++ ) );
	}
}
#endif

CodeToJis( code )
unsigned int	code;
{
#ifndef ONLY_CONVERT_FUNC
	unsigned int	hi=  code>>8;

	if( hi ){
		putmode( kanjimode );
		return	code & 0x7f7f;
	}
	putmode( kanamode );
	return	code & 0x7f;
#endif
}

#ifndef ONLY_CONVERT_FUNC
setjismode()
{
	putmode( romamode );
}
#endif
