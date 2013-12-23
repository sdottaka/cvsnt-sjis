/*	Copyright 1993 H.Ogasawara (COR.)	*/

/* v1.00  1993 10/10	*/
/* v1.10  1994  3/19	*/
/* v1.20  1994  3/21	*/

#define PATHLEN		256

#define	ERRCODE		0xfffff
#define	ERRCHAR		0x2228	/* KomeMark */

#define	PACKWORD(h,l)	(((h)<<8)+(l))

#define	TRUE	1
#define	FALSE	0
#define	SKIP	2

#define	fZKANA		1
#define	fERRCODE	2
#define	fAUTONAME	4
#define	fAUTOKNAME	8
#define	fFULLAUTO	16
#define	fCHECK		32
#define	fUNIX		64
#define	fSHORTNAME	128
#define	fCODEONLY	0x100
#define	fSAVETIME	0x200
#define	fCODESKIP	0x400
#define	fDIRSKIP	0x800

#define	JIS		0
#define	EUC		1
#undef SJIS
#define	SJIS		2
#define	NONE		8

typedef struct {
	int	flag,
		inpcode,
		outcode,
		files,
		time;
	unsigned char	*kanjicheck_h;
	unsigned int	(*kanjitocode)(),
			(*codetokanji)();
	unsigned char	fname[PATHLEN];
} T_KANJI;

#define	ROMA	0
#define	ASCII	1
#define	KANA	2
#define	KANJI	3
#define	QKANJI	4
#define	SI	5
#define	SO	6
#define	KANJI2	7
#define	QKANJI2	8

#ifndef DEFJIS
#  define DEFJIS	0
#endif
#ifndef DEFCODE
#  define DEFCODE	1
#endif

