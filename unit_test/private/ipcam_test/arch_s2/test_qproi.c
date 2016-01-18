#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <basetypes.h>

#include <config.h>
#include "ambas_common.h"
#include "iav_drv.h"

#define MAX_ENCODE_STREAM_NUM		(IAV_STREAM_MAX_NUM_IMPL)
#define MAX_QP_DELTA_NUM            (3)

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif
#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align) ((size) & ~((align) - 1))
#endif

#define VERIFY_STREAMID(x)   do {		\
		if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
			printf ("stream id wrong %d \n", (x));			\
			return -1; 	\
		}	\
	} while (0)


typedef struct qp_roi_s {
	int quality_level;		// 0 means the ROI is in the default QP RC.
	int start_x;
	int start_y;
	int width;
	int height;
} qp_roi_t;

typedef struct stream_roi_s {
	int enable;
	int qp_type;
	int encode_width;
	int encode_height;
	s8 qp_delta[MAX_QP_DELTA_NUM];
} stream_roi_t;

static int exit_flag = 0;

static int fd_iav;
static u8 *qp_matrix_addr = NULL;
static int stream_qp_matrix_size = 0;

static stream_roi_t stream_roi[MAX_ENCODE_STREAM_NUM];
static qp_roi_t qp_roi[MAX_ENCODE_STREAM_NUM];

static void usage(void)
{
	printf("test_qp_roi usage:\n");
	printf("\t test_qp_roi\n"
		"\t Select a stream to set ROI and quality.\n"
		"\t Up to 3 quality levels are supported.\n"
		"\t Shall be used in ENCODE state.");
	printf("\n");
}

static void show_main_menu(void)
{
	printf("\n|-------------------------------|\n");
	printf("| Main Menu              \t|\n");
	printf("| a - Config Stream A ROI\t|\n");
	printf("| b - Config Stream B ROI\t|\n");
	printf("| c - Config Stream C ROI\t|\n");
	printf("| d - Config Stream D ROI\t|\n");
	printf("| e - Config Stream E ROI\t|\n");
	printf("| f - Config Stream F ROI\t|\n");
	printf("| g - Config Stream G ROI\t|\n");
	printf("| h - Config Stream H ROI\t|\n");
	printf("| q - Quit               \t|\n");
	printf("|-------------------------------|\n");
}

static int show_stream_menu(int stream_id)
{
	VERIFY_STREAMID(stream_id);
	printf("\n|-------------------------------|\n");
	printf("| Stream %c                \t|\n", 'A' + stream_id);
	if (!stream_roi[stream_id].qp_type) {
		printf("| 1 - Set quality level \t|\n");
	}
	printf("| 2 - Add an ROI         \t|\n");
	printf("| 3 - Remove an ROI      \t|\n");
	printf("| 4 - Clear all ROIs     \t|\n");
	printf("| 5 - View ROI           \t|\n");
	if (stream_roi[stream_id].qp_type) {
		printf("| 6 - Test ROI list       \t|\n");
	}
	printf("| q - Back to Main menu  \t|\n");
	printf("|-------------------------------|\n");
	return 0;
}

static int map_qp_matrix(void)
{
	iav_mmap_info_t qp_info;
	if (ioctl(fd_iav, IAV_IOC_MAP_QP_ROI_MATRIX_EX, &qp_info) < 0) {
		perror("IAV_IOC_MAP_QP_ROI_MATRIX_EX");
		return -1;
	}
	qp_matrix_addr = qp_info.addr;
	stream_qp_matrix_size = qp_info.length / MAX_ENCODE_STREAM_NUM;
	return 0;
}

