#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <sound/soc.h>
#include <linux/io.h>
#include <sound/fm34_amb.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/delay.h>

#include "fm34_amb.h"

#define M34_WRITE_CHECK

enum{
	MIC_GAIN_CORSE,
};

static unsigned int fm34_i2c_read(struct snd_soc_codec *codec, unsigned int addr)
{
	struct i2c_msg msg[5];
	u8 cmdBuf0[] = {0xFC, 0xF3, 0x37, 0xFF, 0xFF};
	u8 cmdBuf1[] = {0xFC, 0xF3, 0x60, 0x25};
	u8 cmdBuf2[] = {0xFC, 0xF3, 0x60, 0x26};
	u8 dataLow, dataHigh;
	unsigned int pval;
	struct i2c_client *client=to_i2c_client(codec->dev);

	dataLow=dataHigh=0xaa;

	cmdBuf0[3] = (addr & 0xFF00) >> 8;
	cmdBuf0[4] = addr & 0xFF;

	msg[0].addr = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len = sizeof(cmdBuf0);
	msg[0].buf = cmdBuf0;

	i2c_transfer(client->adapter, msg, 1);

	msg[3].addr = client->addr;
	msg[3].flags = client->flags & I2C_M_TEN;
	msg[3].len = sizeof(cmdBuf1);
	msg[3].buf = cmdBuf1;

	msg[4].addr = client->addr;
	msg[4].flags = client->flags & I2C_M_TEN;
	msg[4].flags |= I2C_M_RD;
	msg[4].len = sizeof(dataLow);
	msg[4].buf = &dataLow;

	i2c_transfer(client->adapter, &msg[3], 2);

	msg[1].addr = client->addr;
	msg[1].flags = client->flags & I2C_M_TEN;
	msg[1].len = sizeof(cmdBuf2);
	msg[1].buf = cmdBuf2;

	msg[2].addr = client->addr;
	msg[2].flags = client->flags & I2C_M_TEN;
	msg[2].flags |= I2C_M_RD;
	msg[2].len = sizeof(dataHigh);
	msg[2].buf = &dataHigh;

	i2c_transfer(client->adapter, &msg[1], 2);

	pval = ((dataHigh << 8) | dataLow);

	printk(KERN_DEBUG "ReadFM34Register Addr:0x%02X%02X Data:0x%02X%02X successful\n", cmdBuf0[3], cmdBuf0[4], dataHigh, dataLow);

	return pval&0xffff;
}

static int fm34_i2c_write(struct snd_soc_codec *codec, unsigned int addr, unsigned int value)
{
	u8 cmdBuf[] = {0xFC, 0xF3, 0x3B, 0xFF, 0xFF, 0xFF, 0xFF};
	struct i2c_client *client = to_i2c_client(codec->dev);
#ifdef FM34_WRITE_CHECK
	u16 retval;
	int try=0;

retry:
#endif
	cmdBuf[3] = (addr & 0xFF00) >> 8;
	cmdBuf[4] = addr & 0xFF;
	cmdBuf[5] = (value & 0xFF00) >> 8;
	cmdBuf[6] = value & 0xFF;

	if(sizeof(cmdBuf) != i2c_master_send(client, cmdBuf, sizeof(cmdBuf))) {
		printk(KERN_DEBUG "WriteFM34Register Addr:0x%02X%02X Data:0x%02X%02X failed\n", cmdBuf[3], cmdBuf[4], cmdBuf[5], cmdBuf[6]);
		return -1;
	} else {
#ifdef FM34_WRITE_CHECK
		if(addr==0x22fb)
			return 0;
		retval=snd_soc_read(codec, addr);
		if((retval!=value) && (try++ < 10))
			goto retry;
		if(try >= 10)
			printk("Write error\n");
#endif
		printk(KERN_DEBUG "WriteFM34Register Addr:0x%02X%02X Data:0x%02X%02X successful\n", cmdBuf[3], cmdBuf[4], cmdBuf[5], cmdBuf[6]);
		return 0;
	}
}

