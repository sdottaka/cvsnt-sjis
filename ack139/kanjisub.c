/*
 * Kanji code convert subroutine
 *
 * 1999/10/17 by Y.Kawada
 * $Id: kanjisub.c,v 1.7 2000/07/21 16:12:43 slakichi Exp $
 */
#include "kanjicode.h"
#include "ackstring.h"
#ifndef NO_USE_XMALLOC
#include "cvs.h"
#else
#include <stdlib.h>
#include <string.h>
#define xmalloc malloc
#endif

#undef time

void putcd (unsigned int);
void putcdw (unsigned int);
void ungetcd (unsigned int);
int getcd ();

extern void KanjiSetup (T_KANJI *);
extern void SjisEucCheck (char *, int, T_KANJI *);
static char *ip, *op;

void
k_conv(cp)
T_KANJI	*cp;
{
	int	i, kimode= ROMA;

	for(; (i= getcd()) != -1 ;){
		unsigned int	code, code2;

		switch (cp->kanjicheck_h[code=i]) {
		case 8:
			if (!(cp->flag & fUNIX))
				putcd( code );
			break;
		case 5:	/* SI locking shift G0 -> GL */
			kimode= ROMA;
			break;
		case 6:	/* SO locking shift G0 -> GL */
			kimode= KANA;
			break;
		case 4:	/* ESC */
			code= getcd();
			code2= getcd();
			if( code == '$' ){
				if( code2=='B'||code2=='@' ){
					kimode= KANJI;
					break;
				}else if( code2 == '(' ){
					code= getcd();
					if( code == 'B' || code == '@' ){
						kimode= KANJI;
					}else{
						putcd( 033 );	/* ESC */
						putcd( '$' );
						ungetcd( code2 );
						ungetcd( code );
					}
					break;
				}
			}else if( code == '(' ){
				if( code2=='J' || code2=='B' || code2=='H' ){
					kimode=	ROMA;
					break;
				}else if( code2 == 'I' ){
					kimode=	KANA;
					break;
				}
			}
			putcd( 033 );	/* ESC */
			ungetcd( code2 );
			ungetcd( code );
			break;
		case 3: /* 7bit 1byte code */
			switch( kimode ){
			case KANA:
				code|= 0x80;	/* jiskana to code */
				if( cp->flag & fZKANA )
					code= CodeToZen( code );
				putcdw( (*cp->codetokanji)( code ) );
				continue;
			case KANJI:
			case QKANJI:
				code= PACKWORD( code, code2= getcd() );
				if( cp->kanjicheck_h[code2] == 3 ){
					putcdw( (*cp->codetokanji)( code ) );
				}else{
					putcdw( (cp->flag&fERRCODE) ?
					(*cp->codetokanji)(ERRCHAR):code );
				}
				continue;
			}
		case 0: /* through */
#ifndef ONLY_CONVERT_FUNC
			if( cp->outcode == JIS )
				setjismode();
#endif
			putcd( code );
			break;
		case 2:	/* 2byte code & single shift */
			code= PACKWORD( code, getcd() );
		case 1: /* 1byte code */
			if( (code2= (*cp->kanjitocode)( code )) != ERRCODE ){
				if( cp->flag & fZKANA )
					code2= CodeToZen( code2 );
				putcdw( (*cp->codetokanji)( code2 ) );
			}else{
				putcdw( (cp->flag&fERRCODE) ?
					(*cp->codetokanji)(ERRCHAR):code );
			}
		}
	}
}


char *
k_to_euc(from)
char *from;
{
    T_KANJI   p , *cp;
    char *kbuf;
    
    p.inpcode = NONE;
    p.outcode = EUC;
    p.flag = fUNIX|fZKANA;
    p.time = p.files= 0;
    *p.fname = '\0';
    cp = &p;

    kbuf = (char *) xmalloc (strlen(from) * 2 + 1);
    *kbuf = '\0';
    ip = from;
    op = kbuf;

    SjisEucCheck (from, strlen(from), cp);
    KanjiSetup (cp);
    k_conv (cp);
    *op = '\0';
    if (from)
        free (from);

    return (kbuf);
}

char *
k_to_sjis(from)
char *from;
{
    T_KANJI   p , *cp;
    char *kbuf;
    
    p.inpcode = NONE;
    p.outcode = SJIS;
    p.flag = fUNIX/*|fZKANA*/;
    p.time = p.files= 0;
    *p.fname = '\0';
    cp = &p;

    kbuf = (char *) xmalloc (strlen(from) + 1);
      strcpy(kbuf,from);
    *from = '\0';
    ip = kbuf;
    op = from;

    SjisEucCheck (kbuf, strlen(kbuf), cp);
    KanjiSetup (cp);
    k_conv (cp);
    *op = '\0';
    if (kbuf)
        free (kbuf);

    return (from);
}


void
putcd(code)
unsigned int    code;
{
    *op++ = code;
}

void
putcdw(code)
unsigned int code;
{
    unsigned int    hi= code>>8;

    if (hi)
        putcd (hi);
    putcd (code);
}

int
getcd()
{
    if (*ip == '\0') {
        return -1;
    }
    return 0xff & *ip++;
}

void
ungetcd(code)
unsigned int code;
{
    *--ip = code;
}

#if 0  /* liback.a */
main()
{
    printf("%s", k_to_euc(test));
    printf("%s", k_to_sjis(test));
}
#endif /* liback.a */
