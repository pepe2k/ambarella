#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>


#include "md5_check.h"

#define LOG_MESSG(MSG) do { \
	FILE* LOG; \
	LOG = fopen("/webSvr/web/cgi-bin/debug","a+"); \
	fprintf(LOG,"%s\n", MSG); \
	fclose(LOG); } \
while(0)

const char bb_hexdigits_upcase[] = "0123456789ABCDEF";


char* bin2hex(char *p, const char *cp, int count)
{
	while (count) {
		unsigned char c = *cp++;
		/* put lowercase hex digits */
		*p++ = 0x20 | bb_hexdigits_upcase[c >> 4];
		*p++ = 0x20 | bb_hexdigits_upcase[c & 0xf];
		count--;
	}
	return p;
}
static unsigned char *_hash_bin_to_hex(unsigned char *hash_value,
				unsigned hash_length)
{
	/* xzalloc zero-terminates */
	char *hex_value = (char*)malloc((hash_length * 2) + 1);
	memset(hex_value, 0, (hash_length * 2) + 1);
	if( hex_value == NULL ) {
		LOG_MESSG("hash value mallocate error");
	}
	bin2hex(hex_value, (char*)hash_value, hash_length);
	return (unsigned char *)hex_value;
}

void md5_hash(md5_ctx_t *ctx, const void *buffer, size_t len) {
	unsigned bufpos = ctx->total64 & 63;

	ctx->total64 += len;

	while (1) {
		unsigned remaining = 64 - bufpos;
		if (remaining > len)
			remaining = len;
		/* Copy data into aligned buffer */
		memcpy(ctx->wbuffer + bufpos, buffer, remaining);
		len -= remaining;
		buffer = (const char *)buffer + remaining;
		bufpos += remaining;
		/* clever way to do "if (bufpos != 64) break; ... ; bufpos = 0;" */
		bufpos -= 64;
		if (bufpos != 0)
			break;
		/* Buffer is filled up, process it */
		ctx->process_block(ctx);
		/*bufpos = 0; - already is */
	}
}

void md5_end(md5_ctx_t *ctx, void *resbuf)
{
	unsigned bufpos = ctx->total64 & 63;
	/* Pad the buffer to the next 64-byte boundary with 0x80,0,0,0... */
	ctx->wbuffer[bufpos++] = 0x80;

	/* This loop iterates either once or twice, no more, no less */
	while (1) {
		unsigned remaining = 64 - bufpos;
		memset(ctx->wbuffer + bufpos, 0, remaining);
		/* Do we have enough space for the length count? */
		if (remaining >= 8) {
			/* Store the 64-bit counter of bits in the buffer */
			uint64_t t = ctx->total64 << 3;
			/* wbuffer is suitably aligned for this */
			*(uint64_t *) (&ctx->wbuffer[64 - 8]) = t;
		}
		ctx->process_block(ctx);
		if (remaining >= 8)
			break;
		bufpos = 0;
	}

	/* The MD5 result is in little endian byte order */
	memcpy(resbuf, ctx->hash, sizeof(ctx->hash[0]) * 4);
}

static void md5_process_block64(md5_ctx_t *ctx)
{

	/* Before we start, one word to the strange constants.
	   They are defined in RFC 1321 as
	   T[i] = (int)(4294967296.0 * fabs(sin(i))), i=1..64
	 */
	static const uint32_t C_array[] = {
		/* round 1 */
		0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
		0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
		0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
		0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
		/* round 2 */
		0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
		0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
		0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
		0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
		/* round 3 */
		0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
		0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
		0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x4881d05,
		0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
		/* round 4 */
		0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
		0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
		0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
		0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
	};
	static const char P_array[] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, /* 1 */
		1, 6, 11, 0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, /* 2 */
		5, 8, 11, 14, 1, 4, 7, 10, 13, 0, 3, 6, 9, 12, 15, 2, /* 3 */
		0, 7, 14, 5, 12, 3, 10, 1, 8, 15, 6, 13, 4, 11, 2, 9  /* 4 */
	};

	uint32_t *words = (void*) ctx->wbuffer;
	uint32_t A = ctx->hash[0];
	uint32_t B = ctx->hash[1];
	uint32_t C = ctx->hash[2];
	uint32_t D = ctx->hash[3];

 /* 2 or 3 */

	static const char S_array[] = {
		7, 12, 17, 22,
		5, 9, 14, 20,
		4, 11, 16, 23,
		6, 10, 15, 21
	};
	const uint32_t *pc;
	const char *pp;
	const char *ps;
	int i;
	uint32_t temp;

 /* MD5_SIZE_VS_SPEED == 2 */
	pc = C_array;
	pp = P_array;
	ps = S_array;

	for (i = 0; i < 16; i++) {
		temp = A + FF(B, C, D) + words[(int) (*pp++)] + *pc++;
		temp = rotl32(temp, ps[i & 3]);
		temp += B;
		A = D;
		D = C;
		C = B;
		B = temp;
	}
	ps += 4;
	for (i = 0; i < 16; i++) {
		temp = A + FG(B, C, D) + words[(int) (*pp++)] + *pc++;
		temp = rotl32(temp, ps[i & 3]);
		temp += B;
		A = D;
		D = C;
		C = B;
		B = temp;
	}
	ps += 4;
	for (i = 0; i < 16; i++) {
		temp = A + FH(B, C, D) + words[(int) (*pp++)] + *pc++;
		temp = rotl32(temp, ps[i & 3]);
		temp += B;
		A = D;
		D = C;
		C = B;
		B = temp;
	}
	ps += 4;
	for (i = 0; i < 16; i++) {
		temp = A + FI(B, C, D) + words[(int) (*pp++)] + *pc++;
		temp = rotl32(temp, ps[i & 3]);
		temp += B;
		A = D;
		D = C;
		C = B;
		B = temp;
	}

	/* Add checksum to the starting values */
	ctx->hash[0] += A;
	ctx->hash[1] += B;
	ctx->hash[2] += C;
	ctx->hash[3] += D;

}

void md5_begin(md5_ctx_t *ctx)
{
	ctx->hash[0] = 0x67452301;
	ctx->hash[1] = 0xefcdab89;
	ctx->hash[2] = 0x98badcfe;
	ctx->hash[3] = 0x10325476;
	ctx->total64 = 0;
	ctx->process_block = md5_process_block64;
}

ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t n;

	do {
		n = read(fd, buf, count);
	} while (n < 0);

	return n;
}

uint8_t *_hash_file(const char *filename)
{
	int hash_len, count;
	md5_ctx_t md5;
	FILE* src_fd;
	uint8_t *hash_value;
	void (*update)(void*, const void*, size_t);
	void (*final)(void*, void*);

	src_fd = fopen(filename, "rb+");
	if (src_fd == NULL) {
		return NULL;
	}

	/* figure specific hash algorithms */
		md5_begin(&md5);
		update = (void*)md5_hash;
		final = (void*)md5_end;
		hash_len = 16;

	{
		unsigned char* in_buf = malloc(4096);
		if (in_buf == NULL) {
			return NULL;
		}
		while ((count = fread(in_buf, 1, 4096, src_fd))> 0) {
			update(&md5, in_buf, count);
		}
		hash_value = NULL;
		if (count == 0) {
			final(&md5, in_buf);
			hash_value = _hash_bin_to_hex(in_buf, hash_len);
		}
		free(in_buf);
		in_buf = NULL;
	}

	fclose(src_fd);

	return hash_value;
}

