/*	Copyright 1993,94 H.Ogasawara (COR.)	*/

/* v1.10  1994  3/19	Ogasawara Hiroyuki		*/
/*			oga@dgw.yz.yamagata-u.ac.jp	*/

#include	"kanjicode.h"
#include	"ackstring.h"

SjisEucCheck( ptr, len, cp )
unsigned char	*ptr;
T_KANJI		*cp;
{
	unsigned char	*endp= ptr+len;
	int		sjis= 0, euc= 0, step = 0;   /* step no init original BUG */
	for(; ptr < endp ; ptr+= step ){
		step= 1;
		if( *ptr < 0x80 )
			continue;
		if( *ptr < 0xa0 ){
			if( *ptr == 0x8e && IsKana( ptr[1] ) ){
				euc+= 2;	/* single shift */
				step= 2;
			}
			if( IsSjis2( ptr[1] ) ){
				sjis+= 2;
				step= 2;
			}
			continue;
		}
		if( *ptr < 0xe0 ){
			sjis++;
			if( IsEuc2( ptr[1] ) ){
				step= 2;
				euc+= 2;
				if( IsKana( ptr[1] ) )
					sjis++;
			}
			continue;
		}
		if( *ptr < 0xf0 ){
			if( IsEuc2( ptr[1] ) ){
				step= 2;
				euc+= 2;
			}
			if( IsSjis2( ptr[1] ) ){
				step= 2;
				sjis+= 2;
			}
			continue;
		}
		if( *ptr < 0xff && IsEuc2( ptr[1] ) ){
			step= 2;
			euc+= 2;
		}
	}
/* printf( "chk: %d %d\n", sjis, euc ); */
	cp->inpcode= sjis > euc ? SJIS : EUC;
}