static const char *fm34_mic_gain[] = {"0db", "-12db", "-24db", "-36db", "-48db", "-60db", "-72db", "-84db", "-96db", "-108db", "-132db", "", "", "+24db", "+12db"};
static int fm34_get_mic0_corse_gain(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u16 val;

	val = snd_soc_read(codec, MIC_PGAGAIN);

	ucontrol->value.integer.value[0] = (val&0xf0)>>4;

	return 0;
}

static int fm34_set_mic0_corse_gain(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u16 val, val_old;

	val = snd_soc_read(codec, MIC_PGAGAIN);
	val_old = val;
	val &= 0xf0;
	val >>= 4;

	if(val == ucontrol->value.integer.value[0])
		return 0;

	val_old &= ~0xf0;
	val_old |= (ucontrol->value.integer.value[0]<<4);
	snd_soc_write(codec, MIC_PGAGAIN, val_old);

	return 1;

}

static int fm34_get_mic1_corse_gain(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u16 val;

	val = snd_soc_read(codec, MIC_PGAGAIN);

	ucontrol->value.integer.value[0] = (val&0xf000)>>12;

	return 0;
}

static int fm34_set_mic1_corse_gain(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u16 val, val_old;

	val = snd_soc_read(codec, MIC_PGAGAIN);
	val_old = val;
	val &= 0xf000;
	val >>= 12;

	if(val == ucontrol->value.integer.value[0])
		return 0;

	val_old &= ~0xf000;
	val_old |= (ucontrol->value.integer.value[0]<<12);
	snd_soc_write(codec, MIC_PGAGAIN, val_old);

	return 1;

}


static const struct soc_enum fm34_enum[] = {
		SOC_ENUM_SINGLE_EXT(15, fm34_mic_gain),
};


static DECLARE_TLV_DB_SCALE(fm34_pga_mic_fine, 0, -75, 0);
static DECLARE_TLV_DB_SCALE(fm34_headphone_gain, 0, -75, 0);
static DECLARE_TLV_DB_SCALE(fm34_dac_fine_gain, 0, -75, 0);

static const struct snd_kcontrol_new fm34_snd_controls[] = {
	SOC_SINGLE("Bypass Output", LINE_PASS, 0, 0x5, 0),
	SOC_SINGLE("Idle Noise Suppression", KL_CONFIG, 8, 1, 0),
	SOC_SINGLE("Line Out DRC", KL_CONFIG, 12, 1, 0),
	SOC_SINGLE("Speaker Out HPF", KL_CONFIG, 6, 1, 0),
	SOC_SINGLE("Speaker Out DRC", KL_CONFIG, 4, 1, 0),
	SOC_SINGLE("Frame Processing", KL_CONFIG, 0, 1, 0),

	SOC_SINGLE("Line In AEC", SP_FLAG, 8, 1, 0),
	SOC_SINGLE("Line In AGC", SP_FLAG, 11, 1, 0),
	SOC_SINGLE("Mic AGC", SP_FLAG, 15, 1, 0),

	SOC_ENUM_EXT("Mic0 Corse Gain", fm34_enum[MIC_GAIN_CORSE], fm34_get_mic0_corse_gain, fm34_set_mic0_corse_gain),
	SOC_SINGLE_TLV("Mic0 Fine Gain", MIC_PGAGAIN, 0, 15, 0, fm34_pga_mic_fine),
	SOC_ENUM_EXT("Mic1 Corse Gain", fm34_enum[MIC_GAIN_CORSE], fm34_get_mic1_corse_gain, fm34_set_mic1_corse_gain),
	SOC_SINGLE_TLV("Mic1 Fine Gain", MIC_PGAGAIN, 8, 15, 0, fm34_pga_mic_fine),

	SOC_SINGLE("Mic Diff Gain", MIC_DIFF_GAIN, 0, 0xffff, 0),
	SOC_SINGLE("Line in Pgagain", LINEIN_PGAGAIN, 0, 0x7fff, 0),

	SOC_SINGLE_TLV("Headphone Gain", DAC_PGAGAINA, 13, 7, 0, fm34_headphone_gain),
	SOC_SINGLE_TLV("DAC Fine Gain", DAC_PGAGAINA, 0, 7, 0, fm34_dac_fine_gain),
};

