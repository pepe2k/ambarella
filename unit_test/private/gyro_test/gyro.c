#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <time.h>
#include <basetypes.h>
#include "gyro.h"
#include <sys/time.h>

#define GYRO_SPI_CLK  20000000

static int mpu6000_spi = -1;

static int g_max_read_latency = 0;

static long long g_total_read_latency = 0;
static int g_total_read_counter = 0;

static int mpu6000_check_spi(void) 
{
	int	ret;
	int spi_handle;
	u8	mode	= SPI_MODE_0;
	u8	bits	= 8;
	u32	speed	= GYRO_SPI_CLK;   

	if (mpu6000_spi < 0) {
		spi_handle	= open(MPU6000_SPI_DEV_NODE, O_RDWR, 0);
		if (spi_handle < 0) {
			printf("%s: Unable to open "MPU6000_SPI_DEV_NODE"!", __func__);
			return -1;
		}
	
		ret = ioctl(spi_handle, SPI_IOC_WR_MODE, &mode);
		if(ret < 0) {
			printf("%s: Can't set mode!", __func__);
		}

		ret = ioctl(spi_handle, SPI_IOC_WR_BITS_PER_WORD, &bits);
		if(ret < 0) {
			printf("%s: Can't set bytewide!", __func__);
		}

		ret = ioctl(spi_handle, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
		if(ret < 0) {
			printf("%s: Can't set speed!", __func__);
		}

		mpu6000_spi = spi_handle;
	}

	
	return 0;
}


static int mpu6000_write_reg(int fd, u8 addr, u8 data)
{
	u8		tx[2];
	tx[0] = (addr & 0x7f);
	tx[1] = data;
	return	write(fd, tx, sizeof(tx));
}

static int mpu6000_read_reg(int fd, u8 addr, u8 *data)
{
	int				ret;
	u8				tx[2], rx[2];
	struct spi_ioc_transfer		tr;
#if 0	
	u32	speed	= GYRO_SPI_CLK; 	

	//change speed
	if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
		perror("change speed error in read \n");
	}
#endif
	
	tx[0]			= (addr | 0x80);
	tr.tx_buf		= (unsigned long)tx;
	tr.rx_buf		= (unsigned long)rx;
	tr.len			= 2;
	tr.delay_usecs		= 0;
	tr.speed_hz		= GYRO_SPI_CLK; 
	tr.bits_per_word	= 8;
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

	if (ret < 0) {
		perror("mpu6000_spi ioctl ");
		return -1;
	}
	*data = rx[1];
	return 0;
}




int mpu6000_init(void)
{
	u8 data;
	if (mpu6000_check_spi() < 0) {
		printf("mpu6000 init failed \n");
		return -1;
	}

	mpu6000_write_reg(mpu6000_spi, PWR_MGMT_1_REG,0);
	mpu6000_read_reg(mpu6000_spi, PWR_MGMT_1_REG, &data);

	mpu6000_read_reg(mpu6000_spi, WHO_AM_I_REG, &data);
	mpu6000_write_reg(mpu6000_spi, USER_CTRL_REG,0x10);

	//set sample rate SMPLRT_DIV_REG
	mpu6000_write_reg(mpu6000_spi, SMPLRT_DIV_REG,0x07);

	//set interrupt
	mpu6000_write_reg(mpu6000_spi, INT_PIN_CFG_REG,0xa0);
	mpu6000_write_reg(mpu6000_spi, INT_ENABLE_REG,0xf1);

	//set gyroscope output rate
	mpu6000_write_reg(mpu6000_spi, CONFIG_REG,0x1);

	//set self test
	//mpu6000_write_reg(mpu6000_spi, GYRO_CONFIG_REG,0xe0);
	return 0;
}


int mpu6000_read_gy(void)
{
	s16			x = 0, y = 0, z = 0;
	u8			xl =0, xh = 0, yl = 0, yh = 0, zl = 0, zh = 0;
	u8			ret;
	s16			buff[3];
	struct timeval  time1, time2;
	long long 	timediff;
	u8 			status;

	gettimeofday(&time1, NULL);	
/*
	ret = mpu6000_read_reg(mpu6000_spi, INT_STATUS_REG, &status);
	if(ret < 0) {
		return -1;
	}
*/
	/* x-Axis is at long edge of S2 Ginkgo board, ( perpendicular to CMOS sensor )
	   y-Axis is at short edge of S2 Ginkgo board, (at long edge of CMOS sensor )
	   z-Axis is perpendicular to S2 Ginkgo board, (at short edge of CMOS sensor )

	   so we should use y-Axis and z-Axis for sensor 
	*/
	



//		ret = mpu6000_read_reg(mpu6000_spi, GYRO_XOUT_L_REG, &xl);
//		ret = mpu6000_read_reg(mpu6000_spi, GYRO_XOUT_H_REG, &xh);
	ret = mpu6000_read_reg(mpu6000_spi, GYRO_YOUT_L_REG, &yl);
#if 0
	ret = mpu6000_read_reg(mpu6000_spi, GYRO_YOUT_H_REG, &yh);
	ret = mpu6000_read_reg(mpu6000_spi, GYRO_ZOUT_L_REG, &zl);
	ret = mpu6000_read_reg(mpu6000_spi, GYRO_ZOUT_H_REG, &zh);
#endif	
//		x = (s16)((xh << 8) | xl);
	y = (s16)((yh << 8) | yl);
	z = (s16)((zh << 8) | zl);
/*	
	mpu6000_read_reg(mpu6000_spi, INT_STATUS_REG, &status);
*/

	gettimeofday(&time2, NULL);
	timediff = (time2.tv_sec - time1.tv_sec)* 1000000LL + (time2.tv_usec - time1.tv_usec);
//	printf("SPI read latency is %lld micro seconds \n", timediff);	
	if (timediff > g_max_read_latency)
		g_max_read_latency = timediff;

	g_total_read_latency += timediff;
	g_total_read_counter ++;

	buff[0] = x;
	buff[1] = y;
	buff[2] = z;

/*
	buff[0] = buff[0] * CONVERT_GYRO_X;
	buff[1] = buff[1] * CONVERT_GYRO_Y;
	buff[2] = buff[2] * CONVERT_GYRO_Z;
*/

//	printf("x : %3d, y : %3d,  z : %3d \n", (int)buff[0], (int)buff[1], (int)buff[2]);

	return 0;

}

int mpu6000_get_read_gy_stat(int * max, int * avg)
{
	*max = g_max_read_latency;

	if (g_total_read_counter == 0)
		g_total_read_counter = 1;
	
	*avg = g_total_read_latency/g_total_read_counter;
	return 0;
}



