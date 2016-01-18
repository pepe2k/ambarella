#!/bin/sh

## History:
##    2013/04/26
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

#############
# the definition can be changed for test
#
# VOUT_SIZE 		the list of test vout size
# VOUT_SIZE="480p 720p"

VOUT_SIZE="480p"

BUFFER4_SIZE="480p"
BUFFER4_TYPE="off"

###############################
test_encode_cmd=`which test_encode`

KEY_CHAR="rev"

if [ $1 ]; then
	sensor_name=`echo $1 | sed -e 's/-*//'`
else
	echo;echo "Usage:"
	echo "Please add the option: such as:"
	echo "$0 --imx172"
	exit
fi


FIRST_LOOP=1
FOLDER_NAME="/tmp/mode_switch_test"
RESOLUTION_LIST="/${FOLDER_NAME}/${sensor_name}_vin.list"
COMMAND_LIST="/${FOLDER_NAME}/${sensor_name}_command.list"


clear_switch_test()
{
	if [ -r $FOLDER_NAME ]; then
		rm  -r $FOLDER_NAME
	fi
	mkdir -p $FOLDER_NAME
}

check_cmd()
{
	if [ "x$test_encode_cmd" == "x" ]; then
		echo "Cannot find command test_encode, abort!!!"
		exit 1
	fi

	mount -o remount,rw /
	clear_switch_test
}

stop_test()
{

	if [ -r $RESOLUTION_LIST ]; then
		rm $RESOLUTION_LIST
	fi

}

check_feedback()
{
	if [ $? -gt 0 ]; then
		stop_test
		exit 1
	fi
}

list_resolution()
{
	echo;
	echo "============================================"
	echo "Start to do auto test";
	echo "============================================"
	echo;echo;echo;
	stop_test
	$test_encode_cmd -i0 -V480p --hdmi | grep Active -B 10 | grep ${KEY_CHAR} | sed '1d' >> $RESOLUTION_LIST
	check_feedback

	VIN_MODE_LIST=`cat ${RESOLUTION_LIST} | awk '{ print $1 }'| sed -e 's/P//g'`
}

# Step 1: system init with specified sensor
init_sensor()
{
	echo "# init.sh --"$sensor_name""
	init.sh --"$sensor_name"
}

# Step 2: initialize VIN and VOUT

show_preview_result()
{
	check_feedback

	echo;echo;echo "wait for 3 second"
	sleep 3
	echo;echo "Preview OK!"
	sleep 1
	echo;
}
check_enc_mode0()
{
	echo;echo;echo;
	echo "============================================"
	echo "Checking $vin_mode enc-mode 0."
	echo "============================================"
	echo;
	width=`echo $vin_mode | sed -e 's/x/ /' | awk '{ print $1}'`
	echo;

	if [ $width -ge "1920" ] ; then
		if [ $FIRST_LOOP -eq 1 ]; then
			echo "# $test_encode_cmd -i ${vin_mode} -V${vout_size} --hdmi -f 0 --enc-mode 0 --sharpen-b 1 -X --bsize 1920x1080 --bmaxsize 1920x1080 --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
		fi
			$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f 0 --enc-mode 0 --sharpen-b 1 -X --bsize 1920x1080 --bmaxsize 1920x1080 --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
	else
		if [ $FIRST_LOOP -eq 1 ]; then
			echo "# $test_encode_cmd -i $vin_mode -V${vout_size} -f $vin_fps --hdmi --enc-mode 0 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
		fi
			$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f $vin_fps --enc-mode 0 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
	fi

	show_preview_result
}

