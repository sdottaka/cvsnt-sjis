/* This is actually only included by cvs... libdiff calls it though */
#include "cvs.h"

unsigned short swap2(encoding_type type, unsigned short val)
{
	if(type==ENC_UCS2BE || type==ENC_UCS2BE_BOM)
		val=((val<<8)|(val>>8));
	return val;
}

int file_encoding(const char *buf, size_t len, encoding_type *type, encoding_type force)
{
	const unsigned short *c;
	int lowchar_count, swap_lowchar_count;

	if(buf[0]==(char)0xef && buf[1]==(char)0xbb && buf[2]==(char)0xbf)
	{
		/* UTF8 */
		if(type) *type=ENC_UTF8;
		return 0;
	}

	if(len<2 || len&1)
	{
		if(type) *type=ENC_UNKNOWN; /* We don't use ENC_ANSI for compatibility with older clients */
		return 0; // Odd length files (by definition) can't be encoding
	}

	// Check for encoding header
	if(*(unsigned short *)buf == 0xfeff)
	{
		if(type) *type=ENC_UCS2LE_BOM;
		return 1;
	}

	// Byteswap encoding header
	if(*(unsigned short *)buf == 0xfffe)
	{
		if(type) *type=ENC_UCS2BE_BOM;
		return 1;
	}

	if(force!=ENC_UNKNOWN)
	{
		if(force==ENC_ANSI || force==ENC_UTF8)
			return 0;

		if(type) *type=force;
		return 1;
	}

	// Into uncertain territory...  For stuff like US-ANSI encodings then we can be fairly
	// certain, but once it gets into arabic and stuff there is no good method of autodetection
	lowchar_count=0;
	swap_lowchar_count=0;
	for(c=(const unsigned short*)buf; ((const char *)c)<(buf+len); c++)
	{
		if((*c)<128) lowchar_count++;
		if(((((*c)>>8)+(((*c)&0xff)<<8)))<128) swap_lowchar_count++;
	}

	// If >80% of the buffer is encoding<128
	if(lowchar_count>(len*8)/10)
	{
		if(type) *type=ENC_UCS2LE;
		return 1;
	}
	// same for byteswapped
	if(swap_lowchar_count>(len*8)/10)
	{
		if(type) *type=ENC_UCS2BE;
		return 1;
	}

	if(type) *type=ENC_UNKNOWN; /* We don't use ENC_ANSI for compatibility with older clients */
	return 0; // Not encoding
}

/* NT doesn't support UCS-4 only UCS-2
   Therefore this routine doesn't attempt to encode full UCS-4 encoding */
int convert_encoding_to_utf8(char **buf, size_t len, size_t *bufsize, encoding_type force_convert)
{
	const unsigned short *source;
	unsigned char *dest;
	unsigned char *destbuf;
	unsigned short c;
	encoding_type type;

	if(!file_encoding(*buf,len,&type, force_convert))
	{
		/* Non-unicode encodings */
		switch(type)
		{
		case ENC_UNKNOWN: /* 'unknown' could be UTF8... */
			break;
		case ENC_ANSI:
			error(1,0,"Internal error - attempt to convert ANSI to UTF8 which we don't support yet");
			break;
		case ENC_UTF8:
			break;
		}
		return len;
	}

	// encoding
	source=(const unsigned short*)(*buf);

	switch(type)
	{
	case ENC_UCS2LE_BOM:
	case ENC_UCS2BE_BOM:
		if(len>2 && (*source == 0xfeff || *source == 0xfffe))
		{
			// Skip over header
			source++;
		}
		break;
	case ENC_UCS4LE:
	case ENC_UCS4BE:
	case ENC_UCS4LE_BOM:
	case ENC_UCS4BE_BOM:
		error(1,0,"Internal error - attempt to convert UCS4 which we don't support yet");
		break;
	case ENC_SHIFTJIS:
		error(1,0,"Internal error - attempt to convert shift-JIS which we don't support yet");
		break;
	default:
		break;
	}

	// encoding
	destbuf=xmalloc(len*4); // dest shouldn't be more than 4* source...
	dest=destbuf+3;
	// UTF8 header
	destbuf[0]=0xef;
	destbuf[1]=0xbb;
	destbuf[2]=0xbf;

	while(source<(const unsigned short *)((*buf)+len))
	{
		c=swap2(type,*(source++));
		if(c==0x0d)
		{
			unsigned short d = swap2(type,*source);
			if(d==0x0a)
			{
				source++;
				c=0x0a;
			}
		}
		if(c<0x80)
		{
			*(dest++)=(unsigned char)c;
		}
		else if(c<0x800)
		{
			*(dest++)=0xc0+(c>>6);
			*(dest++)=0x80+(c&0x3f);
		}
		else /* 0x800-0xFFFF */
		{
			*(dest++)=0xe0+(c>>12);
			*(dest++)=0x80+((c>>6)&0x3f);
			*(dest++)=0x80+(c&0x3f);
		}
	}
	xfree(*buf);
	*bufsize=(dest-destbuf)+32;
	*buf=xrealloc(destbuf,*bufsize);
	return dest-destbuf;
}

