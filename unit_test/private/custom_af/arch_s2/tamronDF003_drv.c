#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <assert.h>
#include "basetypes.h"

#include "img_struct_arch.h"
#include "tamronDF003_drv.h"

#ifndef ABS
#define ABS(x) ({						\
		int __x = (x);					\
		(__x < 0) ? -__x : __x;			\
            })
#endif

static int spi_fd = -1;

static void gpio_set(u8 gpio_id)
{
	int direction;
	char buf[128];

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	direction = open(buf, O_WRONLY);
	if (direction < 0) {
		printf("%s: Can't open direction sys file!\n", __func__);
		goto gpio_set_exit;
	}
	sprintf(buf, "high");
	write(direction, buf, sizeof(buf));
	close(direction);

gpio_set_exit:
	return;
}

static void gpio_clr(u8 gpio_id)
{
	int direction;
	char buf[128];

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	direction = open(buf, O_WRONLY);
	if (direction < 0) {
		printf("%s: Can't open direction sys file!\n", __func__);
		goto gpio_clr_exit;
	}
	sprintf(buf, "low");
	write(direction, buf, sizeof(buf));
	close(direction);

gpio_clr_exit:
	return;
}

static int gpio_get(int gpio_id, int *level)
{
	int value;
	char buf[128];

	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio_id);
	value = open(buf, O_RDONLY);
	if (value < 0) {
		printf("%s: Can't open value sys file!\n", __func__);
		goto gpio_get_exit;
	}
	read(value, buf, 1);
	close(value);
	*level = atoi(&buf[0]);

gpio_get_exit:
	return 0;
}

static int	gpio_config(int gpio_id, char* config){
	int _export, direction;
	char buf[128];

	_export = open("/sys/class/gpio/export", O_WRONLY);
	if (_export < 0) {
		printf("%s: Can't open export sys file!\n", __func__);
		goto gpio_config_exit;
	}
	sprintf(buf, "%d", gpio_id);
	write(_export, buf, sizeof(buf));
	close(_export);

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	direction = open(buf, O_WRONLY);
	if (direction < 0) {
		printf("%s: Can't open direction sys file!\n", __func__);
		goto gpio_config_exit;
	}
	sprintf(buf, "in");
	write(direction, buf, sizeof(buf));
	close(direction);

gpio_config_exit:
	return 0;
}

static int open_spi_dev(void)
{
	if ((spi_fd  = open(R2A30404NP_SPI_DEV_NODE, O_RDWR)) < 0) {
		printf("can't open spi device\n");
		return -1;
	}

	return 0;
}

static int setup_spi_dev(void)
{
	u8 mode = SPI_MODE_3;
	u8 bits = 16;
	u32 speed = 5000000;

	if ( ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
		printf("can't set spi mode\n");
		return -1;
	}

	if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
		printf("can't set bits per word\b");
		return -1;
	}

	if ( ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
		printf("can't set spi max speed hz\n");
		return -1;
	}

	return 0;
}

static void r2a30404np_cmd(u16 cmd)
{
	int error;
	error = write(spi_fd, &cmd, (sizeof(u16)/sizeof(u8)));
	if(error<0){
		printf("r2a30404np_cmd error!\n");
	}
}
static int set_shutter_speed(u8 ex_mode, u16 ex_time){
	return FUNC_NOT_SUPPORTED;
}
static int lens_park(void){
	return FUNC_NOT_SUPPORTED;
}
static int zoom_in(u16 pps, u32 distance){
	u32 dist;
	dist = (ABS(distance)) << MICROSTEP_ZOOM;
	r2a30404np_cmd(ZOOM|DIRECTION|REVERSE|MICRO_MODE|NO_ACC_NO_DEC);
	r2a30404np_cmd(ZOOM|PULSE_RATE0|pps);
	r2a30404np_cmd(ZOOM|NO_PULSES1|(dist & 0x00003FF));
	r2a30404np_cmd(ZOOM|NO_PULSES2|(u16)(dist >> 10));
	r2a30404np_cmd(ZOOM|OPERATION_CTRL|ACTIVE|EXCIT_ON);
	return 0;
}
static int zoom_out(u16 pps, u32 distance){
	u32 dist;
	dist = (ABS(distance)) << MICROSTEP_ZOOM;
	r2a30404np_cmd(ZOOM|DIRECTION|FORWARD|MICRO_MODE|NO_ACC_NO_DEC);
	r2a30404np_cmd(ZOOM|PULSE_RATE0|pps);
	r2a30404np_cmd(ZOOM|NO_PULSES1|(dist & 0x00003FF));
	r2a30404np_cmd(ZOOM|NO_PULSES2|(u16)(dist >> 10));
	r2a30404np_cmd(ZOOM|OPERATION_CTRL|ACTIVE|EXCIT_ON);
	return 0;
}
static int zoom_stop(void){
	return FUNC_NOT_SUPPORTED;
}

