#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <signal.h>
#include <basetypes.h>
#include <pthread.h>
#include <semaphore.h>

#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

#define NO_ARG	0
#define HAS_ARG	1

#define MAX_ENCODE_STREAM_NUM 8
#define FILENAME_LENGTH (32)
#define MAX_YUV_FRAME_NUM (120)

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )

typedef struct efm_param {
	int width;
	int height;
	int size_flag;

	char yuv[FILENAME_LENGTH];
	int file_flag;

	int frame_num;
	int frame_num_flag;
	int feed_num;
	int feed_num_flag;

	int frame_factor;
	u32 yuv_pts_distance;

} efm_param;

static efm_param efm = {
		.frame_factor = 1,
};

int fd_iav;
static pthread_t encode_task_id = 0;
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;

static int efm_task_exit_flag = 0;
static int efm_feed_num_count = 0;
static iav_stream_id_t start_stream_id = 0;
int signal0 = 0, signal1 = 1;

static u8 * yuv_addr = NULL;
static u8 * yuv_end = NULL;
static u8 * frame_start = NULL;

static struct option long_options[] = {
		{"streamA",	NO_ARG,	0,	'A'},
		{"streamB",	NO_ARG,	0,	'B'},
		{"streamC",	NO_ARG,	0,	'C'},
		{"streamD",	NO_ARG,	0,	'D'},
		{"streamE",	NO_ARG,	0,	'E'},
		{"streamF",	NO_ARG,	0,	'F'},
		{"efm-yuv-file",	HAS_ARG,	0,	'f'},
		{"efm-total-fnum",	HAS_ARG,	0,	'n'},
		{"efm-fsize",	HAS_ARG,	0,	's'},
		{"efm-feed-frame-num",	HAS_ARG,	0,	'N'},
		{0,	0,	0,	0},
};

static const char *short_options = "n:f:s:N:ABCDEF";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
		{"", "\t\t\t\tstream A"},
		{"", "\t\t\t\tstream B"},
		{"", "\t\t\t\tstream C"},
		{"", "\t\t\t\tstream D"},
		{"", "\t\t\t\tstream E"},
		{"", "\t\t\t\tstream F"},
		{"filename", "\t\tspecify YUV input file for encoding from memory feature."},
		{"1~120", "\t\tspecify the total frame number from YUV file"},
		{"W x H", "\t\t\tspecify frame size for encoding from memory feature"},
		{"feed_frame__num",	"specify feed in frame num"},
};

void usage(void)
{
	int i;
	printf("test_feed_yuv usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
	return ;
}

static int get_arbitrary_resolution(const char *name, int *width, int *height)
{
	sscanf(name, "%dx%d", width, height);
	return 0;
}

int config_efm_params(void)
{
#define MAX_YUV_SOURCE_BUF_WIDTH	(3840)
#define MAX_YUV_SOURCE_BUF_HEIGHT	(2160)

	u32 	fps, fps_q9;
	iav_source_buffer_setup_ex_t	buffer_setup;
	iav_reso_ex_t *yuv_buf_size = NULL;

	memset(&buffer_setup, 0, sizeof(buffer_setup));
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &buffer_setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX");
		return -1;
	}

	if (buffer_setup.type[IAV_ENCODE_SOURCE_DRAM_FIRST] !=
		IAV_SOURCE_BUFFER_TYPE_ENCODE) {
		printf("yuv buf[%d] error, the buffer is not encode type\n",
			IAV_ENCODE_SOURCE_DRAM_FIRST);
		return -1;
	}

	yuv_buf_size = &buffer_setup.size[IAV_ENCODE_SOURCE_DRAM_FIRST];
	if ((yuv_buf_size->width < efm.width) || (yuv_buf_size->height < efm.height)) {
		printf("error:YUV buf[%d] size is smaller than the setting,"
			 "YUV buf:width[%d], height[%d], setting:width[%d], height[%d]\n",
			IAV_ENCODE_SOURCE_DRAM_FIRST,
			yuv_buf_size->width, yuv_buf_size->height, efm.width, efm.height);
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, &fps_q9) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
		return -1;
	}

	fps = DIV_ROUND(512000000, fps_q9);
	if (fps < 1) {
		printf("error:vin fps[%d] < 1\n ", fps);
		return -1;
	}

	if (efm.frame_factor > fps || (efm.frame_factor < 1)) {
		printf("error:the factor must be in 1~%d\n ", fps);
		return -1;
	}

	// both 90000 and 512000000 divide 10000.
	efm.yuv_pts_distance = (u64)fps_q9 * 9 / 51200 * efm.frame_factor;

	return 0;
}

