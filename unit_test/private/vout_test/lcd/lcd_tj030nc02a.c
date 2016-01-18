


#include <linux/spi/spidev.h>
int				spi_fd;

/* ========================================================================== */


#define TJ030_WRITE_REGISTER(data)  	write(spi_fd, data, 2)

/* ========================================================================== */
static void lcd_tj030nc02a_config_960_240()
{
	u8				mode, bits;
	u32				speed,i=0;
	char				buf[64];
	u16				ret,size;

	u16 data_960_240[] = {
	/*	0x1440,
		0x3000,
		0x2410,
		0x2820,
		0x2c20,
		0x1440,
		0x3000,
		0x241f,
		0x2820,
		0x2c20,*/

		0x0802,
		0x0c01,
		0x103b,
		0x1440,
		0x1818,
		0x1c08,
		0x2000,
		0x2420,
		0x2820,
		0x2c20,
		0x3010,
		0x3400,
		0x403b,
		0x443f,
		0x481d,
		0x5078,
		0x5433,
		0x5812,
		0x5c11,
		0x6000,

	/*	0x1440,
		0x1440,
		0x3000,
		0x241f,
		0x2820,
		0x2c20,
		0x3010,	
		0x1440,
		0x3010,
		0x241f,
		0x2820,
		0x2c20*/
	};

	/* Power On */
	lcd_power_on();

	/* Hardware Reset */
	lcd_reset();

		gpio_set(49);
		usleep(200 << 10);
		gpio_clr(49);
		usleep(200 << 10);
		gpio_set(49);
	/* Backlight on */
	lcd_backlight_on();

	if (lcd_spi_dev_node(buf)) {
		perror("Unable to get lcd spi bus_id or cs_id!\n");
		goto lcd_tj030nc02a_config_960x240_exit;
	}

	spi_fd = open(buf, O_RDWR);
	if (spi_fd < 0) {
		perror("Can't open tj030nc02a_SPI_DEV_NODE to write");
		goto lcd_tj030nc02a_config_960x240_exit;
	}

	mode = 0;
	ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret < 0) {
		perror("Can't set SPI mode");
		close(spi_fd);
		goto lcd_tj030nc02a_config_960x240_exit;
	}

	bits = 16;
	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret < 0) {
		perror("Can't set SPI bits");
		close(spi_fd);
		goto lcd_tj030nc02a_config_960x240_exit;
	}

	speed = 500000;
	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret < 0) {
		perror("Can't set SPI speed");
		close(spi_fd);
		goto lcd_tj030nc02a_config_960x240_exit;
	}

	size = sizeof(data_960_240)/2;
	for(i=0;i<size;i++)
		TJ030_WRITE_REGISTER(&data_960_240[i]);
//	TJ030_WRITE_REGISTER(&data_960_240[i]);

	close(spi_fd);

lcd_tj030nc02a_config_960x240_exit:
	return;
}


/* ========================================================================== */
static int lcd_tj030nc02a_960_240(struct amba_video_sink_mode *pvout_cfg)
{
	pvout_cfg->mode			= AMBA_VIDEO_MODE_960_240;
	pvout_cfg->ratio		= AMBA_VIDEO_RATIO_4_3;
	pvout_cfg->bits			= AMBA_VIDEO_BITS_16;
	pvout_cfg->type			= AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format		= AMBA_VIDEO_FORMAT_PROGRESSIVE;
	pvout_cfg->sink_type		= AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y		= 0x00;
	pvout_cfg->bg_color.cb		= 0x80;
	pvout_cfg->bg_color.cr		= 0x80;

	pvout_cfg->lcd_cfg.mode		= AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT;
	pvout_cfg->lcd_cfg.seqt		= AMBA_VOUT_LCD_SEQ_R0_B1_G2;
	pvout_cfg->lcd_cfg.seqb		= AMBA_VOUT_LCD_SEQ_B0_G1_R2;
	pvout_cfg->lcd_cfg.dclk_edge	= AMBA_VOUT_LCD_CLK_RISING_EDGE;
	pvout_cfg->lcd_cfg.dclk_freq_hz	= 27000000;
	pvout_cfg->lcd_cfg.model	= AMBA_VOUT_LCD_MODEL_TJ030;

	lcd_tj030nc02a_config_960_240();

	return 0;
}



int lcd_tj030nc02a_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_960_240:
		errCode = lcd_tj030nc02a_960_240(pcfg);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}

