check_enc_mode1()
{
	echo;echo;echo;
	echo "============================================"
	echo "Checking $vin_mode enc-mode 1."
	echo "============================================"
	echo;
	width=`echo $vin_mode | sed -e 's/x/ /' | awk '{ print $1}'`
	height=`echo $vin_mode | sed -e 's/x/ /' | awk '{ print $2}'`

	echo;
	if [ ${height} -ge "2048" ] ; then
		if [ $FIRST_LOOP -eq 1 ]; then
			echo "# $test_encode_cmd -i $vin_mode -V${vout_size} --hdmi --mixer 0 --enc-mode 1 -X --bsize 2048x2048 --bmaxsize 2048x2048 -J --btype off -K --btype ${BUFFER4_TYPE} --bsize ${vout_size} -P --bsize 2048x2048 --vout-swap 1 -w 2048" >> ${COMMAND_LIST}
		fi
		$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi --mixer 0 --enc-mode 1 -X --bsize 2048x2048 --bmaxsize 2048x2048 -J --btype off -K --btype ${BUFFER4_TYPE} --bsize ${vout_size} -P --bsize 2048x2048 --vout-swap 1 -w 2048
	else
		if [ ${width} -ge "2048" ] ; then
			if [ $FIRST_LOOP -eq 1 ]; then
				echo "# $test_encode_cmd -i $vin_mode -V${vout_size} --hdmi --mixer 0 --enc-mode 1 -X --bsize 2048x${height} --bmaxsize 2048x${height} -J --btype off -K --btype ${BUFFER4_TYPE} --bsize ${vout_size} -P --bsize 2048x${height} --vout-swap 1 -w 2048" >> ${COMMAND_LIST}
			fi
			$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi --mixer 0 --enc-mode 1 -X --bsize 2048x${height} --bmaxsize 2048x${height} -J --btype off -K --btype ${BUFFER4_TYPE} --bsize ${vout_size} -P --bsize 2048x${height} --vout-swap 1 -w 2048
		else
			if [ $FIRST_LOOP -eq 1 ]; then
				echo "# $test_encode_cmd -i $vin_mode -V${vout_size} --hdmi --mixer 0 --enc-mode 1 -X --bsize ${width}x${height} --bmaxsize ${width}x${height} -J --btype off -K --btype ${BUFFER4_TYPE} --bsize ${vout_size} -P --bsize ${width}x${height} --vout-swap 1 -w ${width}" >> ${COMMAND_LIST}
			fi
			$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi --mixer 0 --enc-mode 1 -X --bsize ${width}x${height} --bmaxsize ${width}x${height} -J --btype off -K --btype ${BUFFER4_TYPE} --bsize ${vout_size} -P --bsize ${width}x${height} --vout-swap 1 -w ${width}
		fi
	fi

	show_preview_result
}

check_enc_mode2()
{
	echo;echo;echo;
	echo "============================================"
	echo "Checking $vin_mode enc-mode 2."
	echo "============================================"
	echo;
	width=`echo $vin_mode | sed -e 's/x/ /' | awk '{ print $1}'`
	height=`echo $vin_mode | sed -e 's/x/ /' | awk '{ print $2}'`
	echo;

	if [ ${height} -ge "3000" ] ; then
		if [ $FIRST_LOOP -eq 1 ]; then
			echo "# $test_encode_cmd -i ${vin_mode} -V${vout_size} --hdmi -f 15 --enc-mode 2 --sharpen-b 1 -X --bsize 3840x2160 --bmaxsize 3840x2160 --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
		fi
		$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f 15 --enc-mode 2 --sharpen-b 1 -X --bsize 3840x2160 --bmaxsize 3840x2160 --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
	else
		if [ ${width} -ge "2716" ] ; then
			if [ $FIRST_LOOP -eq 1 ]; then
				echo "# $test_encode_cmd -i ${vin_mode} -V${vout_size} --hdmi -f 15 --enc-mode 2 --sharpen-b 1 -X --bsize ${vin_mode} --bmaxsize ${vin_mode} --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
			fi
			$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f 15 --enc-mode 2 --sharpen-b 1 -X --bsize ${vin_mode} --bmaxsize ${vin_mode} --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
		else
			if [ ${width} -ge "1920" ] ; then
				if [ $vin_fps -ge "60" ] ; then
					if [ $FIRST_LOOP -eq 1 ]; then
						echo "# $test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f 60 --enc-mode 2 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
					fi
					$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f 60 --enc-mode 2 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
				else
					if [ $FIRST_LOOP -eq 1 ]; then
						echo "# $test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f $vin_fps --enc-mode 2 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
					fi
					$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f $vin_fps --enc-mode 2 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
				fi
			else
				if [ $FIRST_LOOP -eq 1 ]; then
					echo "# $test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f $vin_fps --enc-mode 2 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
				fi
				$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f $vin_fps --enc-mode 2 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0

			fi
		fi
	fi
	show_preview_result
}

