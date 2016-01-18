
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>

#include <sched.h>
#include <signal.h>
#include <basetypes.h>

#include <pthread.h>

#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_common.h"
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"
#include "ambas_vin.h"
#include "img_struct_arch.h"
#include "img_dsp_interface_arch.h"
#include "img_api_arch.h"
#include "datatx_lib.h"

#include "linux/watchdog.h"
#include "linux/input.h"

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <termios.h>

// options and usage
#define NO_ARG		0
#define HAS_ARG		1

#define	MAX_SENSOR_PARAM_SIZE	16
#define	MAX_CMD_SIZE				256
#define	UART1_DEV				"/dev/ttyS1"
#define	LED_I2C_DEV			"/dev/i2c-0"
#define	CFG_FILE_PATH			"/tmp/mmcblk0p1/test_chip_cfg"
#define	VIN_MD5_PATH			"/tmp/mmcblk0p1/vin_golden_md5"
#define	VOUT_MD5_PATH			"/tmp/mmcblk0p1/vout_golden_md5"

#define GPIO_IN			0
#define GPIO_OUT		1
#define GPIO_LOW		0
#define GPIO_HIGH		1

#define LED1_I2C_ADD	0x39
#define LED2_I2C_ADD	0x3a

#define LED_0	0x02
#define LED_1	0xcf
#define LED_2	0x11
#define LED_3	0x05
#define LED_4	0x4c
#define LED_5	0x24
#define LED_6	0x20
#define LED_7	0x8f
#define LED_8	0x00
#define LED_9	0x04

#define USB_DEV_ERR		0xffff0001
#define ETH_DEV_ERR		0xffff0002
#define VIN_DEV_ERR		0xffff0003
#define VOUT_DEV_ERR	0xffff0004

static int test_vin_md5_flag = 0;
static int test_usbhost_flag = 0;
static int test_usbdev_flag = 0;
static int test_ethernet_flag = 0;
static int test_watchdog_flag = 0;
static int test_keypad_flag = 0;
static int test_vout_md5_flag = 0;
static int test_default_item_flag = 0;
static int test_save_log = 0;


FILE *fd_log_save = NULL;
static int fd_uart1 = -1;
static char log_string[64] = "";

typedef enum board_type{
	ATB_FPGA,
	ATB_HDMI
}board_type_t;

typedef struct atb_boardinfo_s {
	board_type_t test_board_type;

	char sensorname[MAX_SENSOR_PARAM_SIZE];
	char vin_mode[MAX_SENSOR_PARAM_SIZE];
	char vout_mode[MAX_SENSOR_PARAM_SIZE];
	char encode_mode[MAX_SENSOR_PARAM_SIZE];
	char frame_rate[MAX_SENSOR_PARAM_SIZE];
	int	encode_frame_number_vin;
	int	encode_frame_number_vout;
}atb_boardinfo_t;
static atb_boardinfo_t atb_board_info;

enum number_short_options{
	SAVE_LOG = 0,
	USB_HOST,
	USB_DEV,
	DEFAULT_TEST,
};

static struct option long_options[] = {
	{"vin-md5",	NO_ARG,		0,	'i'},
	{"vout-md5",	NO_ARG,		0,	'o'},
	{"ethernet",	NO_ARG,		0,	'e'},
	{"usbhost",	NO_ARG,		0,	USB_HOST},
	{"usbdev",	NO_ARG,		0,	USB_DEV},
	{"watchdog",	NO_ARG,		0,	'w'},
	{"keypad",	NO_ARG,		0,	'k'},
	{"default",	NO_ARG,		0,	DEFAULT_TEST},
	{"savelog",	NO_ARG,		0,	SAVE_LOG},
	{"help",		NO_ARG,		0,	'h'},
	{0, 0, 0, 0}
};

static const char *short_options = "ioewkh";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t\ttest encode frame checksum(MD5)"},
	{"", "\t\ttest decode frame checksum(MD5)"},
	{"", "\t\ttest ethernet by ping"},
	{"", "\t\ttest usbhost"},
	{"", "\t\ttest usbdev"},
	{"", "\t\ttest watchdog"},
	{"", "\t\ttest keypad"},
	{"", "\t\ttest default items"},
	{"", "\t\tsave log to sdcard"},
	{"", "\t\thelp"},
};

void usage(void)
{
	int i;
	printf("test_chip usage:\n");
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
}

