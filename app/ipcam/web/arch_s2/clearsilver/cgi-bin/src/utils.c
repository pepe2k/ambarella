#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mqueue.h>

#include "utils.h"

mqd_t msg_queue_send, msg_queue_receive;
MESSAGE *send_buffer = NULL;
MESSAGE *receive_buffer = NULL;





int Setup_MQ (const char* send_mq, const char* receive_mq) {
	//create Message Queue

	msg_queue_send = mq_open(send_mq, O_WRONLY);
	if (msg_queue_send < 0)  {
		LOG_MESSG("%s","Error Opening MQ: ");
		return -1;
	}
	msg_queue_receive = mq_open(receive_mq, O_RDONLY|O_NONBLOCK);
	if (msg_queue_receive == -1)  {
		LOG_MESSG("%s","Error Opening MQ: ");
		return -1;
	}

	//create buffers
	send_buffer = (MESSAGE *)malloc(MAX_MESSAGES_SIZE);
	if (send_buffer == NULL){
		LOG_MESSG("%s", "MQ malloc fail!");
		return -1;
	}
	receive_buffer = (MESSAGE *)malloc(MAX_MESSAGES_SIZE);
	if (receive_buffer == NULL){
		LOG_MESSG("%s", "MQ malloc fail!");
		return -1;
	}

	while (mq_receive(msg_queue_receive, (char *)receive_buffer, MAX_MESSAGES_SIZE, NULL) > 0){
	}

	memset(send_buffer, 0, MAX_MESSAGES_SIZE);
	memset(receive_buffer, 0, MAX_MESSAGES_SIZE);

	//bind Receive callback
	//NotifySetup(&msg_queue_receive);

	return 0;
}

void SendData () {
	while (1){
		if (0 != mq_send (msg_queue_send, (char *)send_buffer, MAX_MESSAGES_SIZE, 0)) {
			LOG_MESSG("%s", "mq_send failed!");
			sleep(1);
			continue;
		}
		break;
	}
	memset(send_buffer, 0, MAX_MESSAGES_SIZE);
}