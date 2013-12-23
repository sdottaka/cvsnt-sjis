/*	Copyright 1993 H.Ogasawara (COR.)	*/

/* v1.00  1993 10/10	*/
/* v1.10  1994  3/19	*/
/* v1.20  1994  3/21	*/
/* v1.30  1994  4/16	*/

#ifndef ONLY_CONVERT_FUNC
#include	<stdio.h>
#endif

#include	"kanjicode.h"
#include	"ackstring.h"

extern unsigned int	SjisToCode(),
			CodeToSjis(),
			EucToCode(),
			CodeToEuc(),
			CodeToJis();

extern unsigned char	sjischeck_h[],
			euccheck_h[];

#ifndef ONLY_CONVERT_FUNC
extern FILE		*FP, *FO;
#endif

void
KanjiSetup( cp )
T_KANJI	*cp;
{
    int t;
	static struct {
		unsigned int	(*inputexec)(),
				(*outputexec)();
		unsigned char	*check;
	} inputmap[]= {
		{ SjisToCode, CodeToJis,  sjischeck_h },
		{ EucToCode,  CodeToEuc,  euccheck_h  },
		{ SjisToCode, CodeToSjis, sjischeck_h },
	};

	cp->kanjitocode=  inputmap[cp->inpcode].inputexec;
	cp->kanjicheck_h= inputmap[cp->inpcode].check;
	cp->codetokanji=  inputmap[cp->outcode].outputexec;
    return;
}


#ifndef ONLY_CONVERT_FUNC
static
ofileopen( ofile )
char	*ofile;
{
	if( ofile ){
		if( !(FO= fopen( ofile, "w" )) ){
#if HUMAN
			fputs( ofile, stderr );
			fputs( ":can't create\n", stderr );
#else
			perror( ofile );
#endif
			return	FALSE;
		}
	}else
		FO= stdout;
	return	TRUE;
}

/* static */
int
kanjidrv( ifile, ofile, cp )
char	*ifile, *ofile;
T_KANJI	*cp;
{
	if( cp->flag & fCHECK )
		ofile= NULL;
	if( ifile ){
		if( !(cp->flag & fDIRSKIP) && isdir( ifile ) )
			return	SKIP;
		if( FP= fopen( ifile, "r" ) ){
#if HUMAN
			cp->time= FILEDATE( fileno(FP), 0 );
#else
			cp->time= filedate( fileno(FP) );
#endif
			if( cp->inpcode == NONE )
				preread( cp );
/*			if( (cp->flag&fCODESKIP) && cp->inpcode==cp->outcode ){
				fclose( FP );
				return	SKIP;
			}*/
			if( !ofileopen( ofile ) ){
				fclose( FP );
				return	FALSE;
			}
			KanjiSetup( cp );
			SjisEucConvert( cp );
			fclose( FP );
		}else{
#if HUMAN
			fputs( ifile, stderr );
			fputs( ":can't open\n", stderr );
#else
			perror( ifile );
#endif
			return	FALSE;
		}
	}else{
		FP= stdin;
		if( cp->inpcode == NONE )
			preread( cp );
		if( !ofileopen( ofile ) )
			return	FALSE;
		KanjiSetup( cp );
		SjisEucConvert( cp );
	}
	if( cp->outcode == JIS )
		setjismode();
	if( ofile ){
#if HUMAN
		if( cp->flag & fSAVETIME ){
			fflush( FO );
			FILEDATE( fileno(FO), cp->time );
		}
		fclose( FO );
#else
		fclose( FO );
		if( (cp->flag & fSAVETIME) && cp->time )
			filesetdate( ofile, cp->time );
#endif
	}
	return	TRUE;
}

static char	*_extmap[]= {
#if SHORTNAME
			".jis", ".euc", ".sji", "/", "/", ".uji", NULL };
#else
			".jis", ".euc", ".sjis", "/", "/", ".ujis", NULL };
#endif

ExtToCode( name, code )
char	*name;
{
	char	*ext;
	if( ext= SearchExtPosition( name ) ){
		int	i= 0;
		for(; _extmap[i] ; i++ ){
			if( !StrCmpAL( ext, _extmap[i] ) )
				return	i&3;	/* ujis -> euc */
		}
	}
	return	code;
}

static char *
OutputName( fname, cp )
char	*fname;
T_KANJI	*cp;
{
	static char	buf[256];	/* tenuki */
#if SHORTNAME
	strmfe( buf, fname, _extmap[cp->outcode] );
#else
	strcpy( buf, fname );
	if( ExtToCode( buf, -1 ) != -1 )
		*SearchExtPosition( buf )= '\0';
	strcat( buf, _extmap[cp->outcode] );
#endif
	return	buf;
}

