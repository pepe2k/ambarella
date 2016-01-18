#!/bin/sh

## History:
##    2013/08/09 [Qian Shen] created file
##    2013/12/05 [Shupeng Ren] modified file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

CALIBRATION_FILES_PATH=/ambarella/calibration

test_encode_cmd=`which test_encode`
cali_center_cmd=`which cali_fisheye_center`
test_image_cmd=`which test_image`
test_yuv_cmd=`which test_yuvcap`

check_cmd()
{
	if [ "x$test_encode_cmd" == "x" ]; then
		echo "Cannot find command test_encode, abort!!!"
		exit 1
	fi
	if [ "x$cali_center_cmd" == "x" ]; then
		echo "Cannot find command cali_fisheye_center, abort!!!"
		exit 1
	fi
	if [ "x$test_image_cmd" == "x" ]; then
		echo "Cannot find command test_image, abort!!!"
		exit 1
	fi
	if [ "x$test_yuv_cmd" == "x" ]; then
		echo "Cannot find command test_yuv, abort!!!"
		exit 1
	fi

	mount -o remount,rw /
	mkdir -p $CALIBRATION_FILES_PATH
}

check_feedback()
{
	if [ $? -gt 0 ]; then
		exit 1
	fi
}

init_setting()
{
	echo;
	echo "============================================"
	echo "Start to do fisheye center calibration";
	echo "============================================"

	accept=0
	while [ $accept != 1 ]
	do
		echo "1:	MT9T002 3M sensor"
		echo "2:	AR0330 3M sensor"
		echo "3:	AR0331 3M sensor"
		echo "4:	MN34041PL 2M sensor"
		echo "5:	IMX104 1M sensor"
		echo "6:	IMX136 2M sensor"
		echo "7:	IMX121 12M sensor"
		echo "8:	IMX172 12M sensor"
		echo "9:	IMX178 6M sensor"
		echo "a:	OV2710 2M sensor"
		echo; echo -n "choose sensor type:"
		read sensor_type
		case $sensor_type in
			1|2|3|4|5|6|7|8|9|a)
				accept=1
				;;
			*)
				accept=0
				echo;
				echo "Please input valid number, without any other keypress(backsapce or delete)!"
				;;
		esac
	done

	case "$sensor_type" in
		1	)
			sensor_name=mt9t002
			;;
		2	)
			sensor_name=ar0330
			;;
		3	)
			sensor_name=ar0331
			;;
		4	)
			sensor_name=mn34041pl
			;;
		5	)
			sensor_name=imx104
			;;
		6	)
			sensor_name=imx121
			;;
		7	)
			sensor_name=imx136
			;;
		8	)
			sensor_name=imx172
			;;
		9	)
			sensor_name=imx178
			;;
		a	)
			sensor_name=ov2710
			;;
		*	)
			echo "sensor type unrecognized!"
			exit 1
			;;
	esac

# Step 2: choose VIN mode
	accept=0
	while [ $accept != 1 ]
	do
		echo "1:	2592x1944 (5MP)"
		echo "2:	2048x1536 (3MP)"
		echo "3:	1080p (2MP)"
		echo "4:	720p (1MP)"
		echo "5:	3840x2160 (8MP)"
		echo "6:	4096x2160 (8MP)"
		echo "7:	4000x3000 (12MP)"
		echo "8:	2304x1296 (3MP)"
		echo "9:	2304x1536 (3.4MP)"
		echo "a:	3072x2048 (6MP)"
		echo "b:	2560x2048 (5MP)"
		echo "c:	1536x1024 (1.6MP)"
		echo; echo -n "choose vin mode:"
		read vin_mode
		case $vin_mode in
			1|2|3|4|5|6|7|8|9|a|b|c)
				accept=1
				;;
			*)
				accept=0
				echo "Please input valid number, without any other keypress(backsapce or delete)!"
				;;
		esac
	done

	case "$vin_mode" in
		1	)
			width="2592"
			height="1944"
			enc_mode=0
			;;
		2	)
			width="2048"
			height="1536"
			enc_mode=0
			;;
		3	)
			width="1920"
			height="1080"
			enc_mode=0
			;;
		4	)
			width="1280"
			height="720"
			enc_mode=0
			;;
		5	)
			width="3840"
			height="2160"
			enc_mode=2
			;;
		6	)
			width="4096"
			height="2160"
			enc_mode=2
			;;
		7	)
			width="4000"
			height="3000"
			enc_mode=2
			;;
		8	)
			width="2304"
			height="1296"
			enc_mode=0
			;;
		9	)
			width="2304"
			height="1536"
			enc_mode=0
			;;
		a	)
			width="3072"
			height="2048"
			enc_mode=2
			;;
		b	)
			width="2560"
			height="2048"
			enc_mode=2
			;;
		c	)
			width="1536"
			height="1024"
			enc_mode=0
			;;
		*	)
			echo "vin mode unrecognized!"
			exit 1
			;;
	esac

	CALIBRATION_FILE_PREFIX=cali_fisheye_center
	CALIBRATION_FILE_PREFIX=${CALIBRATION_FILES_PATH}/${CALIBRATION_FILE_PREFIX}
	CALIBRATION_FILE_NAME=${CALIBRATION_FILE_PREFIX}_prev_M_${width}x${height}.yuv

	if [ -e ${CALIBRATION_FILE_PREFIX} ]; then
		rm -f ${CALIBRATION_FILE_PREFIX}
	fi

}

