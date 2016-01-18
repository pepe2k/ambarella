/*
* alc5633.c ---- ALC5633 ALSA Soc Audio Driver
*
* History:
*	2012/07/26 - [ Johson Diao ] created file
*
* Copyright (C) 2012-2012 , Ambarella, Inc.
*
* All rights reserved. No Part of this file may be reproduced, stored
* in a retrieval system, or transmitted, in any form, or by any means,
* electronic, mechanical, photocopying, recording, or otherwise,
* without the prior consent of Ambarella, Inc.
*/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <sound/tlv.h>


#include "alc5633.h"
#define debug_ALC5633 (0)
#if debug_ALC5633
#define PRINTK(arg...) printk(arg)
#else
#define PRINTK(arg...)
#endif
static unsigned int BLCK_FREQ=32;	//32fs,bitclk is 32 fs
static const u16 alc5633_reg[112];


static struct alc5633_init_reg init_list[] = {
	{ALC5633_PWR_MANAG_ADD1,0xbe20},
	{ALC5633_PWR_MANAG_ADD2,0xfffb},
	{ALC5633_PWR_MANAG_ADD3,0xef0b},
	{ALC5633_GBL_CLK_CTRL,0x0000},
	{ALC5633_DEPOP_CTRL_1,0x8000},
	{ALC5633_DEPOP_CTRL_2,0xb000},
	{ALC5633_PWR_MANAG_ADD4,0xc00},
	{ALC5633_HPMIXER_CTRL,0x3e3e},
	{ALC5633_HP_OUT_VOL,0x4040},
	{ALC5633_SPK_HP_MIXER_CTRL,0x30},
	{ALC5633_MIC_CTRL_2,0x5500},
	{ALC5633_MIC_CTRL_1,0x0000},
	{ALC5633_REC_MIXER_CTRL,0x7dfe},
	{ALC5633_ADC_DIG_VOL,0x0000},
};

#define ALC5633_INIT_REG_LEN ARRAY_SIZE(init_list)

static int alc5633_sync(struct snd_soc_codec *codec)
{
	int i;
	for (i=0;i<ALC5633_INIT_REG_LEN;i++){
		snd_soc_write(codec,init_list[i].reg,init_list[i].val);
	}
	return 0;
}


/*
 * read alc5633 register cache
 */
static inline unsigned int alc5633_read_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u16 *cache = codec->reg_cache;
	if (reg < 1 || reg > (ARRAY_SIZE(alc5633_reg) + 1))
		return -1;
	return cache[reg];
}


/*
 * write alc5633 register cache
 */

static inline void alc5633_write_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg, unsigned int value)
{
	u16 *cache = codec->reg_cache;
	if (reg < 0 || reg > 0x7e)
		return;
	cache[reg] = value;
}




static int alc5633_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int val)
{
	PRINTK("WRITE[0x%x]=0x%x\r\n ",reg,val);
	alc5633_write_reg_cache(codec,reg,val);
	val = ((val&0xff00)>>8) | ((val&0x00ff)<<8);
	if (i2c_smbus_write_word_data(codec->control_data,reg,val)){
		printk("alc5633 I2c write failed\r\n");
		return -EIO;
	}
	return val;
}

static unsigned int alc5633_read(struct snd_soc_codec *codec, unsigned int reg)
{
	unsigned int value = 0x0;
	value = i2c_smbus_read_word_data(codec->control_data,reg);
	value = ((value&0xff00)>>8) | ((value&0x00ff)<<8);
	return value;
}

//HP power on depop
static void hp_depop_mode2(struct snd_soc_codec *codec)
{
        snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD3,PWR_MAIN_BIAS|PWR_VREF,PWR_VREF|PWR_MAIN_BIAS);
        snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD4,PWR_HP_L_VOL|PWR_HP_R_VOL,PWR_HP_L_VOL|PWR_HP_R_VOL);
        snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD3,PWR_HP_AMP,PWR_HP_AMP);
        snd_soc_update_bits(codec,ALC5633_DEPOP_CTRL_1,PW_SOFT_GEN|EN_DEPOP_2,PW_SOFT_GEN|EN_DEPOP_2);
        schedule_timeout_uninterruptible(msecs_to_jiffies(300));
        snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD3,PWR_HP_DIS_DEPOP|PWR_HP_AMP_DRI,PWR_HP_DIS_DEPOP|PWR_HP_AMP_DRI);
        snd_soc_update_bits(codec,ALC5633_DEPOP_CTRL_1,0,EN_DEPOP_2);
}

//HP mute/unmute depop
static void hp_mute_unmute_depop(struct snd_soc_codec *codec,unsigned int Mute)
{
	if(Mute)
	{
		snd_soc_update_bits(codec,ALC5633_DEPOP_CTRL_1,PW_SOFT_GEN|EN_SOFT_FOR_S_M_DEPOP|EN_HP_R_M_UM_DEPOP|EN_HP_L_M_UM_DEPOP
							   ,PW_SOFT_GEN|EN_SOFT_FOR_S_M_DEPOP|EN_HP_R_M_UM_DEPOP|EN_HP_L_M_UM_DEPOP);
		snd_soc_update_bits(codec, ALC5633_HP_OUT_VOL, 0x8080, 0x8080);
		mdelay(80);
		snd_soc_update_bits(codec,ALC5633_DEPOP_CTRL_1,PW_SOFT_GEN|EN_SOFT_FOR_S_M_DEPOP|EN_HP_R_M_UM_DEPOP|EN_HP_L_M_UM_DEPOP,0x0);
	}
	else
	{
		snd_soc_update_bits(codec,ALC5633_DEPOP_CTRL_1,PW_SOFT_GEN|EN_SOFT_FOR_S_M_DEPOP|EN_HP_R_M_UM_DEPOP|EN_HP_L_M_UM_DEPOP
							   ,PW_SOFT_GEN|EN_SOFT_FOR_S_M_DEPOP|EN_HP_R_M_UM_DEPOP|EN_HP_L_M_UM_DEPOP);
		snd_soc_update_bits(codec, ALC5633_HP_OUT_VOL, 0x8080,0x0000);
		mdelay(80);
		snd_soc_update_bits(codec,ALC5633_DEPOP_CTRL_1,PW_SOFT_GEN|EN_SOFT_FOR_S_M_DEPOP|EN_HP_R_M_UM_DEPOP|EN_HP_L_M_UM_DEPOP,0x0);
	}

}




static DECLARE_TLV_DB_SCALE(noise_tlv,0,300,0);
static DECLARE_TLV_DB_SCALE(signel_lim_tlv,-4650,150,0);
static DECLARE_TLV_DB_SCALE(noise_gate_tlv,-8250,150,0);
static DECLARE_TLV_DB_SCALE(dac_preg_tlv,0,37,0);
static DECLARE_TLV_DB_SCALE(dac_postg_tlv,0,150,0);



static const char *alc5633_spo_source_sel[] = {"VMID", "HPMIX", "SPKMIX", "AUXMIX"};
static const char *alc5633_input_mode_source_sel[] = {"Single-end", "Differential"};
static const char *alc5633_auxout_mode_source_sel[] = {"Mono", "Stereo"};
static const char *alc5633_mic_boost[] = {"Bypass", "+20db", "+24db", "+30db",
			"+35db", "+40db", "+44db", "+50db", "+52db"};
static const char *alc5633_spor_source_sel[] = {"RN", "RP", "LN", "VMID"};
static const char *alc5633_alc_ctrl[] = {"Disable","Enable"};
static const char *alc5633_alc_path[] = {"DAC","ADC"};
static const char *alc5633_alc_noise_ctrl[]={"Disable","Enable"};
static const char *alc5633_alc_noise_gate_ctrl[]={"Disable","Enable"};
static const char *alc5633_music_style[]={"NORMAL","CLUB","DANCE","LIVE","POP","ROCK",
	"OPPO","TREBLE","BASS","RECORD","HFREQ","SPK_FR"};
