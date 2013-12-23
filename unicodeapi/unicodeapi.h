#ifndef UNICODEAPI__H
#define UNICODEAPI__H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	const char *encoding;
	int bom;
} encoding_type2;

int is_valid_encoding(const char *enc);
int begin_encoding(const encoding_type2 *from, const encoding_type2 *to);
int set_bytestream();
int end_encoding();
int convert_encoding(const char *inbuf, size_t len, char **outbuf, size_t *outlen);
int output_as_encoded(int fd, const char *buf, size_t len);
int strip_crlf(char *buf, size_t *len);
const char *get_local_charset();
int transcode_buffer(const char *from, const char *to, const char *buffer, size_t len, char **outbuf, size_t *olen);

extern const encoding_type2 __encoding_utf8;
extern const encoding_type2 __encoding_null;
#define ENCODING_UTF8 &__encoding_utf8
#define ENCODING_UTF8 &__encoding_utf8
#define ENCODING_NULL &__encoding_null

#ifdef __cplusplus
}
#endif

#endif
