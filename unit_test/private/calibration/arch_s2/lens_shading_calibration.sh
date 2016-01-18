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


#define all the variables here

CALIBRATION_FILES_PATH=/ambarella/calibration

test_encode_cmd=`which test_encode`
cali_lens_shading_cmd=`which cali_lens_shading`
test_idsp_cmd=`which test_idsp`

check_cmd()
{
	if [ "x$test_encode_cmd" == "x" ]; then
		echo "Cannot find command test_encode, abort!!!"
		exit 1
	fi
	if [ "x$cali_lens_shading_cmd" == "x" ]; then
		echo "Cannot find command cali_lens_shading, abort!!!"
		exit 1
	fi
	if [ "x$test_idsp_cmd" == "x" ]; then
		echo "Cannot find command test_idsp, abort!!!"
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
	echo "Start to do lens shading calibration";
	echo "============================================"

	accept=0
	while [ $accept != 1 ]
	do
		echo "1:	MT9T002 3M sensor"
		echo "2:	AR0331 3M sensor"
		echo "3:	MN34041PL 2M sensor"
		echo "4:	IMX104 1M sensor"
		echo "5:	IMX121 12M sensor"
		echo "6:	IMX136 2M sensor(Parallel lvds:1080P120)"
		echo "7:	IMX172 12M sensor"
		echo "8:	IMX178 6M sensor"
		echo "9:	OV2710 2M sensor"
		echo; echo -n "choose sensor type:"
		read sensor_type
		case $sensor_type in
			1|2|3|4|5|6|7|8)
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
			sensor_name=ar0331
			;;
		3	)
			sensor_name=mn34041pl
			;;
		4	)
			sensor_name=imx104
			;;
		5	)
			sensor_name=imx121
			;;
		6	)
			sensor_name=imx136_120fps
			;;
		7	)
			sensor_name=imx172
			;;
		8	)
			sensor_name=imx178
			;;
		9	)
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
			bwidth="2592"
			bheight="1944"
			;;
		2	)
			width="2048"
			height="1536"
			bwidth="2048"
			bheight="1536"
			;;
		3	)
			width="1920"
			height="1080"
			bwidth="1920"
			bheight="1080"
			;;
		4	)
			width="1280"
			height="720"
			bwidth="1280"
			bheight="720"
			;;
		5	)
			width="3840"
			height="2160"
			bwidth="3840"
			bheight="2160"
			;;
		6	)
			width="4096"
			height="2160"
			bwidth="4096"
			bheight="2160"
			;;
		7	)
			width="4000"
			height="3000"
			bwidth="4000"
			bheight="3000"
			;;
		8	)
			width="2304"
			height="1296"
			bwidth="2304"
			bheight="1296"
			;;
		9	)
			width="2304"
			height="1536"
			bwidth="2304"
			bheight="1536"
			;;
		a	)
			width="3072"
			height="2048"
			bwidth="3072"
			bheight="2048"
			;;
		b	)
			width="2560"
			height="2048"
			bwidth="2560"
			bheight="2048"
			;;
		c	)
			width="1536"
			height="1024"
			bwidth="1536"
			bheight="1024"
			;;
		*	)
			echo "vin mode unrecognized!"
			exit 1
			;;
	esac

	CALIBRATION_FILE_NAME="$width"x"$height"_cali_lens_shading.bin
	CALIBRATION_FILE_NAME=${CALIBRATION_FILES_PATH}/${CALIBRATION_FILE_NAME}

	if [ -e ${CALIBRATION_FILE_NAME} ]; then
		rm -f ${CALIBRATION_FILE_NAME}
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
	echo "Step 1: switch to enc-mode 0."
	echo;echo "if VIN_width >= 2716, set 15fps for the DSP bandwidth limitation"
	echo "============================================"
	echo;
	if [ $bwidth -ge "2716" ]; then
		echo "# test_encode -i"$width"x"$height" -f 15 -V480p --hdmi --enc-mode 2 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" --raw-capture 1"
		$test_encode_cmd -i"$width"x"$height" -f 15 -V480p --hdmi --enc-mode 2 --sharpen-b 0 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" --raw-capture 1
	else
		echo "# test_encode -i"$width"x"$height" -V480p --hdmi -f 30 --enc-mode 2 --sharpen-b 0 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" --raw-capture 1"
		$test_encode_cmd -i"$width"x"$height" -V480p --hdmi -f 30 --enc-mode 2 --sharpen-b 0 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight" --raw-capture 1
	fi

	if [ $? -gt 0 ]; then
		exit 1
	fi
}

