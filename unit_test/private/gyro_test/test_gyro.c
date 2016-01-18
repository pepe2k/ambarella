
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif
#include "test_gyro.h"

static int sensor_module =  0;
static int test_module = 0;

static const char *short_options = "S:M:";

#define GYRO_SPI_WR_SCLK  1000000
#define GYRO_SPI_RD_SCLK  20000000

static struct option long_options[] = {
	{"sensor", HAS_ARG, 0, 'S'},
	{"module", HAS_ARG, 0, 'M'},
	{0, 0, 0, 0}
};


void usage(void)
{
	printf("\nUSAGE: test_gpro [OPTION] device\n");
	printf("\t-h, --help		Help\n"
		"\t test_gyro -S mpu6000 -M gyro""\n");
	printf("\n");
}


int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options,
		&option_index)) != -1) {
		switch (ch) {
		case 'S':
			if (!strcmp(optarg, "mpu6000")) {
				sensor_module = SENSOR_MPU6000;
			}else if(!strcmp(optarg, "idg2000")){
				sensor_module = SENSOR_IDG2000;
			}
			break;
		case 'M':
			if (!strcmp(optarg, "gyro")) {
				test_module = TEST_GYRO;
			}else if(!strcmp(optarg, "accel")){
				test_module = TEST_ACCEL;
			}else if(!strcmp(optarg, "temp")){
				test_module = TEST_TEMP;
			}else if(!strcmp(optarg, "dump")){
				test_module = TEST_DUMP;
			}
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}


static int mpu6000_check_spi(void) {
	int		ret;
	int		config_spi = 0;

	if (mpu6000_spi < 0) {
		mpu6000_spi	= open(MPU6000_SPI_DEV_NODE, O_RDWR, 0);
		config_spi	= 1;
	}
	if (mpu6000_spi < 0) {
		printf("%s: Unable to open "MPU6000_SPI_DEV_NODE"!", __func__);
		return -1;
	}

	if (config_spi) {
		u8	mode	= SPI_MODE_0;
		u8	bits	= 8;
		u32	speed	= GYRO_SPI_WR_SCLK;  //SCLK frequency is 1MHz max in Spec

		ret = ioctl(mpu6000_spi, SPI_IOC_WR_MODE, &mode);
		if(ret < 0) {
			printf("%s: Can't set mode!", __func__);
		}

		ret = ioctl(mpu6000_spi, SPI_IOC_WR_BITS_PER_WORD, &bits);
		if(ret < 0) {
			printf("%s: Can't set bytewide!", __func__);
		}

		ret = ioctl(mpu6000_spi, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
		if(ret < 0) {
			printf("%s: Can't set speed!", __func__);
		}
	}

	return 0;
}

static void mpu6000_write_reg(u8 addr, u8 data)
{
	u8		tx[2];
	s32		rval;
	u32		speed	= GYRO_SPI_WR_SCLK;

	rval = ioctl(mpu6000_spi, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if(rval < 0) {
		printf("%s: Can't set speed!", __func__);
	}
	
	tx[0] = (addr & 0x7f);
	tx[1] = data;	
	write(mpu6000_spi, tx, sizeof(tx));
}

static int mpu6000_read_reg(u8 addr)
{	
	static u8				tx[2], rx[2], init = 0;
	static struct spi_ioc_transfer		tr;	
	s32	rval;

	if (!init) {
		init = 1;
		tr.tx_buf		= (unsigned long)tx;
		tr.rx_buf		= (unsigned long)rx;
		tr.len			= 2;
		//tr.delay_usecs		= 0;
		//tr.speed_hz		= GYRO_SPI_RD_SCLK;   //SCLK frequency is 1MHz max in Spec
		//tr.bits_per_word	= 8;
	}
	
	tx[0]			= (addr | 0x80);	
	ioctl(mpu6000_spi, SPI_IOC_MESSAGE(1), &tr);

	return rx[1];
}

static u8* mpu6000_read_reg_2(u8 addr, u8 bytes)
{	
	static u8				tx[8], rx[8], init = 0;
	static struct spi_ioc_transfer		tr;	
	s32	rval;
	
	if (bytes + 1 > 8)
		printf("ERROR: read bytes too long");

	if (!init) {
		init = 1;
		tr.tx_buf		= (unsigned long)tx;
		tr.rx_buf		= (unsigned long)rx;
		tr.len			= bytes + 1;
		//tr.delay_usecs		= 0;
		//tr.speed_hz		= 1000000;   //SCLK frequency is 1MHz max in Spec
		//tr.bits_per_word	= 8;			
	}
	
	tx[0]			= (addr | 0x80);	
	ioctl(mpu6000_spi, SPI_IOC_MESSAGE(1), &tr);

	return (rx + 1);
}



static void mpu6000_dump_reg(void)
{
	int value=0;
	value = mpu6000_read_reg(AUX_VDDIO_REG);
	printf("AUX_VDDIO_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(SMPLRT_DIV_REG);
	printf("SMPLRT_DIV_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(CONFIG_REG);
	printf("CONFIG_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_CONFIG_REG);
	printf("GYRO_CONFIG_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_CONFIG_REG);
	printf("ACCEL_CONFIG_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(FF_THR_REG);
	printf("FF_THR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(FF_DUR_REG);
	printf("FF_DUR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(MOT_THR_REG);
	printf("MOT_THR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(MOT_DUR_REG);
	printf("MOT_DUR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ZRMOT_THR_REG);
	printf("ZRMOT_THR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ZRMOT_DUR_REG);
	printf("ZRMOT_DUR_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(FIFO_EN_REG);
	printf("FIFO_EN_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(INT_PIN_CFG_REG);
	printf("INT_PIN_CFG_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(INT_ENABLE_REG);
	printf("INT_ENABLE_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(INT_STATUS_REG);
	printf("INT_STATUS_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(ACCEL_XOUT_H_REG);
	printf("ACCEL_XOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_XOUT_L_REG);
	printf("ACCEL_XOUT_L_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_YOUT_H_REG);
	printf("ACCEL_YOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_YOUT_L_REG);
	printf("ACCEL_YOUT_L_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_ZOUT_H_REG);
	printf("ACCEL_ZOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(ACCEL_ZOUT_L_REG);
	printf("ACCEL_ZOUT_L_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(TEMP_OUT_H_REG);
	printf("TEMP_OUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(TEMP_OUT_L_REG);
	printf("TEMP_OUT_L_REG=0x%x\r\n",value);

	value = mpu6000_read_reg(GYRO_XOUT_H_REG);
	printf("GYRO_XOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_XOUT_L_REG);
	printf("GYRO_XOUT_L_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_YOUT_H_REG);
	printf("GYRO_YOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_YOUT_L_REG);
	printf("GYRO_YOUT_L_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_ZOUT_H_REG);
	printf("GYRO_ZOUT_H_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(GYRO_ZOUT_L_REG);
	printf("GYRO_ZOUT_L_REG=0x%x\r\n",value);


	value = mpu6000_read_reg(MOT_DETECT_STATUS_REG);
	printf("MOT_DETECT_STATUS_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(SIGNAL_PATH_RESET_REG);
	printf("SIGNAL_PATH_RESET_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(MOT_DETECT_CTRL_REG);
	printf("MOT_DETECT_CTRL_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(USER_CTRL_REG);
	printf("USER_CTRL_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(PWR_MGMT_1_REG);
	printf("PWR_MGMT_1_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(PWR_MGMT_2_REG);
	printf("PWR_MGMT_2_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(FIFO_COUNTH_REG);
	printf("FIFO_COUNTH_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(FIFO_COUNTL_REG);
	printf("FIFO_COUNTL_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(FIFO_R_W_REG);
	printf("FIFO_R_W_REG=0x%x\r\n",value);
	value = mpu6000_read_reg(WHO_AM_I_REG);
	printf("WHO_AM_I_REG=0x%x\r\n",value);

}

void mpu6000_read_gy(void)
{
	s16			x = 0, y = 0, z = 0;
	u8			xl, xh, yl, yh, zl, zh;
	u8			ret, data_ready;
	s32			buff[3];
	struct timeval  	time1, time2;
	u8*			rbuf;
/*
	ret = mpu6000_read_reg(INT_STATUS_REG);
	if(ret & 0x1) {
		data_ready=1;
	}else{
		return;
	}
*/
	gettimeofday(&time1, NULL);
	data_ready = 1;
	if (data_ready)	{
/*
		//xl = mpu6000_read_reg(GYRO_XOUT_L_REG);
		//xh = mpu6000_read_reg(GYRO_XOUT_H_REG);
		yl = mpu6000_read_reg(GYRO_YOUT_L_REG);
		yh = mpu6000_read_reg(GYRO_YOUT_H_REG);
		zl = mpu6000_read_reg(GYRO_ZOUT_L_REG);
		zh = mpu6000_read_reg(GYRO_ZOUT_H_REG);
		//x = (s16)((xh << 8) | xl);
		y = (s16)((yh << 8) | yl);
		z = (s16)((zh << 8) | zl);
		//mpu6000_read_reg(INT_STATUS_REG);
		printf("yh=%d,yl=%d,zh=%d,zl=%d\n",yh,yl,zh,zl);
*/


		rbuf = mpu6000_read_reg_2(GYRO_YOUT_H_REG, 4);
		y = (s16)((*rbuf << 8) | *(rbuf+1));
		z = (s16)((*(rbuf+2) << 8) | *(rbuf+3));
		//printf("yh=%d,yl=%d,zh=%d,zl=%d\n",*rbuf,*(rbuf+1),*(rbuf+2),*(rbuf+3));

	}

	buff[0] = 0;//x + 32768;
	buff[1] = y + 32768;
	buff[2] = z + 32768;

	
	{		
		long long 	timediff;
		
		gettimeofday(&time2, NULL);
		timediff = (time2.tv_sec - time1.tv_sec)* 1000000LL + (time2.tv_usec - time1.tv_usec);
		printf("2>SPI latency=%lld micro seconds \n", timediff);	
	}
	//if(data_ready)
	//	printf("%s: x : %5d, y : %5d,  z : %5d \r", __func__, (int)buff[0], (int)buff[1], (int)buff[2]);
}


void mpu6000_read_accel(void)
{
	s16			x = 0, y = 0, z=0;
	u8			xl, xh, yl, yh, zl, zh;
	u8			ret, data_ready;
	s16			buff[3];

	ret = mpu6000_read_reg(INT_STATUS_REG);
	if(ret & 0x1) {
		data_ready=1;
	}else{
		return;
	}

	if (data_ready)	{
		xl = mpu6000_read_reg(ACCEL_XOUT_L_REG);
		xh = mpu6000_read_reg(ACCEL_XOUT_H_REG);
		yl = mpu6000_read_reg(ACCEL_YOUT_L_REG);
		yh = mpu6000_read_reg(ACCEL_YOUT_H_REG);
		zl = mpu6000_read_reg(ACCEL_ZOUT_L_REG);
		zh = mpu6000_read_reg(ACCEL_ZOUT_H_REG);
		x = (s16)((xh << 8) | xl);
		y = (s16)((yh << 8) | yl);
		z = (s16)((zh << 8) | zl);
		mpu6000_read_reg(INT_STATUS_REG);
	}

	buff[0] = x;
	buff[1] = y;
	buff[2] = z;

//	buff[0] = buff[0] * CONVERT_ACCEL;
//	buff[1] = buff[1] * CONVERT_ACCEL;
//	buff[2] = buff[2] * CONVERT_ACCEL;

	if(data_ready)
		printf("%s: x : %3d, y : %3d,  z : %3d \r", __func__, (int)buff[0], (int)buff[1], (int)buff[2]);
}


void mpu6000_read_temp(void)
{
	s16			t = 0;
	u8			tl, th;
	s16			buff[1];

	th = mpu6000_read_reg(TEMP_OUT_H_REG);
	tl = mpu6000_read_reg(TEMP_OUT_L_REG);
	t = (s16)(( th << 8) | tl);

	buff[0] = t;
	buff[0] = HOMETEMPERATURE + (buff[0] - HOMEOFFSET) * CONVERT_TEMP;

	printf("%s: %d degree centigrade.\r\n", __func__, (int)buff[0]);
}


int mpu6000_init(void)
{
	s32 rval;
	u32 value;
	u32 gyro_id = 0x68;
printf("test_gyro:mpu6000_init\n");
	if (mpu6000_check_spi() < 0) {
		printf("test_gyro: mpu6000_init failed\n");
		return -1;
	}
	
	mpu6000_write_reg(USER_CTRL_REG,0x10);

	mpu6000_write_reg(PWR_MGMT_1_REG, 0x1);
	value = mpu6000_read_reg(PWR_MGMT_1_REG);
	printf("PWR_MGMT_1_REG=0x%x\n",value);

	mpu6000_write_reg(WHO_AM_I_REG, gyro_id);
	value = mpu6000_read_reg(WHO_AM_I_REG);
	if (value != gyro_id)
		printf("ERROR: gyro_id=0x%x is not right\n", value);	

	//set sample rate SMPLRT_DIV_REG
	mpu6000_write_reg(SMPLRT_DIV_REG, 0);
	value = mpu6000_read_reg(SMPLRT_DIV_REG);
	printf("SMPLRT_DIV_REG=0x%x\n",value);

	//set interrupt
	//mpu6000_write_reg(INT_PIN_CFG_REG,0xa0);
	//mpu6000_write_reg(INT_ENABLE_REG,0xf1);

	//set gyroscope output rate
	mpu6000_write_reg(CONFIG_REG, 0);
	value = mpu6000_read_reg(CONFIG_REG);
	printf("CONFIG_REG=0x%x\n",value);

	//set self test
	//mpu6000_write_reg(GYRO_CONFIG_REG,0xe0);

	value = GYRO_SPI_RD_SCLK;
	rval = ioctl(mpu6000_spi, SPI_IOC_WR_MAX_SPEED_HZ, &value);
	if (rval < 0) {
		printf("%s: Can't change SPI speed to 20M!", __func__);
	}

	return 0;
}

static void test_idg2000(void)
{
	char strbuf[1000][100];
	int i,j,k;

	clock_t before, after;
	double duration;

	before = clock();

	for(i=0;i<1000;++i){
		FILE *f = fopen("/sys/kernel/gyro-data/gyro", "r");
		read(fileno(f), strbuf[i], sizeof(strbuf[i]));
		fclose(f);
	}

	after = clock();
	duration = (double)(after - before) / CLOCKS_PER_SEC;

	for(j=0;j<100;j++){
		for(k=0;k<10;k++){
			printf("%s\b",strbuf[j*10+k]);
		}
		printf("\n");
	}
	printf( "1000 samples in %f seconds\n", duration );

}

static void mpu6000_test_gyro(void)
{
	while(1){
		mpu6000_read_gy();
		usleep(500);
	}
}

static void mpu6000_test_accel(void)
{
	while(1){
		mpu6000_read_accel();
		usleep(500);
	}
}

int main(int argc, char **argv)
{

	if (argc < 4 || init_param(argc, argv)) {
		usage();
		return -1;
	}

	if(sensor_module == SENSOR_MPU6000){
		mpu6000_init();

		switch(test_module){
		case TEST_GYRO:
			mpu6000_test_gyro();
			break;
		case TEST_ACCEL:
			mpu6000_test_accel();
			break;
		case TEST_TEMP:
			mpu6000_read_temp();
			break;
		case TEST_DUMP:
			mpu6000_dump_reg();
			break;
		}
	}else if(sensor_module == SENSOR_IDG2000){
		test_idg2000();
	}

	return 0;
}
