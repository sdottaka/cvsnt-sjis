/*	Copyright 1993 H.Ogasawara (COR.)	*/

/* v1.00  1993 10/10	Ogasawara Hiroyuki		*/
/*			oga@dgw.yz.yamagata-u.ac.jp	*/

#define	NULL	0

unsigned char	__code_map[]= {
	 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 8,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	18,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,
	22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,
	22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,
	22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,22,
	 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 0
};

#ifndef ONLY_CONVERT_FUNC
unsigned char *
SearchExtPosition( ptr )
unsigned char	*ptr;
{
	unsigned char	*ext= NULL;
#ifdef KANJINAME
	int	kanji= NULL;
	for(; *ptr ; ptr++ ){
		if( kanji ){
			kanji= NULL;
		}else{
			kanji= Iskanji( *ptr );
#else
	for(; *ptr ; ptr++ ){
		{
#endif
			if( *ptr == '/' || *ptr == '\\' || *ptr == ':' ){
				ext= NULL;
			}else if( *ptr == '.' )
				ext= ptr;
		}
	}
	return	ext;
}

StrCmpAL( ptr1, ptr2 )
unsigned char	*ptr1, *ptr2;
{
	for(; *ptr1 && (*ptr1|0x20) == *ptr2 ; ptr1++, ptr2++ );
	return	*ptr1-*ptr2;
	/* '.'|0x20 -> '.' */
}

#endif