static int focus_far(u16 pps, u32 distance){
	u32 dist;
	dist = (ABS(distance)) << MICROSTEP_FOCUS;
	r2a30404np_cmd(AF|DIRECTION|FORWARD|MICRO_MODE|NO_ACC_NO_DEC);
	r2a30404np_cmd(AF|PULSE_RATE_RANGE|PRR_02u|MICRO512);
   	r2a30404np_cmd(AF|PULSE_RATE0|pps);
   	r2a30404np_cmd(AF|NO_PULSES1|(dist & 0x00003FF));
   	r2a30404np_cmd(AF|NO_PULSES2|(u16)(dist >> 10));
	r2a30404np_cmd(AF|OPERATION_CTRL|ACTIVE|EXCIT_ON);
	return 0;
}
static int focus_near(u16 pps, u32 distance){
	u32 dist;
	dist = (ABS(distance)) << MICROSTEP_ZOOM;
	r2a30404np_cmd(AF|DIRECTION|REVERSE|MICRO_MODE|NO_ACC_NO_DEC);
	r2a30404np_cmd(AF|PULSE_RATE_RANGE|PRR_02u|MICRO512);
   	r2a30404np_cmd(AF|PULSE_RATE0|pps);
   	r2a30404np_cmd(AF|NO_PULSES1|(dist & 0x00003FF));
   	r2a30404np_cmd(AF|NO_PULSES2|(u16)(dist >> 10));
	r2a30404np_cmd(AF|OPERATION_CTRL|ACTIVE|EXCIT_ON);
	return 0;
}
static int focus_stop(void){
	return FUNC_NOT_SUPPORTED;
}
static int set_aperture(u16 aperture_idx){
	return FUNC_NOT_SUPPORTED;
}

static int set_mechanical_shutter(u8 me_shutter){
	return FUNC_NOT_SUPPORTED;
}
static void set_zoom_pi(u8 pi_n){
	const int SLEEP_TIME = 100;
	int ext_zm;
	PI_status zpi, zpi_init;

	gpio_get(Z_PI, (int*)(&zpi));
	zpi_init = zpi;
	while (zpi_init == zpi)
	{
		gpio_get(Z_PI, (int*)(&zpi));
		if (zpi == PI_DN) // tele point, move to the wide point
		{
			zoom_out(500,1);
		}
		if (zpi == PI_EN) // wide point, move to the tele point
		{
			zoom_in(500,1);
		}
		while (ext_zm) { // wait the step motor runing
			gpio_get(EXT_ZOOM, &ext_zm);
			usleep(SLEEP_TIME);
		}
	}
}
static void set_focus_pi(u8 pi_n){
	const int SLEEP_TIME = 100;
	int ext_focus;
	PI_status fpi, fpi_init;

	gpio_get(F_PI, (int*)(&fpi));
	fpi_init = fpi;
	while (fpi_init == fpi)
	{
		gpio_get(F_PI, (int*)(&fpi));
		if (fpi == PI_DN) // near point, move to the far point
		{
			focus_far(500,1);
		}
		if (fpi == PI_EN) // far point, move to the near point
		{
			focus_near(500,1);
		}
		while (ext_focus) { // wait the step motor runing
			gpio_get(EXT_FOCUS, &ext_focus);
			usleep(SLEEP_TIME);
		}
	}
}

static int lens_standby(u8 en){
	r2a30404np_cmd(ZOOM|OPERATION_CTRL|STOP);
	r2a30404np_cmd(AF|OPERATION_CTRL|STOP);
	return 0;
}
static int set_IRCut(u8 enable){
	gpio_set(MOTOR_IN5);
	usleep(50*1000);
	if (enable) {
		r2a30404np_cmd(COMM|CHAN7_CTRL|REVERSE);
	} else {
		r2a30404np_cmd(COMM|CHAN7_CTRL|FORWARD);
	}
	usleep(50*1000);
	gpio_clr(MOTOR_IN5);
	return 0;
}