setAntiFlickerMode() {
	accept=0
	echo "=============================================="
	echo "Step 1: Set anti-flicker mode."
	echo "=============================================="
	echo
	while [ $accept -ne 1 ]
	do
		echo "1:	50 Hz"
		echo "2:	60 Hz"
		echo -n "choose anti-flicker mode:"
		read select
		case $select in
			1)
				flckerMode=50
				accept=1
				;;
			2)
				flckerMode=60
				accept=1
				;;
			*)
				echo "Please input valid number, without any other keypress(backspace or delete)!"
				accept=0
				;;
		esac
	done

	return 0
}

setAETargetValue()
{
	accept=0
	echo "=============================================="
	echo "Step 2: Set AE target value."
	echo "=============================================="
	while [ $accept -ne 1 ]
	do
		echo "1:	1024 (1X)"
		echo "2:	2048 (2X)"
		echo "3:	3072 (3X)"
		echo "4:	4096 (4X)"
		echo -n "set AE target (default is 1X): "
		read select
		case $select in
			1)
				aeTarget=1024
				accept=1
				;;
			2)
				aeTarget=2048
				accept=1
				;;
			3)
				aeTarget=3072
				accept=1
				;;
			4)
				aeTarget=4096
				accept=1
				;;
			*)
				accept=0
				echo "Please input valid number, without any other keypress(backspace or delete)!"
				;;
		esac
	done
	return 0
}

#_MAIN_
check_cmd
init_setting
init_sensor
sleep 1
init_vinvout
echo

setAntiFlickerMode
echo;echo
setAETargetValue
echo;echo

echo "=============================================="
echo "Step 4: Detect lens shading."
echo "=============================================="
echo -n "Please aim the lens at the light box with moderate brightness, then press enter key to continue..."
read keypress

echo
echo "# cali_lens_shading -d -f ${CALIBRATION_FILE_NAME}"
cali_lens_shading -d -f ${CALIBRATION_FILE_NAME}

check_feedback

echo; echo "lens shading detection finished, result saved in ${CALIBRATION_FILE_NAME}"

echo;echo;echo;
echo "=============================================="
echo "Step 5: Verify lens shading calibration effect."
echo "=============================================="
echo -n "Now verify the calibration result. Press enter key to continue..."
read keypress

echo
if [ $bheight -ge "1080" ]; then
	echo "# test_encode -i"$width"x"$height" -f 0 -V480p --hdmi --enc-mode 0 --sharpen-b 0 -X --bsize 1080p --bmaxsize 1080p"
	$test_encode_cmd -i"$width"x"$height" -f 0 -V480p --hdmi --enc-mode 0 --sharpen-b 0 -X --bsize 1080p --bmaxsize 1080p
else
	echo "# test_encode -i "$width"x"$height" --enc-mode 0 --sharpen-b 0 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight""
	$test_encode_cmd -i"$width"x"$height" --enc-mode 0 --sharpen-b 0 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight"
fi

echo "# cali_lens_shading -r"
$cali_lens_shading_cmd -r

echo; echo -n "Now you can see the lens shading. Press enter key to correct lens shading..."
read keypress
echo "# cali_lens_shading -c -f ${CALIBRATION_FILE_NAME}"
$cali_lens_shading_cmd -c -f ${CALIBRATION_FILE_NAME}

echo; echo "Check the screen if shading in the corners disappears"
read keypress
echo; echo "Thanks for using Ambarella calibration tool. "
echo "For any questions please send mail to ipcam-sdk-support@ambarella.com"

