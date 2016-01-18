#!/bin/sh

##
## prebuild/imgproc/img_data/build_aaa_binary_header.sh
##
## History:
##    2012/12/10 - [Jingyang Qiu] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

#####$1: generate param files, $2 source S file,
#####$3 source C file,

if [ $1 ]; then
	TARGET_C_FILE=$1
else
	TARGET_C_FILE=
fi

if [ $2 ]; then
	SOURCE_S_FILE=$2
else
	SOURCE_S_FILE=
fi

if [ $3 ]; then
	SOURCE_C_FILE=$3
else
	SOURCE_C_FILE=
fi

tmp_aaa_file="${SOURCE_S_FILE}.tmp"

source_file_nodir=`echo ${TARGET_C_FILE} | sed 's/^.*\///g'`

parse_s_file()
{
	VAR_NAME_LIST=`grep ".globl" ${SOURCE_S_FILE} | awk '{ print $2}'`
	for var_name in $VAR_NAME_LIST
	do
		align_num=""
		for below_line in `seq 3`
		do
			align_num=`awk '/'${var_name}'/{a=NR}/align/&&NR==a+'$below_line'' ${SOURCE_S_FILE} | head -n 1 | awk '{print $2}'`
			if [ -n "$align_num" ] ; then
				break
		fi
	done

	if [ -z "$align_num" ] ; then
		align_num="1"
	fi

	size=`grep ".size" $SOURCE_S_FILE | grep ${var_name} | head -n 1 | awk '{print $3}'`
	echo "${var_name} ${align_num} ${size}" >> ${SOURCE_S_FILE}.list
	done
}

add_chroma_scale_info()
{
	struct_size=`grep chroma ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $3}'`
	struct_align=`grep chroma ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $2}'`
	struct_total_num=`grep -c chroma ${SOURCE_S_FILE}.list`
	echo "      [LIST_CHROMA_SCALE] = {" >> $tmp_aaa_file
	echo "        .struct_size = $struct_size," >> $tmp_aaa_file
	echo "        .struct_total_num = $struct_total_num," >> $tmp_aaa_file
	echo "        .struct_align = $struct_align," >> $tmp_aaa_file
	echo "      }," >> $tmp_aaa_file
}
add_rgb2yuv_info()
{
	struct_size=`grep rgb2yuv ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $3}'`
	struct_align=`grep rgb2yuv ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $2}'`
	struct_total_num=`grep -c rgb2yuv ${SOURCE_S_FILE}.list`
	echo "      [LIST_RGB2YUV] = {" >> $tmp_aaa_file
	echo "        .struct_size = $struct_size," >> $tmp_aaa_file
	echo "        .struct_total_num = $struct_total_num," >> $tmp_aaa_file
	echo "        .struct_align = $struct_align," >> $tmp_aaa_file
	echo "      }," >> $tmp_aaa_file
}
add_adjparam_info()
{
	struct_size=`grep adj_param ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $3}'`
	struct_align=`grep adj_param ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $2}'`
	struct_total_num=`grep -c adj_param ${SOURCE_S_FILE}.list`
	echo "      [LIST_ADJ_PARAM] = {" >> $tmp_aaa_file
	echo "        .struct_size = $struct_size," >> $tmp_aaa_file
	echo "        .struct_total_num = $struct_total_num," >> $tmp_aaa_file
	echo "        .struct_align = $struct_align," >> $tmp_aaa_file
	echo "      }," >> $tmp_aaa_file
}
add_manual_le_info()
{
	struct_size=`grep manual_LE ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $3}'`
	struct_align=`grep manual_LE ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $2}'`
	struct_total_num=`grep -c manual_LE ${SOURCE_S_FILE}.list`
	echo "      [LIST_MANUAL_LE] = {" >> $tmp_aaa_file
	echo "        .struct_size = $struct_size," >> $tmp_aaa_file
	echo "        .struct_total_num = $struct_total_num," >> $tmp_aaa_file
	echo "        .struct_align = $struct_align," >> $tmp_aaa_file
	echo "      }," >> $tmp_aaa_file
}

