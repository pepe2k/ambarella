/*
 * sound/soc/ambcloudcam.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * History:
 *	2009/08/20 - [Cao Rongrong] Created file
 *	2011/03/20 - [Cao Rongrong] Port to 2.6.38,
 *				    merge coconut and durian into a5sevk
 *	2012/06/27 - [Cao Rongrong] Rename to ambevk.c
 *    2012/10/16 - [Wu Dongge] modify for cloudcam board
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
#include <linux/module.h>
#include <sound/soc.h>
#include <plat/audio.h>

#include "../codecs/wm8940_amb.h"

static unsigned int dai_fmt = 0;
module_param(dai_fmt, uint, 0644);
MODULE_PARM_DESC(dai_fmt, "DAI format.");

static int ambcloudcam_board_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static int ambcloudcam_board_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int errorCode = 0, mclk, mclk_divider, oversample, i2s_mode;
	int pll_out = 0;

	switch (params_rate(params)) {
	case 8000:
		mclk = 12288000;
		mclk_divider = WM8940_MCLKDIV_6;
		pll_out = 0;
		oversample = AudioCodec_1536xfs;
		break;
	case 11025:
		mclk = 11289600;
		mclk_divider = WM8940_MCLKDIV_4;
		pll_out = 0;
		oversample = AudioCodec_1024xfs;
		break;
	case 12000:
		mclk = 12288000;
		mclk_divider = WM8940_MCLKDIV_4;
		pll_out = 0;
		oversample = AudioCodec_1024xfs;
		break;
	case 16000:
		mclk = 12288000;
		mclk_divider = WM8940_MCLKDIV_3;
		pll_out = 0;
		oversample = AudioCodec_768xfs;
		break;
	case 22050:
		mclk = 11289600;
		mclk_divider = WM8940_MCLKDIV_2;
		pll_out = 0;
		oversample = AudioCodec_512xfs;
		break;
	case 24000:
		mclk = 12288000;
		mclk_divider = WM8940_MCLKDIV_2;
		pll_out = 0;
		oversample = AudioCodec_512xfs;
		break;
	case 32000:
		mclk = 12288000;
		mclk_divider = WM8940_MCLKDIV_1_5;
		pll_out = 0;
		oversample = AudioCodec_384xfs;
		break;
	case 44100:
		mclk = 11289600;
		mclk_divider = WM8940_MCLKDIV_1;
		pll_out = 0;
		oversample = AudioCodec_256xfs;
		break;
	case 48000:
		mclk = 12288000;
		mclk_divider = WM8940_MCLKDIV_1;
		pll_out = 0;
		oversample = AudioCodec_256xfs;
		break;
	default:
		errorCode = -EINVAL;
		goto hw_params_exit;
	}

	if (dai_fmt == 0)
		i2s_mode = SND_SOC_DAIFMT_I2S;
	else
		i2s_mode = SND_SOC_DAIFMT_DSP_A;

	/* set the I2S system data format*/
	errorCode = snd_soc_dai_set_fmt(codec_dai,
		i2s_mode | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (errorCode < 0) {
		pr_err("can't set codec DAI configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_fmt(cpu_dai,
		i2s_mode | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (errorCode < 0) {
		pr_err("can't set cpu DAI configuration\n");
		goto hw_params_exit;
	}

	/* set the I2S system clock*/
	errorCode = snd_soc_dai_set_sysclk(codec_dai, WM8940_SYSCLK, mclk, 0);
	if (errorCode < 0) {
		pr_err("can't set codec MCLK configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_sysclk(cpu_dai, AMBARELLA_CLKSRC_ONCHIP, mclk, 0);
	if (errorCode < 0) {
		pr_err("can't set cpu MCLK configuration\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_clkdiv(codec_dai, WM8940_MCLKDIV, mclk_divider);
	if (errorCode < 0) {
		pr_err("can't set codec MCLK/SF ratio\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_clkdiv(cpu_dai, AMBARELLA_CLKDIV_LRCLK, oversample);
	if (errorCode < 0) {
		pr_err("can't set cpu MCLK/SF ratio\n");
		goto hw_params_exit;
	}

	errorCode = snd_soc_dai_set_pll(codec_dai, 0, 0, mclk, pll_out);
	if (errorCode <0) {
		pr_err("can't set codec PLL configuration\n");
		goto hw_params_exit;
	}

hw_params_exit:
	return errorCode;
}

static struct snd_soc_ops ambcloudcam_board_ops = {
	.startup =  ambcloudcam_board_startup,
	.hw_params =  ambcloudcam_board_hw_params,
};

/* ambcloudcam machine dapm widgets */
static const struct snd_soc_dapm_widget ambcloudcam_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Mic External", NULL),
	SND_SOC_DAPM_SPK("Speaker", NULL),
};

/* ambcloudcam machine audio map (connections to wm8940 pins) */
static const struct snd_soc_dapm_route ambcloudcam_audio_map[] = {
	/* Mic External is connected to MICN, MICNP, AUX*/
	{"MICN", NULL, "Mic External"},
	{"MICP", NULL, "Mic External"},

	/* Speaker is connected to SPKOUTP, SPKOUTN */
	{"Speaker", NULL, "SPKOUTP"},
	{"Speaker", NULL, "SPKOUTN"},
};

static int ambcloudcam_wm8940_init(struct snd_soc_pcm_runtime *rtd)
{
	int errorCode = 0;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	/* not connected */
	snd_soc_dapm_nc_pin(dapm, "MONOOUT");
	snd_soc_dapm_nc_pin(dapm, "AUX");

	/* Add ambcloudcam specific widgets */
	errorCode = snd_soc_dapm_new_controls(dapm,
		ambcloudcam_dapm_widgets,
		ARRAY_SIZE(ambcloudcam_dapm_widgets));
	if (errorCode) {
		goto init_exit;
	}

	/* Set up ambcloudcam specific audio path ambcloudcam_audio_map */
	errorCode = snd_soc_dapm_add_routes(dapm,
		ambcloudcam_audio_map,
		ARRAY_SIZE(ambcloudcam_audio_map));
	if (errorCode) {
		goto init_exit;
	}

	errorCode = snd_soc_dapm_sync(dapm);

init_exit:
	return errorCode;
}

/* ambcloudcam digital audio interface glue - connects codec <--> A2S */
static struct snd_soc_dai_link ambcloudcam_dai_link = {
	.name = "WM8940",
	.stream_name = "WM8940-STREAM",
	.cpu_dai_name = "ambarella-i2s.0",
	.platform_name = "ambarella-pcm-audio",
	.codec_dai_name = "wm8940-hifi",
	.codec_name = "wm8940-codec.0-001a",
	.init = ambcloudcam_wm8940_init,
	.ops = &ambcloudcam_board_ops,
};

/* ambcloud audio machine driver */
static struct snd_soc_card snd_soc_card_ambcloudcam = {
	.name = "AMBCLOUDCAM",
	.owner = THIS_MODULE,
	.dai_link = &ambcloudcam_dai_link,
	.num_links = 1,
};

static int ambcloudcam_soc_snd_probe(struct platform_device *pdev)
{
	int errorCode = 0;

	snd_soc_card_ambcloudcam.dev = &pdev->dev;
	errorCode = snd_soc_register_card(&snd_soc_card_ambcloudcam);
	if (errorCode)
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", errorCode);

	return errorCode;
}

static int ambcloudcam_soc_snd_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&snd_soc_card_ambcloudcam);
	return 0;
}

static struct platform_driver ambcloudcam_soc_snd_driver = {
	.driver = {
		.name = "snd_soc_card_ambcloudcam",
		.owner = THIS_MODULE,
	},
	.probe = ambcloudcam_soc_snd_probe,
	.remove = ambcloudcam_soc_snd_remove,
};

module_platform_driver(ambcloudcam_soc_snd_driver);

MODULE_DESCRIPTION("Amabrella Board with WM8940 Codec for ALSA");
MODULE_LICENSE("GPL");
MODULE_ALIAS("snd-soc-coconut");
MODULE_ALIAS("snd-soc-a5scloudcam");


