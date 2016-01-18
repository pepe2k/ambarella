/**
* src/bld/diag_crypto.c
*
* History
* 2012/07/10 - [Jahnson Diao ] created file
*
* Copyright (C) 2012-2012, Ambarella, Inc.
*
* All rights reserved. No Part of this file may be reproduced, stored
* in a retrieval system, or transmitted, in any form, or by any means,
* electronic, mechanical, photocopying, recording, or otherwise,
* without the prior consent of Ambarella, Inc.
*/

#include <bldfunc.h>
#include <ambhw.h>
#define __FLDRV_IMPL__

#if (CHIP_REV == A5S)
extern void a5s_config_aes_256(u32 decrypt, u32 *key);
extern void a5s_do_aes(u32 *input, u32 *output, u32 length);
extern void a5s_config_des(u32 decrypt, u32 *key);
extern void a5s_do_des(u32 *input, u32 *output, u32 length);
#elif (CHIP_REV == A7)
extern void a7_config_aes_256(u32 *key);
extern void a7_do_aes_enc(u32 *input, u32 *output, u32 length);
extern void a7_do_aes_dec(u32 *input, u32 *output, u32 length);
extern void a7_config_des_256(u32 *key);
extern void a7_do_des_enc(u32 *input, u32 *output, u32 length);
extern void a7_do_des_dec(u32 *input, u32 *output, u32 length);
extern void a7_md5_transform(u32 *hash,u32 *in);
extern void a7_sha1_transform(u32 *digest,const char *in);
#elif (CHIP_REV == I1) || (CHIP_REV == S2)
extern void i1_config_aes_256(u32 *key);
extern void i1_do_aes_enc(u32 *input, u32 *output, u32 length);
extern void i1_do_aes_dec(u32 *input, u32 *output, u32 length);
extern void i1_config_des_256(u32 *key);
extern void i1_do_des_enc(u32 *input, u32 *output, u32 length);
extern void i1_do_des_dec(u32 *input, u32 *output, u32 length);
extern void i1_md5_transform(u32 *hash,u32 *in);
extern void i1_sha1_transform(u32 *digest,const char *in);
#endif

#if (CHIP_REV == I1) || (CHIP_REV == A7) || (CHIP_REV == S2)
// use the same funcition and include md5 sha1
#define UNIVERSAL_AES_DES
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == I1) || (CHIP_REV == A7) || (CHIP_REV == S2)
static u32 default_aes_key[]={
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF
};

static u8 default_aes_plaintext[]={
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4A, 0x4B, 0x00, 0x00, 0x00, 0x00
};



static u8 default_aes_ciphertext[]={
	0x50,0xA0,0xC4,0x34,0x05,0x19,0xCE,0x51,
	0x89,0x3A,0xF9,0xB1,0x64,0x86,0x9F,0xF3,
	0x66,0x57,0xFA,0xD5,0x14,0x28,0x4F,0xA7,
	0x1F,0x0A,0x27,0x01,0xD2,0x39,0xC0,0xEA
};
static u32 default_des_key[]={
	0xFFFFFFFF, 0xFFFFFFFF
};

static u8 default_des_plaintext[8]={
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,

};



static u8 default_des_ciphertext[]={
		0x67,0xCB,0x42,0x90,0xC1,0xC8,0x4A,0x52,
};