add_tile_config_info()
{
	struct_size=`grep tile_config ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $3}'`
	struct_align=`grep tile_config ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $2}'`
	struct_total_num=`grep -c tile_config ${SOURCE_S_FILE}.list`
	echo "      [LIST_TILE_CONFIG] = {" >> $tmp_aaa_file
	echo "        .struct_size = $struct_size," >> $tmp_aaa_file
	echo "        .struct_total_num = $struct_total_num," >> $tmp_aaa_file
	echo "        .struct_align = $struct_align," >> $tmp_aaa_file
	echo "      }," >> $tmp_aaa_file
}
add_lines_info()
{
	struct_size=`grep lines ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $3}'`
	struct_align=`grep lines ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $2}'`
	struct_total_num=`grep -c lines ${SOURCE_S_FILE}.list`
	echo "      [LIST_LINES] = {" >> $tmp_aaa_file
	echo "        .struct_size = $struct_size," >> $tmp_aaa_file
	echo "        .struct_total_num = $struct_total_num," >> $tmp_aaa_file
	echo "        .struct_align = $struct_align," >> $tmp_aaa_file
	echo "      }," >> $tmp_aaa_file
}

add_awb_param_info()
{
	struct_size=`grep awb_param ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $3}'`
	struct_align=`grep awb_param ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $2}'`
	struct_total_num=`grep -c awb_param ${SOURCE_S_FILE}.list`
	echo "      [LIST_AWB_PARAM] = {" >> $tmp_aaa_file
	echo "        .struct_size = $struct_size," >> $tmp_aaa_file
	echo "        .struct_total_num = $struct_total_num," >> $tmp_aaa_file
	echo "        .struct_align = $struct_align," >> $tmp_aaa_file
	echo "      }," >> $tmp_aaa_file
}
add_ae_agc_dgain_info()
{
	struct_size=`grep ae_agc_dgain ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $3}'`
	struct_align=`grep ae_agc_dgain ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $2}'`
	struct_total_num=`grep -c ae_agc_dgain ${SOURCE_S_FILE}.list`
	echo "      [LIST_AE_AGC_DGAIN] = {" >> $tmp_aaa_file
	echo "        .struct_size = $struct_size," >> $tmp_aaa_file
	echo "        .struct_total_num = $struct_total_num," >> $tmp_aaa_file
	echo "        .struct_align = $struct_align," >> $tmp_aaa_file
	echo "      }," >> $tmp_aaa_file
}
add_ae_sht_dgain_info()
{
	struct_size=`grep ae_sht_dgain ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $3}'`
	struct_align=`grep ae_sht_dgain ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $2}'`
	struct_total_num=`grep -c ae_sht_dgain ${SOURCE_S_FILE}.list`
	echo "      [LIST_AE_SHT_DGAIN] = {" >> $tmp_aaa_file
	echo "        .struct_size = $struct_size," >> $tmp_aaa_file
	echo "        .struct_total_num = $struct_total_num," >> $tmp_aaa_file
	echo "        .struct_align = $struct_align," >> $tmp_aaa_file
	echo "      }," >> $tmp_aaa_file
}
add_dlight_info()
{
	struct_size=`grep dlight ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $3}'`
	struct_align=`grep dlight ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $2}'`
	struct_total_num=`grep -c dlight ${SOURCE_S_FILE}.list`
	echo "      [LIST_DLIGHT] = {" >> $tmp_aaa_file
	echo "        .struct_size = $struct_size," >> $tmp_aaa_file
	echo "        .struct_total_num = $struct_total_num," >> $tmp_aaa_file
	echo "        .struct_align = $struct_align," >> $tmp_aaa_file
	echo "      }," >> $tmp_aaa_file
}
add_sensor_config_info()
{
	struct_size=`grep sensor_config ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $3}'`
	struct_align=`grep sensor_config ${SOURCE_S_FILE}.list | head -n 1 | awk '{ print $2}'`
	struct_total_num=`grep -c sensor_config ${SOURCE_S_FILE}.list`
	echo "      [LIST_SENSOR_CONFIG] = {" >> $tmp_aaa_file
	echo "        .struct_size = $struct_size," >> $tmp_aaa_file
	echo "        .struct_total_num = $struct_total_num," >> $tmp_aaa_file
	echo "        .struct_align = $struct_align," >> $tmp_aaa_file
	echo "      }," >> $tmp_aaa_file
}