static const struct snd_soc_dapm_widget fm34_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "Playback", DAC_OUPUT_SELECT, 15, 0),
//	SND_SOC_DAPM_ADC("ADC1", "Capture", DV_ENABLE_B, 1, 0),

	SND_SOC_DAPM_INPUT("IN1"),
	SND_SOC_DAPM_OUTPUT("OUT"),
};


static const struct snd_soc_dapm_route fm34_dapm_routes[] = {
	{"ADC1", NULL, "IN1"},
	{"OUT", NULL, "DAC"},
};

static int fm34_i2c_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	return 0;
}

static int fm34_set_dai_sysclk(struct snd_soc_dai *codec_dai, int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int fm34_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec= dai->codec;
	u16 mute_reg;

	if(mute)
		mute_reg = 0x0;
	else
		mute_reg = 0xffff;

	return snd_soc_write(codec, SPK_MUTE, mute_reg);
}

static int fm34_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 conf=snd_soc_read(codec, DV_ENABLE_B);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK)
	{
		case SND_SOC_DAIFMT_I2S:
			conf &= ~0x100;
			break;
		default :
			break;
	}
	snd_soc_write(codec, DV_ENABLE_B, conf);
	return 0;
}

static int fm34_add_widgets(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_dapm_new_controls(dapm, fm34_dapm_widgets,
				  ARRAY_SIZE(fm34_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, fm34_dapm_routes, ARRAY_SIZE(fm34_dapm_routes));

	return 0;
}

int fm34_enable_profile(struct snd_soc_codec *codec)
{
	u16 val;
	int try=0;

	snd_soc_write(codec, PARSER_SYNC_FLAG, 0x0);
	mdelay(15);

waits:
	val=snd_soc_read(codec, PARSER_SYNC_FLAG);
	if(val<0)
	{
		pr_err("%s: I2c error\n", __FUNCTION__);
		return val;
	}
	try++;
	if(try>=100)
		return -1;
	if(val != 0x5a5a)
	{
		WARN_ONCE(1, "Waiting dsp to run.... \n");
		mdelay(1);
		goto waits;
	}

	return 0;

}


static int fm34_set_bias_level(struct snd_soc_codec *codec, enum snd_soc_bias_level level)
{
	struct fm34_platform_data *fm34_pdata = codec->dev->platform_data;

	switch(level)
	{
		case SND_SOC_BIAS_ON :
			break;
		case SND_SOC_BIAS_PREPARE :
			break;
		case SND_SOC_BIAS_STANDBY :
			gpio_direction_output(fm34_pdata->pwdn_pin, GPIO_LOW);
			mdelay(2);
			gpio_direction_output(fm34_pdata->pwdn_pin, GPIO_HIGH);
			break;
		case SND_SOC_BIAS_OFF :
			gpio_direction_output(fm34_pdata->pwdn_pin, GPIO_HIGH);
			mdelay(2);
			gpio_direction_output(fm34_pdata->pwdn_pin, GPIO_LOW);
			break;
	}

	codec->dapm.bias_level = level;

	return 0;
}

static void fm34_default_config(struct snd_soc_codec *codec)
{
	struct fm34_platform_data *fm34_pdata = codec->dev->platform_data;
	u16 index, *conf = (u16*)fm34_pdata->conf;
	int ret;

	if(conf){
		for(index=0; index<fm34_pdata->cconf;index++)
		{
			ret=snd_soc_write(codec, *conf, *(conf+1));
			if(ret<0)
				printk("%s: snd_soc_write addr %d error\n", __FUNCTION__, *conf);
			conf +=2;
		}
	}
}

static int fm34_probe(struct snd_soc_codec *codec)
{
	struct fm34_platform_data *fm34_pdata;
	int ret=0;

	printk(KERN_NOTICE "fm34500b probe ...\n");

	fm34_pdata = codec->dev->platform_data;
	if(!fm34_pdata)
			return -EINVAL;
	codec->write = fm34_i2c_write;
	codec->read = fm34_i2c_read;

	if(gpio_is_valid(fm34_pdata->rst_pin))
	{
		ret = gpio_request(fm34_pdata->rst_pin, "fm34 reset");
		if(ret<0)
			return ret;
	}else
			return -ENODEV;

	if(gpio_is_valid(fm34_pdata->pwdn_pin))
	{
		ret = gpio_request(fm34_pdata->pwdn_pin, "fm34 pwdn");
		if(ret<0)
			goto fail1;
	}else
	{
			ret = -ENODEV;
			goto fail1;
	}

	if(gpio_is_valid(fm34_pdata->spk_mute_pin))
	{
		ret = gpio_request(fm34_pdata->spk_mute_pin, "spk mute");
		if(ret<0)
			goto fail2;
	}else
	{
			ret = -ENODEV;
			goto fail2;
	}

	fm34_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	gpio_direction_output(fm34_pdata->spk_mute_pin, GPIO_HIGH);
	gpio_direction_output(fm34_pdata->rst_pin, GPIO_LOW);
	mdelay(fm34_pdata->rst_delay);
	gpio_direction_output(fm34_pdata->rst_pin, GPIO_HIGH);

	mdelay(15);

	fm34_default_config(codec);
	ret=fm34_enable_profile(codec);
	if(ret<0)
		goto fail;

	//snd_soc_write(codec, DV_ENABLE_B, 0x2481);
	snd_soc_add_codec_controls(codec, fm34_snd_controls, ARRAY_SIZE(fm34_snd_controls));
	fm34_add_widgets(codec);
	printk("FM34-500b init done\n");
	return 0;

fail:
	gpio_free(fm34_pdata->spk_mute_pin);
fail2:
	gpio_free(fm34_pdata->pwdn_pin);
fail1:
	gpio_free(fm34_pdata->rst_pin);
	printk("Fm34 Init Failed\n");

	return ret;
}

static int fm34_remove(struct snd_soc_codec *codec)
{
	struct fm34_platform_data *fm34_pdata=codec->dev->platform_data;

	fm34_set_bias_level(codec, SND_SOC_BIAS_OFF);
	gpio_free(fm34_pdata->pwdn_pin);
	gpio_free(fm34_pdata->rst_pin);
	gpio_free(fm34_pdata->spk_mute_pin);

	return 0;


}

#define FM34_RATES		SNDRV_PCM_RATE_8000_48000
#define FM34_FORMATS	SNDRV_PCM_FMTBIT_S16_LE


static const struct snd_soc_dai_ops fm34_dai_ops = {
	.hw_params = fm34_i2c_hw_params,
	.set_fmt = fm34_set_dai_fmt,
	.digital_mute = fm34_mute,
	.set_sysclk = fm34_set_dai_sysclk,
};

static struct snd_soc_dai_driver fm34_dai = {
	.name = "fm34-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = FM34_RATES,
		.formats = FM34_FORMATS, },
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = FM34_RATES,
		.formats = FM34_FORMATS,},
	.ops = &fm34_dai_ops,
};