/***********************************************************************************/
static int arraycmp(u8 *array1,u8 *array2, int len)
{
	int i;
	for(i=0;i<len;i++) {
		if (array1[i] != array2[i]) {
			return -1;
		}
	}
	return 0;
}
#endif
static inline  void be32_to_cpu(u32 *i)
{
	*i = (((*i&0x000000ff)<< 24)|((*i&0x0000ff00)<<8)|((*i&0x00ff0000)>>8)|((*i&0xff000000)>>24));
}
static inline u64 cpu_to_be64(u64 i)
{
	return (((i&0xffLL)<<56)|
	       ((i&0xff00LL)<<40)|
	       ((i&0xff0000LL)<<24)|
	       ((i&0xff000000LL)<<8)|
	       ((i&0xff00000000LL)>>8)|
	       ((i&0xff0000000000LL)>>24)|
	       ((i&0xff000000000000LL)>>40)|
	       ((i&0xff00000000000000LL)>>56));
}
#if (CHIP_REV == A5S)
void a5s_aes_256(void)
{
	int i;
	u8 aes_temp[32];
	a5s_config_aes_256(0, (u32 *)default_aes_key);
	a5s_do_aes((u32 *)default_aes_plaintext,
		(u32 *)aes_temp, sizeof(default_aes_plaintext));
	putstr("AES encrypt:\r\n");
	putstr("plaintext: ");
	putstr((const char*)default_aes_plaintext);
	putstr("\r\nciphertext:\r\n");
	for (i = 0; i < sizeof(aes_temp); i++) {
		uart_putstr("0x");
		uart_putbyte(aes_temp[i]);
		if (i % 16 == 15) {
			putstr(",\r\n");
		} else {
			putstr(", ");
		}
	}
	putstr("\r\n");
	if (arraycmp(aes_temp,default_aes_ciphertext,sizeof(aes_temp)) == 0) {
		uart_putstr("*******ecryption right*******\r\n");
	}else{
		uart_putstr("!!!!Err: encrytion wrong ");
	}

	a5s_config_aes_256(1, (u32 *)default_aes_key);
	a5s_do_aes((u32 *)default_aes_ciphertext,
			(u32 *)aes_temp, sizeof(default_aes_ciphertext));
	putstr("decrypt:\r\n");
	putstr("ciphertext:\r\n");
	for (i = 0; i < sizeof(default_aes_ciphertext); i++) {
		uart_putstr("0x");
		uart_putbyte(default_aes_ciphertext[i]);
		if (i % 16 == 15) {
			putstr(",\r\n");
		} else {
			putstr(", ");
		}
	}
	putstr("plaintext: ");
	putstr((const char*)aes_temp);
	putstr("\r\n");
	if (arraycmp(aes_temp,default_aes_plaintext,sizeof(aes_temp))==0){
		uart_putstr("*******decryption right*******\r\n");
	}else{
		uart_putstr("!!!!Err: dencrpytion wrong ");
	}
}

void a5s_des(void)
{
	int i;
	u8 des_temp[8];

	a5s_config_des(0, (u32 *)default_des_key);
	a5s_do_des((u32 *)default_des_plaintext,
		(u32 *)des_temp, sizeof(default_des_plaintext));
	putstr("\r\n\r\nDES encrypt:\r\n");
	putstr("plaintext: ");
	for (i=0;i<8;i++){
		uart_putchar(default_des_plaintext[i]);
	}
	putstr("\r\nciphertext:\r\n");
	for (i = 0; i < sizeof(des_temp); i++) {
		uart_putstr("0x");
		uart_putbyte(des_temp[i]);
		putstr(", ");
	}
	putstr("\r\n");
	if (arraycmp(des_temp,default_des_ciphertext,sizeof(des_temp)) == 0) {
		uart_putstr("*******ecryption right*******\r\n");
	}else{
		uart_putstr("!!!!Err: encrytion wrong ");
	}

	a5s_config_des(1, (u32 *)default_des_key);
	a5s_do_des((u32 *)default_des_ciphertext,
			(u32 *)des_temp, sizeof(default_des_ciphertext));
	putstr("decrypt:\r\n");
	putstr("ciphertext:\r\n");
	for (i = 0; i < sizeof(default_des_ciphertext); i++) {
		uart_putstr("0x");
		uart_putbyte(default_des_ciphertext[i]);
		putstr(", ");
	}
	putstr("\r\nplaintext: ");
	for (i=0;i<8;i++){
		uart_putchar(des_temp[i]);
	}
	putstr("\r\n");
	if (arraycmp(des_temp,default_des_plaintext,sizeof(des_temp))==0){
		uart_putstr("*******decryption right*******\r\n");
	}else{
		uart_putstr("!!!!Err: dencrpytion wrong \r\n");
	}
}
#endif // CHIP_REV == A5S
/*******************************************************************************/
#if defined(UNIVERSAL_AES_DES)
static void (*config_aes_256)(u32*);
static void (*do_aes_enc)(u32*,u32*,u32);
static void (*do_aes_dec)(u32*,u32*,u32);
static void (*config_des_256)(u32*);
static void (*do_des_enc)(u32*,u32*,u32);
static void (*do_des_dec)(u32*,u32*,u32);
static void (*md5_transform)(u32*,u32*);
static void (*sha1_transform)(u32*,const char*);
#if (CHIP_REV == I1) || (CHIP_REV == S2)
static void i1_swap(u32 *output,u8 pb)
{
	u32 temp[pb];
	int i,j=0;
	for(i=pb-1;i>=0;i--){
		temp[i] = *(output+j);
		be32_to_cpu(&temp[i]);
		j++;
	}
	memcpy((u8*)output,(u8*)temp,pb*4);
	return;
}
static void i1_config_aes_256_shell(u32 *key)
{
	i1_swap(key,8);
	i1_config_aes_256(key);
	return;
}