# Step 3: system init with specified sensor
init_sensor()
{
	echo "# init.sh --"$sensor_name""
	init.sh --"$sensor_name"
}

# Step 4: initialize VIN and VOUT
init_vinvout()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 1: switch to encode mode $enc_mode."
	echo;echo "if VIN >= 8M, set 15fps for the DSP bandwidth limitation"
	echo "============================================"
	echo;
	killall test_idsp
	killall test_image
	if [ $width -ge "2716" ]; then
		exe_cmd=$test_encode_cmd" -i"$width"x"$height" -f 15 -V480p --hdmi --enc-mode 2 -X --bsize "$width"x"$height" --bmaxsize "$width"x"$height" --vout-swap 0 --mixer 1 -J --btype prev -K --btype off"
	else
		exe_cmd=$test_encode_cmd" -i"$width"x"$height" -V480p --hdmi --enc-mode 0 -X --bsize "$width"x"$height" --bmaxsize "$width"x"$height" --vout-swap 0 --mixer 1 -J --btype prev -K --btype off"
	fi
	echo "# "$exe_cmd
	$exe_cmd
	check_feedback
	echo; echo;
{
$test_image_cmd -i1 <<EOF
e
e
120
EOF
} &
	sleep 3
}

# Step 5: capture yuv file
capture_yuv()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 2: Capture YUV file"
	echo "============================================"
	echo; echo -n "Please make sure the image is focused and cover the lens inside the light box, then press enter key to continue..."
	read keypress

	exe_cmd=$test_yuv_cmd" -b 0 -Y -f "$CALIBRATION_FILE_PREFIX
	echo "# "$exe_cmd
	$exe_cmd
	check_feedback
}

# Step 6: do circle center detection
detect_center()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 3: Detect circel center"
	echo "============================================"
	echo;
	exe_cmd=$cali_center_cmd" -f "${CALIBRATION_FILE_NAME}" -w "$width" -h "$height
	echo "# "$exe_cmd
	$exe_cmd
	check_feedback
}

# Step 7: Decide if do the test one more time
loop_calibration()
{
	again=1
	while [ $again != 0 ]
	do
		accept=0
		while [ $accept != 1 ]
		do
			echo; echo "Run one more test? (Y/N)"
			echo; echo -n "please choose:"
			read action
			case $action in
				y|Y)
					accept=1
					;;
				n|N)
					accept=1
					again=0
					;;
				*)
					;;
			esac
		done
		if [ $again != 0 ]; then
			capture_yuv
			detect_center
		fi
	done
}

finish_calibration()
{
	killall test_idsp
	killall test_image
	echo;echo;
	echo "============================================"
	echo "Step 4: Get the center."
	echo "============================================"
	echo;
	echo "If failed to find center, please check:"
	echo "	1. if the lens is a fisheye lens."
	echo "	2. if the lens is focused or mounted in the right way."
	echo "	3. if the light is not bright enough."
	echo; echo "Thanks for using Ambarella calibration tool. "
	echo "For any questions please send mail to ipcam-sdk-support@ambarella.com"
}

#####main
check_cmd
init_setting
init_sensor
sleep 1
init_vinvout
capture_yuv
detect_center
loop_calibration
finish_calibration

