#!/usr/bin/perl -w
#
# sensor_skeleton.pl
#
# The perl is used to produe a skeleton for a new sensor with I2C bus
#
# NOTE: a) Strings containing "/" must have them escaped with a backslash,
#       just as in a Perl substitution string, or the substitution command
#       will probably fail.
#		b) You should set the *.ini file to unix format before doing the
#		filter job(use cmd:"sed 's/^V^M//g' foo > foo.new"  or use the vim "set ff=unix" and "wq"
#		c) Note, I just found that some wide char(UTF16), remove it first
#
# History:
#      2009/05/20 - [Qiao Wang] Create
#
# Copyright (C) 2004-2009, Ambarella, Inc.
#
# All rights reserved. No Part of this file may be reproduced, stored
# in a retrieval system, or transmitted, in any form, or by any means,
# electronic, mechanical, photocopying, recording, or otherwise,
# without the prior consent of Ambarella, Inc.
#

#files to create
$File_main	= "";
$File_docmd	= "";
$File_arch_reg_table = "";
$File_arch	= "";
$File_pri	= "";
$File_reg_tbl	= "";
$File_video_tbl	= "";
$File_ambaconfig= "AmbaConfig";
$File_kbuild	= "Kbuild";
$File_make	= "Makefile";

#temp file name
$filename	= 'TODO';

#var
$sensor_name	= "mt9m033";
$company	= "micron";
$author		= "Qiao Wang";
$device_address	= "0x21";
$mailname	= "qwang";

#sensor
$sensor_type	= "CMOS";
$sensor_size	= "1/3.3"; #inch
$sensor_Mega	= "1.3"; #megapixel
$sensor_data_type	= "1"; #default RGB raw data
$reg_data_width	= "u8"; #default 8bit width
$reg_addr_width	= "u8"; #default 8bit width

#para for amba soc vin arch
#$amba_input_sel_mode;			#< Set the data input to be 16-bit, LS 12-bit, or MS 8-bit */
#$amba_yuv_mode;			#< Select video input mode to be either RGB, 8 bits YUV or 16 bits YUV */
#$amba_ena_vin_rst_on_interrupt;	#< 1: VIN module resets when IDSP generates an error interrupt to ARM */
#$amba_idsp_hard_rst;
#$amba_sony_field_mode;			#< Select to use SONY sensor specific field mode, or normal field mode */
#$amba_field0_pol;			#< Select field 0 polarity under SONY field mode */
#$amba_sync_dir;
#$amba_emb_sync;			#< Select if sync code is embedded in video input stream or not */
#$amba_w16;				#< Select valid data width of SD bus */
#$amba_data_edge;			#< Select if video input data is valid on rising edge or falling edge of SPCLK */
#$amba_vs_hs_polarity;			#< Select polarity of V-Sync and H-Sync */

#time
$sec	= "";
$min	= "";
$hour	= "";
$day	= "31";
$mon	= "05";
$year	= "2009";
$weekday	= "";
$yeardate	= "";
$savinglightday	= "";
$now	= "";

$fileheader	= "";


print "
	Produce C code skeleton for sensor/decoder driver development.

	Support cmos sensor: YUV/RGB, with I2C(SCCB) interface.

Note:
	1, change to sensor directory first;
	2, current it is test with A5S;

TODO:
	1, support SPI interface
	2, support CCD sensor
	3, add debug code to check the valid of index, format index...
	4, reg bit-width and its sub-address bit-width
	5, support decoder


						--Qiao (Ambarella SH)
						--12/04/2009(update)

";

&main();
print "\n Done, at $now!\n";

sub main
{
	&check_dir();
	&getInput();
	&make_dir();

	$filename = $File_pri;
	&initFileheader($fileheader);
	&produce_pri($filename);

	$filename = $File_main;
	&initFileheader($fileheader);
	&produce_main($filename);

	$filename = $File_arch_reg_table;
	&initFileheader($fileheader);
	&produce_arch_reg_table($filename);

	chdir ".\/arch_a5s";
	$filename = $File_arch;
	&initFileheader($fileheader);
	&produce_arch($filename);
	chdir ".\/..";

	$filename = $File_reg_tbl;
	&initFileheader($fileheader);
	&produce_reg_table($filename);

	$filename = $File_video_tbl;
	&initFileheader($fileheader);
	&produce_video_table($filename);

	$filename = $File_ambaconfig;
	&initFileheader($fileheader);
	&produce_ambaconfig($filename);

	$filename = $File_kbuild;
	&initFileheader($fileheader);
	&produce_kbuild($filename);

	$filename = $File_make;
	&initFileheader($fileheader);
	&produce_make($filename);

	$filename = $File_docmd;
	&initFileheader($fileheader);
	&produce_docmd($filename);

}


# Get the input from shell, TODO check the valid of input
sub getInput()
{
	print"Pls, input the sensor's factory name, e.g. micron,omnivision,sony,adi,techwell,panasonic\n";
	$company = <STDIN>; #debug
	chomp($company);

	print"Pls, input the sensor name, e.g. imx035, ov2710, mt9j001, mt9m033\n";
	$sensor_name = <STDIN>; #debug
	chomp($sensor_name);

	print"Pls, input your name\n";
	$author = <STDIN>; #debug
	chomp($author);

	print"Pls, input your name in mail\n";
	$mailname = <STDIN>; #debug
	chomp($mailname);

	($sec,$min,$hour,$day,$mon,$year,$weekday,$yeardate,$savinglightday) = (localtime(time));
	$sec   =   ($sec   <   10)?   "0$sec":$sec;
	$min   =   ($min   <   10)?   "0$min":$min;
	$hour  =   ($hour  <   10)?   "0$hour":$hour;
	$day   =   ($day   <   10)?   "0$day":$day;
	$mon   =   ($mon   <    9)?   "0".($mon+1):($mon+1);
	$year +=   1900;
	$now   =   "$year-$mon-$day   $hour:$min:$sec";

	print"Pls, input the sensor's (I2C/SPI) device address in hex format,e.g. 0x42\n";
	$device_address = <STDIN>; #debug
	chomp($device_address);

	print"Pls, input the sensor register data width 0:8bit, 1:16bit\n";
	$reg_data_width = <STDIN>; #debug
	chomp($reg_data_width);
	if($reg_data_width == 0){
	   $reg_data_width = "u8";
	} else {
	   $reg_data_width = "u16";
	}

	print"Pls, input the sensor register address width 0:8bit, 1:16bit\n";
	$reg_addr_width = <STDIN>; #debug
	chomp($reg_addr_width);
	if($reg_addr_width == 0){
	   $reg_addr_width = "u8";
	} else {
	   $reg_addr_width = "u16";
	}

	print"Pls, input the sensor output data type 0:YUV, 1:RGB raw\n";
	$sensor_data_type = <STDIN>; #debug
	chomp($sensor_data_type);
	if($sensor_data_type != 0 && $sensor_data_type != 1){
		die "err set output data type!\n"
	}

	#init file name here
	$File_main		= $sensor_name.'.c';
	$File_docmd	= $sensor_name.'_docmd.c';
	$File_arch		= $sensor_name.'_arch.c';
	$File_pri		= $sensor_name.'_pri.h';
	$File_reg_tbl	= $sensor_name.'_reg_tbl.c';
	$File_video_tbl	= $sensor_name.'_video_tbl.c';
	$File_arch_reg_table = $sensor_name.'_arch_reg_tbl.c';
}
sub initFileheader
{
	$_[0]= "/*
 * Filename : $filename
 *
 * History:
 *    $year/$mon/$day - [$author] Create
 *
 * Copyright (C) 2004-$year, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */\n";
#print $fileheader;
}
sub check_dir
{
	$dir = `pwd`;#use Cwd here to enable it more portable
	if(!($dir =~ /private\/drivers\/ambarella\/vin\/sensors/)){#debug
	#if(!($dir =~ /qwang\/work/)){
		print "Currently dir is $dir\n";
		die "Error, you must change to sensor directory first\n";
	}
}

sub make_dir
{
	$dir_name = $company.'_'.$sensor_name;

	opendir DH,'.' || die "can't open current dir:$!";
	foreach $file(readdir DH){
		if( $file eq $dir_name ){
			print "The same name's dir or file exists, we must to remove it\n";
			print "Input y/n to continue or exit\n";
			if("y\n" eq <STDIN> ){ #debug
				&sysFunc("rm -rf $dir_name");
				last;
			}else{
				exit 0;
			}
		}
	}

	mkdir $dir_name, 0755 or die "Can not make dir $dir_name:$!\n";
	mkdir "$dir_name\/arch_a5s", 0755 or die "Can not make dir arch_a5s:$!\n";
	chdir "\.\/$dir_name";
}

#run a system command and check the return
sub sysFunc
{
	my $result;
	$result = system("$_[0]");
	if($result != 0) {
		die ("CMD Failed:$_[0]\n");
	}
}