static const char *alc5633_speak_amp_mode_ctrl[]={"Class-AB","Class-D"};

static const struct soc_enum alc5633_enum[] = {
	SOC_ENUM_SINGLE(ALC5633_SPKMIXER_CTRL, 10, 4, alc5633_spo_source_sel), 	 /*0*/
	SOC_ENUM_SINGLE(ALC5633_MIC_CTRL_1, 15, 2,  alc5633_input_mode_source_sel),	/*1*/
	SOC_ENUM_SINGLE(ALC5633_MIC_CTRL_1, 7, 2,  alc5633_input_mode_source_sel),	/*2*/
	SOC_ENUM_SINGLE(ALC5633_MIC_CTRL_2, 12, 8, alc5633_mic_boost),			/*3*/
	SOC_ENUM_SINGLE(ALC5633_MIC_CTRL_2, 8, 8, alc5633_mic_boost),			/*4*/
	SOC_ENUM_SINGLE(ALC5633_SPK_OUT_VOL, 13, 4, alc5633_spor_source_sel), /*5*/
	SOC_ENUM_SINGLE(ALC5633_AUXOUT_VOL, 14, 2, alc5633_auxout_mode_source_sel), /*6*/
	SOC_ENUM_SINGLE(ALC5633_ALC_CTRL_3,14,2,alc5633_alc_ctrl),/*7*/
	SOC_ENUM_SINGLE(ALC5633_ALC_CTRL_3,15,2,alc5633_alc_path),/*8*/
	SOC_ENUM_SINGLE(ALC5633_ALC_CTRL_3,7,2,alc5633_alc_noise_ctrl),/*9*/
	SOC_ENUM_SINGLE(ALC5633_ALC_CTRL_3,6,2,alc5633_alc_noise_gate_ctrl),/*10*/
	SOC_ENUM_SINGLE_EXT(12,alc5633_music_style), /*11*/
	SOC_ENUM_SINGLE(ALC5633_SPK_AMP_CTRL,15,2,alc5633_speak_amp_mode_ctrl), /*12*/

};

static const struct snd_kcontrol_new alc5633_recmixl_mixer_controls[] = {
	SOC_DAPM_SINGLE("R_L_M HPMIX C_S", ALC5633_REC_MIXER_CTRL, 14, 1, 1),
	SOC_DAPM_SINGLE("R_L_M AUXMIX C_S", ALC5633_REC_MIXER_CTRL, 13, 1, 1),
	SOC_DAPM_SINGLE("R_L_M SPKMIX C_S", ALC5633_REC_MIXER_CTRL, 12, 1, 1),
	SOC_DAPM_SINGLE("R_L_M LINE1 C_S", ALC5633_REC_MIXER_CTRL, 11, 1, 1),
	SOC_DAPM_SINGLE("R_L_M LINE2 C_S", ALC5633_REC_MIXER_CTRL, 10, 1, 1),
	SOC_DAPM_SINGLE("R_L_M MIC1 C_S", ALC5633_REC_MIXER_CTRL, 9, 1, 1),
	SOC_DAPM_SINGLE("R_L_M MIC2 C_S", ALC5633_REC_MIXER_CTRL, 8, 1, 1),
};

static const struct snd_kcontrol_new alc5633_recmixr_mixer_controls[] = {
	SOC_DAPM_SINGLE("R_R_M HPMIX C_S", ALC5633_REC_MIXER_CTRL, 6, 1, 1),
	SOC_DAPM_SINGLE("R_R_M AUXMIX C_S", ALC5633_REC_MIXER_CTRL, 5, 1, 1),
	SOC_DAPM_SINGLE("R_R_M SPKMIX C_S", ALC5633_REC_MIXER_CTRL, 4, 1, 1),
	SOC_DAPM_SINGLE("R_R_M LINE1 C_S", ALC5633_REC_MIXER_CTRL, 3, 1, 1),
	SOC_DAPM_SINGLE("R_R_M LINE2 C_S", ALC5633_REC_MIXER_CTRL, 2, 1, 1),
	SOC_DAPM_SINGLE("R_R_M MIC1 C_S", ALC5633_REC_MIXER_CTRL, 1, 1, 1),
	SOC_DAPM_SINGLE("R_R_M MIC2 C_S", ALC5633_REC_MIXER_CTRL, 0, 1, 1),
};


static const struct snd_kcontrol_new alc5633_hp_mixl_mixer_controls[] = {
	SOC_DAPM_SINGLE("H_L_M RECMIX P_S", ALC5633_HPMIXER_CTRL, 13, 1, 1),
	SOC_DAPM_SINGLE("H_L_M MIC1 P_S", ALC5633_HPMIXER_CTRL, 12, 1, 1),
	SOC_DAPM_SINGLE("H_L_M MIC2 P_S", ALC5633_HPMIXER_CTRL, 11, 1, 1),
	SOC_DAPM_SINGLE("H_L_M LINE1 P_S", ALC5633_HPMIXER_CTRL, 10, 1, 1),
	SOC_DAPM_SINGLE("H_L_M LINE2 P_S", ALC5633_HPMIXER_CTRL, 9, 1, 1),
	SOC_DAPM_SINGLE("H_L_M DAC P_S", ALC5633_HPMIXER_CTRL, 8, 1, 1),
};

static const struct snd_kcontrol_new alc5633_hp_mixr_mixer_controls[] = {
	SOC_DAPM_SINGLE("H_R_M RECMIX P_S", ALC5633_HPMIXER_CTRL, 5, 1, 1),
	SOC_DAPM_SINGLE("H_R_M MIC1 P_S", ALC5633_HPMIXER_CTRL, 4, 1, 1),
	SOC_DAPM_SINGLE("H_R_M MIC2 P_S", ALC5633_HPMIXER_CTRL, 3, 1, 1),
	SOC_DAPM_SINGLE("H_R_M LINE1 P_S", ALC5633_HPMIXER_CTRL, 2, 1, 1),
	SOC_DAPM_SINGLE("H_R_M LINE2 P_S", ALC5633_HPMIXER_CTRL, 1, 1, 1),
	SOC_DAPM_SINGLE("H_R_M DAC P_S", ALC5633_HPMIXER_CTRL, 0, 1, 1),
};

static const struct snd_kcontrol_new alc5633_auxmixl_mixer_controls[] = {
	SOC_DAPM_SINGLE("A_L_M RECMIX P_S", ALC5633_AUXMIXER_CTRL, 13, 1, 1),
	SOC_DAPM_SINGLE("A_L_M MIC1 P_S", ALC5633_AUXMIXER_CTRL, 12, 1, 1),
	SOC_DAPM_SINGLE("A_L_M MIC2 P_S", ALC5633_AUXMIXER_CTRL, 11, 1, 1),
	SOC_DAPM_SINGLE("A_L_M LINE1 P_S", ALC5633_AUXMIXER_CTRL, 10, 1, 1),
	SOC_DAPM_SINGLE("A_L_M LINE2 P_S", ALC5633_AUXMIXER_CTRL, 9, 1, 1),
	SOC_DAPM_SINGLE("A_L_M DAC P_S", ALC5633_AUXMIXER_CTRL, 8, 1, 1),

};

static const struct snd_kcontrol_new alc5633_auxmixr_mixer_controls[] = {
	SOC_DAPM_SINGLE("A_R_M RECMIX P_S", ALC5633_AUXMIXER_CTRL, 5, 1, 1),
	SOC_DAPM_SINGLE("A_R_M MIC1 P_S", ALC5633_AUXMIXER_CTRL, 4, 1, 1),
	SOC_DAPM_SINGLE("A_R_M MIC2 P_S", ALC5633_AUXMIXER_CTRL, 3, 1, 1),
	SOC_DAPM_SINGLE("A_R_M LINE1 P_S", ALC5633_AUXMIXER_CTRL, 2, 1, 1),
	SOC_DAPM_SINGLE("A_R_M LINE2 P_S", ALC5633_AUXMIXER_CTRL, 1, 1, 1),
	SOC_DAPM_SINGLE("A_R_M DAC P_S", ALC5633_AUXMIXER_CTRL, 0, 1, 1),
};