static int isFocusRuning(void){
	int ext_focus;
	gpio_get(EXT_FOCUS, &ext_focus);
	return ext_focus;
}

static int isZoomRuning(){
	int ext_zoom;
	gpio_get(EXT_ZOOM, &ext_zoom);
	return ext_zoom;
}
static int lens_init(void){
	int rval;
	rval = open_spi_dev();
	if (rval == -1)
		return -1;
	rval = setup_spi_dev();
	if (rval == -1)
		return -1;

	gpio_config(R_RESET,"out");
	gpio_config(F_PI,"in");
	gpio_config(Z_PI,"in");
	gpio_config(EXT_FOCUS,"in");
	gpio_config(EXT_ZOOM,"in");
	gpio_config(MOTOR_IN5,"out");

	gpio_clr(MOTOR_IN5);
	r2a30404np_cmd(COMM|SW_RST|ALL_RESET);		/* software reset */

	/* Set TAMRON 18X lens motors parameters */
	r2a30404np_cmd(AF|DIRECTION|MICRO_MODE|NO_ACC_NO_DEC);
	r2a30404np_cmd(AF|PULSE_RATE0|INIT_PPS_FOCUS);
	r2a30404np_cmd(AF|PULSE_RATE_RANGE|PRR_FOCUS|MICRO512);
	r2a30404np_cmd(AF|PRE_EXCIT|EXCITIME_0);
	r2a30404np_cmd(AF|POST_EXCIT|EXCITIME_0);
	r2a30404np_cmd(AF|VOLTAGE|VOLT12_36|CARRFEQ1);

	r2a30404np_cmd(ZOOM|DIRECTION|MICRO_MODE|NO_ACC_NO_DEC);
	r2a30404np_cmd(ZOOM|PULSE_RATE0|INIT_PPS_ZOOM);
	r2a30404np_cmd(ZOOM|PULSE_RATE_RANGE|PRR_ZOOM|MICRO512);
	r2a30404np_cmd(ZOOM|PRE_EXCIT|EXCITIME_0);
	r2a30404np_cmd(ZOOM|POST_EXCIT|EXCITIME_0);
	r2a30404np_cmd(ZOOM|VOLTAGE|VOLT34_34|CARRFEQ1);

	r2a30404np_cmd(COMM|EXT_OUT|EXT_PULSE|EXT12|EXT34|MOB34);
	r2a30404np_cmd(COMM|PI_CTRL|PI1_ON|PI2_ON);		/* Enable af and zoom pi */
	r2a30404np_cmd(COMM|PS_CTRL|PS_ON|PS12_ON|PS3_ON|PS4_ON);
	r2a30404np_cmd(COMM|INPUT_CTRL|IN_CTRL|EDGE_HIGH);

	r2a30404np_cmd(ZOOM|OPERATION_CTRL|STOP|EXCIT_ON);
	r2a30404np_cmd(AF|OPERATION_CTRL|STOP|EXCIT_ON);
	r2a30404np_cmd(AF|DIRECTION|MICRO_MODE|NO_ACC_NO_DEC);

	set_focus_pi(1);
	printf("return to focus PI\n");
	set_zoom_pi(1);
	printf("return to zoom PI\n");
	r2a30404np_cmd(COMM|PI_CTRL|PI1_OFF|PI2_OFF);		/* Enable af and zoom pi */
	printf("lens init ready!\n");
	return 0;
}

lens_dev_drv_t tamronDF003_drv = {
	.set_IRCut = set_IRCut,
	.set_shutter_speed = set_shutter_speed,
	.lens_init = lens_init,
	.lens_park = lens_park,
	.zoom_stop = zoom_stop,
	.focus_near = focus_near,
	.focus_far = focus_far,
	.zoom_in = zoom_in,
	.zoom_out = zoom_out,
	.focus_stop = focus_stop,
	.set_aperture = set_aperture,
	.set_mechanical_shutter = set_mechanical_shutter,
	.set_zoom_pi = set_zoom_pi,
	.set_focus_pi = set_focus_pi,
	.lens_standby = lens_standby,
	.isFocusRuning = isFocusRuning,
	.isZoomRuning = isZoomRuning
};