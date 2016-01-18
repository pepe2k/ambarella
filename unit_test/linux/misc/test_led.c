/*
 * test_led.c
 * Max7219 8-digit led display drivers test.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <time.h>

#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <sched.h>
#include <getopt.h>
#endif

typedef unsigned char  	    u8;	/**< UNSIGNED 8-bit data type */
typedef unsigned short 	    u16;/**< UNSIGNED 16-bit data type */
int		spi_fd;
u16		cmd;

#define MAX7219_SPI_DEV_NODE		"/dev/spidev0.1"
#define MAX7219_WRITE_REGISTER(addr, val)	\
	do {								\
		cmd = ((addr) << 8) | (val);	\
		write(spi_fd, &cmd, 2);		\
	} while (0)

#define DIGITAL_SEGMENT	(8)

static int shut_down_flag =0 ;
static int test_flag =0 ;
static int count_flag =0 ;
static int count = 999 ;
static int Number_flag =0 ;
static int Number = 100 ;

static int Rate = 1000*1000 ;
static int Sleep = 1000 ;

static int Skip_num = -1 ;

static const char *short_options = "stc:N:r:m:k:";

#define NO_ARG		0
#define HAS_ARG		1

static struct option long_options[] = {
	{"shut_down", NO_ARG, 0, 's'},
	{"test", NO_ARG, 0, 't'},
	{"count", HAS_ARG, 0, 'c'},
	{"num", HAS_ARG, 0, 'N'},
	{"rate", HAS_ARG, 0, 'r'},
	{"sleep", HAS_ARG, 0, 'm'},
	{"skip", HAS_ARG, 0, 'k'},
	{0, 0, 0, 0}
};

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 's':
			shut_down_flag = 1;
			break;
		case 't':
			test_flag = 1;
			break;
		case 'c':
			count_flag = 1;
			count = atoi(optarg);
			break;
		case 'N':
			Number_flag = 1;
			Number = atoi(optarg);
			break;
		case 'r':
			Rate = atoi(optarg);
			break;
		case 'm':
			Sleep = atoi(optarg);
			break;
		case 'k':
			Skip_num = atoi(optarg);
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

int convert_decimal(int source, unsigned char * target)
{
	int i = 0;
	do {
		target[i]=source % 10;
		source = source / 10;
		i++;
	} while (source);

	return i;
}

void display_segments(int i, unsigned char * target)
{
	int j;

	for (j = 1; (j <= i) && (j <= DIGITAL_SEGMENT); j++) {
		if((Skip_num <= j) && (Skip_num > -1) && (Skip_num < 8)) {
			MAX7219_WRITE_REGISTER(j+1, target[j-1]);
		} else {
			MAX7219_WRITE_REGISTER(j, target[j-1]);
		}
	}
}

int main(int argc, char **argv)
{
	int		mode = 0, bits = 16;
	int		ret = 0;
	unsigned char 	decimal[DIGITAL_SEGMENT];
	int 		display_number;

	if (init_param(argc, argv) < 0)
		return -1;

	spi_fd = open(MAX7219_SPI_DEV_NODE, O_RDWR);
	if (spi_fd < 0) {
		printf("Can't open MAX7219_SPI_DEV_NODE to write \n");
		return -1;
	}

	/* Mode */
	ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret < 0) {
		perror("SPI_IOC_WR_MODE");
		return -1;
	}

	/* bpw */
	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret < 0) {
		perror("SPI_IOC_WR_BITS_PER_WORD");
		return -1;
	}

	/* speed */
	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &Rate);
	if (ret < 0) {
		perror("SPI_IOC_WR_MAX_SPEED_HZ");
		return -1;
	}

	printf("Mode = %d\n", mode);
	printf("Bpw = %d\n", bits);
	printf("Baudrate = %d\n", Rate);

	MAX7219_WRITE_REGISTER(0xa, 0xa);
	MAX7219_WRITE_REGISTER(0x9, 0xff);
	MAX7219_WRITE_REGISTER(0xb, 0x7);

	if (shut_down_flag) {
		MAX7219_WRITE_REGISTER(0xc, 0x0);
	} else {
		MAX7219_WRITE_REGISTER(0xc, 0x1);
	}

	if (test_flag) {
		MAX7219_WRITE_REGISTER(0xf, 0x1);
		return 0;
	} else {
		MAX7219_WRITE_REGISTER(0xf, 0x0);
		int j;
		for (j = 1; j <= DIGITAL_SEGMENT; j++) {
			MAX7219_WRITE_REGISTER(j, 0x7f);
		}
	}

	if (count_flag) {
		display_number=0;
		do {
			if ((ret = convert_decimal(display_number, decimal)) < 0)
				return -1;
			display_segments(ret, decimal);
			usleep(Sleep);
			display_number++;
		} while (display_number <= count);
	}

	if (Number_flag) {
		printf ("Number = %d \n",Number);
		if ((ret = convert_decimal(Number, decimal)) <= 0)
			return -1;
		display_segments(ret, decimal);
	}

	close(spi_fd);
	return 0;
}