static void i1_do_aes_enc_shell(u32 *input, u32 *output, u32 length)
{
	int i;
	u32 itemp[length];
	u32 otemp[length];
	memcpy((u8*)itemp,(u8*)input,length);
	memcpy((u8*)otemp,(u8*)output,length);

	for(i=0;i< length/16;i++){
		i1_swap(itemp+i*4,4);
	}
	i1_do_aes_enc(itemp,otemp,length);
	for(i=0;i< length/16;i++){
		i1_swap(otemp+i*4,4);
	}
	memcpy((u8*)output,(u8*)otemp,length);
	return;
}
static void i1_do_aes_dec_shell(u32 *input, u32 *output, u32 length)
{
	int i;
	u32 itemp[length];
	u32 otemp[length];
	memcpy((u8*)itemp,(u8*)input,length);
	memcpy((u8*)otemp,(u8*)output,length);


	for(i=0;i< length/16;i++){
		i1_swap(itemp+i*4,4);
	}
	i1_do_aes_dec(itemp,otemp,length);
	for(i=0;i< length/16;i++){
		i1_swap(otemp+i*4,4);
	}
	memcpy((u8*)output,(u8*)otemp,length);
	return;
}

static void i1_config_des_256_shell(u32 *key)
{
	i1_swap(key,2);
	i1_config_des_256(key);
	return;
}
static void i1_do_des_enc_shell(u32 *input, u32 *output, u32 length)
{
	int i;
	u32 itemp[length];
	u32 otemp[length];
	memcpy((u8*)itemp,(u8*)input,length);
	memcpy((u8*)otemp,(u8*)output,length);
	for(i=0;i< length/8;i++){
		i1_swap(itemp+i*2,2);
	}
	i1_do_des_enc(itemp,otemp,length);
	for(i=0;i< length/8;i++){
		i1_swap(otemp+i*2,2);
	}
	memcpy((u8*)output,(u8*)otemp,length);
	return;
}
static void i1_do_des_dec_shell(u32 *input, u32 *output, u32 length)
{
	int i;
	u32 itemp[length];
	u32 otemp[length];
	memcpy((u8*)itemp,(u8*)input,length);
	memcpy((u8*)otemp,(u8*)output,length);
	for(i=0;i< length/8;i++){
		i1_swap(itemp+i*2,2);
	}
	i1_do_des_dec(itemp,otemp,length);
	for(i=0;i< length/8;i++){
		i1_swap(otemp+i*2,2);
	}
	memcpy((u8*)output,(u8*)otemp,length);
	return;
}
#endif // if (CHIP_REV == I1)
void aes_256(void)
{
	int i;
	u8 aes_temp[32];
	config_aes_256((u32 *)default_aes_key);
	do_aes_enc((u32 *)default_aes_plaintext,
		(u32 *)aes_temp, sizeof(default_aes_plaintext));
	putstr("AES encrypt:\r\n");
	putstr("plaintext: ");
	putstr((const char*)default_aes_plaintext);
	putstr("\r\nciphertext:\r\n");
	for (i = 0; i < sizeof(aes_temp); i++) {
		uart_putstr("0x");
		uart_putbyte(aes_temp[i]);
		if (i % 16 == 15) {
			putstr(",\r\n");
		} else {
			putstr(", ");
		}
	}
	if (arraycmp(aes_temp,default_aes_ciphertext,sizeof(aes_temp)) == 0) {
		uart_putstr("*******ecryption right*******\r\n");
	}else{
		uart_putstr("!!!!Err: encrytion wrong ");
	}

	config_aes_256((u32 *)default_aes_key);
	do_aes_dec((u32 *)default_aes_ciphertext,
			(u32 *)aes_temp, sizeof(default_aes_ciphertext));
	putstr("decrypt:\r\n");
	putstr("ciphertext:\r\n");
	for (i = 0; i < sizeof(default_aes_ciphertext); i++) {
		uart_putstr("0x");
		uart_putbyte(default_aes_ciphertext[i]);
		if (i % 16 == 15) {
			putstr(",\r\n");
		} else {
			putstr(", ");
		}
	}
	putstr("plaintext: ");
	putstr((const char*)aes_temp);
	putstr("\r\n");
	if (arraycmp(aes_temp,default_aes_plaintext,sizeof(aes_temp))==0){
		uart_putstr("*******decryption right*******\r\n");
	}else{
		uart_putstr("!!!!Err: dencrpytion wrong ");
	}
}

