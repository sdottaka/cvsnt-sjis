/*	Copyright 1993 H.Ogasawara (COR.)	*/

/* v1.00  1993 10/10	Ogasawara Hiroyuki		*/
/*			oga@dgw.yz.yamagata-u.ac.jp	*/

#include	"kanjicode.h"

CodeToEuc( code )
unsigned int	code;
{
	unsigned int	hi=  code>>8;

	if( hi ){
		return	code|0x8080;
	}
	return	0x8e00+code;	/* EUC hankaku kana */
}

/* code memo

	7bit ascii		0000Å`007f
	2byte kanji code	2121Å`7e7e (jis)
	8bit kana		00a0Å`00df
*/