static const struct snd_kcontrol_new alc5633_spkmixr_mixer_controls[]  = {
	SOC_DAPM_SINGLE("S_P_M MIC1 P_S", ALC5633_SPKMIXER_CTRL, 7, 1, 1),
	SOC_DAPM_SINGLE("S_P_M MIC2 P_S", ALC5633_SPKMIXER_CTRL, 6, 1, 1),
	SOC_DAPM_SINGLE("S_P_M LINE1L P_S", ALC5633_SPKMIXER_CTRL, 5, 1, 1),
	SOC_DAPM_SINGLE("S_P_M LINE1R P_S", ALC5633_SPKMIXER_CTRL, 4, 1, 1),
	SOC_DAPM_SINGLE("S_P_M LINE2L P_S", ALC5633_SPKMIXER_CTRL, 3, 1, 1),
	SOC_DAPM_SINGLE("S_P_M LINE2R P_S", ALC5633_SPKMIXER_CTRL, 2, 1, 1),
	SOC_DAPM_SINGLE("S_P_M DACL P_S", ALC5633_SPKMIXER_CTRL, 1, 1, 1),
	SOC_DAPM_SINGLE("S_P_M DACR P_S", ALC5633_SPKMIXER_CTRL, 0, 1, 1),
};



static const struct snd_kcontrol_new alc5633_spo_mux_control =
SOC_DAPM_ENUM("Route", alc5633_enum[0]);

//*******************************************************************************************************************
//EQ parameter
typedef enum
{
	NORMAL=0,
	CLUB,
	DANCE,
	LIVE,
	POP,
	ROCK,
	OPPO,
	TREBLE,
	BASS,
	RECORD,
	HFREQ,
	SPK_FR,
}HwEqType_t;

typedef struct  _HW_EQ_PRESET
{
	HwEqType_t	HwEqType;
	u16 	EqValue[22];
	u16  HwEQCtrl;

}HW_EQ_PRESET;

static HW_EQ_PRESET HwEq_Preset[]={
/*       0x0    0x1    0x2     0x3   0x4    0x5	   0x6	  0x7	0x8     0x9    0xa    0xb    0xc    0x0d   0x0e    0x0f	   0x10  0x11  0x12   0x13    0x14  0x15    0x70*/
{NORMAL,{0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0x0000,0x0000,0x0000},0x0000},
{CLUB  ,{0x1C10,0x0000,0xC1CC,0x1E5D,0x0699,0xCD48,0x188D,0x0699,0xC3B6,0x1CD0,0x0699,0x0436,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0x0000,0x0000,0x0000},0x000E},
{DANCE ,{0x1F2C,0x095B,0xC071,0x1F95,0x0616,0xC96E,0x1B11,0xFC91,0xDCF2,0x1194,0xFAF2,0x0436,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0x0000,0x0000,0x0000},0x000F},
{LIVE  ,{0x1EB5,0xFCB6,0xC24A,0x1DF8,0x0E7C,0xC883,0x1C10,0x0699,0xDA41,0x1561,0x0295,0x0436,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0x0000,0x0000,0x0000},0x000F},
{POP   ,{0x1EB5,0xFCB6,0xC1D4,0x1E5D,0x0E23,0xD92E,0x16E6,0xFCB6,0x0000,0x0969,0xF988,0x0436,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0x0000,0x0000,0x0000},0x000F},
{ROCK  ,{0x1EB5,0xFCB6,0xC071,0x1F95,0x0424,0xC30A,0x1D27,0xF900,0x0C5D,0x0FC7,0x0E23,0x0436,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0x0000,0x0000,0x0000},0x000F},
{OPPO  ,{0x0000,0x0000,0xCA4A,0x17F8,0x0FEC,0xCA4A,0x17F8,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0x0000,0x0000,0x0000},0x000F},
{TREBLE,{0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x188D,0x1699,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0x0000,0x0000,0x0000},0x0010},
{BASS  ,{0x1A43,0x0C00,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0x0000,0x0000,0x0000},0x0001},
{RECORD,{0x1E3C,0xF405,0xC1E0,0x1E39,0x2298,0xDF29,0x0701,0x1D18,0xF34B,0x0CA9,0xF805,0xF9CC,0xF405,0x0000,0x0000,0x0000,0x0000,0x0000,0x0003,0x0000,0x0000,0x0000},0x001F},
{HFREQ, {0x1C10,0x0000,0xC883,0x1C10,0x0000,0xD588,0x1C10,0x0000,0xE904,0x1C10,0x0000,0xFBCA,0x0699,0x05E9,0x1A97,0x1D2B,0x0000,0x0003,0x0007,0x0388,0x15FF,0xFE43},0x0050},
{SPK_FR,{0x1CD4,0xF405,0xe0bf,0x0f9e,0xfa19,0x07CA,0x12AF,0xF805,0xE904,0x1C10,0x0000,0x1C8B,0x0000,0xc5e1,0x1afb,0x1d46,0x0000,0x0000,0x0003,0x0000,0x0000,0x0000},0x0090},

};
//*******************************************************************************************************************
int music_style=0;
//*******************************************************************************************************************

static int alc5633_write_index(struct snd_soc_codec *codec, unsigned int index,unsigned int mask,unsigned int value)
{
    unsigned char RetVal = 0;

    RetVal = snd_soc_write(codec,ALC5633_PRI_REG_ADD,index);
    PRINTK("WRITE INDEX RETVAL=%x\r\n",RetVal);
    if(RetVal != 0)
      return RetVal;

    RetVal = snd_soc_update_bits(codec,ALC5633_PRI_REG_DATA,mask,value);
    return RetVal;
}

static int alc5633_read_index(struct snd_soc_codec *codec, unsigned int reg)
{
    unsigned char RetVal = 0;

    snd_soc_write(codec,ALC5633_PRI_REG_ADD,reg);
    RetVal = snd_soc_read(codec,ALC5633_PRI_REG_DATA);
	return RetVal;
}

static int updata_alc(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u16 temp;
	temp = snd_soc_read(codec,ALC5633_ALC_CTRL_3);
	temp |= 0x2000;
	snd_soc_write(codec, ALC5633_ALC_CTRL_3,temp);
	return 0;
}

static int get_pri_reg(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int invert = mc->invert;

	ucontrol->value.integer.value[0] =
		(alc5633_read_index(codec, reg) >> shift) & mask;
	if (shift != rshift)
		ucontrol->value.integer.value[1] =
			(alc5633_read_index(codec, reg) >> rshift) & mask;
	if (invert) {
		ucontrol->value.integer.value[0] =
			max - ucontrol->value.integer.value[0];
		if (shift != rshift)
			ucontrol->value.integer.value[1] =
				max - ucontrol->value.integer.value[1];
	}

	return 0;
}

static int set_pri_reg(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int invert = mc->invert;
	unsigned int val, val2, val_mask;

	val = (ucontrol->value.integer.value[0] & mask);
	if (invert)
		val = max - val;
	val_mask = mask << shift;
	val = val << shift;
	if (shift != rshift) {
		val2 = (ucontrol->value.integer.value[1] & mask);
		if (invert)
			val2 = max - val2;
		val_mask |= mask << rshift;
		val |= val2 << rshift;
	}
	return alc5633_write_index(codec,reg,val_mask,val);

}

static int get_music_style(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = music_style;
	return 0;
}