static int check_for_qp_roi(int stream_id)
{
	iav_encode_stream_info_ex_t stream_info;
	iav_encode_format_ex_t encode_format;

	VERIFY_STREAMID(stream_id);

	memset(&stream_info, 0, sizeof(stream_info));
	stream_info.id = (1 << stream_id);
	if (ioctl(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info) < 0) {
		perror("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
		return -1;
	}
	if (stream_info.state != IAV_STREAM_STATE_ENCODING) {
		printf("Stream %c shall be in ENCODE state.\n", 'A' + stream_id);
		return -1;
	}

	memset(&encode_format, 0, sizeof(encode_format));
	encode_format.id = (1 << stream_id);
	if (ioctl(fd_iav, IAV_IOC_GET_ENCODE_FORMAT_EX, &encode_format) < 0) {
		perror("IAV_IOC_GET_ENCODE_FORMAT_EX");
		return -1;
	}
	if (encode_format.encode_type != IAV_ENCODE_H264) {
		printf("Stream %c encode format shall be H.264.\n", 'A' + stream_id);
		return -1;
	}
	stream_roi[stream_id].encode_width = encode_format.encode_width;
	stream_roi[stream_id].encode_height = encode_format.encode_height;
	return 0;
}

static int get_qp_roi(int stream_id, iav_qp_roi_matrix_ex_t *matrix)
{
	VERIFY_STREAMID(stream_id);
	matrix->id = (1 << stream_id);
	if (ioctl(fd_iav, IAV_IOC_GET_QP_ROI_MATRIX_EX, matrix) < 0) {
		perror("IAV_IOC_GET_QP_ROI_MATRIX_EX");
		return -1;
	}
	return 0;
}

static int set_qp_roi(int stream_id)
{
	iav_qp_roi_matrix_ex_t qp_matrix;
	int i, j;
	u32 *addr = (u32 *)(qp_matrix_addr + stream_qp_matrix_size * stream_id);
	u32 buf_width, buf_pitch, buf_height, start_x, start_y, end_x, end_y;
	VERIFY_STREAMID(stream_id);
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;
	qp_matrix.enable = 1;
	qp_matrix.type = stream_roi[stream_id].qp_type;

	// QP matrix is MB level. One MB is 16x16 pixels.
	buf_width = ROUND_UP(stream_roi[stream_id].encode_width, 16) / 16;
	buf_pitch = ROUND_UP(buf_width, 8);
	buf_height = ROUND_UP(stream_roi[stream_id].encode_height, 16) / 16;

	start_x = ROUND_DOWN(qp_roi[stream_id].start_x, 16) / 16;
	start_y = ROUND_DOWN(qp_roi[stream_id].start_y, 16) / 16;
	end_x = ROUND_UP(qp_roi[stream_id].width, 16) / 16 + start_x;
	end_y = ROUND_UP(qp_roi[stream_id].height, 16) / 16 + start_y;

	if (qp_matrix.type) {
		for (i = start_y; i < end_y && i < buf_height; i++) {
			for (j = start_x; j < end_x && j < buf_width; j++) {
				/* bit8~15 is used for advance qp roi type */
				addr[i * buf_pitch + j] = (((qp_roi[stream_id].quality_level +
					QP_MATRIX_ADV_DEFAULT_VALUE) & 0xff) << 8);
			}
		}
	} else {
		for (i = start_y; i < end_y && i < buf_height; i++) {
			for (j = start_x; j < end_x && j < buf_width; j++)
				addr[i * buf_pitch + j] =qp_roi[stream_id].quality_level;
		}
	}

	if (ioctl(fd_iav, IAV_IOC_SET_QP_ROI_MATRIX_EX, &qp_matrix) < 0) {
		perror("IAV_IOC_SET_QP_ROI_MATRIX_EX");
		return -1;
	}
	stream_roi[stream_id].enable = 1;

	return 0;
}

static int clear_qp_roi(int stream_id)
{
	iav_qp_roi_matrix_ex_t qp_matrix;
	int i;
	u32 *addr = (u32 *)(qp_matrix_addr + stream_qp_matrix_size * stream_id);
	int clear_size = stream_qp_matrix_size / sizeof(u32);

	VERIFY_STREAMID(stream_id);

	if (check_for_qp_roi(stream_id) < 0) {
		perror("check_for_qp_roi\n");
		return -1;
	}

	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;
	qp_matrix.enable = 1;
	qp_matrix.type = stream_roi[stream_id].qp_type;

	if (qp_matrix.type) {
		for (i = 0; i < clear_size; i++) {
			addr[i] = QP_MATRIX_ADV_DEFAULT_VALUE << 8;
		}
	} else {
		memset(addr, 0, stream_qp_matrix_size);
	}

	if (ioctl(fd_iav, IAV_IOC_SET_QP_ROI_MATRIX_EX, &qp_matrix) < 0) {
		perror("IAV_IOC_SET_QP_ROI_MATRIX_EX");
		return -1;
	}
	stream_roi[stream_id].enable = 0;
	printf("\nClear all qp matrix for stream %c.\n", 'A' + stream_id);
	return 0;
}

static int display_qp_roi(int stream_id)
{
	VERIFY_STREAMID(stream_id);
	int i, j;
	iav_qp_roi_matrix_ex_t qp_matrix;
	u32 *addr = (u32 *)(qp_matrix_addr + stream_qp_matrix_size * stream_id);
	u32 buf_width, buf_pitch, buf_height;
	buf_width = ROUND_UP(stream_roi[stream_id].encode_width, 16) / 16;
	buf_pitch = ROUND_UP(buf_width, 8);
	buf_height = ROUND_UP(stream_roi[stream_id].encode_height, 16) / 16;
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;
	if (qp_matrix.enable) {
		if (qp_matrix.type) {
			printf("\n\n===============================================\n");
			printf("Qisplay qp roi advance map\n");
			printf("\n\n===============================================\n");
			for (i = 0; i < buf_height; i++) {
				printf("\n");
				for (j = 0; j < buf_width; j++)
					/* bit8~15 is used for advance qp roi type */
					printf("%4d", ((addr[i * buf_pitch + j] >> 8) & 0xff)
						- QP_MATRIX_ADV_DEFAULT_VALUE);
			}
		} else {
			for (i = 0; i < QP_FRAME_TYPE_NUM; i++) {
				printf("\n\n===============================================\n");
				printf("Quality level: 0-[%d], 1-[%d], 2-[%d], 3-[%d]\n",
				       qp_matrix.delta[i][0], qp_matrix.delta[i][1],
				       qp_matrix.delta[i][2],qp_matrix.delta[i][3]);
				printf("===============================================\n");
			}
			for (i = 0; i < buf_height; i++) {
				printf("\n");
				for (j = 0; j < buf_width; j++)
					printf("%-2d", addr[i * buf_pitch + j]);
			}
		}
	printf("\n");
	}
	return 0;
}

static int get_quality_level(int stream_id)
{
	iav_qp_roi_matrix_ex_t qp_matrix;
	VERIFY_STREAMID(stream_id);
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;
	stream_roi[stream_id].qp_delta[0] = qp_matrix.delta[0][1];
	stream_roi[stream_id].qp_delta[1] = qp_matrix.delta[0][2];
	stream_roi[stream_id].qp_delta[2] = qp_matrix.delta[0][3];
	return 0;
}

static int set_quality_level(int stream_id)
{
	iav_qp_roi_matrix_ex_t qp_matrix;
	int i;
	VERIFY_STREAMID(stream_id);
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;
	qp_matrix.enable = 1;
	qp_matrix.type = stream_roi[stream_id].qp_type;

	for (i = 0; i < QP_FRAME_TYPE_NUM; i++) {
		qp_matrix.delta[i][0] = 0;
		qp_matrix.delta[i][1] = stream_roi[stream_id].qp_delta[0];
		qp_matrix.delta[i][2] = stream_roi[stream_id].qp_delta[1];
		qp_matrix.delta[i][3] = stream_roi[stream_id].qp_delta[2];
	}
	if (ioctl(fd_iav, IAV_IOC_SET_QP_ROI_MATRIX_EX, &qp_matrix) < 0) {
		perror("IAV_IOC_SET_QP_ROI_MATRIX_EX");
		return -1;
	}
	return 0;
}

static int input_value(int min, int max)
{
	int retry, i, input = 0;
#define MAX_LENGTH	16
	char tmp[MAX_LENGTH];

	do {
		retry = 0;
		scanf("%s", tmp);
		for (i = 0; i < MAX_LENGTH; i++) {
			if ((i == 0) && (tmp[i] == '-')) {
				continue;
			}
			if (tmp[i] == 0x0) {
				if (i == 0) {
					retry = 1;
				}
				break;
			}
			if ((tmp[i]  < '0') || (tmp[i] > '9')) {
				printf("error input:%s\n", tmp);
				retry = 1;
				break;
			}
		}

		input = atoi(tmp);
		if (input > max || input < min) {
			printf("\nInvalid. Enter a number (%d~%d): ", min, max);
			retry = 1;
		}

		if (retry) {
			printf("\nInput again: ");
			continue;
		}

	} while (retry);

	return input;
}

static int list_adv_qp_roi_index_table(int stream_id)
{
	iav_qp_roi_matrix_ex_t qp_matrix;
	int i, j;
	u32 *addr = (u32 *)(qp_matrix_addr + stream_qp_matrix_size * stream_id);
	u32 buf_width, buf_pitch, buf_height, start_x, start_y, end_x, end_y;
	u32 data;
	VERIFY_STREAMID(stream_id);
	if (check_for_qp_roi(stream_id) < 0) {
		return -1;
	}
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;
	qp_matrix.enable = 1;
	qp_matrix.type = stream_roi[stream_id].qp_type;

	printf("=========Reset qp roi table============\n");
	if (clear_qp_roi(stream_id) < 0) {
		printf("Error: clear_qp_roi\n");
		return -1;
	}

	// QP matrix is MB level. One MB is 16x16 pixels.
	buf_width = ROUND_UP(stream_roi[stream_id].encode_width, 16) / 16;
	buf_pitch = ROUND_UP(buf_width, 8);
	buf_height = ROUND_UP(stream_roi[stream_id].encode_height, 16) / 16;

	start_x = 0;
	start_y = 0;
	end_x = buf_width;
	end_y = buf_height;

	for (i = start_y; i < end_y; i++) {
		for (j = start_x; j < end_x; j++) {
			data = (i * buf_width +  j) & 0xff;
			if (data < QP_MATRIX_VALUE_MIN_ADV) {
				data = QP_MATRIX_VALUE_MIN_ADV;
			} else if (data > QP_MATRIX_VALUE_MAX_ADV){
				data = QP_MATRIX_VALUE_MAX_ADV;
			}
			/* bit8~15 is used for advance qp roi type */
			addr[i * buf_pitch + j] = (data << 8);
		}
	}
	if (ioctl(fd_iav, IAV_IOC_SET_QP_ROI_MATRIX_EX, &qp_matrix) < 0) {
		perror("IAV_IOC_SET_QP_ROI_MATRIX_EX");
		return -1;
	}
	stream_roi[stream_id].enable = 1;
	return 0;
}

static int stream_roi_type_menu(stream_id)
{
	VERIFY_STREAMID(stream_id);
	printf("\n|-------------------------------|\n");
	printf("| Stream %c                \t|\n", 'A' + stream_id);
	printf("| 0 - Base type \t\t|\n");
	printf("| 1 - Advance type\t\t|\n");
	printf("|-------------------------------|\n");
	return 0;
}

static int config_stream_roi_type(int stream_id)
{
	stream_roi[stream_id].qp_type = 0;
	stream_roi_type_menu(stream_id);
	printf("Your choice: ");
	stream_roi[stream_id].qp_type = input_value(QP_ROI_TYPE_FIRST,
		QP_ROI_TYPE_LAST - 1);
	printf("Reset qp roi offset table for new roi type");
	if (clear_qp_roi(stream_id) < 0) {
		printf("Error: clear_qp_roi\n");
		return -1;
	}

	return 0;
}

static int config_stream_roi(int stream_id)
{
	int back2main = 0, i;
	char opt, input[16];
	while (back2main == 0) {
		show_stream_menu(stream_id);
		printf("Your choice: ");
		scanf("%s", input);
		opt = input[0];
		tolower(opt);
		switch(opt) {
		case '1':
			if (stream_roi[stream_id].qp_type) {
				printf("Qp delta just used for base type\n");
				break;
			}
			if (check_for_qp_roi(stream_id) < 0)
				break;
			if (get_quality_level(stream_id) < 0)
				break;
			printf("\nCurrent quality level is 1:[%d], 2:[%d], 3:[%d].\n",
					stream_roi[stream_id].qp_delta[0],
					stream_roi[stream_id].qp_delta[1],
					stream_roi[stream_id].qp_delta[2]);
			i = 0;
			do {
				printf("Input QP delta (-51~51) for level %d: ", i+1);
				stream_roi[stream_id].qp_delta[i] = input_value(-51, 51);
			} while (++i < MAX_QP_DELTA_NUM);
			set_quality_level(stream_id);
			break;
		case '2':
		case '3':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			printf("\nInput ROI offset x (0~%d): ", stream_roi[stream_id].encode_width - 1);
			qp_roi[stream_id].start_x = input_value(0, stream_roi[stream_id].encode_width - 1);
			printf("Input ROI offset y (0~%d): ", stream_roi[stream_id].encode_height - 1);
			qp_roi[stream_id].start_y = input_value(0, stream_roi[stream_id].encode_height -1);
			printf("Input ROI width (1~%d): ", stream_roi[stream_id].encode_width
		        - qp_roi[stream_id].start_x);
			qp_roi[stream_id].width = input_value(1, stream_roi[stream_id].encode_width
				- qp_roi[stream_id].start_x);
			printf("Input ROI height (1~%d): ", stream_roi[stream_id].encode_height
			    - qp_roi[stream_id].start_y);
			qp_roi[stream_id].height = input_value(1, stream_roi[stream_id].encode_height
				- qp_roi[stream_id].start_y);
			if (opt == '2') {
				if (stream_roi[stream_id].qp_type) {
					printf("Input ROI quality level (-51~51): ");
					qp_roi[stream_id].quality_level = input_value(-51, 51);
				} else {
					printf("Input ROI quality level (1~%d): ", MAX_QP_DELTA_NUM);
					qp_roi[stream_id].quality_level = input_value(1, MAX_QP_DELTA_NUM);
				}
			} else {
				qp_roi[stream_id].quality_level = 0;
			}
			set_qp_roi(stream_id);
			break;
		case '4':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			clear_qp_roi(stream_id);
			break;
		case '5':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			display_qp_roi(stream_id);
			break;
		case '6':
			if (!stream_roi[stream_id].qp_type) {
				printf("List roi offset table for advance type\n");
				break;
			}
			if (list_adv_qp_roi_index_table(stream_id) < 0) {
				return -1;
			}
			break;
		case 'q':
			back2main = 1;
			break;
		default:
			printf("Unknown option %d.", opt);
			break;
		}
	}
	return 0;
}

static void quit_qp_roi()
{
	int i;
	exit_flag = 1;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++)
		if (stream_roi[i].enable)
			clear_qp_roi(i);
	exit(0);
}

int main(int argc, char **argv)
{
	char opt;
	char input[16];
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	if (argc > 1) {
		usage();
		return 0;
	}

	if (map_qp_matrix() < 0)
		return -1;

	signal(SIGINT, quit_qp_roi);
	signal(SIGTERM, quit_qp_roi);
	signal(SIGQUIT, quit_qp_roi);

	while (exit_flag == 0) {
		show_main_menu();
		printf("Your choice: ");
		fflush(stdin);
		scanf("%s", input);
		opt = input[0];
		tolower(opt);
		fflush(stdin);
		if ( opt >= 'a' && opt <= 'h') {
			if (config_stream_roi_type(opt - 'a') < 0) {
				continue;
			}
			config_stream_roi(opt - 'a');
		}
		else if (opt == 'q')
			exit_flag = 1;
		else
			printf("Unknown option %d.", opt);
	}
	return 0;
}
