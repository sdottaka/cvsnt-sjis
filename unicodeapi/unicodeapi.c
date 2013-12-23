#include <config.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "unicodeapi.h"

/* Don't know what to do if there's no iconv.h... probably
   we'll disable all the unicode/codepage conversions */
#ifdef HAVE_ICONV_H
#include <iconv.h>
#else
#error need iconv.h
#endif

#ifdef HAVE_LIBCHARSET_H
#include <libcharset.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#ifndef HAVE_LOCALE_CHARSET
#ifdef HAVE_NL_LANGINFO
const char *locale_charset()
{
	return nl_langinfo(CODESET);
}
#else
const char *loclale_charset()
{
	return "";
}
#endif
#endif

static iconv_t g_ic;
static int g_blockcount;
static encoding_type2 g_from, g_to;

/* The 'default' encoding is for UTF8-BOM for storage in the RCS files */
const encoding_type2 __encoding_utf8 = { "UTF-8" ,1 };

/* Force detection - should not be used as target */
const encoding_type2 __encoding_null = { NULL ,0 };

/* People sometimes abbreviate character set names... */
static const char *common_mapping(const char *cp)
{
	if(!strcasecmp(cp,"utf8"))
		return "UTF-8";
	if(!strcasecmp(cp,"ucs2"))
		return "UCS-2";
	if(!strcasecmp(cp,"ucs4"))
		return "UCS-4";
	if(!strcasecmp(cp,"utf16"))
		return "UTF-16";
	if(!strcasecmp(cp,"utf32"))
		return "UTF-32";
	return cp;
}

static int guess_encoding(const char *buf, size_t len, encoding_type2 *type, const encoding_type2 *hint)
{
	const unsigned short *c;
	int lowchar_count, swap_lowchar_count;

	if(len>2 && (buf[0]==(char)0xef && buf[1]==(char)0xbb && buf[2]==(char)0xbf))
	{
		/* UTF8 */
		if(type)
		{
			type->encoding="UTF-8";
			type->bom=1;
		}
		return 0;
	}

	if(len<2 || len&1)
	{
		if(type)
		{
			type->encoding=NULL; 
			type->bom=0;
		}
		return 0; // Odd length files (by definition) can't be encoding
	}

	// Check for encoding header
	if(buf[0]==(char)0xff && buf[1]==(char)0xfe)
	{
		if(type)
		{
			type->encoding="UCS-2LE";
			type->bom=1;
		}
		return 0;
	}

	// Byteswap encoding header
	if(buf[0]==(char)0xfe && buf[1]==(char)0xff)
	{
		if(type)
		{
			type->encoding="UCS-2BE";
			type->bom=1;
		}
		return 0;
	}

	if(hint)
	{
		if(type) *type=*hint;
		return 0;
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
		if(type)
		{
			type->encoding="UCS-2LE";
			type->bom=0;
		}
		return 0;
	}
	// same for byteswapped
	if(swap_lowchar_count>(len*8)/10)
	{
		if(type)
		{
			type->encoding="UCS-2BE";
			type->bom=0;
		}
		return 0;
	}

	if(type)
	{
		type->encoding=NULL;
		type->bom=0;
	}
	return 0; 
}

int is_valid_encoding(const char *enc)
{
	iconv_t ic;
	if((ic = iconv_open(enc,locale_charset()))<0)
		return 0;
	iconv_close(ic);
	return 1;
}

int begin_encoding(const encoding_type2 *from, const encoding_type2 *to)
{
	g_blockcount=0;
	g_from = *from;
	g_to = *to;
	g_ic = 0;

	//TRACE(3,"begin_encoding(%s,%s)",PATCH_NULL(from->encoding),PATCH_NULL(to->encoding));
	return 0;
}

int set_bytestream()
{
	if(g_blockcount)
	  return 0;

	if(((!g_from.encoding && !g_to.encoding) || !strcmp(g_from.encoding?g_from.encoding:locale_charset(), g_to.encoding?g_to.encoding:locale_charset())))
	{
		//TRACE(3,"set_bytestream: no conversion needed - disabling");
	    g_blockcount=-1;
		return 0;
	}

	if((g_ic = iconv_open(g_to.encoding?g_to.encoding:locale_charset(),g_from.encoding?g_from.encoding:locale_charset()))<0)
		return -1;
//		error(1,errno,"Unable to convert between %s and %s",g_from.encoding?g_from.encoding:locale_charset(), g_to.encoding?g_to.encoding:locale_charset());
	g_blockcount++;
	return 1;
}

int end_encoding()
{
	if(g_ic && g_blockcount>=0)
	  iconv_close(g_ic);
	g_ic = 0;
	//TRACE(3,"end_encoding()");
	return 0;
}