static int set_music_style(struct snd_kcontrol *kcontrol,struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u16 HwEqIndex=0;
	music_style = ucontrol->value.integer.value[0];


	if(music_style==RECORD)
	{
		snd_soc_update_bits(codec, 0x3a,0x1800,0x1800); //enable ADC power
	}
	else
	{
		snd_soc_update_bits(codec, 0x3a,0x0700,0x0700);//enable DAC power
	}

	if(music_style==NORMAL)
	{
		/*clear EQ parameter*/
		for(HwEqIndex=0;HwEqIndex<=0x15;HwEqIndex++)
		{
			alc5633_write_index(codec, HwEqIndex, 0xffff,HwEq_Preset[music_style].EqValue[HwEqIndex]);
		}

		snd_soc_write(codec, 0x70,0x0);		/*disable EQ block*/
	}
	else
	{
		snd_soc_write(codec, 0x70,HwEq_Preset[music_style].HwEQCtrl);

		/*Fill EQ parameter*/
		for(HwEqIndex=0;HwEqIndex<=0x15;HwEqIndex++)
		{
			alc5633_write_index(codec, HwEqIndex,0xffff, HwEq_Preset[music_style].EqValue[HwEqIndex]);
		}

		if(music_style==RECORD)
		{	//for ADC update EQ
			snd_soc_update_bits(codec, 0x6e,0xa000,0xa000);
		}
		else
		{	//for DAC update EQ
			snd_soc_update_bits(codec, 0x6e,0xa000,0x2000);
		}
	}
	return 0;
}
//*******************************************************************************************************************

static int spk_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	unsigned int l, r;
	PRINTK("==IN SPK EVENT\r\n");
	l = (snd_soc_read(codec, ALC5633_PWR_MANAG_ADD4) & (0x01 << 15)) >> 15;
	r = (snd_soc_read(codec, ALC5633_PWR_MANAG_ADD4) & (0x01 << 14)) >> 14;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMD:
		if (l && r)
		{
			snd_soc_update_bits(codec, ALC5633_SPK_OUT_VOL, 0xE000, 0xE000);
			snd_soc_update_bits(codec, ALC5633_PWR_MANAG_ADD1, 0x2020,0x0000);
		}

		break;
	case SND_SOC_DAPM_POST_PMU:
		if (l && r)
		{
			snd_soc_update_bits(codec, ALC5633_PWR_MANAG_ADD1, 0x2020, 0x2020);
			snd_soc_update_bits(codec, ALC5633_SPK_OUT_VOL, 0xE000,0x0000);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


static int open_hp_end_widgets(struct snd_soc_codec *codec)
{

	hp_mute_unmute_depop(codec,0);

	return 0;
}

static int close_hp_end_widgets(struct snd_soc_codec *codec)
{
	hp_mute_unmute_depop(codec,1);

	//if write 0x00 to ALC5633_PWR_MANAG_ADD3,will cause a big switch noise
	//snd_soc_update_bits(codec, ALC5633_PWR_MANAG_ADD3,0x000f,0x0000);

	return 0;
}

static int hp_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	unsigned int l, r;
	static unsigned int hp_out_enable=0;
	PRINTK("===IN HP EVENT \r\n");
	l = (snd_soc_read(codec, ALC5633_PWR_MANAG_ADD4) & (0x01 << 11)) >> 11;
	r = (snd_soc_read(codec, ALC5633_PWR_MANAG_ADD4) & (0x01 << 10)) >> 10;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMD:

		if ((l && r)&&(hp_out_enable))
		{
			close_hp_end_widgets(codec);
			hp_out_enable=0;
		}

	break;

	case SND_SOC_DAPM_POST_PMU:

		if ((l && r)&&(!hp_out_enable))
		{
		 	hp_depop_mode2(codec);
			open_hp_end_widgets(codec);
			hp_out_enable=1;
		}

	break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int auxout_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	unsigned int l, r;
	static unsigned int aux_out_enable=0;
	PRINTK("===IN AUXOUT EVENT\r\n");
	l = (snd_soc_read(codec, ALC5633_PWR_MANAG_ADD4) & (0x01 << 9)) >> 9;
	r = (snd_soc_read(codec, ALC5633_PWR_MANAG_ADD4) & (0x01 << 8)) >> 8;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMD:

		if ((l && r)&&(aux_out_enable))
		{
			snd_soc_update_bits(codec, ALC5633_AUXOUT_VOL, 0x8080, 0x8080);
			aux_out_enable=0;
		}

	break;

	case SND_SOC_DAPM_POST_PMU:

		if ((l && r)&&(!aux_out_enable))
		{
			snd_soc_update_bits(codec, ALC5633_AUXOUT_VOL, 0x8080, 0x0000);
			aux_out_enable=1;
		}

	break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int dac_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	static unsigned int dac_enable=0;
	PRINTK("====IN DAC EVENT \r\n");
	switch (event) {

	case SND_SOC_DAPM_PRE_PMD:

		PRINTK("dac_event --SND_SOC_DAPM_PRE_PMD\n");
		if (dac_enable)
		{
			dac_enable=0;
		}
		break;

	case SND_SOC_DAPM_POST_PMU:

		PRINTK("dac_event --SND_SOC_DAPM_POST_PMU\n");
		if(!dac_enable)
		{
			dac_enable=1;
		}
		break;
	default:
		return 0;
	}

	return 0;
}


static int adc_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	static unsigned int adc_enable=0;
	PRINTK("==== IN adc event\r\n");
	switch (event) {
	case SND_SOC_DAPM_PRE_PMD:
		PRINTK("adc_event --SND_SOC_DAPM_PRE_PMD\n");
		if (adc_enable)
		{
			adc_enable=0;
		}
		break;
	case SND_SOC_DAPM_POST_PMU:
		PRINTK("adc_event --SND_SOC_DAPM_POST_PMU\n");
		if(!adc_enable)
		{
			adc_enable=1;
		}
		break;
	default:
		return 0;
	}

	return 0;
}

static const struct snd_kcontrol_new alc5633_snd_controls[] = {
	SOC_ENUM("MIC1 Mode Control",  alc5633_enum[1]),
	SOC_ENUM("MIC1 Boost", alc5633_enum[3]),

	SOC_ENUM("MIC2 Mode Control", alc5633_enum[2]),
	SOC_ENUM("MIC2 Boost", alc5633_enum[4]),
	SOC_ENUM("SPKR Out Control", alc5633_enum[5]),
	SOC_ENUM("AUXOUT Control", alc5633_enum[6]),

	SOC_DOUBLE("Line1 Capture Volume", ALC5633_LINE_IN_1_VOL, 8, 0, 31, 1),
	SOC_DOUBLE("Line2 Capture Volume", ALC5633_LINE_IN_2_VOL, 8, 0, 31, 1),

	SOC_SINGLE("MIC1 Playback Volume", ALC5633_MIC_CTRL_1, 8, 31, 1),
	SOC_SINGLE("MIC2 Playback Volume", ALC5633_MIC_CTRL_1, 0, 31, 1),

	SOC_SINGLE("AXOL Playback Switch", ALC5633_AUXOUT_VOL, 15, 1, 1),
	SOC_SINGLE("AXOR Playback Switch", ALC5633_AUXOUT_VOL, 7, 1, 1),
	SOC_DOUBLE("AUX Playback Volume", ALC5633_AUXOUT_VOL, 8, 0, 31, 1),
	SOC_SINGLE("SPK Playback Switch", ALC5633_SPK_OUT_VOL, 15, 1, 1),
	SOC_DOUBLE("SPK Playback Volume", ALC5633_SPK_OUT_VOL, 5, 0, 31, 1),
	SOC_SINGLE("HPL Playback Switch", ALC5633_HP_OUT_VOL, 15, 1, 1),
	SOC_SINGLE("HPR Playback Switch", ALC5633_HP_OUT_VOL, 7, 1, 1),
	SOC_DOUBLE("HP Playback Volume", ALC5633_HP_OUT_VOL, 8, 0, 31, 1),

	SOC_DOUBLE("DAC Playback Volume", ALC5633_DAC_DIG_VOL,8,0,256,1),

	//ALC CONTROLS
	SOC_ENUM("ALC Enable",alc5633_enum[7]),
	SOC_ENUM("ALC Path Switch",alc5633_enum[8]),
	SOC_SINGLE("ALC Attack Rate",ALC5633_ALC_CTRL_1,8,17,0),
	SOC_SINGLE("ALC Recovery Rate",ALC5633_ALC_CTRL_1,0,17,0),
	SOC_SINGLE_TLV("ALC Noise Level",ALC5633_ALC_CTRL_2,0,16,0,noise_tlv),
	SOC_SINGLE_TLV("ALC Signel Limiter Level",ALC5633_ALC_CTRL_3,8,32,1,signel_lim_tlv),
	SOC_ENUM("ALC Noise Func Enable",alc5633_enum[9]),
	SOC_ENUM("ALC Noise Gate Hold Data Enable",alc5633_enum[10]),
	SOC_SINGLE_TLV("ALC Noise Gate Threshold",ALC5633_ALC_CTRL_3,0,32,1,noise_gate_tlv),
	SOC_SINGLE_EXT("ALC Update Parameter",ALC5633_ALC_CTRL_3,13,1,0,updata_alc,updata_alc),

	//music style
	SOC_ENUM_EXT("Music Style",alc5633_enum[11],get_music_style,set_music_style),

	//DAC pre gain
	SOC_SINGLE_TLV("DAC Pre Gain",ALC5633_DAC_CTRL,0,75,0,dac_preg_tlv),
	SOC_SINGLE_EXT_TLV("DAC Post Gain",ALC5633_PRI_ALC_CTRL_4,4,19,0,
						get_pri_reg,set_pri_reg,dac_postg_tlv),
	SOC_ENUM("Speaker Amp Mode",alc5633_enum[12]),
};

