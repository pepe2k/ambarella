/*
 * sound/soc/ambarella/dvw101.c
 *
 * Author: Ken He <jianhe@ambarella.com>
 *
 * History:
 *	2012/11/27 - [Ken He] Create file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <mach/hardware.h>
#include <plat/audio.h>

#include "ambarella_i2s.h"
#include "../codecs/alc5633.h"

static const struct snd_soc_dapm_widget dvw_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
};

static const struct snd_soc_dapm_route dvw_dapm_routes[]={
	/* Mic Jack --> MIC_IN*/
	{"Mic Bias Switch", NULL, "Mic Jack"},
	{"MIC1", NULL, "Mic Bias Switch"},
	/* LINE_OUT --> Ext Speaker */
	{"Ext Spk", NULL, "SPOL"},
	{"Ext Spk", NULL, "SPOR"},

} ;

static int dvw_board_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static int dvw_alc5633_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int errorCode = 0;

	errorCode = snd_soc_dapm_new_controls(dapm,dvw_dapm_widgets,
				ARRAY_SIZE(dvw_dapm_widgets));
	if (errorCode) {
		goto init_exit;
	}

	errorCode = snd_soc_dapm_add_routes(dapm,dvw_dapm_routes,
				ARRAY_SIZE(dvw_dapm_routes));
	if (errorCode) {
		goto init_exit;
	}

	errorCode = snd_soc_dapm_sync(dapm);

init_exit:
	return errorCode;
}

static int dvw_board_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int errorCode = 0, mclk, oversample;

	switch (params_rate(params)) {
	case 8000:
		mclk = 4096000;
		oversample = AudioCodec_512xfs;
		break;
	case 11025:
		mclk = 5644800;
		oversample = AudioCodec_512xfs;
		break;
	case 16000:
		mclk = 4096000;
		oversample = AudioCodec_256xfs;
		break;
	case 22050:
		mclk = 5644800;
		oversample = AudioCodec_256xfs;
		break;
	case 24000:
		mclk = 6144000;
		oversample = AudioCodec_256xfs;
		break;
	case 32000:
		mclk = 8192000;
		oversample = AudioCodec_256xfs;
		break;
	case 44100:
		mclk = 11289600;
		oversample = AudioCodec_256xfs;
		break;
	case 48000:
		mclk = 12288000;
		oversample = AudioCodec_256xfs;
		break;
	default:
		errorCode = -EINVAL;
		goto hw_params_exit;
	}

	/* set the I2S system data format*/
	errorCode = snd_soc_dai_set_fmt(codec_dai,
		SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (errorCode < 0) {
		pr_err("can't set codec DAI configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_fmt(cpu_dai,
		SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (errorCode < 0) {
		pr_err("can't set cpu DAI configuration\n");
		goto hw_params_exit;
	}

	/* set the I2S system clock*/
	errorCode = snd_soc_dai_set_sysclk(cpu_dai, AMBARELLA_CLKSRC_ONCHIP, mclk, 0);
	if (errorCode < 0) {
		pr_err("can't set cpu MCLK configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_clkdiv(cpu_dai, AMBARELLA_CLKDIV_LRCLK, oversample);
	if (errorCode < 0) {
		pr_err("can't set cpu MCLK/SF ratio\n");
		goto hw_params_exit;
	}
	errorCode = snd_soc_dai_set_sysclk(codec_dai, 1, mclk, 0);
	if (errorCode < 0) {
		pr_err("can't set cpu MCLK configuration\n");
		goto hw_params_exit;
	}

#if 0
	//use the coeff_div to make sure the I2s_sysclk = 256*fs
	//so ,when we use pll .set the pll in = pll out = mclk
	errorCode = snd_soc_dai_set_pll(codec_dai,0,0,mclk, mclk);
	if (errorCode < 0) {
		pr_err("Can't set pll configuration\n");
		goto hw_params_exit;
	}
#endif

hw_params_exit:
	return errorCode;
}


static struct snd_soc_ops dvw_board_ops = {
	.startup = dvw_board_startup,
	.hw_params = dvw_board_hw_params,
};

static struct snd_soc_dai_link dvw_dai_link = {
		.name = "alc5633amb",
		.stream_name = "ALC5633-STREAM",
		.cpu_dai_name = "ambarella-i2s.0",
		.platform_name = "ambarella-pcm-audio",
		.codec_dai_name = "alc5633",
		.codec_name = "alc5633-codec.0-001c",
		.init = dvw_alc5633_init,
		.ops = &dvw_board_ops,
};

static struct snd_soc_card snd_soc_card_dvw = {
	.name = "DVW",
	.dai_link = &dvw_dai_link,
	.num_links = 1,
};

static struct platform_device *dvw_snd_device;

static int __init dvw_board_init(void)
{
	int errCode = 0;

	dvw_snd_device = platform_device_alloc("soc-audio",-1);
	if (!dvw_snd_device) {
		printk("failed in device alloc \n");
		return -ENOMEM;
	}

	platform_set_drvdata(dvw_snd_device,&snd_soc_card_dvw);

	errCode = platform_device_add(dvw_snd_device);
	if (errCode){
		printk("failed to add device \r\n");
		goto dvw_device_init_exit;
	}

	return 0;

dvw_device_init_exit:
	platform_device_put(dvw_snd_device);
	return errCode;
}

static void __exit dvw_board_exit(void)
{
	platform_device_unregister(dvw_snd_device);
}

module_init(dvw_board_init);
module_exit(dvw_board_exit);

MODULE_AUTHOR("Ken He <jianhe@ambarella.com>");
MODULE_DESCRIPTION("DVW101 Board with alc5633 Codec for ALSA");
MODULE_LICENSE("GPL");