check_enc_mode6()
{
	echo;echo;echo;
	echo "============================================"
	echo "Checking $vin_mode enc-mode 6."
	echo "============================================"
	echo;
	width=`echo $vin_mode | sed -e 's/x/ /' | awk '{ print $1}'`
	height=`echo $vin_mode | sed -e 's/x/ /' | awk '{ print $2}'`
	echo;
	if [ ${height} -ge "3000" ] ; then
		if [ $FIRST_LOOP -eq 1 ]; then
			echo "# $test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f 25 --enc-mode 6 --sharpen-b 0 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
		fi
		$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f 20 --enc-mode 6 --sharpen-b 0 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
	else
		if [ ${width} -ge "3840" ] ; then
			if [ $FIRST_LOOP -eq 1 ]; then
				echo "# $test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f 30 --enc-mode 6 --sharpen-b 0 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
			fi
			$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f 30 --enc-mode 6 --sharpen-b 0 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
		else
			if [ $FIRST_LOOP -eq 1 ]; then
				echo "# $test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f $vin_fps --enc-mode 6 --sharpen-b 0 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
			fi
			$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f $vin_fps --enc-mode 6 --sharpen-b 0 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
		fi
	fi

	show_preview_result
}

check_enc_mode7()
{
	echo;echo;echo;
	echo "============================================"
	echo "Checking $vin_mode enc-mode 7."
	echo "============================================"
	echo;
	width=`echo $vin_mode | sed -e 's/x/ /' | awk '{ print $1}'`
	echo;

	if [ $width -gt "1920" ] ; then
		if [ $FIRST_LOOP -eq 1 ]; then
			echo "# $test_encode_cmd -i ${vin_mode} -V${vout_size} --hdmi -f 0 --enc-mode 7 --sharpen-b 1 -X --bsize 1920x1080 --bmaxsize 1920x1080 --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
		fi
		$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f 0 --enc-mode 7 --sharpen-b 1 -X --bsize 1920x1080 --bmaxsize 1920x1080 --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
	else
		if [ $width -lt "1920"  ]; then
			if [ $FIRST_LOOP -eq 1 ]; then
				echo "# $test_encode_cmd -i $vin_mode -V${vout_size} -f $vin_fps --hdmi --enc-mode 7 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype enc -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
			fi
			$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f $vin_fps --enc-mode 7 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype enc -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
		else
			if [ $FIRST_LOOP -eq 1 ]; then
				echo "# $test_encode_cmd -i $vin_mode -V${vout_size} -f $vin_fps --hdmi --enc-mode 7 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0" >> ${COMMAND_LIST}
			fi
			$test_encode_cmd -i $vin_mode -V${vout_size} --hdmi -f $vin_fps --enc-mode 7 --sharpen-b 1 -X --bsize $vin_mode --bmaxsize $vin_mode --mixer 1 -J --btype prev -K --btype ${BUFFER4_TYPE} --bsize ${BUFFER4_SIZE} --bmaxsize ${BUFFER4_SIZE} --vout-swap 0
		fi
	fi

	show_preview_result
}

init_vinvout()
{
	BUFFER4_TYPE="off"
	check_enc_mode0
	check_enc_mode1
	check_enc_mode2
	check_enc_mode6
	check_enc_mode7

	BUFFER4_TYPE="prev"
	check_enc_mode1

	BUFFER4_TYPE="enc"
	check_enc_mode0
	check_enc_mode1
	check_enc_mode2
	check_enc_mode6
	check_enc_mode7

}

switch_encode_mode_test()
{
	echo;
	for vout_size in $VOUT_SIZE ; do
		for vin_mode in $VIN_MODE_LIST ; do
			VIN_FPS_LIST=`grep $vin_mode ${RESOLUTION_LIST} | awk '{ print $2 }'`
			for vin_fps in $VIN_FPS_LIST ; do
				init_vinvout
			done
		done
	done
}

loop_test()
{
	count=0
	while [ 1 ]; do
		echo;echo;echo "----count:$count ------"
		echo;echo;
		let count++
		switch_encode_mode_test
		FIRST_LOOP=0
	done

}

#_MAIN_
check_cmd
init_sensor
list_resolution
loop_test
stop_test

echo
echo; echo "Thanks for using Ambarella mode switch test tool. "
echo "For any questions please send mail to ipcam-sdk-support@ambarella.com"