static const struct snd_soc_dapm_widget alc5633_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MIC1"),
	SND_SOC_DAPM_INPUT("MIC2"),
	SND_SOC_DAPM_INPUT("LINE1L"),
	SND_SOC_DAPM_INPUT("LINE2L"),
	SND_SOC_DAPM_INPUT("LINE1R"),
	SND_SOC_DAPM_INPUT("LINE2R"),

	SND_SOC_DAPM_PGA("Mic1 Boost", ALC5633_PWR_MANAG_ADD2, 5, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Mic2 Boost", ALC5633_PWR_MANAG_ADD2, 4, 0, NULL, 0),

	SND_SOC_DAPM_PGA("Line1 Left Mixer", ALC5633_PWR_MANAG_ADD2, 9, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Line1 Right Mixer", ALC5633_PWR_MANAG_ADD2, 8, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Line2 Left Mixer", ALC5633_PWR_MANAG_ADD2, 7, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Line2 Right Mixer", ALC5633_PWR_MANAG_ADD2, 6, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("Record Left Mixer", ALC5633_PWR_MANAG_ADD2, 11, 0,
		&alc5633_recmixl_mixer_controls[0], ARRAY_SIZE(alc5633_recmixl_mixer_controls)),
	SND_SOC_DAPM_MIXER("Record Right Mixer", ALC5633_PWR_MANAG_ADD2, 10, 0,
		&alc5633_recmixr_mixer_controls[0], ARRAY_SIZE(alc5633_recmixr_mixer_controls)),


	SND_SOC_DAPM_ADC_E("Left ADC","Left ADC HIFI Capture", ALC5633_PWR_MANAG_ADD1,12, 0,
			adc_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_ADC_E("Right ADC","Right ADC HIFI Capture", ALC5633_PWR_MANAG_ADD1,11, 0,
			adc_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_DAC_E("Left DAC", "Left DAC HIFI Playback", ALC5633_PWR_MANAG_ADD1, 10, 0,
			dac_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_DAC_E("Right DAC", "Right DAC HIFI Playback", ALC5633_PWR_MANAG_ADD1, 9, 0,
			dac_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),



	SND_SOC_DAPM_MIXER("HP Left Mixer", ALC5633_PWR_MANAG_ADD2, 15, 0,
		&alc5633_hp_mixl_mixer_controls[0], ARRAY_SIZE(alc5633_hp_mixl_mixer_controls)),
	SND_SOC_DAPM_MIXER("HP Right Mixer", ALC5633_PWR_MANAG_ADD2, 14, 0,
		&alc5633_hp_mixr_mixer_controls[0], ARRAY_SIZE(alc5633_hp_mixr_mixer_controls)),
	SND_SOC_DAPM_MIXER("AUX Left Mixer", ALC5633_PWR_MANAG_ADD2, 13, 0,
		&alc5633_auxmixl_mixer_controls[0], ARRAY_SIZE(alc5633_auxmixl_mixer_controls)),
	SND_SOC_DAPM_MIXER("AUX Right Mixer", ALC5633_PWR_MANAG_ADD2, 12, 0,
		&alc5633_auxmixr_mixer_controls[0], ARRAY_SIZE(alc5633_auxmixr_mixer_controls)),
	SND_SOC_DAPM_MIXER("Speaker Mixer", ALC5633_PWR_MANAG_ADD2, 0, 0,
		&alc5633_spkmixr_mixer_controls[0], ARRAY_SIZE(alc5633_spkmixr_mixer_controls)),

	SND_SOC_DAPM_PGA_E("Left Speaker Event", ALC5633_PWR_MANAG_ADD4, 15, 0, NULL, 0,
			spk_event, SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_PGA_E("Right Speaker Event", ALC5633_PWR_MANAG_ADD4, 14, 0, NULL, 0,
			spk_event, SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_PGA_E("Left HP Event", ALC5633_PWR_MANAG_ADD4, 11, 0, NULL, 0,
			hp_event, SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_PGA_E("Right HP Event", ALC5633_PWR_MANAG_ADD4, 10, 0, NULL, 0,
			hp_event, SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_PGA_E("Left AUX Out Event", ALC5633_PWR_MANAG_ADD4, 9, 0, NULL, 0,
			auxout_event, SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_PGA_E("Right AUX Out Event", ALC5633_PWR_MANAG_ADD4, 8, 0, NULL, 0,
			auxout_event, SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),

	SND_SOC_DAPM_MUX("Speaker Output Mux", SND_SOC_NOPM, 0, 0, &alc5633_spo_mux_control),

	SND_SOC_DAPM_MICBIAS("Mic Bias Switch", ALC5633_PWR_MANAG_ADD2, 3, 0),
//	SND_SOC_DAPM_MICBIAS("Mic Bias2", ALC5633_PWR_MANAG_ADD2, 2, 0),

	SND_SOC_DAPM_OUTPUT("AUXOUTL"),
	SND_SOC_DAPM_OUTPUT("AUXOUTR"),
	SND_SOC_DAPM_OUTPUT("SPOL"),
	SND_SOC_DAPM_OUTPUT("SPOR"),
	SND_SOC_DAPM_OUTPUT("HPOL"),
	SND_SOC_DAPM_OUTPUT("HPOR"),

};

static const struct snd_soc_dapm_route audio_map[] = {

	{"Mic1 Boost", NULL, "MIC1"},
	{"Mic2 Boost", NULL, "MIC2"},


	{"Line1 Left Mixer", NULL, "LINE1L"},
	{"Line1 Right Mixer", NULL, "LINE1R"},

	{"Line2 Left Mixer", NULL, "LINE2L"},
	{"Line2 Right Mixer", NULL, "LINE2R"},


	{"Record Left Mixer", "R_L_M HPMIX C_S", "HP Left Mixer"},
	{"Record Left Mixer", "R_L_M AUXMIX C_S", "AUX Left Mixer"},
	{"Record Left Mixer", "R_L_M SPKMIX C_S", "Speaker Mixer"},
	{"Record Left Mixer", "R_L_M LINE1 C_S", "Line1 Left Mixer"},
	{"Record Left Mixer", "R_L_M LINE2 C_S", "Line2 Left Mixer"},
	{"Record Left Mixer", "R_L_M MIC1 C_S", "Mic1 Boost"},
	{"Record Left Mixer", "R_L_M MIC2 C_S", "Mic2 Boost"},

	{"Record Right Mixer", "R_R_M HPMIX C_S", "HP Right Mixer"},
	{"Record Right Mixer", "R_R_M AUXMIX C_S", "AUX Right Mixer"},
	{"Record Right Mixer", "R_R_M SPKMIX C_S", "Speaker Mixer"},
	{"Record Right Mixer", "R_R_M LINE1 C_S", "Line1 Right Mixer"},
	{"Record Right Mixer", "R_R_M LINE2 C_S", "Line2 Right Mixer"},
	{"Record Right Mixer", "R_R_M MIC1 C_S", "Mic1 Boost"},
	{"Record Right Mixer", "R_R_M MIC2 C_S", "Mic2 Boost"},

	{"Left ADC", NULL, "Record Left Mixer"},
	{"Right ADC", NULL, "Record Right Mixer"},

	{"HP Left Mixer", "H_L_M RECMIX P_S", "Record Left Mixer"},
	{"HP Left Mixer", "H_L_M MIC1 P_S", "Mic1 Boost"},
	{"HP Left Mixer", "H_L_M MIC2 P_S", "Mic2 Boost"},
	{"HP Left Mixer", "H_L_M LINE1 P_S", "Line1 Left Mixer"},
	{"HP Left Mixer", "H_L_M LINE2 P_S", "Line2 Left Mixer"},
	{"HP Left Mixer", "H_L_M DAC P_S", "Left DAC"},

	{"HP Right Mixer", "H_R_M RECMIX P_S", "Record Right Mixer"},
	{"HP Right Mixer", "H_R_M MIC1 P_S", "Mic1 Boost"},
	{"HP Right Mixer", "H_R_M MIC2 P_S", "Mic2 Boost"},
	{"HP Right Mixer", "H_R_M LINE1 P_S", "Line1 Right Mixer"},
	{"HP Right Mixer", "H_R_M LINE2 P_S", "Line2 Right Mixer"},
	{"HP Right Mixer", "H_R_M DAC P_S", "Right DAC"},

	{"AUX Left Mixer", "A_L_M RECMIX P_S", "Record Left Mixer"},
	{"AUX Left Mixer", "A_L_M MIC1 P_S", "Mic1 Boost"},
	{"AUX Left Mixer", "A_L_M MIC2 P_S", "Mic2 Boost"},
	{"AUX Left Mixer", "A_L_M LINE1 P_S", "Line1 Left Mixer"},
	{"AUX Left Mixer", "A_L_M LINE2 P_S", "Line2 Left Mixer"},
	{"AUX Left Mixer", "A_L_M DAC P_S", "Left DAC"},

	{"AUX Right Mixer", "A_R_M RECMIX P_S", "Record Right Mixer"},
	{"AUX Right Mixer", "A_R_M MIC1 P_S", "Mic1 Boost"},
	{"AUX Right Mixer", "A_R_M MIC2 P_S", "Mic2 Boost"},
	{"AUX Right Mixer", "A_R_M LINE1 P_S", "Line1 Right Mixer"},
	{"AUX Right Mixer", "A_R_M LINE2 P_S", "Line2 Right Mixer"},
	{"AUX Right Mixer", "A_R_M DAC P_S", "Right DAC"},

	{"Speaker Mixer", "S_P_M MIC1 P_S", "Mic1 Boost"},
	{"Speaker Mixer", "S_P_M MIC2 P_S", "Mic2 Boost"},
	{"Speaker Mixer", "S_P_M DACL P_S", "Left DAC"},
	{"Speaker Mixer", "S_P_M DACR P_S", "Right DAC"},
	{"Speaker Mixer", "S_P_M LINE1L P_S", "Line1 Left Mixer"},
	{"Speaker Mixer", "S_P_M LINE1R P_S", "Line1 Right Mixer"},
	{"Speaker Mixer", "S_P_M LINE2L P_S", "Line2 Left Mixer"},
	{"Speaker Mixer", "S_P_M LINE2R P_S", "Line2 Right Mixer"},

	{"Speaker Output Mux", "HPMIX", "HP Left Mixer"},
	{"Speaker Output Mux", "SPKMIX", "Speaker Mixer"},
	{"Speaker Output Mux", "AUXMIX", "AUX Left Mixer"},

	{"Left Speaker Event",  NULL, "Speaker Output Mux"},
	{"Right Speaker Event",  NULL, "Speaker Output Mux"},

	{"Right HP Event",  NULL, "HP Right Mixer"},
	{"Left HP Event",  NULL, "HP Left Mixer"},

	{"Left AUX Out Event",  NULL, "AUX Left Mixer"},
	{"Right AUX Out Event",  NULL, "AUX Right Mixer"},

	{"AUXOUTL", NULL, "Left AUX Out Event"},
	{"AUXOUTR", NULL, "Right AUX Out Event"},
	{"SPOL", NULL, "Left Speaker Event"},
	{"SPOR", NULL, "Right Speaker Event"},
	{"HPOL", NULL, "Left HP Event"},
	{"HPOR", NULL, "Right HP Event"},


};



static int alc5633_add_widgets(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	snd_soc_dapm_new_controls(dapm,alc5633_dapm_widgets,ARRAY_SIZE(alc5633_dapm_widgets));
	snd_soc_dapm_add_routes(dapm,audio_map,ARRAY_SIZE(audio_map));
	return 0;
}




struct _coeff_div{
	unsigned int mclk;       //pllout or MCLK
	unsigned int bclk;       //master mode
	unsigned int rate;
	unsigned int reg_val;
};
/*PLL divisors*/
struct _pll_div {
	u32 pll_in;
	u32 pll_out;
	u16 regvalue;
};
//set MCLK as the source of PLL
//set the pll out is  the same as pll in
//so ,use the coeff_div to make sure the I2S_SYSCLK=256*fs
static const struct _pll_div codec_master_pll_div[] = {
	{  4096000,   4096000,  0x6a0 },
	{  5644800,   5644800,  0x6a0 },
	{  4096000,   4096000,  0x6a0 },
	{  5644800,   5644800,  0x6a0 },
	{  6144000,   6144000,  0x6a0 },
	{  8192000,   8192000,  0x6a0 },
	{ 11289600,  11289600,  0x6a0 },
	{ 12288000,  12288000,  0x6a0 },

};

static const struct _pll_div codec_slave_pll_div[] = {

	{  4096000,   4096000,  0x6a0 },
	{  5644800,   5644800,  0x6a0 },
	{  4096000,   4096000,  0x6a0 },
	{  5644800,   5644800,  0x6a0 },
	{  6144000,   6144000,  0x6a0 },
	{  8192000,   8192000,  0x6a0 },
	{ 11289600,  11289600,  0x6a0 },
	{ 12288000,  12288000,  0x6a0 },

};

struct _coeff_div coeff_div[] = {

	//sysclk is 256fs
	{ 2048000,  8000 * 32,  8000, 0x1a00},
	{ 2822400, 11025 * 32, 11025, 0x1a00},
	{ 4096000, 16000 * 32, 16000, 0x1a00},
	{ 5644800, 22050 * 32, 22050, 0x1a00},
	{ 6144000, 24000 * 32, 24000, 0x1a00},
	{ 8192000, 32000 * 32, 32000, 0x1a00},
	{11289600, 44100 * 32, 44100, 0x1a00},
	{12288000, 48000 * 32, 48000, 0x1a00},
	//sysclk is 512fs
	{ 4096000,  8000 * 32,  8000, 0x3a00},
	{ 5644800, 11025 * 32, 11025, 0x3a00},
	{ 8192000, 16000 * 32, 16000, 0x3a00},
	{11289600, 22050 * 32, 22050, 0x3a00},
	{16384000, 32000 * 32, 32000, 0x3a00},
	{22579200, 44100 * 32, 44100, 0x3a00},
	{24576000, 48000 * 32, 48000, 0x3a00},
};



static int get_coeff(int mclk, int rate, int timesofbclk)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(coeff_div); i++) {
		if ((coeff_div[i].mclk == mclk)
				&& (coeff_div[i].rate == rate)
				&& ((coeff_div[i].bclk / coeff_div[i].rate) == timesofbclk))
				return i;
	}

		return -1;
}

static int get_coeff_in_slave_mode(int mclk, int rate)
{
	return get_coeff(mclk, rate, BLCK_FREQ);
}

static int get_coeff_in_master_mode(int mclk, int rate)
{
	return get_coeff(mclk, rate ,BLCK_FREQ);
}

static int alc5633_hifi_codec_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct alc5633_priv *alc5633 = snd_soc_codec_get_drvdata(codec);

	PRINTK( "enter %s\n", __func__);
	if ((freq >= (256 * 8000)) && (freq <= (512 * 48000))) {
		alc5633->sysclk = freq;
		return 0;
	}

	printk("unsupported sysclk freq %u for audio i2s\n", freq);
	return 0;
}

static int alc5633_codec_set_dai_pll(struct snd_soc_dai *codec_dai,
		int pll_id, int source, unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct alc5633_priv *alc5633 = snd_soc_codec_get_drvdata(codec);
	int i;
	int ret = -EINVAL;

	PRINTK("enter %s\n", __func__);
	snd_soc_update_bits(codec, ALC5633_PWR_MANAG_ADD2,PWR_PLL,0);
	PRINTK("freq_in=%d frep_out=%d\r\n",freq_in,freq_out);
	if (!freq_in || !freq_out)
		return 0;

	PRINTK("alc5633->master %d\r\n",alc5633->master);
	if (alc5633->master) {
		for (i = 0; i < ARRAY_SIZE(codec_master_pll_div); i ++) {
			PRINTK("i=%d\r\n");
			PRINTK("pll_in=%d\r\n",codec_master_pll_div[i].pll_in);
			PRINTK("pll_out=%d\r\n",codec_master_pll_div[i].pll_out);
			if ((freq_in == codec_master_pll_div[i].pll_in) && (freq_out == codec_master_pll_div[i].pll_out)) {
				PRINTK("FIND A VALUE \r\n");
				snd_soc_write(codec, ALC5633_PLL_CTRL, codec_master_pll_div[i].regvalue);
				snd_soc_update_bits(codec, ALC5633_PWR_MANAG_ADD2, PWR_PLL, PWR_PLL);
				schedule_timeout_uninterruptible(msecs_to_jiffies(20));
				snd_soc_write(codec, ALC5633_GBL_CLK_CTRL, 0x4800);
				ret = 0;
				break;
			}
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(codec_slave_pll_div); i ++) {
			PRINTK("i=%d\r\n");
			PRINTK("pll_in=%d\r\n",codec_slave_pll_div[i].pll_in);
			PRINTK("pll_out=%d\r\n",codec_slave_pll_div[i].pll_out);

			if ((freq_in == codec_slave_pll_div[i].pll_in) && (freq_out == codec_slave_pll_div[i].pll_out))  {
				snd_soc_write(codec, ALC5633_PLL_CTRL, codec_slave_pll_div[i].regvalue);
				snd_soc_update_bits(codec, ALC5633_PWR_MANAG_ADD2, PWR_PLL, PWR_PLL);
				schedule_timeout_uninterruptible(msecs_to_jiffies(20));
				snd_soc_write(codec, ALC5633_GBL_CLK_CTRL, 0x4000);
				ret = 0;
				break;
			}
		}
	}

	return ret;
}


static int alc5633_hifi_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params,
			struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec =rtd->codec;
	struct alc5633_priv *alc5633 = snd_soc_codec_get_drvdata(codec);
	unsigned int iface = 0;
	int rate = params_rate(params);
	int coeff = 0;

	PRINTK("enter %s\n", __func__);
	if (!alc5633->master) {
	 	coeff = get_coeff_in_slave_mode(alc5633->sysclk, rate);
		PRINTK("In slave mode\r\n ");
	}else{
		coeff = get_coeff_in_master_mode(alc5633->sysclk, rate);
		PRINTK("In master mode\r\n  ");
	}
	if (coeff < 0) {
		printk( "%s get_coeff err!\n", __func__);
		return -EINVAL;
	}
	switch (params_format(params))
	{
		case SNDRV_PCM_FORMAT_S16_LE:
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			iface |= 0x0004;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			iface |= 0x0008;
			break;
		case SNDRV_PCM_FORMAT_S8:
			iface |= 0x000c;
			break;
		default:
			return -EINVAL;
	}

	snd_soc_update_bits(codec, ALC5633_SDP_CTRL, SDP_I2S_DL_MASK,iface );
	snd_soc_write(codec, ALC5633_STEREO_AD_DA_CLK_CTRL, coeff_div[coeff].reg_val);
	snd_soc_update_bits(codec, ALC5633_PWR_MANAG_ADD1, 0x81C0, 0x81C0);

	return 0;
}

static int alc5633_hifi_codec_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct alc5633_priv *alc5633 = snd_soc_codec_get_drvdata(codec);
	u16 iface = 0;

	PRINTK( "enter %s\n", __func__);
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		alc5633->master = 1;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		iface |= (0x0001 << 15);
		alc5633->master = 0;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iface |= (0x0001);
		break;
	case SND_SOC_DAIFMT_DSP_A:
		iface |= (0x0002);
		break;
	case SND_SOC_DAIFMT_DSP_B:
		iface  |= (0x0003);
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |= (0x0001 << 7);
		break;
	default:
		return -EINVAL;
	}

	snd_soc_write(codec, ALC5633_SDP_CTRL, iface);
	return 0;
}
#define ALC5633_STEREO_RATES	SNDRV_PCM_RATE_8000_48000
#define ALC5633_FORMAT	SNDRV_PCM_FMTBIT_S16_LE

struct snd_soc_dai_ops alc5633_ops = {
	.hw_params = alc5633_hifi_hw_params,
	.set_fmt = alc5633_hifi_codec_set_dai_fmt,
	.set_sysclk = alc5633_hifi_codec_set_dai_sysclk,
	.set_pll = alc5633_codec_set_dai_pll,
};

struct snd_soc_dai_driver alc5633_dai = {
	.name = "alc5633",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = ALC5633_STEREO_RATES,
		.formats = ALC5633_FORMAT,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = ALC5633_STEREO_RATES,
		.formats = ALC5633_FORMAT,
	},
	.ops =&alc5633_ops,
};



static int alc5633_set_bias_level(struct snd_soc_codec *codec, enum snd_soc_bias_level level)
{
	PRINTK( "enter %s\n", __func__);

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		snd_soc_update_bits(codec, ALC5633_PWR_MANAG_ADD3,PWR_VREF|PWR_MAIN_BIAS, PWR_VREF|PWR_MAIN_BIAS);
		snd_soc_update_bits(codec, ALC5633_PWR_MANAG_ADD2,0x0008, 0x0008);
		break;
	case SND_SOC_BIAS_STANDBY:
		break;
	case SND_SOC_BIAS_OFF:
		snd_soc_update_bits(codec, ALC5633_SPK_OUT_VOL, 0x8080, 0x8080);	//mute speaker volume
		snd_soc_update_bits(codec, ALC5633_HP_OUT_VOL, 0x8080, 0x8080);	//mute hp volume
		snd_soc_write(codec, ALC5633_PWR_MANAG_ADD1, 0x0000);
		snd_soc_write(codec, ALC5633_PWR_MANAG_ADD2, 0x0000);
		snd_soc_write(codec, ALC5633_PWR_MANAG_ADD3, 0x0000);
		snd_soc_write(codec, ALC5633_PWR_MANAG_ADD4, 0x0000);
		break;
	}

	codec->dapm.bias_level = level;
	return 0;
}

