#ifndef __BASE64_H__
#define __BASE64_H__

#define BASE64_INVALID -1

#define BASE64_ENCODE_OUT_SIZE(s)	(((s) + 2) / 3 * 4)
#define BASE64_DECODE_OUT_SIZE(s)	(((s)) / 4 * 3)

#ifdef __cplusplus
extern "C"
{
#endif
int
base64_encode(unsigned char *in, int inlen, char *out);

int
base64_decode(char *in, int inlen, unsigned char *out);

#ifdef __cplusplus
}
#endif

#endif /* __BASE64_H__ */