void des(void)
{
	int i;
	u8 des_temp[8];
	config_des_256((u32 *)default_des_key);
	do_des_enc((u32 *)default_des_plaintext,
		(u32 *)des_temp, sizeof(default_des_plaintext));
	putstr("\r\n\r\nDES encrypt:\r\n");
	putstr("plaintext: ");
	for (i=0;i<8;i++){
		uart_putchar(default_des_plaintext[i]);
	}
	putstr("\r\nciphertext:\r\n");
	for (i = 0; i < sizeof(des_temp); i++) {
		uart_putstr("0x");
		uart_putbyte(des_temp[i]);
		putstr(", ");
	}
	putstr("\r\n");
	if (arraycmp(des_temp,default_des_ciphertext,sizeof(des_temp)) == 0) {
		uart_putstr("*******ecryption right*******\r\n");
	}else{
		uart_putstr("!!!!Err: encrytion wrong ");
	}

	config_des_256((u32 *)default_des_key);
	do_des_dec((u32 *)default_des_ciphertext,
			(u32 *)des_temp, sizeof(default_des_ciphertext));
	putstr("decrypt:\r\n");
	putstr("ciphertext:\r\n");
	for (i = 0; i < sizeof(default_des_ciphertext); i++) {
		uart_putstr("0x");
		uart_putbyte(default_des_ciphertext[i]);
		putstr(", ");
	}
	putstr("\r\nplaintext: ");
	for (i=0;i<8;i++){
		uart_putchar(des_temp[i]);
	}
	putstr("\r\n");
	if (arraycmp(des_temp,default_des_plaintext,sizeof(des_temp))==0){
		uart_putstr("*******decryption right*******\r\n");
	}else{
		uart_putstr("!!!!Err: dencrpytion wrong \r\n");
	}
}

/*******************************************************************************/

#define MD5_DIGEST_SIZE		16
#define MD5_HMAC_BLOCK_SIZE	64
#define MD5_BLOCK_WORDS		16
#define MD5_HASH_WORDS		4

struct md5_state {
	u32 hash[MD5_HASH_WORDS];
	u32 block[MD5_BLOCK_WORDS];
	u64 byte_count;
};
struct md5_state mctx_s;
const char  *md5_plaintext="The quick brown fox jumps over the lazy dog";
static u8 correctmd5[]={
	0x9e,0x10,0x7d,0x9d,0x37,0x2b,
	0xb6,0x82,0x6b,0xd8,0x1d,0x35,
	0x42,0xa4,0x19,0xd6,'\0'
};

/* XXX: this stuff can be optimized */
static inline void le32_to_cpu_array(u32 *buf, unsigned int words)
{
	while (words--) {
//		__cpu_to_le32s(buf);
		buf++;
	}
	uart_putstr("\r\n");
}

static inline void cpu_to_le32_array(u32 *buf, unsigned int words)
{
	while (words--) {
//		__cpu_to_le32s(buf);
		buf++;
	}
}

static inline void md5_transform_helper(struct md5_state *ctx)
{
	le32_to_cpu_array(ctx->block, sizeof(ctx->block) / sizeof(u32));
	md5_transform(ctx->hash, ctx->block);
}

