#!/bin/sh

##
## packaes/img_mw/mw/arch_s2/src/build_lens_binary.sh
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

###   generate tmp header file for _param_adv.c ###
generate_adv_header_file()
{
echo "" >> $tmp_aaa_file
echo "#define SIZE_TOTAL_LISTS		32" >> $tmp_aaa_file
echo "const u32 size_lists[SIZE_TOTAL_LISTS] = {" >> $tmp_aaa_file

VAR_NAME_LIST=`grep ".size" ${SOURCE_S_FILE} | awk '{ print $3}'`
for var_name in $VAR_NAME_LIST
do
	echo "${var_name}," >>  $tmp_aaa_file
done

echo "};" >> $tmp_aaa_file
}

generate_file()
{
generate_adv_header_file
if [ `echo ${SOURCE_S_FILE} | grep "piris_param"` ] ; then
	sed -e '/img_struct_arch.h/r '$tmp_aaa_file'' ${SOURCE_C_FILE} > ${TARGET_C_FILE}
fi
rm $tmp_aaa_file -f
rm ${SOURCE_S_FILE} -f
rm ${SOURCE_S_FILE}.list -f
}

generate_file