int convert_encoding(const char *inbuf, size_t len, char **outbuf, size_t *outlen)
{
	encoding_type2 from;
	size_t in_remaining;
	size_t out_remaining;
	const char *inbufp = inbuf;
	char *outbufp = *outbuf;

	if(!len)
		return 0;

	if(g_blockcount<0)
	  return 0; /* A previous encoding failed, so we've stopped */

	if(!g_blockcount)
	{
		guess_encoding(inbuf,len,&from, &g_from);

		//TRACE(3,"unicode conversion %s => %s",from.encoding?from.encoding:locale_charset(), g_to.encoding?g_to.encoding:locale_charset());

		if(((!from.encoding && !g_to.encoding) || !strcmp(from.encoding?from.encoding:locale_charset(), g_to.encoding?g_to.encoding:locale_charset())) && from.bom==g_to.bom)
		{
			//TRACE(3,"convert_encoding: no conversion needed - disabling");
			g_blockcount=-1;
			return 0;
		}

		if((g_ic = iconv_open(g_to.encoding?g_to.encoding:locale_charset(),from.encoding?from.encoding:locale_charset()))<0)
			return -1;
//			error(1,errno,"Unable to convert between %s and %s",from.encoding?from.encoding:locale_charset(), g_to.encoding?g_to.encoding:locale_charset());
	}

	if(!*outbuf)
	{
		*outlen = (len * 4) + 4; /* Enough for ansi -> ucs4-le + a BOM */
		*outbuf = (char*)malloc(*outlen);
		outbufp=*outbuf;
	}

	in_remaining = len;
	out_remaining = *outlen;
	if(!g_blockcount)
	{
		if(from.bom)
		{
			if(!strcmp(from.encoding,"UTF-8")) { if(in_remaining>2 && (inbuf[0]==(char)0xef && inbuf[1]==(char)0xbf && inbuf[2]==(char)0xbb)) { inbufp+=3; in_remaining-=3; } }
			else if(!strcmp(from.encoding,"UCS-2LE")) { if(inbuf[0]==(char)0xff && inbuf[1]==(char)0xfe) { inbufp+=2; in_remaining-=2; } }
			else if(!strcmp(from.encoding,"UCS-2BE")) { if(inbuf[0]==(char)0xfe && inbuf[1]==(char)0xff) { inbufp+=2; in_remaining-=2; } }
		}

		if(g_to.bom)
		{
			if(!strcmp(g_to.encoding,"UTF-8")) { (*outbuf)[0]=(char)0xef; (*outbuf)[1]=(char)0xbb; (*outbuf)[2]=(char)0xbf; outbufp+=3; out_remaining-=3; }
			else if(!strcmp(g_to.encoding,"UCS-2LE")) { (*outbuf)[0]=(char)0xff; (*outbuf)[1]=(char)0xfe; outbufp+=2; out_remaining-=2; }
			else if(!strcmp(g_to.encoding,"UCS-2BE")) { (*outbuf)[0]=(char)0xfe; (*outbuf)[1]=(char)0xff; outbufp+=2; out_remaining-=2; }
		}
	}
	g_blockcount++;

	if(iconv(g_ic,(iconv_arg2_t)&inbufp,&in_remaining,&outbufp,&out_remaining)<0)
		return -1;
//		error(1,errno,"Buffer conversion failed");
	*outlen-= out_remaining;
	return 1;
}


/* The input will either be UTF-8 or some kind of codepage from the server.
   Expand the buffer to UCS-2-LE, and also expand a CR and CRLF buffer (so
   we don't have to worry about platform specifics).  Then add the cr/lf pairs
   as required.

   For Unix we don't have to any of this, of course :) */
