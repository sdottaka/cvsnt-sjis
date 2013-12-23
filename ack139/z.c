/* Copyright 1993 H.Ogasawara (COR.) */

main()
{
	int	i, j;
	printf( "static unsigned short	zenmap1[]= {\n\t" );
	for(; (i= getchar()) >= 0 ;){
		if( i == '\n' ){
			putchar( '\n' );
			putchar( '\t' );
			continue;
		}
		j= getchar()&0xff;
		if( i < 0x80 )
			printf( "0x0000," );
		else
			printf( "0x%02x%02x,", i&0x7f, j&0x7f );
	}
	printf( "};\n" );
	return	0;
}