static unsigned char str2hexnum(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return 0; /* foo */
}
static unsigned long str2hex(unsigned char *str)
{
	int value = 0;

	while (*str) {
		value = value << 4;
		value |= str2hexnum(*str++);
	}

	return value;
}

int init_param(int argc, char **argv)
{
	int ch, option_index = 0;
	while((ch = getopt_long(argc, argv, short_options, long_options, &option_index))
		!= (-1)){
		switch (ch) {
			case 'i':
				test_vin_md5_flag = 1;
				break;
			case 'o':
				test_vout_md5_flag = 1;
				break;
			case 'e':
				test_ethernet_flag = 1;
				break;
			case USB_HOST:
				test_usbhost_flag = 1;
				break;
			case USB_DEV:
				test_usbdev_flag = 1;
				break;
			case 'w':
				test_watchdog_flag = 1;
				break;
			case 'k':
				test_keypad_flag = 1;
				break;
			case SAVE_LOG:
				test_save_log = 1;
				break;
			case DEFAULT_TEST:
				test_default_item_flag = 1;
				break;
			case 'h':
				usage();
				exit(1);
				break;
			default:
				printf("unknown command %s \n", optarg);
				return -1;
				break;
		}
	}

	return 0;
}


static void sigstop()
{
	exit(1);
}