int output_utf8_as_encoding(int fd, const unsigned char *buf, size_t len, encoding_type type)
{
	int expand_crlf = 0;
	const unsigned char *p = buf+3;
	unsigned char c,d,e;
	unsigned short *dest;
	unsigned short *destbuf = (unsigned short *)xmalloc(len*4);

	switch(type)
	{
	case ENC_ANSI:
		error(1,0,"Internal error - Attempt to convert ANSI to UTF8");
		break;
	case ENC_UNKNOWN: /* 'unknown' could be UTF8... */
	case ENC_UTF8:
		/* We treat these as equivalent, basically because we don't have a means at the moment to
		   translate ANSI to UTF8.  This is technically a bug */
		if(write(fd,buf,len)<len)
		{
			/* error - reported by caller */
			return 1;
		}
		return 0;
	case ENC_UCS4LE:
	case ENC_UCS4BE:
	case ENC_UCS4LE_BOM:
	case ENC_UCS4BE_BOM:
		error(1,0,"Internal error - Attempt to write unsupported UCS4 encoding");
		break;
	case ENC_SHIFTJIS:
		error(1,0,"Internal error - Attempt to write unsupported shift-JIS encoding");
		break;
	case ENC_UCS2LE:
	case ENC_UCS2BE:
	case ENC_UCS2LE_BOM:
	case ENC_UCS2BE_BOM:
		break;
	}

#ifdef _WIN32
	if(getmode(fd)==_O_TEXT)
		expand_crlf = 1;
#endif

	if(buf[0]!=0xef || buf[1]!=0xbb || buf[2]!=0xbf)
		p=buf;
	else
		p=buf+3;
#ifdef _WIN32
	setmode(fd,_O_BINARY); // Don't want CRLF default stuff
#endif

	dest = destbuf;
	if(type==ENC_UCS2LE_BOM || type==ENC_UCS2BE_BOM)
		*(dest++)=swap2(type,0xfeff); 

	while(p<buf+len)
	{
		c=*(p++);
		if(c<0x80)
		{
			if(c==0x0a && expand_crlf)
				*(dest++)=swap2(type,0x0d);
			*(dest++)=swap2(type,c);
		}
		else if(c<0xe0)
		{
			d=*(p++);
			*(dest++)=swap2(type,((c&0x1f)<<6)+(d&0x3f));
		}
		else
		{
			d=*(p++);
			e=*(p++);
			*(dest++)=swap2(type,((c&0x0f)<<12)+((d&0x3f)<<6)+(e&0x3f));
		}
	}
	len=(dest-destbuf)*2;
	if(write(fd,destbuf,len)<len)
	{
		/* error - reported by caller */
		xfree(destbuf);
		return 1;
	}
	xfree(destbuf);
	return 0;
}

int convert_encoding_buffer_to_utf8(const char *inbuf, size_t inlen, char *outbuf, size_t *outlen, int first, encoding_type *type)
{
	const unsigned short *source;
	unsigned char *dest;
	unsigned short c;

	if(first)
	{
		if(!file_encoding(inbuf,inlen,type, *type))
		{
			/* Non-unicode encodings */
			switch(*type)
			{
			case ENC_ANSI:
				error(1,0,"Internal error - attempt to convert ANSI to UTF8 which we don't support yet");
				break;
			case ENC_UNKNOWN:
				/* 'unknown' means do nothing... */
				memcpy(outbuf,inbuf,inlen);
				*outlen=inlen;
				break;
			case ENC_UTF8:
				dest=outbuf;
				*outlen=0;
				if(inbuf[0]!=(char)0xef || inbuf[1]!=(char)0xbb || inbuf[2]!=(char)0xbf)
				{
					dest[0]=0xef;
					dest[1]=0xbb;
					dest[2]=0xbf;
					dest+=3;
					*outlen=3;
				}
				memcpy(dest,inbuf,inlen);
				*outlen+=inlen;
				break;
			}
			return 0;
		}

		switch(*type)
		{
		case ENC_UCS2LE_BOM:
		case ENC_UCS2BE_BOM:
			if(inlen>2 && (*(unsigned short *)inbuf == 0xfeff || *(unsigned short *)inbuf == 0xfffe))
			{
				// Skip over header
				inbuf+=2;
				inlen-=2;
			}
			break;
		case ENC_UCS4LE:
		case ENC_UCS4BE:
		case ENC_UCS4LE_BOM:
		case ENC_UCS4BE_BOM:
			error(1,0,"Internal error - attempt to convert UCS4 which we don't support yet");
			break;
		case ENC_SHIFTJIS:
			error(1,0,"Internal error - attempt to convert shift-JIS which we don't support yet");
			break;
		default:
			break;
		}
	}

	source=(const unsigned short*)inbuf;
	dest = outbuf;

	if(first)
	{
		// UTF8 header
		dest[0]=0xef;
		dest[1]=0xbb;
		dest[2]=0xbf;
		dest+=3;
	}

	while(source<(const unsigned short *)(inbuf+inlen))
	{
		c=swap2(*type,*(source++));
#ifdef _WIN32
		if(c==0x0d)
		{
			unsigned short d = swap2(*type,*source);
			if(d==0x0a)
			{
				source++;
				c=0x0a;
			}
		}
#endif
		if(c<0x80)
		{
			*(dest++)=c;
		}
		else if(c<0x800)
		{
			*(dest++)=0xc0+(c>>6);
			*(dest++)=0x80+(c&0x3f);
		}
		else /* 0x800-0xFFFF */
		{
			*(dest++)=0xe0+(c>>12);
			*(dest++)=0x80+((c>>6)&0x3f);
			*(dest++)=0x80+(c&0x3f);
		}
	}
	*outlen=dest-(unsigned char *)outbuf;
	return 0;
}