static char *
TmpName( fname )
char	*fname;
{
	static char	buf[256];
#if SHORTNAME
	strmfe( buf, fname, ".t$p" );
#else
	strcpy( buf, fname );
	strcat( buf, ".tmp~" );	/* tottemo an'i */
#endif
	return	buf;
}

static char *
BakName( fname )
char	*fname;
{
	static char	buf[256];
#if SHORTNAME
	strmfe( buf, fname, ".bak" );
#else
	strcpy( buf, fname );
	strcat( buf, ".bak" );	/* tottemo an'i */
#endif
	return	buf;
}


static void
usage()
{
#if 0
	puts( "\
ack v1.39 Copyright 1993,94 Ogasawara Hiroyuki (COR.)\n\
usage: ack [-{e|s|j|c[c]}] [-{a|A|o<file>}] [-zCntud] [-{E|S}] [<file>..]\
" );
	exit( 1 );
#endif
}

static void
defaultsetoutput( cp )
T_KANJI	*cp;
{
#if 0
#if ENVACK
	if( cp->outcode == NONE )
		cp->outcode= edefauto();
#endif
#endif
#if LANGCHK
	if( cp->outcode == NONE )
		cp->outcode= ldefauto();
#endif
#if NEWSAUTO
	if( cp->outcode == NONE )
		cp->outcode= defauto();
#endif
	if( cp->outcode == NONE )
		cp->outcode= DEFCODE;
}

static
setswitch( cp, p )
T_KANJI	*cp;
char	*p;
{
#if JCONVSW
	static int	outset= FALSE;
#endif
	for(; *p ; p++ ) switch( *p ){
	case 'u':
		cp->flag|= fUNIX;
		break;
	case 'c':
		cp->flag|= fCHECK;
		if( p[1] == 'c' ){
			cp->flag|= fCODEONLY;
			p++;
		}
		break;
	case 'S':
		cp->inpcode= SJIS;
		break;
	case 'E':
		cp->inpcode= EUC;
		break;
	case 's':
#if JCONVSW
		if( outset ){
			cp->inpcode= cp->outcode;
			cp->outcode= SJIS;
			break;
		}
		outset= TRUE;
#endif
		cp->outcode= SJIS;
		break;
	case 'j':
#if JCONVSW
		if( outset ){
			cp->inpcode= cp->outcode;
			cp->outcode= JIS;
			if( Isdigit(p[1]) )
				SetJisMode( *++p & 15 );
			break;
		}
		outset= TRUE;
#endif
		cp->outcode= JIS;
		if( Isdigit(p[1]) )
			SetJisMode( *++p & 15 );
		break;
	case 'e':
#if JCONVSW
		if( outset ){
			cp->inpcode= cp->outcode;
			cp->outcode= EUC;
			break;
		}
		outset= TRUE;
#endif
		cp->outcode= EUC;
		break;
	case 'z':
		cp->flag|= fZKANA;
		break;
	case 'C':
		cp->flag|= fERRCODE;
		break;
	case 'a':
		cp->flag|= fAUTONAME/*|fCODESKIP*/;
		break;
	case 'A':
		cp->flag|= fFULLAUTO/*|fCODESKIP*/;
		break;
/*	case 'x':
		if( Isdigit(p[1]) && *++p == '0' ){
			cp->flag &= ~fCODESKIP;
		}else
			cp->flag|= fCODESKIP;
		break;*/
	case 'd':
		cp->flag|= fDIRSKIP;
		break;
	case 'n':
		cp->flag|= fAUTOKNAME;
		break;
	case 't':
		cp->flag|= fSAVETIME;
		break;
	case 'o':
		return	1;
#if 0
		if( argc >= 2 ){
			ofile= *++argv;
			argc--;
			goto _forbreak;
		}
#endif
	default:
		usage();
	}
	return	0;
/*	_forbreak:;*/
}