static int alc5633_suspend(struct snd_soc_codec *codec)
{
	PRINTK("IN SUSPEND\n");
	alc5633_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int alc5633_resume(struct snd_soc_codec *codec)
{
	PRINTK("in resume\n");
	alc5633_sync(codec);
	alc5633_set_bias_level(codec, SND_SOC_BIAS_ON);
	return 0;
}

static int alc5633_probe(struct snd_soc_codec *codec)
{
	struct alc5633_priv *alc5633 = snd_soc_codec_get_drvdata(codec);
	//struct alc5633_platform_data *alc5633_pdata;
	int ret;
	PRINTK("IN THE ALC5633_PROBE\n");

	codec->control_data = alc5633->control_data;
#if 0
	alc5633_pdata = codec->dev->platform_data;
	if (!alc5633_pdata){
		if (gpio_is_valid(alc5633_pdata->power_en)){
			ret = gpio_request(alc5633_pdata->power_en,"ALC5633POWERPIN");
			if(ret < 0){
				printk("alc5633_probe request gpio error\n");
			}
		}else{
			printk("alc5633_probe power_en is valid\n");
		}
		gpio_direction_output(alc5633_pdata->power_en,GPIO_LOW);
		mdelay(300);
		gpio_direction_output(alc5633_pdata->power_en,GPIO_HIGH);
	}
#endif
	//reset
	snd_soc_write(codec,ALC5633_RESET,0xff);
	alc5633_set_bias_level(codec,SND_SOC_BIAS_ON);

#if 1
	//power on I2s
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD1,0x8000,0x8000);
	//power on class D modulaton
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD1,0x2000,0x2000);
	//power on ADC
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD1,0x1800,0x1800);
	//power on DAC
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD1, 0x600, 0x600);
	//power on class-ab Amp
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD1,  0x20,  0x20);
	//power on HP mixer
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD2,0xc000,0xc000);
	//power on AUX mixer
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD2,0x3000,0x3000);
	//power record mixer
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD2,0xc00,0xc00);
	//power Line In 1 volume power
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD2,0x300,0x300);
	//power Line In 2 volume power
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD2,0xc0,0xc0);
	//power MIC1 boost Power
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD2,0x20,0x20);
	//power Mic2 boost power
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD2,0x10,0x10);
	//power microphone bias poweer
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD2,0x8,0x8);
	//power PLL power
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD2,0x2,0x2);
	//power Speaker mixer
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD2,0x1,0x1);
	//power on Vref voltage for all circuit buffer
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD3,0x8000,0x8000);
	//power on all analog circuit bias
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD3,0x2000,0x2000);
	//power on headphone amplifier
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD3,0x8,0x8);
	// power on speaker volume
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD3,0xc000,0xc000);
	//power on headphone Volume
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD3,0xc00,0xc00);
	//power on AUX volume
	snd_soc_update_bits(codec,ALC5633_PWR_MANAG_ADD3,0x300,0x300);

	//set clock without pll,default ,when set pll in pve.c,the set_pll function can
	//power pll module
	snd_soc_update_bits(codec,ALC5633_GBL_CLK_CTRL,0x0000,0x0000);
	//set clock with pll