static int md5_init(void)
{
	struct md5_state *mctx = &mctx_s;

	mctx->hash[0] = 0x67452301;
	mctx->hash[1] = 0xefcdab89;
	mctx->hash[2] = 0x98badcfe;
	mctx->hash[3] = 0x10325476;
	mctx->byte_count = 0;
	return 0;
}

static int md5_update(const char  *data, unsigned int len)
{
	struct md5_state *mctx = &mctx_s;
	const u32 avail = sizeof(mctx->block) - (mctx->byte_count & 0x3f);

	mctx->byte_count += len;
	if (avail > len) {
		memcpy((char *)mctx->block + (sizeof(mctx->block) - avail),
		       data, len);
		return 0;
	}
	memcpy((char *)mctx->block + (sizeof(mctx->block) - avail),
	       data, avail);

	md5_transform_helper(mctx);
	data += avail;
	len -= avail;

	while (len >= sizeof(mctx->block)) {
		memcpy(mctx->block, data, sizeof(mctx->block));
		md5_transform_helper(mctx);
		data += sizeof(mctx->block);
		len -= sizeof(mctx->block);
	}

	memcpy(mctx->block, data, len);

	return 0;
}

static int md5_final(u8 *out)
{
	struct md5_state *mctx = &mctx_s;
	const unsigned int offset = mctx->byte_count & 0x3f;
	char *p = (char *)mctx->block + offset;
	int padding = 56 - (offset + 1);
	*p++ = 0x80;
	if (padding < 0) {
		memset(p, 0x00, padding + sizeof (u64));
		md5_transform_helper(mctx);
		p = (char *)mctx->block;
		padding = 56;
	}
	memset(p, 0, padding);
	mctx->block[14] = mctx->byte_count << 3;
	mctx->block[15] = mctx->byte_count >> 29;
	le32_to_cpu_array(mctx->block, (sizeof(mctx->block) -
	                  sizeof(u64)) / sizeof(u32));
	md5_transform(mctx->hash, mctx->block);
	cpu_to_le32_array(mctx->hash, sizeof(mctx->hash) / sizeof(u32));
	memcpy(out, mctx->hash, sizeof(mctx->hash));
	memset(mctx, 0, sizeof(*mctx));

	return 0;
}

static void md5(void)
{
	u8 out[16];
	int i;
	uart_putstr("\r\nMD5 test\r\ninput=");
	uart_putstr(md5_plaintext);
	md5_init();
	md5_update(md5_plaintext,strlen(md5_plaintext));
	md5_final(out);
	uart_putstr("MD5=");
	for(i=0;i<16;i++){
		uart_putbyte(out[i]);
	}
	uart_putstr("\r\n");
	if (arraycmp(out,correctmd5,sizeof(out))==0){
		uart_putstr("*******MD5 RIGHT*******\r\n");
	}else{
		uart_putstr("!!!Err:MD5 WRONG\r\n");
		uart_putstr("The correct MD5 should be 9e107d9d372bb6826bd81d3542a419d6\r\n");
	}
}

/********************************************************************************/
#define SHA1_DIGEST_SIZE        20
#define SHA1_BLOCK_SIZE         64

#define SHA1_H0		0x67452301UL
#define SHA1_H1		0xefcdab89UL
#define SHA1_H2		0x98badcfeUL
#define SHA1_H3		0x10325476UL
#define SHA1_H4		0xc3d2e1f0UL

#define SHA_WORKSPACE_WORDS 80

static u8 correctsha1[]={
	0x2f,0xd4,0xe1,0xc6,0x7a,0x2d,0x28,0xfc,
	0xed,0x84,0x9e,0xe1,0xbb,0x76,0xe7,0x39,
	0x1b,0x93,0xeb,0x12,'\0'
};

struct sha1_state {
	u64 count;
	u32 state[SHA1_DIGEST_SIZE / 4];
	u8 buffer[SHA1_BLOCK_SIZE];
};

struct sha1_state desc_s;