#if 0 /* liback.a */
main( argc, argv )
char	**argv;
{
	int	err= TRUE;
	char	*ofile= NULL;
	T_KANJI	cp;

	cp.inpcode= cp.outcode= NONE;
	cp.flag= FALSE;
	cp.time= cp.files= 0;
	*cp.fname= '\0';

	SetJisMode( DEFJIS );

#if ENVACK
	{
		char	*ptr= (char*)getenv( "ACK" );
		if( ptr && *ptr == '-' )
			setswitch( &cp, ptr+1 );
	}
#endif

	for(; --argc ;){
		if( **++argv == '-' ){
			if( setswitch( &cp, *argv+1 ) ){
				if( argc >= 2 ){
					ofile= *++argv;
					argc--;
				}else
					usage();
			}
#if 0
			char	*p= *argv+1;
			for(; *p ; p++ ) switch( *p ){
			case 'u':
				cp.flag|= fUNIX;
				break;
			case 'c':
				cp.flag|= fCHECK;
				if( p[1] == 'c' ){
					cp.flag|= fCODEONLY;
					p++;
				}
				break;
			case 'S':
				cp.inpcode= SJIS;
				break;
			case 'E':
				cp.inpcode= EUC;
				break;
			case 's':
#if JCONVSW
				if( outset ){
					cp.inpcode= cp.outcode;
					cp.outcode= SJIS;
					break;
				}
				outset= TRUE;
#endif
				cp.outcode= SJIS;
				break;
			case 'j':
#if JCONVSW
				if( outset ){
					cp.inpcode= cp.outcode;
					cp.outcode= JIS;
					if( Isdigit(p[1]) )
						SetJisMode( *++p & 15 );
					break;
				}
				outset= TRUE;
#endif
				cp.outcode= JIS;
				if( Isdigit(p[1]) )
					SetJisMode( *++p & 15 );
				break;
			case 'e':
#if JCONVSW
				if( outset ){
					cp.inpcode= cp.outcode;
					cp.outcode= EUC;
					break;
				}
				outset= TRUE;
#endif
				cp.outcode= EUC;
				break;
			case 'z':
				cp.flag|= fZKANA;
				break;
			case 'C':
				cp.flag|= fERRCODE;
				break;
			case 'a':
				cp.flag|= fAUTONAME/*|fCODESKIP*/;
				break;
			case 'A':
/*				cp.flag|= fFULLAUTO|fCODESKIP;*/
				cp.flag|= fFULLAUTO;
				break;
/*			case 'x':
				if( Isdigit(p[1]) && *++p == '0' ){
					cp.flag &= ~fCODESKIP;
				}else
					cp.flag|= fCODESKIP;
				break;*/
			case 'd':
				cp.flag|= fDIRSKIP;
				break;
			case 'n':
				cp.flag|= fAUTOKNAME;
				break;
			case 't':
				cp.flag|= fSAVETIME;
				break;
			case 'o':
				if( argc >= 2 ){
					ofile= *++argv;
					argc--;
					goto _forbreak;
				}
			default:
				usage();
			}
			_forbreak:;
#endif
		}else{
			int	ic= cp.inpcode;
			defaultsetoutput( &cp );
			cp.files++;
			strcpy( cp.fname, *argv );
			if( cp.inpcode == NONE && !(cp.flag & fAUTOKNAME) )
				cp.inpcode= ExtToCode( *argv, cp.inpcode );
			if( ofile ){
				err= kanjidrv( *argv, ofile, &cp );
			}else if( cp.flag & fFULLAUTO ){
				err= kanjidrv( *argv, TmpName(*argv), &cp );
				if( err == TRUE ){
					unlink( BakName(*argv) );
					rename( *argv, BakName(*argv) );
					rename( TmpName(*argv), *argv );
				}
			}else{
				if( cp.flag & fAUTONAME )
					ofile= OutputName( *argv, &cp );
				err= kanjidrv( *argv, ofile, &cp );
			}
			ofile= NULL;
			cp.inpcode= ic;
		}
	}
	if( !cp.files ){
		defaultsetoutput( &cp );
		err= kanjidrv( NULL, ofile, &cp );
	}
	return	!err;
}
#endif /* liback.a */

#if NEWSAUTO
#include	<sgtty.h>
#define	TTYNAME	"/dev/tty"
defauto()
{
	int	kanji, fn;
	if( (fn= open( TTYNAME, 0 )) >= 0 ){
		ioctl( fn, TIOCKGET, &kanji );
		close( fn );
		if( kanji & KM_SYSEUC )
			return	EUC;
		return	SJIS;
	}
	return	NONE;
}
#endif

#if LANGCHK
static
declang( ptr )
char	*ptr;
{
	if( ptr= (char*)strchr( ptr, '.' ) ){
		if( !StrCmpAL( ptr, ".sjis" ) )
			return	SJIS;
		if( !StrCmpAL( ptr, ".euc" ) )
			return	EUC;
	}
	return	NONE;
}
ldefauto()
{
	int	kanji= NONE;
	char	*ptr= (char*)getenv( "LANG" );
	if( ptr )
		kanji= declang( ptr );
	if( kanji == NONE ){
		FILE	*fp;
		char	buf[256];
		if( fp= fopen( "/etc/sysinfo", "r" ) ){
			for(; fgets( buf, 256, fp ) ;){
				if( *buf == 'L' ){
					kanji= declang( buf );
					break;
				}
			}
			fclose( fp );
		}
	}
	return	kanji;
}
#endif

#if 0
#if ENVACK
edefauto()
{
	char	*ptr= (char*)getenv( "ACK" );
	if( ptr ){
		if( !StrCmpAL( ptr, "sjis" ) )
			return	SJIS;
		if( !StrCmpAL( ptr, "euc" ) )
			return	EUC;
		if( !StrCmpAL( ptr, "jis" ) )
			return	JIS;
	}
	return	NONE;
}
#endif
#endif

#endif /* ONLY_CONVERT_FUNC */