#produce the pri.h file
sub produce_pri
{
	my @outLines;  # Data we are going to output
	open(OUTFILE, ">$_[0]") or die "Can not open $_[0]:$!\n";
	push(@outLines, $fileheader);
	push(@outLines,"#ifndef __\U$sensor_name\E_PRI_H__
#define __\U$sensor_name\E_PRI_H__\n");
	push(@outLines,"
	\/\/	<add the register defines here, which are produced by filter1>

		\n");
	push(@outLines, "
#define \U$sensor_name\E_VIDEO_FPS_REG_NUM				(0)
#define \U$sensor_name\E_VIDEO_FORMAT_REG_NUM			(0)
#define \U$sensor_name\E_VIDEO_FORMAT_REG_TABLE_SIZE	(5)
#define \U$sensor_name\E_VIDEO_PLL_REG_TABLE_SIZE		(1)

/* ========================================================================== */
struct $sensor_name\E_reg_table {
	u8 reg;
	u8 data;
};

struct $sensor_name\E_pll_reg_table {
	u32 pixclk;
	u32 extclk;
	struct $sensor_name\E_reg_table regs[\U$sensor_name\E_VIDEO_PLL_REG_TABLE_SIZE];
};

struct $sensor_name\E_video_fps_reg_table {
	u16 reg[\U$sensor_name\E_VIDEO_FPS_REG_NUM];
	struct {
		const struct $sensor_name\E_pll_reg_table *pll_reg_table;
		u32 fps;
		u8 system;
		u16 data[\U$sensor_name\E_VIDEO_FPS_REG_NUM];
		} table[AMBA_VIN_MAX_FPS_TABLE_SIZE];
};

struct $sensor_name\E_video_format_reg_table {
	u16 reg[\U$sensor_name\E_VIDEO_FORMAT_REG_NUM];
	struct {
		void (*ext_reg_fill)(struct __amba_vin_source *src);
		u8 data[\U$sensor_name\E_VIDEO_FORMAT_REG_NUM];
		const struct $sensor_name\E_video_fps_reg_table *fps_table;
		u16 width;
		u16 height;
		u8 format;
		u8 type;
		u8 bits;
		u8 ratio;
		u32 srm;
		u32 sw_limit;
		} table[\U$sensor_name\E_VIDEO_FORMAT_REG_TABLE_SIZE];
};

struct $sensor_name\E_video_info {
	u32 format_index;
	u32 fps_index;
	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u16 sync_start;
	u8 type_ext;
	u8 bayer_pattern;
};

struct $sensor_name\E_video_mode {
	enum amba_video_mode mode;
	u32 preview_index;
	u32 preview_mode_type;
	u32 still_index;
	u32 still_mode_type;
};
");
	push(@outLines, "#endif /* __\U$sensor_name\E_PRI_H__ */\n\n");

	print(OUTFILE @outLines);
	close OUTFILE;
	undef( @outLines );
}



#produce the main.c file
sub produce_main
{
	my @outLines;  # Data we are going to output
	open(OUTFILE, ">$_[0]") or die "Can not open $_[0]:$!\n";
	push(@outLines, $fileheader);
	push(@outLines, "
#include <amba_common.h>

#include <dsp_cmd.h>
#include <vin_pri.h>

#include \"$sensor_name\E_pri.h\"
");
	if($sensor_data_type){ #for RGB raw
	push(@outLines, "
");
	}
	push(@outLines, "
/* ========================================================================== */
#ifndef CONFIG_I2C_AMBARELLA_\U$sensor_name\E_ADDR
	#define CONFIG_I2C_AMBARELLA_\U$sensor_name\E_ADDR	($device_address>>1)
#endif

AMBA_VIN_BASIC_PARAM_CALL($sensor_name\E, CONFIG_I2C_AMBARELLA_\U$sensor_name\E_ADDR, 0, 0644);

/* ========================================================================== */
struct $sensor_name\E_info {
	struct i2c_client		*client;
	struct __amba_vin_source	source;
	int				current_video_index;
	enum amba_video_mode		current_vin_mode;
	u16				cap_start_x;
	u16				cap_start_y;
	u16				cap_cap_w;
	u16				cap_cap_h;
	u8				active_capinfo_num;
	u8				bayer_pattern;
	u32				fps_index;
	u32				mode_type;
	u32				mode_index;
	struct amba_vin_black_level_compensation current_sw_blc;
	u32				current_shutter_time;
	u32				current_gain_db;
	u16				min_agc_index;
	u16				max_agc_index;

");
	if($sensor_data_type){ #for RGB raw
	push(@outLines, "	int				current_slowshutter_mode;
");
	}
	push(@outLines, "	amba_vin_agc_info_t		agc_info;
	amba_vin_shutter_info_t		shutter_info;
};
/* ========================================================================== */

static const char $sensor_name\E_name[] = \"$sensor_name\E\";
static struct i2c_driver i2c_driver_$sensor_name\E;
\n");
push(@outLines, "
static int $sensor_name\E_write_reg( struct __amba_vin_source *src, $reg_addr_width subaddr, $reg_data_width data)
{
	int errCode = 0;
	struct $sensor_name\E_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	vin_dbg(\"$sensor_name\E write reg - 0x\%x, data - 0x\%x\\n\", subaddr, data);
	pinfo = (struct $sensor_name\E_info *)src->pinfo;
	client = pinfo->client;

\n");
if ($reg_addr_width eq "u16" && $reg_data_width eq "u16") {
	push(@outLines, "
	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = (data >> 8);
	pbuf[3] = (data & 0xff);

	msgs[0].len = 4;");
} elsif($reg_addr_width eq "u16" && $reg_data_width eq "u8") {
	push(@outLines, "
	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = (data & 0xff);

	msgs[0].len = 3;");
} elsif($reg_addr_width eq "u8" && $reg_data_width eq "u16") {
	push(@outLines, "
	pbuf[0] = (subaddr & 0xff);
	pbuf[1] = (data >> 8);
	pbuf[2] = (data & 0xff);

	msgs[0].len = 3;");
} else {
	push(@outLines, "
	pbuf[0] = (subaddr & 0xff);
	pbuf[1] = (data & 0xff);

	msgs[0].len = 2;");
};

	push(@outLines, "
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		printk(\"$sensor_name\E_write_reg(error) \%d [0x\%x:0x\%x]\\n\",
			errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int $sensor_name\E_read_reg( struct __amba_vin_source *src, $reg_addr_width subaddr, $reg_data_width *pdata)
{
	int	errCode = 0;
	struct $sensor_name\E_info *pinfo;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[6];

	pinfo = (struct $sensor_name\E_info *)src->pinfo;
	client = pinfo->client;
\n");
if ($reg_addr_width eq "u16") {
	push(@outLines, "
	pbuf0[0] = (subaddr >> 8);
	pbuf0[1] = (subaddr & 0xff);

	msgs[0].len = 2;");
} else {
	push(@outLines, "
	pbuf0[0] = (subaddr & 0xff);

	msgs[0].len = 1;");
};

push(@outLines, "
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf0;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].buf = pbuf;");
if ($reg_data_width eq "u16") {
	push(@outLines, "
	msgs[1].len = 2;\n");
} else {
	push(@outLines, "
	msgs[1].len = 1;\n");
};

push(@outLines, "
	errCode = i2c_transfer(client->adapter, msgs, 2);
	if ((errCode != 1) && (errCode != 2)){
		printk(\"$sensor_name\E_read_reg(error) \%d [0x\%x]\\n\",
			errCode, subaddr);
		errCode = -EIO;
	} else {");
if ($reg_data_width eq "u16") {
	push(@outLines, "
		*pdata = ((pbuf[0] << 8) | pbuf[1]);");
} else {
	push(@outLines, "
		*pdata = pbuf[0];");
};

push(@outLines, "
		errCode = 0;
	}

	return errCode;
}
\n");
	push(@outLines, "
#if 0
/*
 *	Write any register field
 *	Para:
 *   	\@reg, register
 *		\@bitfild, mask
 *      \@value, value to set
 *	<registername>, [<bitfieldname>,] <value>
 */
static int $sensor_name\E_write_reg_field(struct __amba_vin_source *src, $reg_addr_width add, $reg_data_width bitfield, $reg_data_width value)
{
	int errCode = 0;
	$reg_data_width regdata;
	int mask_bit_start = 0;
	errCode = $sensor_name\E_read_reg(src, add, &regdata);

	if (errCode != 0)
		return -1;
	regdata &= (~bitfield);
	while ((bitfield & 0x01) == 0) {
		mask_bit_start++;
		bitfield >>= 0x01;
	}
	value <<= mask_bit_start;
	regdata |= value;
	vin_dbg(\"regdata = 0x%04x, value_byshift = 0x%04x, mask_bit_start =  %04d\\n\", regdata, value,mask_bit_start);
	errCode = $sensor_name\E_write_reg(src, add, regdata);

	return errCode;
}
#endif
/* ========================================================================== */

#include \"$sensor_name\E_arch_reg_tbl.c\"
#include \"$sensor_name\E_reg_tbl.c\"
#include \"$sensor_name\E_video_tbl.c\"

static void $sensor_name\E_print_info(struct __amba_vin_source *src)
{

}

/* ========================================================================== */
#include \"arch/$sensor_name\E_arch.c\"
#include \"$sensor_name\E_docmd.c\"
/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int $sensor_name\E_probe(struct i2c_adapter *adap)
{
	int	errCode = 0;
	struct $sensor_name\E_info	*pinfo;
	struct i2c_client *client;
	u16 sen_id = 0;

	struct __amba_vin_source	*src;

	errCode = amba_vin_check_i2c(adap);
	if (errCode)
		goto $sensor_name\E_exit;

	if (strcmp(adap->name, \"ambarella-i2c\") != 0) {
		errCode = -EACCES;
		goto $sensor_name\E_exit;
	}

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client) {
		errCode = -ENOMEM;
		goto $sensor_name\E_exit;
	}
	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		errCode = -ENOMEM;
		goto $sensor_name\E_free_client;
	}

	client->addr = $sensor_name\E_addr;
	client->adapter = adap;
	client->driver = &i2c_driver_$sensor_name\E;
	client->flags = 0;
	strlcpy(client->name, $sensor_name\E_name, sizeof(client->name));
	i2c_set_clientdata(client, pinfo);

	pinfo->client = client;
	pinfo->current_video_index = -1;
	pinfo->fps_index = 0;
	pinfo->bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
");
	if($sensor_data_type){ #for RGB raw
	push(@outLines, "
	pinfo->min_agc_index = 0;
	pinfo->max_agc_index = \U$sensor_name\E_GAIN_ROWS - 1;
    /* TODO, update the db info for each sensor */
	pinfo->agc_info.db_max = 0x24000000;	// 36dB
	pinfo->agc_info.db_min = 0x00000000;	// 0dB
	pinfo->agc_info.db_step = 0x00600000;	// 0.375dB

	pinfo->current_slowshutter_mode = 0;

");
	}
	push(@outLines, "
	errCode = i2c_attach_client(client);
	if (errCode)
		goto $sensor_name\E_free_pinfo;


	src = &pinfo->source;
	src->adapid = adapter_id;
	src->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	strlcpy(src->name, $sensor_name\E_name, sizeof(src->name));

	src->owner = THIS_MODULE;
	src->pinfo = pinfo;
	src->docmd = $sensor_name\E_docmd;
	src->active_channel_id = 0;
	src->total_channel_num = 1;

	errCode = amba_vin_add_source(src);
	if (errCode)
		goto $sensor_name\E_detach_client;
	errCode = $sensor_name\E_init_vin_clock(src, 0);
	if (errCode)
		goto $sensor_name\E_del_source;

	mdelay(10);/* wait for sensor/decoder pll stable */
	$sensor_name\E_reset(src);
	errCode = $sensor_name\E_query_sensor_id(src, &sen_id);
	if (errCode)
		goto $sensor_name\E_del_source;
	printk(\"\U$sensor_name\E sensor ID is 0x\%x\\n\", sen_id);

	goto $sensor_name\E_exit;

$sensor_name\E_del_source:
	amba_vin_del_source(src);
$sensor_name\E_detach_client:
	i2c_detach_client(client);
$sensor_name\E_free_pinfo:
	kfree(pinfo);
$sensor_name\E_free_client:
	kfree(client);
	printk(\"insmod $sensor_name err \%d\\n\", errCode);
$sensor_name\E_exit:
	return errCode;
}

static int $sensor_name\E_remove(struct i2c_client *client)
{
	int	errCode = 0;
	struct $sensor_name\E_info *pinfo;
	struct __amba_vin_source *src;

	pinfo = (struct $sensor_name\E_info *)i2c_get_clientdata(client);

	if (pinfo) {
		errCode = i2c_detach_client(pinfo->client);
		i2c_set_clientdata(client, NULL);
		kfree(client);

		src = &pinfo->source;
		errCode |= amba_vin_del_source(src);

		kfree(pinfo);
	}

	vin_notice(\"\%s removed!\\n\", $sensor_name\E_name);

	return errCode;
}

static struct i2c_driver i2c_driver_$sensor_name\E = {
	.driver = {
		.name	= $sensor_name\E_name,
	},
	.id		= I2C_DRIVERID_I2CDEV,
	.attach_adapter = $sensor_name\E_probe,
	.detach_client	= $sensor_name\E_remove,
	.command	= NULL
};

static int __init $sensor_name\E_init(void)
{
	return i2c_add_driver(&i2c_driver_$sensor_name\E);
}

static void __exit $sensor_name\E_exit(void)
{
	i2c_del_driver(&i2c_driver_$sensor_name\E);
}

module_init($sensor_name\E_init);
module_exit($sensor_name\E_exit);

MODULE_DESCRIPTION(\"\U$sensor_name\E $sensor_size-Inch $sensor_Mega-Megapixel $sensor_type Digital Image Sensor\");
MODULE_AUTHOR(\"$author, <$mailname\@ambarella.com>\");
MODULE_LICENSE(\"Proprietary\");
\n");

	print(OUTFILE @outLines);
	close OUTFILE;
	undef( @outLines );
}



#produce the arch_reg_table file
sub produce_arch_reg_table
{
	my @outLines;  # Data we are going to output
	open(OUTFILE, ">$_[0]") or die "Can not open $_[0]:$!\n";
	push(@outLines, $fileheader);
	push(@outLines, "
static const struct $sensor_name\E_reg_table $sensor_name\E_share_regs[] = {
	/* <add share regs setting here> */
};
#define \U$sensor_name\E_SHARE_REG_SZIE		ARRAY_SIZE($sensor_name\E_share_regs)
");
	push(@outLines, "
/*
 *  1.rember to update \U$sensor_name\E_VIDEO_PLL_REG_TABLE_SIZE if you add/remove the regs
 *	2.see rct.h for pixclk/extclk value
 */
static const struct $sensor_name\E_pll_reg_table $sensor_name\E_pll_tbl[] = {
	[0] = {
		.pixclk = 80000000,  /* clock output from sensor/decoder */
		.extclk = PLL_CLK_27MHZ, /* clock from Amba soc to sensor/decoder, if you use external oscillator, ignore it */
		.regs = {
		/* set pll register value here */
		}
	},
	[1] = {
		.pixclk = 79920080,
		.extclk = PLL_CLK_27D1001MHZ,
		.regs = {
		/* set pll register value here */
		}
	},

		/* << add pll config here if necessary >> */
};


/* ========================================================================== */
static const struct $sensor_name\E_video_fps_reg_table $sensor_name\E_video_fps_1080p = {
	.reg		= {
		/* add fps related register here */
	},
	.table		= {
		{ // 30fps
		.pll_reg_table	= &$sensor_name\E_pll_tbl[0],
		.fps		= AMBA_VIDEO_FPS_30,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {
		/* set register value here */
		}
		},
		{ // 29.97fps
		.pll_reg_table	= &$sensor_name\E_pll_tbl[0],
		.fps		= AMBA_VIDEO_FPS_29_97,
		.system		= AMBA_VIDEO_SYSTEM_AUTO,
		.data		= {
		/* set register value here */
		}
		},
		{//End
		.pll_reg_table	= NULL,
		},
		/* << add different fps table if necessary >> */
	},
};

	/* << add other fps tables for differnent video format here if necessary >> */

\n");


	print(OUTFILE @outLines);
	close OUTFILE;
	undef( @outLines );
}


#produce the arch file
sub produce_arch
{
	my @outLines;  # Data we are going to output
	my $yuv_mode = "AMBA_VIDEO_ADAP_YUV_8B_CR_Y0_CB_Y1";
	my $input_format = "AMBA_VIN_INPUT_FORMAT_YUV_422_PROG";
	open(OUTFILE, ">$_[0]") or die "Can not open $_[0]:$!\n";

	if($sensor_data_type){ #for RGB raw
		$yuv_mode = "AMBA_VIDEO_ADAP_RGB_MODE";
		$input_format = "AMBA_VIN_INPUT_FORMAT_RGB_RAW";
	}

	push(@outLines, $fileheader);
	push(@outLines, "
static int $sensor_name\E_init_vin_clock(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct amba_vin_clk_info adap_arg;
	u32 fps_index;
	u32 format_index;
	struct $sensor_name\E_info *pinfo;
	const struct $sensor_name\E_video_fps_reg_table *fps_table;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;

	format_index = $sensor_name\E_video_info_table[index].format_index;
	fps_index = pinfo->fps_index;
	fps_table = $sensor_name\E_video_format_tbl.table[format_index].fps_table;

	adap_arg.mode = 0;
	adap_arg.so_freq_hz = fps_table->table[fps_index].pll_reg_table->extclk;
	adap_arg.so_pclk_freq_hz = fps_table->table[fps_index].pll_reg_table->pixclk;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIN_CLOCK, &adap_arg);

	return errCode;
}

static int $sensor_name\E_pre_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	struct $sensor_name\E_info *pinfo;
	u32 adap_arg;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;

	adap_arg = 0;
	errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_INIT, &adap_arg);

	return errCode;
}

static int $sensor_name\E_set_vin_mode(struct __amba_vin_source *src)
{
	int	errCode = 0;
	struct $sensor_name\E_info		*pinfo;
	struct amba_vin_adap_config	$sensor_name\E_config;
	u32				index;
	u16				phase;
	u32				width;
	u32				height;
	u32				nstartx;
	u32				nstarty;
	u32				nendx;
	u32				nendy;
	struct amba_vin_cap_window	window_info;
	struct amba_vin_min_HV_sync	min_HV_sync;
	u32				adap_arg;

	pinfo = (struct $sensor_name\E_info *)src->pinfo;
	index = pinfo->current_video_index;

	errCode |= $sensor_name\E_init_vin_clock(src, index);

	memset(&$sensor_name\E_config, 0, sizeof ($sensor_name\E_config));

	$sensor_name\E_config.hsync_mask = HSYNC_MASK_CLR;
	$sensor_name\E_config.sony_field_mode = AMBA_VIDEO_ADAP_NORMAL_FIELD;
	$sensor_name\E_config.field0_pol = VIN_DONT_CARE;

	phase = AMBA_VIDEO_ADAP_VD_HIGH_HD_HIGH;
	$sensor_name\E_config.hs_polarity = (phase & 0x1);
	$sensor_name\E_config.vs_polarity = (phase & 0x2) >> 1;

	$sensor_name\E_config.emb_sync_loc = VIN_DONT_CARE;
	$sensor_name\E_config.emb_sync = VIN_DONT_CARE;
	$sensor_name\E_config.sync_mode = SYNC_MODE_SLAVE;

	$sensor_name\E_config.data_edge = PCLK_RISING_EDGE;
	$sensor_name\E_config.src_data_width = SRC_DATA_12B;
	$sensor_name\E_config.input_mode = VIN_RGB_LVDS_1PEL_SDR_LVCMOS;

	$sensor_name\E_config.slvs_act_lanes = VIN_DONT_CARE;
	$sensor_name\E_config.serial_mode = SERIAL_VIN_MODE_DISABLE;
	$sensor_name\E_config.sony_slvs_mode = SONY_SLVS_MODE_DISABLE;

	$sensor_name\E_config.clk_select_slvs = CLK_SELECT_SPCLK;
	$sensor_name\E_config.slvs_eav_col = VIN_DONT_CARE;
	$sensor_name\E_config.slvs_sav2sav_dist = VIN_DONT_CARE;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_CONFIG, &$sensor_name\E_config);


	nstartx = pinfo->cap_start_x;
	nstarty = pinfo->cap_start_y;
	/* for 656 mode we need double the capture width */
	width = pinfo->cap_cap_w;
	/*
	* for interlace video, height is half of cap_h; for some
	* source is lines is less than it delcares, so we should
	* adjust the height
	*/
	height = pinfo->cap_cap_h;

	nendx = (width + nstartx);
	nendy = (height + nstarty);

	window_info.start_x = nstartx;
	window_info.start_y = nstarty;
	window_info.end_x = nendx - 1;
	window_info.end_y = nendy - 1;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_CAPTURE_WINDOW, &window_info);

	min_HV_sync.hs_min = nendx - 20;
	min_HV_sync.vs_min = nendy - 20;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_MIN_HW_SYNC_WIDTH, &min_HV_sync);

	adap_arg = $sensor_name\E_video_info_table[index].sync_start;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VOUT_SYNC_START_LINE, &adap_arg);

	return errCode;
}
static int $sensor_name\E_post_set_vin_mode(struct __amba_vin_source *src)
{
	int errCode = 0;
	u32 adap_arg;

	adap_arg = AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_ENABLE;
	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_VIDEO_CAPTURE_WINDOW, &adap_arg);

	return errCode;
}

static int $sensor_name\E_get_capability(struct __amba_vin_source *src, struct amba_vin_src_capability *args)
{
	int errCode = 0;
	struct $sensor_name\E_info *pinfo;
	u32 i;
	u32 index = -1;
	u32 fps_index;
	u32 format_index;
	const struct $sensor_name\E_video_fps_reg_table *fps_table;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;
	if (pinfo->current_video_index == -1) {
		vin_err(\"$sensor_name\E isn't ready!\\n\");
		errCode = -EINVAL;
		goto $sensor_name\E_get_video_preprocessing_exit;
	}
	index = pinfo->current_video_index;

	format_index = $sensor_name\E_video_info_table[index].format_index;
	fps_index = pinfo->fps_index;
	fps_table = $sensor_name\E_video_format_tbl.table[format_index].fps_table;

	memset(args, 0, sizeof (struct amba_vin_src_capability));

	args->input_type = $sensor_name\E_video_format_tbl.table[format_index].type;
	args->dev_type = AMBA_VIN_SRC_DEV_TYPE_CMOS;
	args->frame_rate = fps_table->table[fps_index].fps;
	args->video_format = $sensor_name\E_video_format_tbl.table[format_index].format;
	args->bit_resolution = $sensor_name\E_video_format_tbl.table[format_index].bits;
	args->aspect_ratio = $sensor_name\E_video_format_tbl.table[format_index].ratio;
	args->video_system = fps_table->table[fps_index].system;
	//args->type_ext = $sensor_name\E_video_info_table[index].type_ext;

	/* TODO update it if necessary */
	args->max_width = 1296;
	args->max_height = 818;

	args->def_cap_w = $sensor_name\E_video_format_tbl.table[format_index].width;
	args->def_cap_h = $sensor_name\E_video_format_tbl.table[format_index].height;

	args->cap_start_x = pinfo->cap_start_x;
	args->cap_start_y = pinfo->cap_start_y;
	args->cap_cap_w = pinfo->cap_cap_w;
	args->cap_cap_h = pinfo->cap_cap_h;

	args->sensor_id = GENERIC_SENSOR; /* TODO, some workaround is done in ucode for each sensor, so set it carefully */
	args->bayer_pattern = pinfo->bayer_pattern;
	args->field_format = 1;
	args->active_capinfo_num = pinfo->active_capinfo_num;
	args->bin_max = 4;
	args->skip_max = 8;
	args->sensor_readout_mode = $sensor_name\E_video_format_tbl.table[format_index].srm;
	args->input_format = $input_format; /* TODO update it if it yuv interlaced*/
	args->column_bin = 0;
	args->row_bin = 0;
	args->column_skip = 0;
	args->row_skip = 0;

	for (i = 0; i < AMBA_VIN_MAX_FPS_TABLE_SIZE; i++) {
		if (fps_table->table[i].pll_reg_table == NULL) {
			args->ext_fps[i] = AMBA_VIDEO_FPS_AUTO;
			continue;
		}
		args->ext_fps[i] = fps_table->table[i].fps;
	}

	args->mode_type = pinfo->mode_type;
	args->current_vin_mode = pinfo->current_vin_mode;
	args->current_shutter_time = pinfo->current_shutter_time;
	args->current_gain_db = pinfo->current_gain_db;
	args->current_sw_blc.bl_oo = pinfo->current_sw_blc.bl_oo;
	args->current_sw_blc.bl_oe = pinfo->current_sw_blc.bl_oe;
	args->current_sw_blc.bl_eo = pinfo->current_sw_blc.bl_eo;
	args->current_sw_blc.bl_ee = pinfo->current_sw_blc.bl_ee;
	args->current_fps = fps_table->table[fps_index].fps;

	errCode |= amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_GET_VIN_CAP_INFO, &(args->vin_cap_info));

$sensor_name\E_get_video_preprocessing_exit:
	return errCode;
}
\n");

	print(OUTFILE @outLines);
	close OUTFILE;
	undef( @outLines );
}


#produce the reg_table file
sub produce_reg_table
{
	my @outLines;  # Data we are going to output
	open(OUTFILE, ">$_[0]") or die "Can not open $_[0]:$!\n";
	push(@outLines, $fileheader);
	push(@outLines, "
	\/\*   < rember to update \U$sensor_name\E_VIDEO_FORMAT_REG_TABLE_SIZE, once you add or remove table items \*\/
	\/\*   < rember to update \U$sensor_name\E_VIDEO_FORMAT_REG_NUM, once you add or remove register here\*\/
static const struct $sensor_name\E_video_format_reg_table $sensor_name\E_video_format_tbl = {
	.reg = {
		\/\* add video format related register here \*\/
		},
	.table[0] = {		//1080p
		.ext_reg_fill = NULL,
		.data = {
			\/\* set video format related register here \*\/
		},
		.fps_table = &$sensor_name\E_video_fps_1080p,
		.width	= 1920,
		.height	= 1080,
		.format	= AMBA_VIDEO_FORMAT_PROGRESSIVE,
		.type	= AMBA_VIDEO_TYPE_RGB_RAW,//TODO update it if it is YUV mode
		.bits	= AMBA_VIDEO_BITS_12,//TODO
		.ratio	= AMBA_VIDEO_RATIO_16_9,
		.srm	= 0,
		.sw_limit	= -1,
		},
		\/\* add video format table here, if necessary \*\/
};
");

	if($sensor_data_type){ #for RGB raw
	push(@outLines, "
#define \U$sensor_name\E_GAIN_ROWS		256
#define \U$sensor_name\E_GAIN_COLS		3
#define \U$sensor_name\E_GAIN_DOUBLE	32
#define \U$sensor_name\E_GAIN_0DB		224

/* This is 32-step gain table, \U$sensor_name\E_GAIN_ROWS = 162, \U$sensor_name\E_GAIN_COLS = 3 */
const s16 \U$sensor_name\E_GAIN_TABLE[\U$sensor_name\E_GAIN_ROWS][\U$sensor_name\E_GAIN_COLS] = {

/* gain_value*256,  log2(gain)*1000,  register */
	/* error */
	{0x8000, 7000, 0x397f},	/* index 0   -0.002121     : 42dB */
	{0x7d41, 6968, 0x387f},	/* index 1   0.051357      */
	{0x7a92, 6937, 0x367f},	/* index 2   -0.036266     */
	{0x77f2, 6906, 0x357f},	/* index 3   0.010639      */
	{0x7560, 6875, 0x347f},	/* index 4   0.055210      */
	{0x72dc, 6843, 0x327f},	/* index 5   -0.051109     */
	{0x7066, 6812, 0x317f},	/* index 6   -0.014027     */
	{0x6dfd, 6781, 0x307f},	/* index 7   0.020378      */
	{0x6ba2, 6750, 0x2f7f},	/* index 8   0.052017      */
	{0x6954, 6718, 0x2e7f},	/* index 9   0.080780      */
	{0x6712, 6687, 0x2c7f},	/* index 10  -0.058884     */
	{0x64dc, 6656, 0x2b7f},	/* index 11  -0.039406     */
	{0x62b3, 6625, 0x2a7f},	/* index 12  -0.023262     */
	{0x6096, 6593, 0x297f},	/* index 13  -0.010597     */
	{0x5e84, 6562, 0x287f},	/* index 14  -0.001549     */
	{0x5c7d, 6531, 0x277f},	/* index 15  0.003723      */
	{0x5a82, 6500, 0x267f},	/* index 16  0.005066      */
	{0x5891, 6468, 0x257f},	/* index 17  0.002308      */
	{0x56ac, 6437, 0x247f},	/* index 18  -0.004745     */
	{0x54d0, 6406, 0x237f},	/* index 19  -0.016289     */
	{0x52ff, 6375, 0x227f},	/* index 20  -0.032528     */
	{0x5138, 6343, 0x217f},	/* index 21  -0.053692     */
	{0x4f7a, 6312, 0x207f},	/* index 22  -0.080025     */
	{0x4dc6, 6281, 0x207f},	/* index 23  0.108116      */
	{0x4c1b, 6250, 0x1f7f},	/* index 24  0.076355      */
	{0x4a7a, 6218, 0x1e7f},	/* index 25  0.038879      */
	{0x48e1, 6187, 0x1d7f},	/* index 26  -0.004616     */
	{0x4752, 6156, 0x1c7f},	/* index 27  -0.054459     */
	{0x45ca, 6125, 0x1b7f},	/* index 28  -0.111004     */
	{0x444c, 6093, 0x1b7f},	/* index 29  0.077141      */
	{0x42d5, 6062, 0x1a7f},	/* index 30  0.013504      */
	{0x4166, 6031, 0x197f},	/* index 31  -0.057655     */
	{0x4000, 6000, 0x197f},	/* index 32  0.130489     : 36dB */
	{0x3ea0, 5968, 0x187f},	/* index 33  0.051353      */
	{0x3d49, 5937, 0x177f},	/* index 34  -0.036266     */
	{0x3bf9, 5906, 0x167f},	/* index 35  -0.132935     */
	{0x3ab0, 5875, 0x167f},	/* index 36  0.055210      */
	{0x396e, 5843, 0x157f},	/* index 37  -0.051109     */
	{0x3833, 5812, 0x157f},	/* index 38  0.137035      */
	{0x36fe, 5781, 0x147f},	/* index 39  0.020378      */
	{0x35d1, 5750, 0x137f},	/* index 40  -0.107365     */
	{0x34aa, 5718, 0x137f},	/* index 41  0.080780      */
	{0x3389, 5687, 0x127f},	/* index 42  -0.058884     */
	{0x326e, 5656, 0x127f},	/* index 43  0.129257      */
	{0x3159, 5625, 0x117f},	/* index 44  -0.023266     */
	{0x304b, 5593, 0x117f},	/* index 45  0.164879      */
	{0x2f42, 5562, 0x107f},	/* index 46  -0.001549     */
	{0x2e3e, 5531, 0x0f7f},	/* index 47  -0.183079     */
	{0x2d41, 5500, 0x0f7f},	/* index 48  0.005066      */
	{0x2c48, 5468, 0x0e7f},	/* index 49  -0.192890     */
	{0x2b56, 5437, 0x0e7f},	/* index 50  -0.004745     */
	{0x2a68, 5406, 0x0e7f},	/* index 51  0.183395      */
	{0x297f, 5375, 0x0d7f},	/* index 52  -0.032528     */
	{0x289c, 5343, 0x0d7f},	/* index 53  0.155617      */
	{0x27bd, 5312, 0x0c7f},	/* index 54  -0.080027     */
	{0x26e3, 5281, 0x0c7f},	/* index 55  0.108118      */
	{0x260d, 5250, 0x0b7f},	/* index 56  -0.149267     */
	{0x253d, 5218, 0x0b7f},	/* index 57  0.038877      */
	{0x2470, 5187, 0x0b7f},	/* index 58  0.227022      */
	{0x23a9, 5156, 0x0a7f},	/* index 59  -0.054457     */
	{0x22e5, 5125, 0x0a7f},	/* index 60  0.133688      */
	{0x2226, 5093, 0x097f},	/* index 61  -0.174641     */
	{0x216a, 5062, 0x097f},	/* index 62  0.013504      */
	{0x20b3, 5031, 0x097f},	/* index 63  0.201647      */
	{0x2000, 5000, 0x087f},	/* index 64  -0.136789    : 30dB */
	{0x1f50, 4968, 0x087f},	/* index 65  0.051353      */
	{0x1ea4, 4937, 0x087f},	/* index 66  0.239498      */
	{0x1dfc, 4906, 0x077f},	/* index 67  -0.132933     */
	{0x1d58, 4875, 0x077f},	/* index 68  0.055212      */
	{0x1cb7, 4843, 0x077f},	/* index 69  0.243355      */
	{0x1c19, 4812, 0x067f},	/* index 70  -0.167765     */
	{0x1b7f, 4781, 0x067f},	/* index 71  0.020378      */
	{0x1ae8, 4750, 0x067f},	/* index 72  0.208523      */
	{0x1a55, 4718, 0x057f},	/* index 73  -0.247028     */
	{0x19c4, 4687, 0x057f},	/* index 74  -0.058884     */
	{0x1937, 4656, 0x057f},	/* index 75  0.129259      */
	{0x18ac, 4625, 0x057f},	/* index 76  0.317404      */
	{0x1825, 4593, 0x047f},	/* index 77  -0.189695     */
	{0x17a1, 4562, 0x047f},	/* index 78  -0.001551     */
	{0x171f, 4531, 0x047f},	/* index 79  0.186592      */
	{0x16a0, 4500, 0x047f},	/* index 80  0.374737      */
	{0x1624, 4468, 0x037f},	/* index 81  -0.192892     */
	{0x15ab, 4437, 0x037f},	/* index 82  -0.004747     */
	{0x1534, 4406, 0x037f},	/* index 83  0.183395      */
	{0x14bf, 4375, 0x037f},	/* index 84  0.371540      */
	{0x144e, 4343, 0x027f},	/* index 85  -0.268171     */
	{0x13de, 4312, 0x027f},	/* index 86  -0.080027     */
	{0x1371, 4281, 0x027f},	/* index 87  0.108116      */
	{0x1306, 4250, 0x027f},	/* index 88  0.296261      */
	{0x129e, 4218, 0x017f},	/* index 89  -0.430746     */
	{0x1238, 4187, 0x017f},	/* index 90  -0.242601     */
	{0x11d4, 4156, 0x017f},	/* index 91  -0.054459     */
	{0x1172, 4125, 0x017f},	/* index 92  0.133686      */
	{0x1113, 4093, 0x017f},	/* index 93  0.321829      */
	{0x10b5, 4062, 0x017f},	/* index 94  0.509974      */
	{0x1059, 4031, 0x007f},	/* index 95  -0.324934     */
	{0x1000, 4000, 0x0860},	/* index 96  0.000000     : 24dB */
	{0x0fa8, 3968, 0x007f},	/* index 97  0.051353      */
	{0x0f52, 3937, 0x007d},	/* index 98  -0.040716     */
	{0x0efe, 3906, 0x007c},	/* index 99  0.003855      */
	{0x0eac, 3875, 0x007b},	/* index 100 0.046015      */
	{0x0e5b, 3843, 0x0079},	/* index 101 -0.065386     */
	{0x0e0c, 3812, 0x0078},	/* index 102 -0.030977     */
	{0x0dbf, 3781, 0x0077},	/* index 103 0.000660      */
	{0x0d74, 3750, 0x0076},	/* index 104 0.029425      */
	{0x0d2a, 3718, 0x0075},	/* index 105 0.055210      */
	{0x0ce2, 3687, 0x0074},	/* index 106 0.077904      */
	{0x0c9b, 3656, 0x0072},	/* index 107 -0.074619     */
	{0x0c56, 3625, 0x0071},	/* index 108 -0.061953     */
	{0x0c12, 3593, 0x0070},	/* index 109 -0.052906     */
	{0x0bd0, 3562, 0x006f},	/* index 110 -0.047630     */
	{0x0b8f, 3531, 0x006e},	/* index 111 -0.046288     */
	{0x0b50, 3500, 0x006d},	/* index 112 -0.049049     */
	{0x0b12, 3468, 0x006c},	/* index 113 -0.056101     */
	{0x0ad5, 3437, 0x006b},	/* index 114 -0.067642     */
	{0x0a9a, 3406, 0x006a},	/* index 115 -0.083881     */
	{0x0a5f, 3375, 0x006a},	/* index 116 0.104261      */
	{0x0a27, 3343, 0x0069},	/* index 117 0.083097      */
	{0x09ef, 3312, 0x0068},	/* index 118 0.056763      */
	{0x09b8, 3281, 0x0067},	/* index 119 0.025000      */
	{0x0983, 3250, 0x0066},	/* index 120 -0.012478     */
	{0x094f, 3218, 0x0065},	/* index 121 -0.055971     */
	{0x091c, 3187, 0x0064},	/* index 122 -0.105812     */
	{0x08ea, 3156, 0x0064},	/* index 123 0.082333      */
	{0x08b9, 3125, 0x0063},	/* index 124 0.025785      */
	{0x0889, 3093, 0x0062},	/* index 125 -0.037851     */
	{0x085a, 3062, 0x0061},	/* index 126 -0.109009     */
	{0x082c, 3031, 0x0061},	/* index 127 0.079136      */
	{0x0800, 3000, 0x0060},	/* index 128 0.000000      : 18dB */
	{0x07d4, 2968, 0x003f},	/* index 129 0.051355      */
	{0x07a9, 2937, 0x003d},	/* index 130 -0.040716     */
	{0x077f, 2906, 0x003c},	/* index 131 0.003857      */
	{0x0756, 2875, 0x003b},	/* index 132 0.046015      */
	{0x072d, 2843, 0x0039},	/* index 133 -0.065384     */
	{0x0706, 2812, 0x0038},	/* index 134 -0.030977     */
	{0x06df, 2781, 0x0037},	/* index 135 0.000662      */
	{0x06ba, 2750, 0x0036},	/* index 136 0.029425      */
	{0x0695, 2718, 0x0035},	/* index 137 0.055212      */
	{0x0671, 2687, 0x0034},	/* index 138 0.077904      */
	{0x064d, 2656, 0x0032},	/* index 139 -0.074618     */
	{0x062b, 2625, 0x0031},	/* index 140 -0.061954     */
	{0x0609, 2593, 0x0030},	/* index 141 -0.052905     */
	{0x05e8, 2562, 0x002f},	/* index 142 -0.047629     */
	{0x05c7, 2531, 0x002e},	/* index 143 -0.046287     */
	{0x05a8, 2500, 0x002d},	/* index 144 -0.049048     */
	{0x0589, 2468, 0x002c},	/* index 145 -0.056102     */
	{0x056a, 2437, 0x002b},	/* index 146 -0.067642     */
	{0x054d, 2406, 0x002a},	/* index 147 -0.083882     */
	{0x052f, 2375, 0x002a},	/* index 148 0.104261      */
	{0x0513, 2343, 0x0029},	/* index 149 0.083097      */
	{0x04f7, 2312, 0x0028},	/* index 150 0.056763      */
	{0x04dc, 2281, 0x0027},	/* index 151 0.025000      */
	{0x04c1, 2250, 0x0026},	/* index 152 -0.012477     */
	{0x04a7, 2218, 0x0025},	/* index 153 -0.055971     */
	{0x048e, 2187, 0x0024},	/* index 154 -0.105812     */
	{0x0475, 2156, 0x0024},	/* index 155 0.082332      */
	{0x045c, 2125, 0x0023},	/* index 156 0.025786      */
	{0x0444, 2093, 0x0022},	/* index 157 -0.037852     */
	{0x042d, 2062, 0x0021},	/* index 158 -0.109008     */
	{0x0416, 2031, 0x0021},	/* index 159 0.079136      */
	{0x0400, 2000, 0x0020},	/* index 160 0.000000     : 12dB */
	{0x03ea, 1968, 0x0219},	/* index 161 -0.017856     */
	{0x03d4, 1937, 0x0513},	/* index 162 0.065427      */
	{0x03bf, 1906, 0x001e},	/* index 163 0.003857      */
	{0x03ab, 1875, 0x011a},	/* index 164 -0.027907     */
	{0x0396, 1843, 0x0217},	/* index 165 0.010476      */
	{0x0383, 1812, 0x0119},	/* index 166 0.007713      */
	{0x036f, 1781, 0x0216},	/* index 167 0.000660      */
	{0x035d, 1750, 0x001b},	/* index 168 0.029426      */
	{0x034a, 1718, 0x0215},	/* index 169 -0.027120     */
	{0x0338, 1687, 0x0117},	/* index 170 0.036045      */
	{0x0326, 1656, 0x0019},	/* index 171 -0.074618     */
	{0x0315, 1625, 0x0116},	/* index 172 0.026229      */
	{0x0304, 1593, 0x0018},	/* index 173 -0.052906     */
	{0x02f4, 1562, 0x0115},	/* index 174 -0.001551     */
	{0x02e3, 1531, 0x0017},	/* index 175 -0.046287     */
	{0x02d4, 1500, 0x050e},	/* index 176 0.046928      */
	{0x02c4, 1468, 0x0016},	/* index 177 -0.056103     */
	{0x02b5, 1437, 0x0113},	/* index 178 -0.118290     */
	{0x02a6, 1406, 0x0945},	/* index 179 0.018909      */
	{0x0297, 1375, 0x030f},	/* index 180 -0.052246     */
	{0x0289, 1343, 0x0112},	/* index 181 -0.023480     */
	{0x027b, 1312, 0x0845},	/* index 182 0.056762      */
	{0x026e, 1281, 0x040d},	/* index 183 0.024999      */
	{0x0260, 1250, 0x0013},	/* index 184 -0.012477     */
	{0x0253, 1218, 0x0745},	/* index 185 0.060619      */
	{0x0247, 1187, 0x0d07},	/* index 186 0.073286      */
	{0x023a, 1156, 0x030d},	/* index 187 0.021802      */
	{0x022e, 1125, 0x0247},	/* index 188 0.025786      */
	{0x0222, 1093, 0x0011},	/* index 189 -0.037852     */
	{0x0216, 1062, 0x0b07},	/* index 190 -0.043454     */
	{0x020b, 1031, 0x020d},	/* index 191 -0.053476     */
	{0x0200, 1000, 0x0010},	/* index 192 0.000000      : 6dB */
	{0x01f5, 968, 0x0a07},	/* index 193 0.051355      */
	{0x01ea, 937, 0x030b},	/* index 194 -0.112205     */
	{0x01df, 906, 0x0445},	/* index 195 0.003856      */
	{0x01d5, 875, 0x010d},	/* index 196 -0.027907     */
	{0x01cb, 843, 0x0f05},	/* index 197 0.010476      */
	{0x01c1, 812, 0x000e},	/* index 198 -0.030976     */
	{0x01b7, 781, 0x0e05},	/* index 199 0.000660      */
	{0x01ae, 750, 0x0a06},	/* index 200 0.029425      */
	{0x01a5, 718, 0x0d05},	/* index 201 -0.027120     */
	{0x019c, 687, 0x000d},	/* index 202 0.077905      */
	{0x0193, 656, 0x020a},	/* index 203 -0.074618     */
	{0x018a, 625, 0x010b},	/* index 204 0.026230      */
	{0x0182, 593, 0x000c},	/* index 205 -0.052906     */
	{0x017a, 562, 0x0b05},	/* index 206 0.044285      */
	{0x0171, 531, 0x0f04},	/* index 207 -0.046286     */
	{0x016a, 500, 0x0507},	/* index 208 0.046928      */
	{0x0162, 468, 0x000b},	/* index 209 -0.056102     */
	{0x015a, 437, 0x0e04},	/* index 210 0.132041      */
	{0x0153, 406, 0x0905},	/* index 211 0.018910      */
	{0x014b, 375, 0x0407},	/* index 212 0.104261      */
	{0x0144, 343, 0x0109},	/* index 213 -0.023480     */
	{0x013d, 312, 0x0805},	/* index 214 0.056763      */
	{0x0137, 281, 0x0506},	/* index 215 0.024999      */
	{0x0130, 250, 0x0b04},	/* index 216 -0.012478     */
	{0x0129, 218, 0x0705},	/* index 217 0.060619      */
	{0x0123, 187, 0x0009},	/* index 218 -0.105812     */
	{0x011d, 156, 0x0406},	/* index 219 0.082332      */
	{0x0117, 125, 0x0207},	/* index 220 0.025786      */
	{0x0111, 93, 0x0904},	/* index 221 -0.037852     */
	{0x010b, 62, 0x0306},	/* index 222 -0.109008     */
	{0x0105, 31, 0x0505},	/* index 223 -0.053476     */
	{0x0100, 0, 0x0008},	/* index 224 0.000000      : 0dB */
	{0x00fa, -31, 0x0107},	/* index 225  0.051355    */
	{0x00f5, -62, 0x0206},	/* index 226  -0.184287   */
	{0x00ef, -93, 0x0405},	/* index 227  0.003856    */
	{0x00ea, -125, 0x0405},	/* index 228  0.192000    */
	{0x00e5, -156, 0x0604},	/* index 229  -0.219120   */
	{0x00e0, -187, 0x0604},	/* index 230 -0.030976    */
	{0x00db, -218, 0x0305},	/* index 231 0.000660     */
	{0x00d7, -250, 0x0106},	/* index 232 0.029426     */
	{0x00d2, -281, 0x0504},	/* index 233 -0.110239    */
	{0x00ce, -312, 0x0504},	/* index 234 0.077905     */
	{0x00c9, -343, 0x0205},	/* index 235 -0.074618    */
	{0x00c5, -375, 0x0205},	/* index 236 0.113525     */
	{0x00c1, -406, 0x0006},	/* index 237 -0.052906    */
	{0x00bd, -437, 0x0006},	/* index 238 0.135238     */
	{0x00b8, -468, 0x0105},	/* index 239 -0.237193    */
	{0x00b5, -500, 0x0105},	/* index 240 -0.049050    :-3dB */
	{0x00b1, -531, 0x0304},	/* index 241 -0.056102    */
	{0x00ad, -562, 0x0304},	/* index 242 0.132041     */
	{0x00a9, -593, 0x0304},	/* index 243 0.320185     */
	{0x00a5, -625, 0x0204},	/* index 244 -0.319525    */
	{0x00a2, -656, 0x0204},	/* index 245 -0.131381    */
	{0x009e, -687, 0x0005},	/* index 246 0.056763     */
	{0x009b, -718, 0x0005},	/* index 247 0.244906     */
	{0x0098, -750, 0x0005},	/* index 248 0.433050     */
	{0x0094, -781, 0x0104},	/* index 249 -0.293956    */
	{0x0091, -812, 0x0104},	/* index 250 -0.105812    */
	{0x008e, -843, 0x0104},	/* index 251 0.082332     */
	{0x008b, -875, 0x0104},	/* index 252 0.270475     */
	{0x0088, -906, 0x0104},	/* index 253 0.458619     */
	{0x0085, -937, 0x0004},	/* index 254  -0.376287   */
	{0x0082, -968, 0x0004}	/* index 255  -0.188144   */

};

");
	}
	print(OUTFILE @outLines);
	close OUTFILE;
	undef( @outLines );
}


#produce the video table file
sub produce_video_table
{
	my @outLines;  # Data we are going to output
	open(OUTFILE, ">$_[0]") or die "Can not open $_[0]:$!\n";
	push(@outLines, $fileheader);
	push(@outLines, "
/* the table is used to provide video info to application */
static const struct $sensor_name\E_video_info $sensor_name\E_video_info_table[] = {
	[0] = {
		.format_index	= 0,	/* select $sensor_name\E_video_format_tbl */
		.fps_index	= 0,	/* select fps_table */
		.def_start_x	= 0,	/* tell amba soc the capture x start offset */
		.def_start_y	= 0,	/* tell amba soc the capture y start offset */
		.def_width	= 1920,
		.def_height	= 1080,
		.sync_start	= 0,
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},

	[1] = {
		.format_index	= 1,
		.fps_index	= 0,
		.def_start_x	= 0,
		.def_start_y	= 0,
		.def_width	= 1280,
		.def_height	= 720,
		.sync_start	= 0,
		.bayer_pattern	= AMBA_VIN_SRC_BAYER_PATTERN_GR,
		},
	/*  >> add more video info here, if necessary << */

};

#define \U$sensor_name\E_VIDEO_INFO_TABLE_SZIE	ARRAY_SIZE($sensor_name\E_video_info_table)
#define \U$sensor_name\E_DEFAULT_VIDEO_INDEX	(0)

struct $sensor_name\E_video_mode $sensor_name\E_video_mode_table[] = {
	{AMBA_VIDEO_MODE_AUTO,
	0, /* select the index from above info table */
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0, /* select the index from above info table */
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	{AMBA_VIDEO_MODE_720P,
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL),
	0,
	(AMBA_VIN_SRC_ENABLED_FOR_VIDEO | AMBA_VIN_SRC_ENABLED_FOR_STILL)},

	\/\*  << add other video mode here if necessary >> \*\/
};

#define \U$sensor_name\E_VIDEO_MODE_TABLE_SZIE	ARRAY_SIZE($sensor_name\E_video_mode_table)
#define \U$sensor_name\E_VIDEO_MODE_TABLE_AUTO	AMBA_VIDEO_MODE_720P
\n");

	print(OUTFILE @outLines);
	close OUTFILE;
	undef( @outLines );
}

#produce the amba configfile
sub produce_ambaconfig
{
	my @tmplines;
	my @outLines;  # Data we are going to output
	my $line;
	open(OUTFILE, ">$_[0]") or die "Can not open $_[0]:$!\n";
	@tmplines = split/\n/,$fileheader;
	while($line = shift(@tmplines)){
		$line  =~ s/^ \*/##/i;
		$line  =~ s/^\/\*/##/i;
		$line  =~ s/^ \*\//##/i;
		push @outLines, "$line\n";
		#print "$line\n";
	}

	push(@outLines, "
menu \"\u\L$company\E Sensor\"
depends on CONFIG_SENSOR_\U$sensor_name\E
endmenu
\n");

	print(OUTFILE @outLines);
	close OUTFILE;
	undef( @outLines );
}


#produce the kbuildfile
sub produce_kbuild
{
	my @tmplines;
	my @outLines;  # Data we are going to output
	my $line;
	open(OUTFILE, ">$_[0]") or die "Can not open $_[0]:$!\n";
	@tmplines = split/\n/,$fileheader;
	while($line = shift(@tmplines)){
		$line  =~ s/^ \*/##/i;
		$line  =~ s/^\/\*/##/i;
		$line  =~ s/^ \*\//##/i;
		push @outLines, "$line\n";
		#print "$line\n";
	}

	push(@outLines, "

EXTRA_CFLAGS		+= \$(PRIVATE_CFLAGS) -I\$(AMBABUILD_TOPDIR)/kernel/private/drivers/ambarella/vin
EXTRA_AFLAGS		+= \$(PRIVATE_AFLAGS)
EXTRA_LDFLAGS		+= \$(PRIVATE_LDFLAGS)
EXTRA_ARFLAGS		+= \$(PRIVATE_ARFLAGS)

	obj-m := $sensor_name\E.o

");

	print(OUTFILE @outLines);
	close OUTFILE;
	undef( @outLines );
}

#produce the makefile
sub produce_make
{
	my @tmplines;
	my @outLines;  # Data we are going to output
	my $line;
	open(OUTFILE, ">$_[0]") or die "Can not open $_[0]:$!\n";
	@tmplines = split/\n/,$fileheader;
	while($line = shift(@tmplines)){
		$line  =~ s/^ \*/##/i;
		$line  =~ s/^\/\*/##/i;
		$line  =~ s/^ \*\//##/i;
		push @outLines, "$line\n";
		#print "$line\n";
	}

	push(@outLines, "
default: all

PWD				:= \$(shell pwd)
MODULE_DIR		:= \$(word 2, \$(subst /kernel/private/, ,\$(PWD)))
TMP_DIR	:= \$(shell echo ./\$(MODULE_DIR)|sed 's/\\\/[0-9a-zA-Z_]*/\\\/../g' )
AMBABUILD_TOPDIR?= \$(PWD)/\$(TMP_DIR)/../..
MODULE_NAME		:= \U$sensor_name\E

export AMBABUILD_TOPDIR

include \$(AMBABUILD_TOPDIR)/kernel/private/common.mk

.PHONY: all clean link_arch unlink_arch

all:
	\$(AMBA_MAKEFILE_V)\$(MAKE) build_linux_module

clean:
	\$(AMBA_MAKEFILE_V)\$(MAKE) clean_linux_module

link_arch:
	\$(MAKE) common_link_arch

unlink_arch:
	\$(MAKE) common_unlink_arch

");

	print(OUTFILE @outLines);
	close OUTFILE;
	undef( @outLines );
}


#produce the docmd file
sub produce_docmd
{
	my @outLines;  # Data we are going to output
	open(OUTFILE, ">$_[0]") or die "Can not open $_[0]:$!\n";
	push(@outLines, $fileheader);

	push(@outLines, "
static void $sensor_name\E_fill_video_format_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 format_index;
	struct $sensor_name\E_info *pinfo;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;

	vin_dbg(\"$sensor_name\E_fill_video_format_regs \\n\");
	index = pinfo->current_video_index;
	format_index = $sensor_name\E_video_info_table[index].format_index;

	for (i = 0; i < \U$sensor_name\E_VIDEO_FORMAT_REG_NUM; i++) {
		if ($sensor_name\E_video_format_tbl.reg[i] == 0)
			break;

		$sensor_name\E_write_reg(src,
				$sensor_name\E_video_format_tbl.reg[i],
				$sensor_name\E_video_format_tbl.table[format_index].data[i]);
	}

	if ($sensor_name\E_video_format_tbl.table[format_index].ext_reg_fill)
		$sensor_name\E_video_format_tbl.table[format_index].ext_reg_fill(src);

}

static void $sensor_name\E_fill_share_regs(struct __amba_vin_source *src)
{
	int i;
	const struct $sensor_name\E_reg_table *reg_tbl;
	struct $sensor_name\E_info *pinfo;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;
	reg_tbl = $sensor_name\E_share_regs;
	for (i = 0; i < \U$sensor_name\E_SHARE_REG_SZIE; i++) {
		$sensor_name\E_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void $sensor_name\E_sw_reset(struct __amba_vin_source *src)
{
		/* >> TODO << */
}

static void $sensor_name\E_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	$sensor_name\E_sw_reset(src);
}

static void $sensor_name\E_fill_video_fps_regs(struct __amba_vin_source *src)
{

	int i;
	u32 index;
	u32 fps_index;
	u32 format_index;
	struct $sensor_name\E_info *pinfo;
	const struct $sensor_name\E_video_fps_reg_table *fps_table;
	const struct $sensor_name\E_reg_table *pll_tbl;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;

	vin_dbg(\"$sensor_name\E_fill_video_fps_regs\\n\");
	index = pinfo->current_video_index;
	format_index = $sensor_name\E_video_info_table[index].format_index;
	fps_index = pinfo->fps_index;
	fps_table = $sensor_name\E_video_format_tbl.table[format_index].fps_table;
	pll_tbl = fps_table->table[fps_index].pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < \U$sensor_name\E_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			$sensor_name\E_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}

	for (i = 0; i < \U$sensor_name\E_VIDEO_FPS_REG_NUM; i++) {
		$sensor_name\E_write_reg(src, fps_table->reg[i], fps_table->table[fps_index].data[i]);
	}
}

static int $sensor_name\E_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct $sensor_name\E_info  *pinfo;

	pinfo = (struct $sensor_name\E_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int $sensor_name\E_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int errCode = 0;
	u32 index;
	u32 fps_index;
	u32 format_index;
	const struct $sensor_name\E_video_fps_reg_table *fps_table;
	struct $sensor_name\E_info *pinfo;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;

	index = pinfo->current_video_index;

	if (index >= \U$sensor_name\E_VIDEO_INFO_TABLE_SZIE) {
		p_video_info->width = 0;
		p_video_info->height = 0;
		p_video_info->fps = 0;
		p_video_info->format = 0;
		p_video_info->type = 0;
		p_video_info->bits = 0;
		p_video_info->ratio = 0;
		p_video_info->system = 0;
		p_video_info->rev = 0;

		errCode = -EPERM;
	} else {
		format_index = $sensor_name\E_video_info_table[index].format_index;
		fps_index = pinfo->fps_index;
		fps_table = $sensor_name\E_video_format_tbl.table[format_index].fps_table;

		p_video_info->width = $sensor_name\E_video_format_tbl.table[format_index].width;
		p_video_info->height = $sensor_name\E_video_format_tbl.table[format_index].height;
		p_video_info->fps = fps_table->table[fps_index].fps;
		p_video_info->format = $sensor_name\E_video_format_tbl.table[format_index].format;
		p_video_info->type = $sensor_name\E_video_format_tbl.table[format_index].type;
		p_video_info->bits = $sensor_name\E_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = $sensor_name\E_video_format_tbl.table[format_index].ratio;
		p_video_info->system = fps_table->table[fps_index].system;
		p_video_info->rev = 0;
	}

	return errCode;
}
");
	if($sensor_data_type){#RGB raw mode
	push(@outLines, "
static int $sensor_name\E_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct $sensor_name\E_info *pinfo;

	pinfo = (struct $sensor_name\E_info *)src->pinfo;
	p_agc_info = &(pinfo->agc_info);

	return errCode;
}
static int $sensor_name\E_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof (pshutter_info));
	return 0;
}
");
	}
	push(@outLines, "
static int $sensor_name\E_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i, j;
	struct $sensor_name\E_info *pinfo;
	u32 index;
	u32 fps_index;
	u32 format_index;
	const struct $sensor_name\E_video_fps_reg_table *fps_table;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < \U$sensor_name\E_VIDEO_MODE_TABLE_SZIE; i++) {
		if ($sensor_name\E_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = \U$sensor_name\E_VIDEO_MODE_TABLE_AUTO;
			p_mode_info->is_supported = 1;

			index = $sensor_name\E_video_mode_table[i].still_index;
			format_index = $sensor_name\E_video_info_table[index].format_index;
			fps_index = $sensor_name\E_video_info_table[index].fps_index;
			fps_table = $sensor_name\E_video_format_tbl.table[format_index].fps_table;

			p_mode_info->video_info.width = $sensor_name\E_video_info_table[index].def_width;
			p_mode_info->video_info.height = $sensor_name\E_video_info_table[index].def_height;
			p_mode_info->video_info.fps = fps_table->table[fps_index].fps;
			p_mode_info->video_info.format = $sensor_name\E_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type = $sensor_name\E_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits = $sensor_name\E_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio = $sensor_name\E_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system = fps_table->table[fps_index].system;
			p_mode_info->video_info.rev = 0;

			for (j = 0; j < AMBA_VIN_MAX_FPS_TABLE_SIZE; j++) {
				if (fps_table->table[j].pll_reg_table == NULL)
					break;

				amba_vin_source_set_fps_flag(p_mode_info, fps_table->table[j].fps);
			}

			break;
		}
	}

	return errCode;
}

static int $sensor_name\E_query_sensor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	struct $sensor_name\E_info *pinfo;
	int errCode = 0;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;

		/* >> TODO << */
	return errCode;

}

static int $sensor_name\E_query_sensor_version(struct __amba_vin_source *src, u16 * ver)
{
		/* >> TODO << */
	return 0;
}

static int $sensor_name\E_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct $sensor_name\E_info *pinfo;
	u32 format_index;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;

	if (index >= \U$sensor_name\E_VIDEO_INFO_TABLE_SZIE) {
		vin_err(\"$sensor_name\E_set_video_index do not support mode %d!\\n\", index);
		errCode = -EINVAL;
		goto $sensor_name\E_set_mode_exit;
	}

	errCode |= $sensor_name\E_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = $sensor_name\E_video_info_table[index].format_index;

	pinfo->cap_start_x = $sensor_name\E_video_info_table[index].def_start_x;
	pinfo->cap_start_y = $sensor_name\E_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = $sensor_name\E_video_format_tbl.table[format_index].width;
	pinfo->cap_cap_h = $sensor_name\E_video_format_tbl.table[format_index].height;
	pinfo->bayer_pattern = $sensor_name\E_video_info_table[index].bayer_pattern;
	pinfo->fps_index = $sensor_name\E_video_info_table[index].fps_index;
	$sensor_name\E_print_info(src);

	errCode |= $sensor_name\E_set_vin_mode(src);

	$sensor_name\E_fill_share_regs(src);

	$sensor_name\E_fill_video_fps_regs(src);

	$sensor_name\E_fill_video_format_regs(src);

	errCode |= $sensor_name\E_post_set_vin_mode(src);

	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

$sensor_name\E_set_mode_exit:
	return errCode;
}

static int $sensor_name\E_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int errCode = -EINVAL;
	int i;
	struct $sensor_name\E_info *pinfo;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	for (i = 0; i < \U$sensor_name\E_VIDEO_MODE_TABLE_SZIE; i++) {
		if ($sensor_name\E_video_mode_table[i].mode == mode) {
			errCode = $sensor_name\E_set_video_index(src, $sensor_name\E_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= \U$sensor_name\E_VIDEO_MODE_TABLE_SZIE) {
		vin_err(\"$sensor_name\E_set_video_mode do not support %d, %d!\\n\", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = $sensor_name\E_video_mode_table[i].mode;
		pinfo->mode_type = $sensor_name\E_video_mode_table[i].preview_mode_type;
	}

	return errCode;
}

static int $sensor_name\E_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int errCode = 0;
	struct $sensor_name\E_info *pinfo;
	u32 i;
	u32 index;
	u32 fps_index = -1;
	u32 format_index;
	const struct $sensor_name\E_video_fps_reg_table *fps_table;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= \U$sensor_name\E_VIDEO_INFO_TABLE_SZIE) {
		vin_err(\"$sensor_name\E_set_fps index = %d!\\n\", index);
		errCode = -EPERM;
		goto $sensor_name\E_set_fps_exit;
	}

	format_index = $sensor_name\E_video_info_table[index].format_index;
	fps_table = $sensor_name\E_video_format_tbl.table[format_index].fps_table;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps_index = 0;

	for (i = 0; i < AMBA_VIN_MAX_FPS_TABLE_SIZE; i++) {
		if (fps_table->table[i].pll_reg_table == NULL)
			break;

		if (fps_table->table[i].fps == fps) {
			fps_index = i;
			break;
		}
	}

	if (fps_index != -1) {
		pinfo->fps_index = fps_index;
		$sensor_name\E_fill_video_fps_regs(src);
	} else {
		errCode = -EINVAL;
	}
	$sensor_name\E_print_info(src);

$sensor_name\E_set_fps_exit:
	return errCode;
}

static int $sensor_name\E_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{

	return 0;

}

static int $sensor_name\E_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
		/* >> TODO, for RGB mode only << */
	return 0;

}

static int $sensor_name\E_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
");
	if($sensor_data_type){#for RGB mode output
	push(@outLines, "
	u64 exposure_time_q9;
	u32 line_length;
	//u8 shr_width_h, shr_width_l;
	int mode_index;
	//int num_time_h;
	u32 fps_index,format_index;

	struct $sensor_name\E_info *pinfo;
	const struct $sensor_name\E_video_fps_reg_table *fps_table;

	pinfo		= (struct $sensor_name\E_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= $sensor_name\E_video_info_table[mode_index].format_index;
	fps_index	= pinfo->fps_index;
	fps_table	= $sensor_name\E_video_format_tbl.table[format_index].fps_table;

	exposure_time_q9 = shutter_time;
	pinfo->current_shutter_time = shutter_time;

	line_length = 1656; /* TODO get line length in pixclk unit, e.g. read it from sensor reg */
	/* t(frame)/t(per line) = t(exposure, in lines)*/
	exposure_time_q9 = exposure_time_q9 * fps_table->table[fps_index].pll_reg_table->pixclk;

	if(unlikely(!line_length)){
		vin_err(\"line_length = 0\");
	}
	do_div(exposure_time_q9, line_length);
	do_div(exposure_time_q9, (512000000));

	/* TODO check if the exposure time is larger than 1/frame_rate */

	/* TODO set the exposure related registers here */

	//vin_dbg(\"shutter_width:\%d\\n\", num_time_h);

	return 0;
"); }else{
	push(@outLines, "
		/* >> For RGB mode only, not used in YUV mode << */
	return 0;
");}
	push(@outLines, "
}

static int $sensor_name\E_set_gain_db(struct __amba_vin_source *src, s32 agc_db)
{
");
	if($sensor_data_type){#for RGB mode
	push(@outLines, "
	u16 idc_reg;
	struct $sensor_name\E_info *pinfo;
	u32 gain_index;
	s32 db_max;
	s32 db_step;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;
	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	gain_index = (db_max - agc_db) / db_step;

	if (gain_index > \U$sensor_name\E_GAIN_0DB)
		gain_index = \U$sensor_name\E_GAIN_0DB;
	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {
		idc_reg = \U$sensor_name\E_GAIN_TABLE[gain_index][\U$sensor_name\E_GAIN_COL_REG];
		$sensor_name\E_write_reg(src, 0x00/*\U$sensor_name\E_GLOBAL_GAIN_REG*/, idc_reg);
		pinfo->current_gain_db = agc_db;

		return 0;
	} else{
		return -1;
	}

");
	}else{
	push(@outLines, "
		/* >> TODO, for RGB output mode only << */
	return 0;
");}
	push(@outLines, "
}
static int $sensor_name\E_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int errCode = 0;
	/* >> Here is a example for ov sensor, however some sensor,like imx035, does not support this feature << */
	/*
	uint readmode;
	struct $sensor_name\E_info *pinfo;
	u8 tmp_reg;
	u8 vstartLSBs;

	pinfo = (struct $sensor_name\E_info *) src->pinfo;
	switch (mirror_mode->pattern) {
	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = $sensor_name\E_MIRROR_ROW + $sensor_name\E_MIRROR_COLUMN;
		vstartLSBs = 01;
		break;
	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		readmode = $sensor_name\E_MIRROR_ROW;
		vstartLSBs = 01;
		break;
	case AMBA_VIN_SRC_MIRROR_VERTICALLY:
		readmode = $sensor_name\E_MIRROR_COLUMN;
		vstartLSBs = 00;
		break;
	case AMBA_VIN_SRC_MIRROR_NONE:
		readmode = 0;
		vstartLSBs = 00;
		break;
	default:
		vin_err(\"do not support cmd mirror mode\\n\");
		return -EINVAL;
		break;
	}
	pinfo->bayer_pattern = mirror_mode->bayer_pattern;
	errCode |= $sensor_name\E_read_reg(src, $sensor_name\E_TIMING_CONTROL18, &tmp_reg);
	tmp_reg &= (~$sensor_name\E_MIRROR_MASK);
	tmp_reg |= readmode;
	errCode |= $sensor_name\E_write_reg(src, $sensor_name\E_TIMING_CONTROL18, tmp_reg);
	*/
	return errCode;
}
static int $sensor_name\E_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
		/* >> TODO, for YUV output mode only << */
	return errCode;
}
static int $sensor_name\E_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;
		/* >> TODO, for RGB raw output mode only << */
		/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}

static int $sensor_name\E_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;

	vin_dbg(\"\\t\\t---->cmd is %d\\n\", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_POWER:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_GET_INFO:
		{
			struct amba_vin_source_info *pub_src;

			pub_src = (struct amba_vin_source_info *) args;
			pub_src->id = src->id;
			pub_src->dev_type = src->dev_type;
			strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
			pub_src->total_channel_num = src->total_channel_num;
			pub_src->active_channel_id = src->active_channel_id;
			pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = $sensor_name\E_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = $sensor_name\E_get_video_info(src, (struct amba_video_info *) args);
		break;
");
	if($sensor_data_type){#for RGB output mode
	push(@outLines, "
	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = $sensor_name\E_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = $sensor_name\E_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;
");
	}
	push(@outLines, "
	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = $sensor_name\E_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = $sensor_name\E_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = $sensor_name\E_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = $sensor_name\E_set_still_mode(src, (struct amba_vin_src_still_info *) args);
		break;

	case AMBA_VIN_SRC_GET_BLC:
		errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_GET_SW_BLC, (void *) args);
		break;

	case AMBA_VIN_SRC_SET_BLC:
		errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_SW_BLC, (void *) args);
		break;

	case AMBA_VIN_SRC_GET_FRAME_RATE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_FRAME_RATE:
		errCode = $sensor_name\E_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = $sensor_name\E_set_gain_db(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = $sensor_name\E_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = $sensor_name\E_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = $sensor_name\E_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = $sensor_name\E_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		{
			u16 sen_id = 0;
			u16 sen_ver = 0;
			u32 *pdata = (u32 *) args;

			errCode = $sensor_name\E_query_sensor_id(src, &sen_id);
			if (errCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;
			errCode = $sensor_name\E_query_sensor_version(src, &sen_ver);
			if (errCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;

			*pdata = (sen_id << 16) | sen_ver;
		}
		exit_AMBA_VIN_SRC_TEST_GET_DEV_ID:
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			$reg_addr_width subaddr;
			$reg_data_width data = 0;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;

			errCode = $sensor_name\E_read_reg(src, subaddr, &data);

			reg_data->data = data;
		}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			$reg_addr_width subaddr;
			$reg_data_width data = 0;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;
			data = reg_data->data;

			errCode = $sensor_name\E_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = $sensor_name\E_set_slowshutter_mode(src, *(int *) args);
		break;

	default:
		vin_err(\"%s-%d do not support cmd %d!\\n\", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
");

	print(OUTFILE @outLines);
	close OUTFILE;
	undef( @outLines );
}






# end.
