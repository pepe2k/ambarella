#include "encrypt.h"
#include "datatx_lib.h"


int oneshot_ecb_aes(unsigned char *in,int in_len,
						struct encrypt_buff_s *encrypt_buff,struct encrypt_s *encpoint)
{
	int opfd;
	int tfmfd;
	struct sockaddr_alg sa = {
		.salg_family = AF_ALG,
		.salg_type = "skcipher",
		.salg_name = "ecb(aes)",
	};
	struct msghdr msg = {};
	struct cmsghdr *cmsg;
	char cbuf[CMSG_SPACE(4)];
	struct iovec iov;
	int *offset = &encpoint->buf_offset;
	unsigned char *buf = encpoint->enc_buf;
	unsigned char *key = encpoint->key;
	unsigned char key_len = encpoint->key_len;
	unsigned char *out = encrypt_buff->cipher_addr;
	int *out_len = &encrypt_buff->cipher_len;
	unsigned char padding = encpoint->padding;

	tfmfd = socket(AF_ALG, SOCK_SEQPACKET, 0);

	bind(tfmfd, (struct sockaddr *)&sa, sizeof(sa));

	setsockopt(tfmfd, SOL_ALG, ALG_SET_KEY,
			key,key_len);

	opfd = accept(tfmfd, NULL, 0);

	msg.msg_control = cbuf;
	msg.msg_controllen = sizeof(cbuf);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_ALG;
	cmsg->cmsg_type = ALG_SET_OP;
	cmsg->cmsg_len = CMSG_LEN(4);
#ifdef CONFIG_ARCH_A5S
	*(__u32 *)CMSG_DATA(cmsg) = encpoint->op;// we have to use __u32 on A5s
#else
	*CMSG_DATA(cmsg) = encpoint->op;
#endif

	if(padding == 1){
		iov.iov_base = buf;
		iov.iov_len = 16;

		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		sendmsg(opfd, &msg, 0);
		read(opfd, out, 16);
		*out_len = 16;
		*offset = 0;
	}else if(*offset == 0){
		if (in_len & 0xf){
			*offset = in_len & 0xf;
			in_len = in_len & (~0xf);
			memcpy(buf,in+in_len,*offset);
		}
		iov.iov_base = in;
		iov.iov_len = in_len;

		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		sendmsg(opfd, &msg, 0);
		read(opfd, out, in_len);
		*out_len = in_len;
	}else{
		memcpy(buf+*offset,in,(16-*offset));
		iov.iov_base = buf;
		iov.iov_len = 16;

		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		sendmsg(opfd, &msg, 0);
		read(opfd, out, 16);
		*out_len = 16;
		*offset = 0;

	}

	close(opfd);
	close(tfmfd);
	return *offset;
}

static int check_hex(int in)
{
    if( ( (in>='0')&&(in<='9') )
      ||( (in>='a')&&(in<='f') )
      ||( (in>='A')&&(in<='F')) ){
		return 0;
	}else{
		return -1;
	}
}

static unsigned char ctoh(char in)
{
	if (in >='0' && in<='9'){
		return (in-'0');
	}
	if (in >='a' && in<='f'){
		return (in - 'a' + 0xa);
	}
	if (in >='A' && in<= 'F'){
		return (in - 'A' + 0xa);
	}
	return 0;
}
static char buf[32]={0};

const  char *key_atoh(char *in)
{
	int i,step=0,j=0;
	for(i=0;*(in+i)!='\0';i++){
		if (!check_hex(*(in+i))){
			if(step == 0){
				buf[j] = ctoh(*(in+i)) << 4;
				step = 1;
			}else{
				buf[j] |= ctoh(*(in+i)) ;
				step = 0;
				j++;
			}
		}else{
			printf("warning !!! -- the key is not in  HEX !!!	\n");
		}
	}
	return buf;

}