static int init_param(int argc, char **argv)
{
	int i, ch;
	int width, height;
	int option_index = 0;

	opterr = 0;

	while((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1){
		switch (ch) {
		case 'A':
			start_stream_id |= ( 1<< 0);
			break;
		case 'B':
			start_stream_id |= ( 1<< 1);
			break;
		case 'C':
			start_stream_id |= ( 1<< 2);
			break;
		case 'D':
			start_stream_id |= ( 1<< 3);
			break;
		case 'E':
			start_stream_id |= ( 1<< 4);
			break;
		case 'F':
			start_stream_id |= ( 1<< 5);
			break;
		case 'f':
			if (strlen(optarg) >= FILENAME_LENGTH) {
				printf("File name [%s] is too long. It cannot be longer than "
						"%d characters.\n", optarg, (FILENAME_LENGTH - 1));
				return -1;
			}
			strcpy(efm.yuv, optarg);
			printf("YUV input file [%s].\n", efm.yuv);
			efm.file_flag = 1;
			break;
		case 'n':
			i = atoi(optarg);
			if (i < 1 || i > MAX_YUV_FRAME_NUM) {
				printf("Total frame number [%d] must be in the range of [1~120].\n", i);
				return -1;
			}
			efm.frame_num = i;
			efm.frame_num_flag = 1;
			break;
		case 's':
			if (get_arbitrary_resolution(optarg, &width, &height) < 0) {
				printf("Failed to get resolution from [%s].\n", optarg);
				return -1;
			}
			efm.width = width;
			efm.height = height;
			efm.size_flag = 1;
			break;
		case 'N':
			i = atoi(optarg);
			efm.feed_num = i;
			efm.feed_num_flag = 1;
			break;
		}
	}

	if (config_efm_params() < 0) {
		printf("config_efm_params error\n");
		return -1;
	}

	return 0;
}

static int prepare_efm_mem(u32 total_size)
{
	yuv_addr = (u8*)malloc(total_size);
	if (!yuv_addr) {
		printf("Failed to allocate buffers.\n");
		return -1;
	}
	return 0;
}

static int prepare_efm_files(u32 total_size)
{
	static int init_efm_file = 0;
	int yuv_fd = -1;
	char file_name[256] = {0};

	if (init_efm_file == 0) {
		// read YV12 planar format
		sprintf(file_name, "%s", efm.yuv);

		if ((yuv_fd = open(file_name, O_RDONLY)) < 0) {
			printf("Failed to open YUV file [%s].\n", file_name);
			goto ERROR_EXIT;
		}
		read(yuv_fd, yuv_addr, total_size);
		if (yuv_fd >= 0) {
			close(yuv_fd);
			yuv_fd = -1;
		}
		frame_start = yuv_addr;
		yuv_end = yuv_addr + total_size;
		init_efm_file = 1;
	}
	return 0;

ERROR_EXIT:
	if (yuv_addr) {
		free(yuv_addr);
		yuv_addr = NULL;
	}
	if (yuv_fd >= 0) {
		close(yuv_fd);
		yuv_fd = -1;
	}
	return -1;
}

static int feed_buf_from_file(iav_enc_dram_buf_frame_ex_t *buf)
{
	#define ROW_MAX	(4)
	#define COL_MAX		(4)
	u16 yuv_pitch;
	u16 i, j, row, col;
	u32 frame_size, me1_data;
	u8 * src = NULL, * dst = NULL;

	// Feed YUV data
	frame_size = efm.width * efm.height * 3 / 2;
	yuv_pitch = buf->max_size.width;
	for (i = 0; i < (efm.height / ROW_MAX); ++i) {
		// Read Y data
		src = frame_start + i * ROW_MAX * efm.width;
		dst = buf->y_addr + i * ROW_MAX * yuv_pitch;
		for (row = 0; row < ROW_MAX; ++row, src += efm.width, dst += yuv_pitch) {
			memcpy(dst, src, efm.width);
		}

		// Read UV data
		src = frame_start + (efm.height + i * ROW_MAX / 2) * efm.width;
		dst = buf->uv_addr + i * ROW_MAX / 2 * yuv_pitch;
		for (row = 0; row < ROW_MAX / 2; ++row, src += efm.width, dst += yuv_pitch) {
			memcpy(dst, src, efm.width);
		}

		// Read ME1 data
		src = frame_start + i * ROW_MAX * efm.width;
		dst = buf->me1_addr + i * yuv_pitch;
		for (col = 0; col < (efm.width / COL_MAX); ++col) {
			for (row = 0, j = col * COL_MAX, me1_data = 0;
					row < efm.width * ROW_MAX; row += efm.width) {
				me1_data += (src[row + j] + src[row + j + 1] +
					src[row + j + 2] + src[row + j + 3]);
			}
			dst[col] = me1_data >> 4;
		}
	}

	if (efm.feed_num_flag == 1) {
		efm_feed_num_count++;
	}
	// Update frame start address
	if (frame_start + frame_size < yuv_end) {
		frame_start += frame_size;
	} else {
		frame_start = yuv_addr;
	}
	return 0;
}

static int feed_efm_data(void)
{
	static u32 pts = 0;
	iav_enc_dram_buf_frame_ex_t buf_frame;
	iav_enc_dram_buf_update_ex_t buf_update;

	memset(&buf_frame, 0, sizeof(buf_frame));
	buf_frame.buf_id = IAV_ENCODE_SOURCE_MAIN_DRAM;

	AM_IOCTL(fd_iav, IAV_IOC_ENC_DRAM_REQUEST_FRAME_EX, &buf_frame);

	if (feed_buf_from_file(&buf_frame) < 0) {
		printf("feed_buf_from_file error\n");
		return -1;
	}

	// Update ME1 & YUV data
	memset(&buf_update, 0, sizeof(buf_update));
	buf_update.buf_id = IAV_ENCODE_SOURCE_MAIN_DRAM;
	buf_update.frame_id = buf_frame.frame_id;
	buf_update.frame_pts = pts;
	buf_update.size.width = efm.width;
	buf_update.size.height = efm.height;

	pts += efm.yuv_pts_distance;

	AM_IOCTL(fd_iav, IAV_IOC_ENC_DRAM_UPDATE_FRAME_EX, &buf_update);

	printf("\nframe:%d\n",efm_feed_num_count);
	return 0;
}

void efm_feed_data_loop(void)
{
	char *vsync_proc = "/proc/ambarella/vin0_vsync";
	int vin_tick = -1;
	int skip = 0;
	char vin_array[8];
	u32 frame_size = -1, total_size  = -1;
	vin_tick = open(vsync_proc, O_RDONLY);
	if (vin_tick < 0) {
		printf("Cannot open [%s].\n", vsync_proc);
		return ;
	}

	if (efm.size_flag == 0) {
		printf("Need frame resolution for EFM!\n");
		return ;
	}
	frame_size = efm.width * efm.height;
	total_size = frame_size * 3 / 2 * efm.frame_num;
	if (prepare_efm_mem(total_size) < 0) {
		printf("prepare_efm_mem error!\n");
		return ;
	}

	while (!efm_task_exit_flag) {
		prepare_efm_files(total_size);
		if (efm.feed_num_flag == 1) {
			if (efm_feed_num_count == efm.feed_num) {
				skip = 1;
			}
		}
		if (skip == 0) {
			pthread_mutex_lock(&mut);
			while (signal0 == 0) {
				pthread_cond_wait(&cond, &mut);
			}
			signal0 = signal0 - 1;
			read(vin_tick, vin_array, 8);
			feed_efm_data();
			if (signal1 == 0) {
				pthread_cond_signal(&cond1);
			}
			signal1 = signal1 + 1;
			pthread_mutex_unlock(&mut);
		} else {
			efm_task_exit_flag = 1;
		}
	}
	printf("efm_feed_num_count:%d\n", efm_feed_num_count);

	if (yuv_addr != NULL) {
		free(yuv_addr);
		yuv_addr = NULL;
	}

	if (vin_tick >= 0) {
		close(vin_tick);
		vin_tick = -1;
	}

	return ;
}

static int start_encode(void)
{
	int i,j;
	iav_encode_stream_info_ex_t info;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		if (start_stream_id & (1 << i)) {
			info.id = (1 << i);
			AM_IOCTL(fd_iav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &info);
			if (info.state == IAV_STREAM_STATE_ENCODING) {
				start_stream_id &= ~(1 << i);
			}
		}
	}
	if (start_stream_id == 0) {
		printf("already in encoding, nothing to do \n");
		return 0;
	}
	// start encode
	for(j = 0; j < efm.feed_num; j++){
		pthread_mutex_lock(&mut);
		while (signal1 == 0){
			pthread_cond_wait(&cond1,&mut);
		}
		signal1 = signal1 - 1;
		usleep(130000);
		AM_IOCTL(fd_iav, IAV_IOC_START_ENCODE_EX, start_stream_id);
		printf("Start encoding for stream 0x%x successfully\n", start_stream_id);
		if (signal0 == 0){
			pthread_cond_signal(&cond);
		}
		signal0 = signal0 + 1;
		pthread_mutex_unlock(&mut);
	}
	return 0;
}