//	snd_soc_update_bits(codec,ALC5633_GBL_CLK_CTRL,0x4000,0x4000);
	snd_soc_write(codec,ALC5633_DEPOP_CTRL_1,0x8000);// power hp soft generator
	snd_soc_write(codec,ALC5633_DEPOP_CTRL_2, 0xB000);

#endif
	// init hp path
	snd_soc_write(codec,ALC5633_PWR_MANAG_ADD4,0xc00);//power on hp volume
	snd_soc_write(codec,ALC5633_PWR_MANAG_ADD3,0xa00b);// power hp ampli and depop
	snd_soc_write(codec,ALC5633_PWR_MANAG_ADD2,0xc000);// poweron the hp
	snd_soc_write(codec,ALC5633_HPMIXER_CTRL,0x3e3e);//open HPMIXL to DAC
	snd_soc_write(codec,ALC5633_HP_OUT_VOL,0x4040);// open the HPL/R VOL
	snd_soc_write(codec,ALC5633_SPK_HP_MIXER_CTRL,0x30);//set HPO_MIX
#if 0
	//INIT SPK PATH
	snd_soc_write(codec,ALC5633_SPKMIXER_CTRL,0x800);//set spkmix to dac
	snd_soc_write(codec,ALC5633_SPK_OUT_VOL,0x2000);//set spo_n and volume
	//snd_soc_write(codec,ALC5633_SPK_HP_MIXER_CTRL,)
	snd_soc_write(codec,ALC5633_SPK_AMP_CTRL,0x8000);// class d
