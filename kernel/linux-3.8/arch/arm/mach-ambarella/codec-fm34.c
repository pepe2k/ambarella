#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <sound/fm34_amb.h>

static struct fm34_platform_data fm34_pdata;


static u16 fm34_config[][2] = {
		{0x22d2, 0x0000},
		{0x22f2, 0x0048},
		{0x22fa, 0x2483},
		{0x2303, 0x0801},
		{0x2304, 0x4310},
		{0x2305, 0x0001},
		{0x2301, 0x0001},
		{0x22fe, 0x0fa0},
		{0x22ff, 0x0813},
		{0x230e, 0x0000},
		{0x230c, 0x0360},
		{0x230d, 0x0480},
		{0x22f6, 0x0003},
};


static struct i2c_board_info ambarella_fm34_board_info = {
	I2C_BOARD_INFO("fm34_500b", 0x60),
	.platform_data = &fm34_pdata,
};

int __init ambarella_init_fm34(u8 i2c_bus_num, u8 rst_pin, u8 rst_delay, u8 pd_pin, u8 spk_mute_pin)
{
	if(!rst_delay)
		rst_delay = 1;
	fm34_pdata.rst_pin= rst_pin;
	fm34_pdata.rst_delay= rst_delay;
	fm34_pdata.pwdn_pin = pd_pin;
	fm34_pdata.spk_mute_pin = spk_mute_pin;
	fm34_pdata.conf = (unsigned short *)fm34_config;
	fm34_pdata.cconf = sizeof(fm34_config)/sizeof(u16)/2;

	i2c_register_board_info(i2c_bus_num, &ambarella_fm34_board_info,  1);

	return 0;
}