###  generate aeb tmp header file for aeb_param.c ###
generate_aeb_header_file()
{
echo "" >> $tmp_aaa_file
echo "#include \"mw_aaa_params.h\"" >> $tmp_aaa_file
echo "" >> $tmp_aaa_file

echo "ambarella_adj_aeb_bin_parse_t tmp_for_parse = {" >> $tmp_aaa_file
echo "  .bin_header = {" >> $tmp_aaa_file
echo "    .magic_number = 0x12345678," >> $tmp_aaa_file
echo "    .header_ver_major = 0x32," >> $tmp_aaa_file
echo "    .header_ver_minor = 0x1," >> $tmp_aaa_file
echo "    .header_size = sizeof(ambarella_adj_aeb_bin_header_t)," >> $tmp_aaa_file
echo "  }," >> $tmp_aaa_file
echo "  .data_header = {" >> $tmp_aaa_file
echo "    .file_name = \"$source_file_nodir\"," >> $tmp_aaa_file
echo "    .struct_type = {" >> $tmp_aaa_file

add_tile_config_info
add_lines_info
add_awb_param_info
add_ae_agc_dgain_info
add_ae_sht_dgain_info
add_dlight_info
add_sensor_config_info

echo "    }," >> $tmp_aaa_file
echo "  }," >> $tmp_aaa_file
echo "};">> $tmp_aaa_file
}


###   generate aeb tmp header file for aeb_param.c ###
generate_adj_header_file()
{
echo "" >> $tmp_aaa_file
echo "#include \"mw_aaa_params.h\"" >> $tmp_aaa_file
echo "" >> $tmp_aaa_file

echo "ambarella_adj_aeb_bin_parse_t tmp_for_parse = {" >> $tmp_aaa_file
echo "  .bin_header = {" >> $tmp_aaa_file
echo "    .magic_number = 0x12345678," >> $tmp_aaa_file
echo "    .header_ver_major = 0x32," >> $tmp_aaa_file
echo "    .header_ver_minor = 0x1," >> $tmp_aaa_file
echo "    .header_size = sizeof(ambarella_adj_aeb_bin_header_t)," >> $tmp_aaa_file
echo "  }," >> $tmp_aaa_file
echo "  .data_header = {" >> $tmp_aaa_file
echo "    .file_name = \"$source_file_nodir\"," >> $tmp_aaa_file
echo "    .struct_type = {" >> $tmp_aaa_file

add_chroma_scale_info
add_rgb2yuv_info
add_adjparam_info
add_manual_le_info

echo "    }," >> $tmp_aaa_file
echo "  }," >> $tmp_aaa_file
echo "};">> $tmp_aaa_file
}


generate_file()
{
if [ `echo ${SOURCE_S_FILE} | grep "adj_param"` ] ; then
	parse_s_file
	generate_adj_header_file
	sed -e '/img_struct_arch.h/r '$tmp_aaa_file'' ${SOURCE_C_FILE} > ${TARGET_C_FILE}
elif [ `echo ${SOURCE_S_FILE} | grep "aeb_param"` ] ; then
	parse_s_file
	generate_aeb_header_file
	sed -e '/img_struct_arch.h/r '$tmp_aaa_file'' ${SOURCE_C_FILE} > ${TARGET_C_FILE}
fi
rm $tmp_aaa_file -f
rm ${SOURCE_S_FILE} -f
rm ${SOURCE_S_FILE}.list -f
}

generate_file

