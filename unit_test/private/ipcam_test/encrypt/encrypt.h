#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_alg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define SOL_ALG		279
#define ALG_OP_DECRYPT			0
#define ALG_OP_ENCRYPT			1

struct encrypt_s{
	int fd_cipher; // for encrypt file
	unsigned char enc_buf[16];   // used for align the block
	int enc_align; // used for align the start address
	int buf_offset; // used for align in the temp buffer
	int enable;   //enable encrypt

	unsigned char key[32];
	int key_len;
	unsigned int op;
	unsigned char padding;
};

struct encrypt_buff_s{
	unsigned char  *cipher_addr;
	int cipher_len;
	int buff_size;
};

extern const  char *key_atoh(char *in);
extern int oneshot_ecb_aes(unsigned char *in,int in_len,struct encrypt_buff_s *encrypt_buff,struct encrypt_s *encpoint);


