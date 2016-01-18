#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef unsigned int	u32;
typedef unsigned short	u16;
typedef unsigned char 	u8;
typedef signed int	s32;
typedef signed short	s16;
typedef signed char	s8;

#define GPIO(x)		(x)

#define IRQF_TRIGGER_NONE	0x00000000
#define IRQF_TRIGGER_RISING	0x00000001
#define IRQF_TRIGGER_FALLING	0x00000002
#define IRQF_TRIGGER_HIGH	0x00000004
#define IRQF_TRIGGER_LOW	0x00000008

#include "ambinput.h"
#include <linux/input.h>

static struct ambarella_key_table ambarella_key_table[AMBINPUT_TABLE_SIZE] = {
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_1,	0,	0x848424db}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_2,	0,	0x848414eb}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_3,	0,	0x84847887}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_4,	0,	0x8484d42b}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_ZOOMIN,	0,	0x848444bb}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_ZOOMOUT,	0,	0x8484847b}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_PREVIOUS,0,	0x8484e41b}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_NEXT,	0,	0x8484f807}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_BACK,	0,	0x848418e7}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_FORWARD,	0,	0x848458a7}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_STOP,	0,	0x8484a857}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_PLAYPAUSE,0,	0x84849867}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_SLOW,	0,	0x8484d827}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_MENU,	0,	0x8484f40b}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_NEWS,	0,	0x84844cb3}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_UP,	0,	0x84841ce3}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_DOWN,	0,	0x84849c63}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_LEFT,	0,	0x8484ec13}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_RIGHT,	0,	0x84846c93}}},
	{AMBINPUT_IR_KEY,	{.ir_key	= {KEY_OK,	0,	0x8484b847}}},

	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RESERVED,0,	0,	983,	1023}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_DELETE,	0,	0,	880,	920,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_5,	0,	0,	780,	820,}}},	//S8
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_MENU,	0,	0,	690,	730,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_OK,	0,	0,	620,	640,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RIGHT,	0,	0,	490,	530,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_LEFT,	0,	0,	360,	400,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_DOWN,	0,	0,	250,	300,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_UP,	0,	0,	120,	160,}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_BATTERY,	0,	0,	0,	40,}}},		//S1

	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RESERVED,0,	1,	983,	1023}}},
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_6,	0,	1,	880,	920,}}},	//S14
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_PHONE,	0,	1,	780,	820,}}},	//S13
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_7,	0,	1,	690,	730,}}},	//S12
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_RECORD,	0,	1,	620,	640,}}},	//S11
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_PLAY,	0,	1,	490,	530,}}},	//S10
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_8,	0,	1,	360,	400,}}},	//TP10
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_9,	0,	1,	250,	300,}}},	//S15
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_0,	0,	1,	120,	160,}}},	//(S1)
	{AMBINPUT_ADC_KEY,	{.adc_key	= {KEY_A,	0,	1,	0,	40,}}},		//(S2)
#if defined(CONFIG_BSP_BOARD_COCONUT)
	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_ESC,	0,	0,	GPIO(13),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},
	{AMBINPUT_GPIO_KEY,	{.gpio_key	= {KEY_POWER,	0,	1,	GPIO(11),	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING}}},
#endif

	{AMBINPUT_VI_KEY,	{.vi_key	= {0,	0,	0}}},
	{AMBINPUT_VI_REL,	{.vi_rel	= {0,	0,	0}}},
	{AMBINPUT_VI_ABS,	{.vi_abs	= {0,	0,	0}}},

	{AMBINPUT_END}
};

static char ambarella_keymap[] = "ambarella-keymap.bin";

int main(int argc, char **argv)
{
	FILE *fp;

	if (argc < 2) {
		if ((fp = fopen(ambarella_keymap, "wb")) == NULL) {
			perror(ambarella_keymap);
			return -1;
		}
	} else {
		if ((fp = fopen(argv[1], "wb")) == NULL) {
			perror(argv[1]);
			return -1;
		}
	}

	if (fwrite(ambarella_key_table, 1,
		sizeof(ambarella_key_table), fp) !=
		sizeof(ambarella_key_table)) {
		perror("fwrite");
		return -1;
	}

	fclose(fp);

	return 0;
}

