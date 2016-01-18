#ifndef UTIL_H

#define UTIL_H

#define TRUE 1
#define FALSE 0
//Ambapack

#define MSG_NAME_LEN 24
#define MSG_INFO_LEN 4096
#define PARAM_NAME_LEN 24
#define PARAM_VALUE_LEN 64
#define IP_ADDR_LEN  16


typedef enum
{
	REQ_SET_FORCEIDR = 17,
	REQ_GET_VIDEO_PORT,
	REQ_STREAM_START,
	REQ_STREAM_STOP,
	REQ_CHANGE_BR,
	REQ_CHANGE_FR,
	REQ_CHANGE_BRC,
	REQ_GET_PARAM = 100,
	REQ_SET_PARAM,
	REQ_AAA_CONTROL

}RequestId_Index;

typedef struct
{
	unsigned int req_Id;
	unsigned int info;
	unsigned int infoSize;
}req_Msg;

typedef struct
{
	unsigned int result;
	unsigned int info_length;
}ack_Msg;

typedef struct
{
	int req_cnt;
	char section_Name[MSG_NAME_LEN];
	char msg[MSG_INFO_LEN];
}Message;

typedef struct
{
	int (*pack_req)(req_Msg*, int, int, int);
	int (*pack_msg)(int,char*, char*, Message*);
	int (*parse_postSec) (char, Message*, int);
	int (*parse_postData)(char, Message*, int);
	int (*url_decode)(char, char*, int);

}AmbaPack;

typedef struct
{
	int value;
	char param_value[PARAM_VALUE_LEN];
	char param_name[PARAM_NAME_LEN];
}ParamData;

typedef struct
{
	char* sectionName;
	int sectionPort;
	ParamData* paramData;
	char* extroInfo;
	int paramDataNum;
}section_Param;

int pack_init (AmbaPack* pack);

int pack_req (req_Msg* req, int Id, int info, int infosize);

int pack_msg (int req_cnt, char* sectionName, char* extroInfo, Message* msg);

int parse_postSec (char url_string[], Message* postdata, int datanum);

int parse_postData (char url_string[], Message* postdata, int datanum);

int url_decode (char string_buffer[], char* input_str, int str_length);

int parse_sectionData (Message message, ParamData data[]);

int decode_Param (section_Param* section_param, char* recv);

//Socket Variable

#define ENCODE_PORT 20000
#define IMAGE_PORT 20002


#define HOST "10.0.0.2"
#define PORT 20000



typedef enum
{
	LIVE = ENCODE_PORT,
	DPTZ = ENCODE_PORT,
	ENCODE = ENCODE_PORT,
	PRIMASK = ENCODE_PORT,
	OSD = ENCODE_PORT,
	VINVOUT = ENCODE_PORT,
	STREAM0 = ENCODE_PORT,
	STREAM1 = ENCODE_PORT,
	STREAM2 = ENCODE_PORT,
	STREAM3 = ENCODE_PORT,
	IMAGE = IMAGE_PORT,
	IQAF = IMAGE_PORT

}SectionPort_Index;


typedef struct
{
	char host[IP_ADDR_LEN];
	int Port;
	int sockfd;

}Socket_member;


typedef struct
{
	Socket_member sk_member;
	int (*socket_connect)(int, char*);
	int (*close_connect)(int);
	int (*Send_Msg)(int, char*, int);
	int (*Recv_Msg)(char*, int, int);

}AmbaSocket;

int socket_init (char* Host, int Port, AmbaSocket* Socket);

int socket_connect (int Port, char* IP);

int close_connect (int sockfd);

int Send_Msg (int sockfd, char* msg, int length);

int Recv_Msg (char* ack, int sockfd, int msg_Length);

// AmbaTransfer

typedef struct
{
	int (*send_get_request) (section_Param*, int, Message);
	int (*send_set_request) (int, int, Message);
	int (*send_fly_request) (int, int, int);
}AmbaTransfer;

int transfer_init (AmbaTransfer* transfer);

int send_get_request (section_Param* section_param, int RequestId, Message Msg);
int send_set_request (int RequestId, int SectionPort, Message Msg);

int send_fly_request (int RequestId, int Info, int data);


#endif

