#ifndef MD5_CHECK_H
#define MD5_CHECK_H 1

typedef struct md5_ctx_t {
	uint8_t wbuffer[64]; /* always correctly aligned for uint64_t */
	void (*process_block)(struct md5_ctx_t*);
	uint64_t total64;    /* must be directly before hash[] */
	uint32_t hash[8];    /* 4 elements for md5, 5 for sha1, 8 for sha256 */
} md5_ctx_t;

#undef FF
#undef FG
#undef FH
#undef FI
#define FF(b, c, d) (d ^ (b & (c ^ d)))
#define FG(b, c, d) FF(d, b, c)
#define FH(b, c, d) (b ^ c ^ d)
#define FI(b, c, d) (c ^ (b | ~d))
#define rotl32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

char* bin2hex(char *p, const char *cp, int count);
uint8_t *_hash_file(const char *filename);

#endif

