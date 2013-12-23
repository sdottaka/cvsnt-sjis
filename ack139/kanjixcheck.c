/*	Copyright 1993,94 H.Ogasawara (COR.)	*/

/* v1.00  1993 10/10 */
/* v1.10  1994 03/19 */
/* v1.20  1994 03/21 */

#include	<stdio.h>
#include	"kanjicode.h"
#include	"ackstring.h"
#define KCCHECK 1

#if KCCHECK
# define	putcode(cmd)
# define	putcodew(cmd)
# define	setjismode()
# define	JISKANAINC	JisKana++
# define	JISKANJIINC	JisKanji++
# define	OTHERINC	Other++
# define	KANJIINC	Kanji++
# define	KANJIONLYINC	KanjiOnly++
# define	ERRCODEINC	ErrCode++
# define	CTRLINC		CtrlCode++
# define	ASCIIINC	Ascii++
extern unsigned char	sjischeck_h[], euccheck_h[];
#else
# define	JISKANAINC
# define	JISKANJIINC
# define	OTHERINC
# define	KANJIINC
# define	KANJIONLYINC
# define	ERRCODEINC
# define	CTRLINC
# define	ASCIIINC
unsigned char	sjischeck_h[]={
	0,0,0,0,0,0,0,0,0,0,0,0,0,8,5,6,
	0,0,0,0,0,0,0,0,0,0,8,4,0,0,0,0,
	0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,
	0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
unsigned char	euccheck_h[]={
	0,0,0,0,0,0,0,0,0,0,0,0,0,8,5,6,
	0,0,0,0,0,0,0,0,0,0,8,4,0,0,0,0,
	0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,
};
#endif

#ifndef ONLY_CONVERT_FUNC
#if KCCHECK
CodeCheckAll( cp )
#else
SjisEucConvert( cp )
#endif
T_KANJI	*cp;
{
	int	i, kimode= ROMA;
#if KCCHECK
	int	JisKana= 0, JisKanji= 0, Other= 0, Kanji= 0, KanjiOnly= 0,
		ErrCode= 0, CtrlCode= 0, Ascii= 0;
#else
	if( cp->flag & fCHECK )
		return	CodeCheckAll( cp );
#endif
	for(; (i= getcode()) != -1 ;){
		int	code, code2;
		switch( cp->kanjicheck_h[code= i] ){
#if 0
		case 7:	/* LF */
			CTRLINC;
			if( cp->outlf )
				putcodew( cp->outlf );
			else
				putcode( code );
			break;
		case 8:	/* CR */
			CTRLINC;
			if( cp->outcr )
				putcodew( cp->outcr );
			else
				putcode( code );
			break;
#endif
		case 8:
			CTRLINC;
			if( !(cp->flag & fUNIX) )
				putcode( code );
			break;
		case 5:	/* SI locking shift G0 -> GL */
			CTRLINC;
			kimode= ROMA;
			break;
		case 6:	/* SO locking shift G0 -> GL */
			CTRLINC;
			kimode= KANA;
			break;
		case 4:	/* ESC */
			CTRLINC;
			code= getcode();
			code2= getcode();
			if( code == '$' ){
				if( code2=='B'||code2=='@' ){
					kimode= KANJI;
					break;
				}else if( code2 == '(' ){
					code= getcode();
					if( code == 'B' || code == '@' ){
						kimode= KANJI;
					}else{
						putcode( 033 );	/* ESC */
						putcode( '$' );
						ungetcode( code2 );
						ungetcode( code );
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
			putcode( 033 );	/* ESC */
			ungetcode( code2 );
			ungetcode( code );
			break;
		case 3: /* 7bit 1byte code */
			switch( kimode ){
			case KANA:
				JISKANAINC;
				code|= 0x80;	/* jiskana to code */
				if( cp->flag & fZKANA )
					code= CodeToZen( code );
				putcodew( (*cp->codetokanji)( code ) );
				continue;
			case KANJI:
			case QKANJI:
				code= PACKWORD( code, code2= getcode() );
				if( cp->kanjicheck_h[code2] == 3 ){
					JISKANJIINC;
					putcodew( (*cp->codetokanji)( code ) );
				}else{
					ERRCODEINC;
					putcodew( (cp->flag&fERRCODE) ?
					(*cp->codetokanji)(ERRCHAR):code );
				}
				continue;
			}
			ASCIIINC;
		case 0: /* through */
			if( cp->outcode == JIS )
				setjismode();
#if KCCHECK
			if( code == ' ' )
				ASCIIINC;
			if( cp->inpcode == EUC )
				code&= 0x7f;
			if( IsCtrl(code) )
				CTRLINC;
			else
				OTHERINC;
#endif
			putcode( code );
			break;
		case 2:	/* 2byte code & single shift */
			code= PACKWORD( code, getcode() );
			KANJIONLYINC;
		case 1: /* 1byte code */
			if( (code2= (*cp->kanjitocode)( code )) != ERRCODE ){
				KANJIINC;
				if( cp->flag & fZKANA )
					code2= CodeToZen( code2 );
				putcodew( (*cp->codetokanji)( code2 ) );
			}else{
				ERRCODEINC;
				KANJIINC;
				putcodew( (cp->flag&fERRCODE) ?
					(*cp->codetokanji)(ERRCHAR):code );
			}
		}
	}
#if KCCHECK
	{
	char	*cname= "ascii";
	if( Kanji )
		cname= cp->inpcode == SJIS ? "sjis " : "euc  ";
	else if( JisKanji || JisKana )
		cname= "jis  ";
	if( cp->flag & fCODEONLY ){
		puts( cname );
		return;
	}
	if( cp->files <= 1 ){
		if( *cp->fname )
			fputs( "filename        ", stdout );
		puts( "\
code   kanji   kana kanji7  kana7    err  ascii   ctrl  other" );
	}
	if( *cp->fname )
		printf( "%-16s", cp->fname );
	printf( "%s%7d%7d%7d%7d%7d%7d%7d%7d\n",
				cname,
				KanjiOnly,
				Kanji-KanjiOnly,
				JisKanji,
				JisKana,
				ErrCode,
				Ascii,
				CtrlCode,
				Other-Ascii
			);
	}
#endif
}
#endif