static int fm34_suspend(struct snd_soc_codec *codec)
{
	fm34_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int fm34_resume(struct snd_soc_codec *codec)
{
	fm34_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_fm34 = {
	.probe			=	fm34_probe,
	.remove			=	fm34_remove,
	.suspend			=	fm34_suspend,
	.resume			=	fm34_resume,
	.set_bias_level	=    fm34_set_bias_level,
	.reg_word_size = sizeof(u16),
};

static int fm34_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	return snd_soc_register_codec(&i2c->dev, &soc_codec_dev_fm34, &fm34_dai, 1);
}

static int fm34_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);

	return 0;
}

static const struct i2c_device_id fm34_i2c_id[] = {
	{ "fm34_500b", 0 },
	{ }
};

static struct i2c_driver fm34_i2c_driver = {
	.driver = {
		.name = "fm34-codec",
		.owner = THIS_MODULE,
	},
	.probe = fm34_i2c_probe,
	.remove =fm34_i2c_remove,
	.id_table = fm34_i2c_id,
};

static int __init fm34_init(void)
{
	int ret;

	ret = i2c_add_driver(&fm34_i2c_driver);
	if(ret!=0)
			printk("Failed to register fm34-500b i2c driver: %d\n", ret);

	return ret;
}
module_init(fm34_init);

static void __exit fm34_exit(void)
{
	i2c_del_driver(&fm34_i2c_driver);
}
module_exit(fm34_exit);

MODULE_DESCRIPTION("Soc fm34-500b driver");
MODULE_LICENSE("GPL");