static int do_start_encoding(void)
{
	if (start_encode() < 0) {
		return -1;
	}
	return 0;
}

int map_bsb(void)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped) {
		return 0;
	}
	/* Map for EFM feature */
	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}
	mem_mapped = 1;
	return 0;
}

static void sigstop()
{
	efm_task_exit_flag = 1;
	/* Free up the YUV data reading from file */
	if (yuv_addr) {
		free(yuv_addr);
		yuv_addr = NULL;
	}
	exit(1);
}

static int finish_work(void)
{
	if (yuv_addr) {
		free(yuv_addr);
		yuv_addr = NULL;
	}
	if (encode_task_id != 0) {
		if (pthread_join(encode_task_id, NULL) != 0){
			printf("!! Fail to destory (join) thread <start_encoding task>\n");
		}
		printf("\n== Destroy (join) thread <EFM task> successful ==\n");
	}
	encode_task_id = 0;
	return 0;
}

int main(int argc, char **argv)
{
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	if (argc < 2) {
		usage();
		return -1;
	}
	if (map_bsb() < 0) {
		printf("map bsb failed\n");
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		printf("init param failed \n");
		return -1;
	}

	//sem_init(&sem_0, 0, 0);
	//sem_init(&sem_1, 0, 0);
	if (pthread_create(&encode_task_id, NULL, (void *)do_start_encoding,
			NULL) != 0) {
		printf("!! Fail to create thread <start_encoding task>\n");
	}
	printf("\n== Create thread <start_encoding task> successful ==\n");

	efm_feed_data_loop();
	finish_work();
	pthread_mutex_destroy(&mut);
	pthread_cond_destroy(&cond);
	pthread_cond_destroy(&cond1);
	close(fd_iav);
	return 0;
}
