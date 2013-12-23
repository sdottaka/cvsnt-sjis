#ifndef UNICODE_STUFF__H
#define UNICODE_STUFF__H

typedef enum
{
	ENC_UNKNOWN,
	ENC_ANSI,
	ENC_UTF8,
	ENC_UCS2LE,
	ENC_UCS2BE,
	ENC_UCS2LE_BOM,
	ENC_UCS2BE_BOM,
	ENC_UCS4LE,
	ENC_UCS4BE,
	ENC_UCS4LE_BOM,
	ENC_UCS4BE_BOM,
	ENC_SHIFTJIS
} encoding_type;

int file_encoding(const char *buf, size_t len, encoding_type *type, encoding_type force);
int convert_encoding_to_utf8(char **buf, size_t len, size_t *bufsize, encoding_type force_convert);
int output_utf8_as_encoding(int fd, const unsigned char *buf, size_t len, encoding_type type);
int convert_encoding_buffer_to_utf8(const char *inbuf, size_t inlen, char *outbuf, size_t *outlen, int first, encoding_type *type);

#endif