static int sha1_init(void)
{
	struct sha1_state *sctx = &desc_s;
	*sctx = (struct sha1_state){
		.state = { SHA1_H0, SHA1_H1, SHA1_H2, SHA1_H3, SHA1_H4 },
	};

	return 0;
}


static inline void cpu_to_be32_array(u32 *buf, unsigned int words)
{
	while (words--) {
		be32_to_cpu(buf);
		buf++;
	}
}

static void sha_transform(u32 *digest,const char*in)
{
	cpu_to_be32_array(digest,5);
	sha1_transform(digest,in);
}

static int sha1_update( const u8 *data,unsigned int len)
{
	struct sha1_state *sctx = &desc_s;
	unsigned int partial, done;
	const u8 *src;
	partial = sctx->count & 0x3f;
	sctx->count += len;
	done = 0;
	src = data;
	if ((partial + len) > 63) {
		if (partial) {
			done = -partial;
			memcpy(sctx->buffer + partial, data, done + 64);
			src = sctx->buffer;
		}

		do {
			sha_transform(sctx->state,(const char *) src);
			done += 64;
			src = data + done;
		} while (done + 63 < len);
		partial = 0;
	}
	memcpy(sctx->buffer + partial, src, len - done);

	return 0;
}


/* Add padding and return the message digest. */
static int sha1_final(u8 *out)
{
	struct sha1_state *sctx = &desc_s;
	int *dst = (int *)out;
	u32 i, index, padlen;
	s64 bits;
	static const u8 padding[64] = { 0x80, };
	bits=cpu_to_be64(sctx->count << 3);

	/* Pad out to 56 mod 64 */
	index = sctx->count & 0x3f;
	padlen = (index < 56) ? (56 - index) : ((64+56) - index);
	sha1_update(padding, padlen);

	/* Append length */
	sha1_update((const u8 *)&bits, sizeof(bits));

	/* Store state in digest */
	for (i = 0; i < 5; i++)
		dst[i] = sctx->state[i];

	/* Wipe context */
	memset(sctx, 0, sizeof *sctx);

	return 0;
}

static void sha1()
{
	u8 out[20];
	int i;
	uart_putstr("\r\nSHA1 test\r\ninput=");
	uart_putstr(md5_plaintext);
	sha1_init();
	sha1_update((const u8*)md5_plaintext,strlen(md5_plaintext));
	sha1_final(out);
	uart_putstr("\r\nSHA1=");
	for(i=0;i<20;i++){
		uart_putbyte(out[i]);
	}
	uart_putstr("\r\n");
	if (arraycmp(out,correctsha1,sizeof(out))==0){
		uart_putstr("*******SHA1 RIGHT*******\r\n");
	}else{
		uart_putstr("!!!Err:SHA1 WRONG\r\n");
		uart_putstr("The correct SHA1 should be 2fd4e1c67a2d28fced849ee1bb76e7391b93eb12\r\n");
	}

}
#endif // if defined(UNIVERSAL_AES_DES)
/********************************************************************************/

int crypto_diag(void)
{
#if (CHIP_REV == A5S)
	a5s_aes_256();
	a5s_des();
#elif (CHIP_REV == A7)
	config_aes_256=a7_config_aes_256;
	do_aes_enc=a7_do_aes_enc;
	do_aes_dec=a7_do_aes_dec;
	config_des_256=a7_config_des_256;
	do_des_enc=a7_do_des_enc;
	do_des_dec=a7_do_des_dec;
	md5_transform=a7_md5_transform;
	sha1_transform=a7_sha1_transform;
	aes_256();
	des();
	md5();
	sha1();
#elif	(CHIP_REV == I1) || (CHIP_REV == S2)
	config_aes_256=i1_config_aes_256_shell;
	do_aes_enc=i1_do_aes_enc_shell;
	do_aes_dec=i1_do_aes_dec_shell;
	config_des_256=i1_config_des_256_shell;
	do_des_enc=i1_do_des_enc_shell;
	do_des_dec=i1_do_des_dec_shell;
	md5_transform=i1_md5_transform;
	sha1_transform=i1_sha1_transform;
#if (CHIP_REV == S2)
	uart_putstr("TEST S2 ARM11\r\n");
#endif
	aes_256();
	des();
	md5();
	sha1();
#endif



	return 0;
}



