#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <getopt.h>
#include <errno.h>
#include <basetypes.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/signal.h>

#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#include "ambas_vin.h"


#define VIN_ARRAY_SIZE	(8)
static char * VIN_VSYNC = "/proc/ambarella/vin0_vsync";


static const char* short_options = "f:d:w:h:src:";
static struct option long_options[] = {
	{"file", 1, 0, 'f'},
	{"stat", 0, 0, 's'},
	{"raw", 0, 0, 'r'},
	{"decode-width", 1, 0, 'd'},
	{"width", 1, 0, 'w'},
	{"height", 1, 0, 'h'},
	{"count", 1, 0, 'c'},
	};
static int raw_file_mode;
static int raw_cap_mode;
static int stat_read_mode;
static char raw_file_name[128];
static int raw_width;
static int raw_height;
static int raw_count;

static int raw_init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch(ch) {
		case 'f':
			raw_file_mode = 1;
			memcpy(raw_file_name, optarg, 128);
			break;
//		case 'd':
//			decode_width = atoi(optarg);
//			break;
		case 'w':
			raw_width = atoi(optarg);
			break;
		case 'h':
			raw_height = atoi(optarg);
			break;
		case 's':
			stat_read_mode = 1;
			break;
		case 'r':
			raw_cap_mode = 1;
			break;
		case 'c':
			raw_count = atoi(optarg);
			break;
		default:
			printf("unknown option %c\n", ch);
			return -1;
		}
	}

	return 0;
}


static int get_raw_size(int fd_iav, iav_raw_info_t* p_raw_info)
{

	if (ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, p_raw_info) < 0) {
		perror("IAV_IOC_IMG_READ_RAW_INFO");
		return -1;
	}

	printf("raw pattern %d width %d height %d\n", p_raw_info->bayer_pattern, p_raw_info->width, p_raw_info->height);
	return 0;
}


static int save_raw2file(u8* src, int size, int cnt)
{
	int fd_raw;
	char pic_file_name[128];

	sprintf(pic_file_name, "hiso_imx104_%d.raw", cnt);
	fd_raw = open(pic_file_name, O_WRONLY | O_CREAT, 0666);
	if (write(fd_raw, src, size) < 0) {
		perror("write(save_raw)");
		return -1;
	}

	printf("raw picture written to %s\n", pic_file_name);
	close(fd_raw);
	return 0;
}

static int save_raw2mem(int fd_iav, u8* mem, int size)
{
	iav_raw_info_t raw_info;

	if (ioctl(fd_iav, IAV_IOC_READ_RAW_INFO, &raw_info) < 0) {
		perror("IAV_IOC_IMG_READ_RAW_INFO");
		return -1;
	}
	memcpy(mem, raw_info.raw_addr, size);
	return 0;
}

int map_bsb(int fd_iav)
{
	static int mem_mapped = 0;
	iav_mmap_info_t info;

	if (mem_mapped)
		return 0;

	if (ioctl(fd_iav, IAV_IOC_MAP_BSB, &info) < 0) {
		perror("IAV_IOC_MAP_BSB");
		return -1;
	}

	if (ioctl(fd_iav, IAV_IOC_MAP_DSP, &info) < 0) {
		perror("IAV_IOC_MAP_DSP");
		return -1;
	}

	mem_mapped = 1;
	return 0;
}


u8* raw_buf;
u32 raw_size;
int main(int argc, char **argv)
{
	int fd_iav, vin_tick;
	char vin_int_array[VIN_ARRAY_SIZE];
	iav_raw_info_t raw_info;

	if(raw_init_param(argc, argv)<0 || argc<2)
		return -1;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	map_bsb(fd_iav);
	vin_tick = open(VIN_VSYNC, O_RDONLY);


	if(get_raw_size(fd_iav,&raw_info)<0)
		return -1;
	raw_size = raw_info.height*raw_info.width*2;
	printf("allocate mem for %d x %d\n", raw_size,raw_count);
	raw_buf = malloc(raw_size* raw_count);
	if(raw_buf == NULL) {
		return -1;
	}

	if(raw_cap_mode) {
		int i=0;
		u8* wp = raw_buf;
		for(i=0; i<raw_count; i++) {
			read(vin_tick, vin_int_array, VIN_ARRAY_SIZE);
			save_raw2mem(fd_iav,wp,raw_size);
			wp += raw_size;
		}
		wp = raw_buf;
		for(i=0; i<raw_count; i++) {
			save_raw2file(wp, raw_size, i);
			wp += raw_size;
		}
		
	}

	free(raw_buf);
	return 0;
}