int output_as_encoded(int fd, const char *buf, size_t len)
{
	char *outbuf = NULL;
	size_t outlen;
	int expand_crlf = 0;

	//TRACE(3,"output_as_encoded()");
#ifdef _WIN32
	if(getmode(fd)==_O_TEXT)
		expand_crlf = 1;
	setmode(fd,_O_BINARY); // Don't want CRLF default stuff
#endif

	if(!expand_crlf)
	{
		convert_encoding(buf,len,&outbuf,&len);
		if(write(fd,outbuf,len)<len)
		{
			free(outbuf);
			/* error - reported by caller */
			return 1;
		}
		free(outbuf);
		return 0;
	}
	else
	{
		char lf[2] = { 0x0a, 0x00 };
		char crlf[4] = { 0x0d, 0x00, 0x0a, 0x00 };
		char outch[256];

		encoding_type2 from;
		size_t in_remaining;
		size_t out_remaining;
		const char *inbufp = buf;
		char *outbufp;
		char *p;
		int n;

		guess_encoding(buf,len,&from, &g_from);

		//TRACE(3,"unicode conversion & crlf expand %s => %s",from.encoding?from.encoding:locale_charset(), g_to.encoding?g_to.encoding:locale_charset());

		if((!from.encoding && !g_to.encoding))
		{
			//TRACE(3,"no conversion - writing as normal");
#ifdef _WIN32
			setmode(fd,_O_TEXT); // Want crlf default stuff
#endif
			if(write(fd,buf,len)<len)
			{
				/* error - reported by caller */
				return 1;
			}
			return 0;
		}

		if((g_ic = iconv_open("UCS-2LE",from.encoding?from.encoding:locale_charset()))<0)
			return -1;
//				error(1,errno,"Unable to convert between %s and UCS2-LE",from.encoding?from.encoding:locale_charset());

		outlen = (len * 4); /* Enough for ansi -> ucs2-le w/some overhead for crlf*/
		outbuf = (char*)malloc(outlen);
		outbufp=outbuf;

		in_remaining = len;
		out_remaining = outlen;
		if(from.bom)
		{
			if(!strcmp(from.encoding,"UTF-8")) { if(buf[0]==(char)0xef && buf[1]==(char)0xbb && buf[2]==(char)0xbf) { inbufp+=3; in_remaining-=3; } }
			else if(!strcmp(from.encoding,"UCS-2LE")) { if(buf[0]==(char)0xff && buf[1]==(char)0xfe) { inbufp+=2; in_remaining-=2; } }
			else if(!strcmp(from.encoding,"UCS-2BE")) { if(buf[0]==(char)0xfe && buf[1]==(char)0xff) { inbufp+=2; in_remaining-=2; } }
		}

		if(iconv(g_ic,(iconv_arg2_t)&inbufp,&in_remaining,&outbufp,&out_remaining)<0)
			return -1;
//			error(1,errno,"Buffer conversion failed");
		outlen-= out_remaining;

		for(p=outbuf, n=outlen; n; p+=2, n-=2)
		{
			if(!memcmp(p,lf,2))
			{
				memmove(p+4,p+2,n-2);
				memcpy(p,crlf,4);
				p+=2;
				outlen+=2;
			}
		}
		
		iconv_close(g_ic);
		if((g_ic = iconv_open(g_to.encoding?g_to.encoding:locale_charset(),"UCS-2LE"))<0)
			return -1;
//				error(1,errno,"Unable to convert between UCS2-LE and %s",g_to.encoding?g_to.encoding:locale_charset());

		if(g_to.bom)
		{
			if(!strcmp(g_to.encoding,"UTF-8")) { outch[0]=(char)0xef; outch[1]=(char)0xbb; outch[2]=(char)0xbf; len=3; }
			else if(!strcmp(g_to.encoding,"UCS-2LE")) { outch[0]=(char)0xff; outch[1]=(char)0xfe; len=2; }
			else if(!strcmp(g_to.encoding,"UCS-2BE")) { outch[0]=(char)0xfe; outch[1]=(char)0xff; len=2; }
			else len=0;
			if(len)
			{
				if(write(fd,outch,len)<len)
				{
					/* error - reported by caller */
					return 1;
				}
			}
		}

		inbufp=outbuf;
		while(outlen)
		{
			size_t inr;
			in_remaining=sizeof(outch)/4;
			if(in_remaining>outlen) in_remaining=outlen;
			inr = in_remaining;
			out_remaining=sizeof(outch);
			outbufp=outch;
			if(iconv(g_ic,(iconv_arg2_t)&inbufp,&in_remaining,&outbufp,&out_remaining)<0)
				return -1;
//				error(1,errno,"Buffer conversion failed");
			len=sizeof(outch)-out_remaining;
			if(len)
			{
				if(write(fd,outch,len)<len)
				{
					/* error - reported by caller */
					return 1;
				}
			}
			outlen-=inr;
		}

		return 0;
	}
}

int strip_crlf(char *buf, size_t *len)
{
#ifdef _WIN32
	char *p;
	for(p=buf; p<buf+(*len)-1; p++)
	{
		if(p[0]=='\x0d' && p[1]=='\x0a')
		{
			memcpy(p,p+1,(*len)-((p-buf)+1));
			(*len)--;
		}
	}
#endif
	return 0;
}

const char *get_local_charset()
{
	return locale_charset();
}

/* Fast transcode, used by the client to cover filenames etc. */
int transcode_buffer(const char *from, const char *to, const char *buffer, size_t len, char **outbuf, size_t *olen)
{
	const char *inbufp=buffer;
	size_t in_remaining = len?len:strlen(buffer)+1;
	char *outbufp=*outbuf=(char*)malloc(in_remaining*4);
	size_t outlen, out_remaining = outlen = in_remaining*4;
	iconv_t ic;
	int chars_deleted = 0;

	to = common_mapping(to);
	from = common_mapping(from);

	if(((long)(ic = iconv_open(to,from)))<0)
	{
		//TRACE(3,"Transcode between %s and %s not possible",from,to);
		strcpy(*outbuf,buffer);
		return -1;
	}
	do
	{
		if(iconv(ic,(iconv_arg2_t)&inbufp,&in_remaining,&outbufp,&out_remaining)<0)
		{
			//TRACE(3,"Transcode between %s and %s failed",from,to);
			strcpy(*outbuf,buffer);
			return -1;
		}
		if(in_remaining)
		{
			inbufp++; in_remaining--;
			chars_deleted++;
		}
	} while(in_remaining);
	iconv_close(ic);
	*olen = outlen - out_remaining;
	if(!len)
		(*olen)--; /* Compensate for NULL */
	return chars_deleted;
}