#endif
	//init the mic
	snd_soc_write(codec,ALC5633_MIC_CTRL_2,0x5500);//set mic to 40db
	snd_soc_write(codec,ALC5633_MIC_CTRL_1,0x0000);//default set 12db
	snd_soc_write(codec,ALC5633_REC_MIXER_CTRL,0x7dfe);//set recmix
	snd_soc_write(codec,ALC5633_ADC_DIG_VOL,0x0000);//rec volume 0db


	ret = snd_soc_add_codec_controls(codec, alc5633_snd_controls,ARRAY_SIZE(alc5633_snd_controls));
	if (ret){
		printk("snd_soc_add_controls err!!\r\n");
	}
	alc5633_add_widgets(codec);
	return 0;

}

static int alc5633_remove(struct snd_soc_codec *codec)
{
#if 0
	struct alc5633_platform_data *alc5633_pdata;
	alc5633_pdata = codec->dev->platform_data;
	gpio_free(alc5633_pdata->power_en);
#endif
	alc5633_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_alc5633 = {
	.probe = alc5633_probe,
	.remove = alc5633_remove,
	.suspend = alc5633_suspend,
	.resume = alc5633_resume,
//	.read = alc5633_read_reg_cache,
	.read = alc5633_read,
	.write = alc5633_write,
	.set_bias_level = alc5633_set_bias_level,
	.reg_cache_size = ARRAY_SIZE(alc5633_reg),
	.reg_word_size = sizeof(u16),
	.reg_cache_default = alc5633_reg,
	.reg_cache_step = 1,
};

#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)

static int alc5633_i2c_probe(struct i2c_client *i2c,
		    const struct i2c_device_id *id)
{
	struct alc5633_priv *alc5633;
	int ret;
	PRINTK("IN I2C PROBE\n");
	alc5633 = kzalloc(sizeof(struct alc5633_priv), GFP_KERNEL);
	if (alc5633 == NULL)
		return -ENOMEM;
	i2c_set_clientdata(i2c,alc5633);
	alc5633->control_data = i2c;

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_alc5633,
						&alc5633_dai, 1);
	if (ret < 0)
		kfree(alc5633);

	PRINTK("ret = %d\n",ret);
	return ret;

}

static int alc5633_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);
	kfree(i2c_get_clientdata(i2c));
	return 0;
}

static int alc5633_i2c_resume(struct i2c_client *i2c)
{
#if 0
	struct alc5633_platform_data *alc5633_pdata;
	alc5633_pdata = i2c->dev.platform_data;
	if (!alc5633_pdata)
		return -EINVAL;
#endif
	PRINTK("IN ALC5633_I2C_RESUME\n");
	return 0;
}

static const struct i2c_device_id alc5633_i2c_id[] = {
		{"alc5633", 0},
		{}
};

MODULE_DEVICE_TABLE(i2c, alc5633_i2c_id);

static struct i2c_driver alc5633_i2c_driver = {
	.driver = {
		.name = "alc5633-codec",
		.owner = THIS_MODULE,
	},
	.probe = alc5633_i2c_probe,
	.remove = alc5633_i2c_remove,
	.resume = alc5633_i2c_resume,
	.id_table = alc5633_i2c_id,
};
#endif


static int __init alc5633_modinit(void)
{
		int ret;
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
		PRINTK("init ALC6533 modinit\n");
		ret = i2c_add_driver(&alc5633_i2c_driver);
		if (ret != 0)
			pr_err("Failed to register DAC31 I2C driver: %d\n", ret);
#endif
		return ret;
}

static void __exit alc5633_modexit(void)
{
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
		i2c_del_driver(&alc5633_i2c_driver);
#endif
	return;
}
module_init(alc5633_modinit);
module_exit(alc5633_modexit);

MODULE_DESCRIPTION("Soc ALC5633 Driver");
MODULE_AUTHOR("Johnson Diao (cddiao@ambarella.com)");
MODULE_LICENSE("GPL");