#define print_uart1(fmt, args...)	\
	do { \
		sprintf(log_string, fmt, ##args); \
		write(fd_uart1, log_string,strlen(log_string)); \
		printf(fmt, ##args); \
	} while (0)

#define printlog_save(fmt, args...)	\
	do { \
		if(test_save_log){ \
			sprintf(log_string, fmt, ##args); \
			fwrite(log_string,strlen(log_string),1,fd_log_save);} \
		printf(fmt, ##args); \
	} while (0)


static int open_led1(int *fd)
{
	int ret = -1;

	fd[0] = open(LED_I2C_DEV, O_RDWR, 0);

	if (fd[0] < 0) {
		printf("i2c open error \n");
		return -1;
	}

	ret = ioctl(fd[0], I2C_TENBIT, 0);
	if (ret < 0) {
		printf("I2c ioctl error \n");
		return -1;
	}

	ret = ioctl(fd[0], I2C_SLAVE, LED1_I2C_ADD);
	if (ret < 0) {
		printf("I2c slave setting error \n");
		return -1;
	}

	return ret;
}

static int open_led2(int *fd)
{
	int ret = -1;

	fd[0] = open(LED_I2C_DEV, O_RDWR, 0);

	if (fd[0] < 0) {
		printf("i2c open error \n");
		return -1;
	}

	ret = ioctl(fd[0], I2C_TENBIT, 0);
	if (ret < 0) {
		printf("I2c ioctl error \n");
		return -1;
	}

	ret = ioctl(fd[0], I2C_SLAVE, LED2_I2C_ADD);
	if (ret < 0) {
		printf("I2c slave setting error \n");
		return -1;
	}

	return ret;
}

static int led_i2c_write(int fd, int value)
{
	int ret = 0;
	u8 w8[2];

	w8[0] = 0x00;
	w8[1] = value;
	ret = write(fd, w8, 2);
	if (ret < 0) {
		printf("i2c write error \n");
		return -1;
	}

	return ret;
}

static int write_led1(int value)
{
	int i2c_fd;
	if(open_led1(&i2c_fd) < 0) {
		printf("Open device failed\n");
		return -1;
	}
	led_i2c_write(i2c_fd, value);
	close(i2c_fd);
	return 0;
}

static int write_led2(int value)
{
	int i2c_fd;
	if(open_led2(&i2c_fd) < 0) {
		printf("Open device failed\n");
		return -1;
	}
	led_i2c_write(i2c_fd, value);
	close(i2c_fd);
	return 0;
}

static int read_cfg_file(void)
{
	FILE *test_cfg = NULL;
	int ret = 0;
	char param[MAX_SENSOR_PARAM_SIZE];

//	printlog_save("\ntest sdcard start\n");

	test_cfg = fopen(CFG_FILE_PATH,"r");
	if(test_cfg == NULL){
		printlog_save("Can't open %s\n",CFG_FILE_PATH);
		ret = -1;
		return ret;
	}

	fscanf(test_cfg, "#\r\n");
	fscanf(test_cfg, "# test_chip configuration file\r\n");
	fscanf(test_cfg, "#\r\n");

	fscanf(test_cfg, "sensor: ");
	fscanf(test_cfg,"%s\n",param);
	memcpy(atb_board_info.sensorname, param, MAX_SENSOR_PARAM_SIZE);
	printlog_save("sensorname:%s \n",atb_board_info.sensorname);

	fscanf(test_cfg, "vin_mode: ");
	fscanf(test_cfg,"%s\n",param);
	memcpy(atb_board_info.vin_mode, param, MAX_SENSOR_PARAM_SIZE);
	printlog_save("vin_mode:%s \n",atb_board_info.vin_mode);

	fscanf(test_cfg, "vout_mode: ");
	fscanf(test_cfg,"%s\n",param);
	memcpy(atb_board_info.vout_mode, param, MAX_SENSOR_PARAM_SIZE);
	printlog_save("vout_mode:%s \n",atb_board_info.vout_mode);

	fscanf(test_cfg, "encode_mode: ");
	fscanf(test_cfg,"%s\n",param);
	memcpy(atb_board_info.encode_mode, param, MAX_SENSOR_PARAM_SIZE);
	printlog_save("encode_mode:%s \n",atb_board_info.encode_mode);

	fscanf(test_cfg, "frame_rate: ");
	fscanf(test_cfg,"%s\n",param);
	memcpy(atb_board_info.frame_rate, param, MAX_SENSOR_PARAM_SIZE);
	printlog_save("frame_rate:%s \n",atb_board_info.frame_rate);

	fscanf(test_cfg, "encode_frame_number_vin: ");
	fscanf(test_cfg,"%s\n",param);
	atb_board_info.encode_frame_number_vin = atoi(param);
	printlog_save("encode_frame_number_vin:%d \n",atb_board_info.encode_frame_number_vin);

	fscanf(test_cfg, "encode_frame_number_vout: ");
	fscanf(test_cfg,"%s\n",param);
	atb_board_info.encode_frame_number_vout = atoi(param);
	printlog_save("encode_frame_number_vout:%d \n",atb_board_info.encode_frame_number_vout);

	fclose(test_cfg);

//	printlog_save("test sdcard end\n");
	return ret;
}


static int systemcall(char *string)
{
	int ret = 0;
	pid_t status;

	status = system(string);
	if(status == -1){
		printlog_save("system error!\n");
		ret = status;
	}else{
		if(WIFEXITED(status)){
			if(WEXITSTATUS(status) == 0){
				//printf("run shell script successfully.\n");
			}else{
				printlog_save("run %s fail, script exit code: %d\n",
					string, WEXITSTATUS(status));
				ret = status;
			}
		}else{
			printlog_save("exit status = [%d]\n", WIFEXITED(status));
			ret = -2;
		}
	}
	return ret;
}


static void config_gpio_direction(int gpio, int direction)
{
	char string[MAX_CMD_SIZE];
	sprintf(string,"echo %d > /sys/class/gpio/export",gpio);
	systemcall(string);

	if(direction == GPIO_OUT)
		sprintf(string,"echo out > /sys/class/gpio/gpio%d/direction",gpio);
	else if(direction == GPIO_IN)
		sprintf(string,"echo in > /sys/class/gpio/gpio%d/direction",gpio);
	systemcall(string);
}

static void set_gpio_level(int gpio,int value)
{
	char string[MAX_CMD_SIZE];
	sprintf(string,"echo %d > /sys/class/gpio/gpio%d/value",value,gpio);
	systemcall(string);
}

static int get_gpio_level(int gpio)
{
	FILE *fd;
	int ret;
	char path[MAX_CMD_SIZE];
	sprintf(path,"/sys/class/gpio/gpio%d/value",gpio);
	fd = fopen(path,"r");
	if(fd == NULL){
		printlog_save("Can't open %s\r\n",path);
		return -1;
	}

	fscanf(fd,"%s\n",path);
	ret = str2hex((unsigned char *)path);
	//printf("gpio%d value = 0x%x\n",gpio,ret);
	fclose(fd);
	return ret;
}

static void *test_ethernet(void *args)
{
	int ret = 0;
	char string[MAX_CMD_SIZE];

	printlog_save("\ntest ethernet start\n");

	sprintf(string,"ifconfig eth0 10.2.5.20");
	ret = systemcall(string);
	if(ret !=0 ){
		printlog_save("test ethernet fail !\n\n");
	}

	sprintf(string,"ping 10.2.5.21 -c 3 -W 2");
	//printf("%s \n",string);
	ret = systemcall(string);
	if(ret !=0 ){
		printlog_save("test ethernet fail !\n\n");
	}

	printlog_save("test ethernet end\n");
	return (void *)ret;
}

static int test_USBHostController(void)
{
	int ret = 0, sda1_dev = -1;
	char string[MAX_CMD_SIZE];
	FILE *file = NULL;

	printlog_save("\ntest USBHost start\n");

	sprintf(string, "echo host > /proc/ambarella/uport");
	printf("%s \n",string);
	ret = systemcall(string);
	if(ret != 0){
		printlog_save("%s failed\n",string);
		return ret;
	}

	sprintf(string, "mkdir -p /mnt/UDisk");
	printf("%s \n",string);
	ret = systemcall(string);
	if(ret != 0){
		printlog_save("%s failed\n",string);
		return ret;
	}

	while(sda1_dev < 0){
		sda1_dev = open("/dev/sda1", O_RDONLY, 0);
	}
	close(sda1_dev);

	sprintf(string, "mount /dev/sda1 /mnt/UDisk");
	printf("%s \n",string);
	ret = systemcall(string);
	if(ret != 0){
		printlog_save("%s failed\n",string);
		return ret;
	}

	file = fopen("/mnt/UDisk/test_chip_cfg","r");
	if(file == NULL){
		printlog_save("UsbHost test failed\n");
		ret = -1;
		return ret;
	}

	printlog_save("UsbHost test end\n");
	return ret;
}

static void *test_USBdev(void *args)
{
	int ret = 0, timeout = 0;
	char string[MAX_CMD_SIZE];

	printlog_save("\ntest USBdev start\n");

	sprintf(string, "dd if=/dev/zero of=/tmp/usb bs=1M count=64");
	//printf("%s \n",string);
	ret = systemcall(string);
	if(ret != 0){
		printlog_save("%s failed\n",string);
		goto END;
	}

	sprintf(string, "mkdir -p /mnt/usb");
	//printf("%s \n",string);
	ret = systemcall(string);
	if(ret != 0){
		printlog_save("%s failed\n",string);
		goto END;
	}

	sprintf(string, "mkfs.vfat -n atbusb /tmp/usb");
	//printf("%s \n",string);
	ret = systemcall(string);
	if(ret != 0){
		printlog_save("%s failed\n",string);
		goto END;
	}

	sprintf(string, "mount /tmp/usb /mnt/usb");
	printf("%s \n",string);
	ret = systemcall(string);
	if(ret != 0){
		printlog_save("%s failed\n",string);
		return (void *)ret;
	}

	sprintf(string, "mkdir /mnt/usb/usb_test");
	printf("%s \n",string);
	ret = systemcall(string);
	if(ret != 0){
		printlog_save("%s failed\n",string);
		return (void *)ret;
	}

	sprintf(string, "umount /mnt/usb");
	printf("%s \n",string);
	ret = systemcall(string);
	if(ret != 0){
		printlog_save("%s failed\n",string);
		return (void *)ret;
	}

	sprintf(string, "modprobe g_mass_storage stall=0 removable=1 file=/tmp/usb");
	//printf("%s \n",string);
	ret = systemcall(string);
	if(ret != 0){
		printlog_save("%s failed\n",string);
		goto END;
	}

	while(timeout < 10){
		sprintf(string, "mount /tmp/usb /mnt/usb");
		ret = systemcall(string);
		if(ret != 0){
			printlog_save("%s failed\n",string);
			goto END;
		}

		if( fopen("/mnt/usb/usb_test/USB.OK","r") == NULL){
			sprintf(string, "umount /mnt/usb");
			ret = systemcall(string);
			if(ret != 0){
				printlog_save("%s failed\n",string);
				goto END;
			}
			timeout = timeout+1;
			sleep(1);
		}else{
			break;
		}
	}

	if(timeout >= 10){
		ret = -1;
		printlog_save(" Usbdev test failed\n");
	}

END:
	return (void *)ret;
}


static void *stream_md5_test(void *arg)
{
	int ret = 0;
	char string[MAX_CMD_SIZE];
//	atb_boardinfo_t *atb_board_info = (atb_boardinfo_t *)arg;

	if(atb_board_info.test_board_type == ATB_FPGA){
		sprintf(string,"/usr/local/bin/test_stream -f atb_encode --md5test %d --verbose --only-filename",
			atb_board_info.encode_frame_number_vin);
	}else if(atb_board_info.test_board_type == ATB_HDMI){
		sprintf(string,"/usr/local/bin/test_stream -f atb_encode --md5test %d --verbose --only-filename",
			atb_board_info.encode_frame_number_vout);
	}
//	sprintf(string,"test_stream --verbose&");
	if((systemcall(string)) !=0 ){
		printlog_save("%s failed\n",string);
		ret = -1;
	}
	return (void *)ret;
}

static int md5_check(void)
{
	int ret = 0,i=0;
	char string[MAX_CMD_SIZE];
	FILE *fd_md5 = NULL;
	FILE *fd_md5_check = NULL;
	unsigned char buf_md5[32];
	unsigned char buf_md5_check[32];

	sprintf(string,"/usr/bin/md5sum atb_encode.h264 > atb_encode.h264.md5");
	ret = systemcall(string);
	if(ret !=0 ){
		goto END;
	}

	fd_md5 = fopen("atb_encode.h264.md5","r");
	if(fd_md5 == NULL){
		printlog_save("Can't open atb_encode.h264.md5\r\n");
		ret = -1;
		goto END;
	}

	fscanf(fd_md5,"%s\n",buf_md5);
	printf("md5 = %s\n",buf_md5);

	fd_md5 = fopen("atb_encode.h264.md5","r");
	if(fd_md5 == NULL){
		printlog_save("Can't open atb_encode.h264.md5\r\n");
		ret = -1;
		goto END;
	}

	if(atb_board_info.test_board_type == ATB_FPGA){
		fd_md5_check = fopen(VIN_MD5_PATH,"r");
	}else if(atb_board_info.test_board_type == ATB_HDMI){
		fd_md5_check = fopen(VOUT_MD5_PATH,"r");
	}
	if(fd_md5_check == NULL){
		printlog_save("Can't open md5 check file\r\n");
		ret = -1;
		goto END;
	}

	fscanf(fd_md5_check,"%s\n",buf_md5_check);
	printf("md5_check_%d = %s\n",i,buf_md5_check);
	while(memcmp((void *)(buf_md5_check),"END",3)){
		if(!memcmp((void *)buf_md5_check,(void *)buf_md5,32))
			goto END;
		i=i+1;
		fscanf(fd_md5_check,"  atb_encode.h264\r\n");
		fscanf(fd_md5_check,"%s\n",buf_md5_check);
		printf("md5_check_%d = %s\n",i,buf_md5_check);
	}
	ret = -1;
END:
	if(fd_md5 != NULL)
		fclose(fd_md5);
	if(fd_md5_check != NULL)
		fclose(fd_md5_check);

//	sprintf(string,"/bin/rm atb_encode.h264.md5");
//	systemcall(string);
//	sprintf(string,"/bin/rm atb_encode.h264");
//	systemcall(string);
	return ret;
}

static int test_vin_md5()
{
	int ret = 0;
	void *streamret;
	char string[MAX_CMD_SIZE];
	pthread_t stream_thread;

	printlog_save("\ntest VIN md5 start\n");

	sprintf(string,"/usr/local/bin/init.sh --%s", atb_board_info.sensorname);
	printf("%s \r\n",string);
	ret = systemcall(string);
	if(ret !=0 ){
		printf("%s failed\n",string);
		goto END;
	}

	sprintf(string,
		"/usr/local/bin/test_encode -i%s -V%s --hdmi -A -h %s --smaxsize %s -X --bsize 3840x2160 --bmax 3840x2160 --enc-mode 6 --sharpen-b 0",
		atb_board_info.vin_mode,atb_board_info.vout_mode,atb_board_info.encode_mode,atb_board_info.encode_mode);

	printf("%s \r\n",string);
	ret = systemcall(string);
	if(ret !=0 ){
		printf("%s failed\n",string);
		goto END;
	}

	sprintf(string,"/usr/local/bin/test_image -l&");
	printf("%s \r\n",string);
	ret = systemcall(string);
	if(ret !=0 ){
		printf("%s failed\n",string);
		goto END;
	}

	if ((pthread_create(&stream_thread ,NULL, stream_md5_test,NULL))<0)
	{
		printf("can't create pthread");
		   goto END;
	}

	sprintf(string,"/usr/local/bin/test_encode -A -e");
	printf("%s \r\n",string);
	ret = systemcall(string);
	if(ret !=0 ){
		printf("%s failed\n",string);
		goto END;
	}


	pthread_join(stream_thread, &streamret);
	if((int)streamret !=0 ){
		printf("stream_md5_test failed\n");
		ret = -1;
	}

	sprintf(string,"/usr/local/bin/test_encode -A -s");
	printf("%s \r\n",string);
	systemcall(string);

	ret = md5_check();

END:
	printlog_save("test VIN md5 end\n");
	return ret;
}

static int test_config_vout()
{
	int ret = 0;
	char string[MAX_CMD_SIZE];

	sprintf(string,"/usr/local/bin/init.sh --na && /usr/local/bin/test2 -V2160p30 --hdmi --cs rgb && /usr/local/bin/amba_debug -M rct -w 0x33c -d 0x08000000 && /usr/local/bin/amba_debug -M hdmi -w 0x388 -d 0xff000006");
	printf("%s \r\n",string);
	ret = systemcall(string);
	if(ret !=0 ){
		printf("%s failed\n",string);
		goto END;
	}

END:
	return ret;
}

static void *adv7619_preview(void *arg)
{
	int ret = 0;
	char string[MAX_CMD_SIZE];

	sprintf(string,"/usr/local/bin/test_encode -i1920x2160 -V480p --hdmi -X --bsize 1920x2160 --bmax 1920x2160 --enc-mode 6  --raw-cap 1");
	printf("%s \r\n",string);
//	sprintf(string,"test_encode -i%s -V%s --hdmi -A -h 1080p -X --bsize 3840x2160 --bmax 3840x2160 --enc-mode 6 --sharpen 0",
//		vin_mode,vout_mode);
//	sprintf(string,"test_encode -i1080p -V480p --hdmi -X --bsize 1080p --bmax 1080p --enc-mode 2 --sharpen 0",
//		vin_mode,vout_mode);
	if((systemcall(string)) !=0 ){
		printlog_save("%s failed\n",string);
		ret = -1;
	}
	return (void *)ret;
}

static int test_vout_md5()
{
	int ret = 0;
	char string[MAX_CMD_SIZE];
	pthread_t adv7619_thread;
	pthread_t stream_thread;
	void *adv7619ret;
	void *streamret;

	printlog_save("\ntest VOUT md5 start\n");


//	sprintf(string,"/usr/local/bin/init.sh --%s", atb_board_info->sensorname);
	sprintf(string,"/usr/local/bin/init.sh --adv7619");
	printf("%s \r\n",string);
	ret = systemcall(string);
	if(ret !=0 ){
		printf("%s failed\n",string);
		goto END;
	}

	if ((pthread_create(&adv7619_thread ,NULL, adv7619_preview, NULL))<0)
	{
		printf("can't create pthread");
		   goto END;
	}

	sleep(5);

	/*sprintf(string,"./test.sh");
	printf("%s \r\n",string);
	ret = systemcall(string);
	if(ret !=0 ){
		printf("%s failed\n",string);
		goto END;
	}*/

	set_gpio_level(88,GPIO_HIGH);

	pthread_join(adv7619_thread, &adv7619ret);
	if((int)adv7619ret !=0 ){
		printf("adv7619_preview failed\n");
		ret = -1;
		set_gpio_level(89,GPIO_LOW);
		goto END;
	}

	sprintf(string,"/usr/local/bin/test_encode -A -h1080p");
	printf("%s \r\n",string);
	ret = systemcall(string);
	if(ret !=0 ){
		printf("%s failed\n",string);
		goto END;
	}

	if ((pthread_create(&stream_thread ,NULL, stream_md5_test,NULL))<0)
	{
		printf("can't create stream_thread pthread");
		   goto END;
	}

	sprintf(string,"/usr/local/bin/test_encode -A -e");
	ret = systemcall(string);
	if(ret !=0 ){
		printf("%s failed\n",string);
		goto END;
	}

	pthread_join(stream_thread, &streamret);
	if((int)streamret !=0 ){
		printf("stream_md5_test failed\n");
		ret = -1;
	}

	sprintf(string,"/usr/local/bin/test_encode -A -s");
	systemcall(string);

#if 0
	sprintf(string,"/usr/bin/md5sum -c /tmp/mmcblk0p1/vout_golden_md5");
	ret = systemcall(string);
	if(ret !=0 ){
		printf("%s failed 0x%x\n",string,ret);
		set_gpio_level(88,GPIO_LOW);
		goto END;
	}
#endif

	ret = md5_check();
	if(ret !=0 ){
		set_gpio_level(88,GPIO_LOW);
		goto END;
	}

	set_gpio_level(89,GPIO_HIGH);

END:
	printlog_save("test VOUT md5 end\n");
	return ret;
}


static int test_watchdog(void)
{
	int fd_wdt;//watchdog file handler
	int tmo = 0;

	printlog_save("\ntest watchdog start\n");

	if((fd_wdt = open("/dev/watchdog",O_RDWR,0)) < 0){
		printlog_save("open /dev/watchdog failed!\n");
		return -1;
	}

	tmo = 10;
	if (ioctl(fd_wdt, WDIOC_SETTIMEOUT, &tmo) < 0) {
		printlog_save("WDIOC_GETTIMEOUT");
		return -1;
	}

	tmo = 0;
	if (ioctl(fd_wdt, WDIOC_GETTIMEOUT, &tmo) < 0) {
		printlog_save("WDIOC_GETTIMEOUT");
		return -1;
	}
	printlog_save("restart in %d secs\n",tmo);

	close(fd_wdt);
	return 0;
}

static int test_keypad(void)
{
	int fd_key,ret = -1, press_time = 0;
	struct input_event buf;

	printlog_save("\ntest keypad start\n");
	printf("Press Anykey for 3 secs quit!\n");

	if((fd_key = open("/dev/input/event0",O_RDONLY)) < 0){
		printf("open /dev/input/event0 failed!\n");
		return -1;
	}
	while(1){
		ret = read(fd_key, &buf, sizeof(struct input_event));
		if(ret != 0)
			printf("type = %d code = %d value = %d sec = %ld, usec = %ld\n",
				buf.type, buf.code, buf.value, buf.time.tv_sec,buf.time.tv_usec);
		if(press_time == 0){
			press_time  = buf.time.tv_sec;
			continue;
		}
		if((buf.time.tv_sec - press_time) >=3)
			break;
		press_time  = buf.time.tv_sec;
	}
	printlog_save("test keypad end\n");
	return 0;
}

static int uart_config(int fd, int speed, int flow)
{
	struct termios ti;

	tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, &ti) < 0) {
		perror("Can't get port settings");
		return -1;
	}

	cfmakeraw(&ti);

	ti.c_cflag |= CLOCAL;
	if (flow)
		ti.c_cflag |= CRTSCTS;
	else
		ti.c_cflag &= ~CRTSCTS;

	cfsetospeed(&ti, speed);
	cfsetispeed(&ti, speed);

	if (tcsetattr(fd, TCSANOW, &ti) < 0) {
		perror("Can't set port settings");
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	return 0;
}



static int whichboard()
{
	FILE *fd = NULL;
	int adc_v = 0;
	unsigned char buf[16];


	fd = fopen("/sys/class/hwmon/hwmon0/device/adc7","r");
	if(fd == NULL){
		printlog_save("Can't open /sys/class/hwmon/hwmon0/device/adc_ch7\r\n");
		return -1;
	}

	fscanf(fd, "adc_ch7 value: ");
	fscanf(fd,"%s\n",buf);
	adc_v = str2hex(buf);
	printf("adc_v = 0x%x\n",adc_v);

	config_gpio_direction(88,GPIO_OUT);
	config_gpio_direction(89,GPIO_OUT);
	config_gpio_direction(90,GPIO_IN);
	config_gpio_direction(91,GPIO_IN);

	if(adc_v >= 0xf00){
		atb_board_info.test_board_type = ATB_FPGA;
		if(test_default_item_flag){
			test_vin_md5_flag = 1;
			test_vout_md5_flag = 1;
			test_ethernet_flag = 1;
			test_usbdev_flag = 1;
		}
	}else{
		atb_board_info.test_board_type = ATB_HDMI;
		//systemcall("cp /tmp/mmcblk0p1/test.sh  ./");
		if(test_default_item_flag){
			test_vout_md5_flag = 1;
		}
	}
	fclose(fd);
	return 0;
}

static int config_uart1()
{
	char rd_buf[5];
	//start communicate UART1
	fd_uart1 = open(UART1_DEV,O_RDWR | O_NOCTTY | O_NONBLOCK);
	if(fd_uart1 < 0){
		perror("Can not open UART 1");
		return -1;
	}
	uart_config(fd_uart1, B115200, 0);
	print_uart1("#START\r\n");
	memset(rd_buf,0,sizeof(rd_buf));
	while(1){
		read(fd_uart1,rd_buf,sizeof(rd_buf));
		if(memcmp(rd_buf,"DOSOK",5) == 0){
			//printf("receive DOSOK\n");
			break;
		}
	}
	print_uart1("#NEXT\r\n");
	return 0;
}



int main(int argc, char **argv)
{
	struct timeval  begin_time,end_time;
	int ret = -1;
	u32 timeinterval_sec;
	pthread_t tid_eth;
	pthread_t tid_udev;
	void *eth_test;
	void *udev_test;


	signal(SIGINT,	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if(argc < 2){
		usage();
		ret = -1;
		goto END;
	}

	if(init_param(argc,argv) == (-1)){
		perror("init param failed \n");
		ret = -1;
		goto END;
	}

	if(whichboard() < 0)
		goto END;

	if(atb_board_info.test_board_type == ATB_FPGA){
		if(config_uart1() < 0)
			goto END;
	}

	write_led1(LED_0);
	write_led2(LED_1);

	if(test_save_log){
		fd_log_save =  fopen("/tmp/mmcblk1p1/test_log","a+");
		if(fd_log_save == NULL){
			perror("/sdcard/mmcblk1p1/test_log\n");
			ret = -1;
			goto END;
		}
	}

	//printlog_save("************Test start************\n");

	gettimeofday(&begin_time, NULL);

	//usbdev test
	if(test_usbdev_flag){
		write_led1(LED_0);
		write_led2(LED_2);
		ret = pthread_create(&tid_udev,NULL,test_USBdev,NULL);
		if (ret < 0){
			perror("pthread_create test_USBdev failed\n");
			goto END;
		}
		pthread_join(tid_udev, &udev_test);
		ret = (int)udev_test;
		if((ret !=0)){
			perror("usb dev test failed\n");
			ret = USB_DEV_ERR;
			print_uart1("#fail1\r\n");
			goto END;
		}
	}
	//ethernet ping
	if(test_ethernet_flag){
		write_led1(LED_0);
		write_led2(LED_3);
		ret = pthread_create(&tid_eth,NULL,test_ethernet,NULL);
		if (ret < 0){
			perror("pthread_create test_ethernet failed\n");
			goto END;
		}
		pthread_join(tid_eth, &eth_test);
		ret = (int)eth_test;
		if((ret !=0)){
			perror("ether net test failed\n");
			print_uart1("#fail2\r\n");
			ret = ETH_DEV_ERR;
			goto END;
		}
	}

	//read configuration file
	ret = read_cfg_file();
	if(ret != 0)
		goto END;

	//test usb hostcontroller
	if(test_usbhost_flag){
		ret = test_USBHostController();
		if(ret !=0 )
			goto END;
	}

	if(test_vin_md5_flag){
		write_led1(LED_0);
		write_led2(LED_4);
		ret = read_cfg_file();
		if(ret != 0)
			goto END;
		ret = test_vin_md5();
		if(ret != 0){
			ret = VIN_DEV_ERR;
			print_uart1("#fail3\r\n");
			goto END;
		}
	}

	if(test_vout_md5_flag){
		write_led1(LED_0);
		write_led2(LED_5);
		if(atb_board_info.test_board_type == ATB_FPGA){
			while(GPIO_HIGH !=get_gpio_level(91));
			ret = test_config_vout();
			if(ret != 0)
				goto END;
			while(GPIO_HIGH !=get_gpio_level(90)){
				if(GPIO_LOW == get_gpio_level(91)){
					ret = VOUT_DEV_ERR;
					print_uart1("#fail4\r\n");
					goto END;
				}
			}
		}else{
			ret = test_vout_md5();
			if(ret != 0)
				goto END;

		}
	}

	if(test_watchdog_flag){
		ret = test_watchdog();
		if(ret != 0)
			goto END;
	}

	if(test_keypad_flag){
		ret = test_keypad();
		if(ret != 0)
			goto END;
	}
#if 0
	if(test_ethernet_flag){
		pthread_join(tid_eth, &eth_test);
		ret = (int)eth_test;
	}

	if(test_usbdev_flag){
		pthread_join(tid_udev, &udev_test);
		ret = (int)udev_test;
	}
#endif
END:
	if(ret !=0){
		printlog_save("Test failed!\n");
	}else{
		gettimeofday(&end_time, NULL);
		timeinterval_sec = end_time.tv_sec - begin_time.tv_sec;
		printlog_save("spend time :%d seconds\n",timeinterval_sec);
		printlog_save("Test Pass!\n");
	}
	//printlog_save("************Test End!************\n");
	systemcall("echo 88 > /sys/class/gpio/unexport");
	systemcall("echo 89 > /sys/class/gpio/unexport");
	systemcall("echo 90 > /sys/class/gpio/unexport");
	systemcall("echo 91 > /sys/class/gpio/unexport");

	if(test_save_log){
		fclose(fd_log_save);
	}
	return ret;
}
