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

CALIBRATION_FILES_PATH=/ambarella/calibration

test_encode_cmd=`which test_encode`
amba_debug_cmd=`which amba_debug`
cali_bad_pix_cmd=`which cali_bad_pixel`
test_idsp_cmd=`which test_idsp`
bitmap_merger_cmd=`which bitmap_merger`

check_cmd()
{
	if [ "x$test_encode_cmd" == "x" ]; then
		echo "Cannot find command test_encode, abort!!!"
		exit 1
	fi
	if [ "x$amba_debug_cmd" == "x" ]; then
		echo "Cannot find command amba_debug, abort!!!"
		exit 1
	fi
	if [ "x$cali_bad_pix_cmd" == "x" ]; then
		echo "Cannot find command cali_bad_pixel, abort!!!"
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
	echo "Start to do bad pixel calibration";
	echo "============================================"

	accept=0
	while [ $accept != 1 ]
	do
		echo "1:	MT9T002 3M sensor"
		echo "2:	AR0331 3M sensor"
		echo "3:	MN34041PL 2M sensor"
		echo "4:	IMX104 1M sensor"
		echo "5:	IMX121 12M sensor"
		echo "6:	IMX136 2M sensor(120fps)"
		echo "7:	IMX172 12M sensor"
		echo "8:	IMX178 6M sensor"
		echo "9:	OV2710 2M sensor"
		echo; echo -n "choose sensor type:"
		read sensor_type
		case $sensor_type in
			1|2|3|4|5|6|7|8|9)
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
			high_thresh=200
			low_thresh=200
			agc_index=640		# 30dB
			;;
		2	)
			sensor_name=ar0331
			high_thresh=200
			low_thresh=200
			agc_index=640		# 30dB
			;;
		3	)
			sensor_name=mn34041pl
			high_thresh=200
			low_thresh=200
			agc_index=640		# 30dB
			;;
		4	)
			sensor_name=imx104
			high_thresh=200
			low_thresh=200
			agc_index=768		# 36dB
			;;
		5	)
			sensor_name=imx121
			high_thresh=200
			low_thresh=200
			agc_index=768		# 36dB
			;;
		6	)
			sensor_name=imx136_120fps
			high_thresh=200
			low_thresh=200
			agc_index=768		# 36dB
			;;
		7	)
			sensor_name=imx172
			high_thresh=200
			low_thresh=200
			agc_index=768		# 36dB
			;;
		8	)
			sensor_name=imx178
			high_thresh=200
			low_thresh=200
			agc_index=768		# 36dB
			;;
		9	)
			sensor_name=ov2710
			high_thresh=200
			low_thresh=200
			agc_index=640		# 30dB
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
			bwidth="1920"
			bheight="1080"
			;;
		2	)
			width="2048"
			height="1536"
			bwidth="1920"
			bheight="1080"
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
			bwidth="3840"
			bheight="2160"
			;;
		7	)
			width="4000"
			height="3000"
			bwidth="3840"
			bheight="2160"
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
			bwidth="1920"
			bheight="1080"
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

	CALIBRATION_FILE_NAME="$width"x"$height"_cali_bad_pixel.bin
	CALIBRATION_FILE_NAME=${CALIBRATION_FILES_PATH}/${CALIBRATION_FILE_NAME}

	CALIBRATION_FILE_NAME_1="$width"x"$height"_cali_bad_pixel_1.bin
	CALIBRATION_FILE_NAME_1=${CALIBRATION_FILES_PATH}/${CALIBRATION_FILE_NAME_1}
	CALIBRATION_FILE_NAME_2="$width"x"$height"_cali_bad_pixel_2.bin
	CALIBRATION_FILE_NAME_2=${CALIBRATION_FILES_PATH}/${CALIBRATION_FILE_NAME_2}

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
init_vinvout_mode3()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 1: switch to calibration mode(enc-mode 3)."
	echo;echo "if VIN >= 8M, set 15fps for the DSP bandwidth limitation"
	echo "============================================"
	echo;
	if [ $bwidth -ge "2716" ]; then
		echo "# test_encode -i"$width"x"$height" -f 15 -V480p --hdmi --enc-mode 3 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight""
		$test_encode_cmd -i"$width"x"$height" -f 15 -V480p --hdmi --enc-mode 3 --sharpen-b 0 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight"
	else
		echo "# test_encode -i"$width"x"$height" -V480p --hdmi --enc-mode 3 --sharpen-b 0 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight""
		$test_encode_cmd -i"$width"x"$height" -V480p --hdmi --enc-mode 3 --sharpen-b 0 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight"
	fi

	check_feedback
}

init_vinvout_mode2()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 1: enter preview."
	echo;echo "if VIN >= 8M, set 15fps for the DSP bandwidth limitation"
	echo "============================================"
	echo;
	echo "# test_encode -i"$width"x"$height" -f 15 -V480p --hdmi --enc-mode 2 --sharpen-b 1 -X --bsize "$width"x"$height" --bmaxsize "$width"x"$height" --raw-capture 1"
	$test_encode_cmd -i"$width"x"$height" -f 15 -V480p --hdmi --enc-mode 2 --sharpen-b 1 -X --bsize "$width"x"$height" --bmaxsize "$width"x"$height" --raw-capture 1

	check_feedback
	sleep 1
	test_image -i 0 &
	sleep 3
	killall -9 test_image
}

restore_with_vinvout_mode0()
{
	if [ $bheight -ge "1080" ]; then
		echo "# test_encode -i"$width"x"$height" -f 0 -V480p --hdmi --enc-mode 0 --sharpen-b 1 -X --bsize 1080p --bmaxsize 1080p"
		$test_encode_cmd -i"$width"x"$height" -f 0 -V480p --hdmi --enc-mode 0 --sharpen-b 1 -X --bsize 1080p --bmaxsize 1080p
	else
		echo "# test_encode -i "$width"x"$height" --enc-mode 0 --sharpen-b 1 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight""
		$test_encode_cmd -i"$width"x"$height" --enc-mode 0 --sharpen-b 1 -X --bsize "$bwidth"x"$bheight" --bmaxsize "$bwidth"x"$bheight"
	fi
	test_image -i 0 &
	sleep 2
	killall -9 test_image
	$test_idsp_cmd -A$agc_index
	$test_idsp_cmd -S1012
	echo "# cali_bad_pixel -r $width,$height"
	$cali_bad_pix_cmd -r $width,$height
	sleep 2
}

# Step 5: do bad pixel detection
detect_bright_bad_pixel()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 2: Detect bright bad pixel."
	echo "============================================"
	echo; echo -n "Please put on the lens cap, and then press enter key..."
	read keypress

	echo "# cali_bad_pixel -d $width,$height,160,160,0,$high_thresh,$low_thresh,3,$agc_index,1012 -f ${CALIBRATION_FILE_NAME_1}"
	$cali_bad_pix_cmd -d $width,$height,160,160,0,$high_thresh,$low_thresh,3,$agc_index,1012 -f ${CALIBRATION_FILE_NAME_1}
	echo; echo "bright bad pixel detection finished, result saved in ${CALIBRATION_FILE_NAME_1}"

	check_feedback
}

# Step 6: verify the calibration result
verify_bright_bad_pixel()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 3: Switch to enc-mode 0, Verify bad pixel effect."
	echo "============================================"

	echo;echo -n "Press enter key to restore bad pixel effect..."
	read keypress

	restore_with_vinvout_mode0
	echo;echo;echo -n "Now you can see the bright bad pixel, press enter key to continue..."
	read keypress
}

# Step 7: correct bad pixels
correct_bright_bad_pixel()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 4: Correct bright bad pixel, Verify bad pixel correction."
	echo "============================================"

	echo;echo -n "Press enter key to correct bad pixels......"
	read keypress

	echo "# cali_bad_pixel -c $width,$height -f ${CALIBRATION_FILE_NAME_1}"
	$cali_bad_pix_cmd -c $width,$height -f ${CALIBRATION_FILE_NAME_1}

	echo; echo -n "Check the screen if bright bad pixels disappear, Press enter key to continue..."
	read keypress
	echo;echo;echo;
	echo "============================================"
	echo; echo -n "Start dark bad pixel, Press enter key to continue....."
	read keypress
}


# Step 8: do bad pixel detection
detect_dark_bad_pixel()
{

	echo;echo;echo;
	echo "============================================"
	echo "Step 5: Detect dark bad pixel."
	echo "============================================"
	echo; echo -n "Please remove the lens cap, and make the lens faced to a light box, and then press enter key..."
	read keypress

	echo "# cali_bad_pixel -d $width,$height,16,16,1,20,40,3,0,1400 -f ${CALIBRATION_FILE_NAME_2}"
	$cali_bad_pix_cmd -d $width,$height,16,16,1,20,40,3,0,1400 -f ${CALIBRATION_FILE_NAME_2}
	echo; echo "bad pixel detection finished, result saved in ${CALIBRATION_FILE_NAME_2}"

	check_feedback
}

# Step 9: verify the calibration result
verify_dark_bad_pixel()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 6: Switch to enc-mode 0, Verify bad pixel effect."
	echo "============================================"
	echo; echo -n "Press enter key to restore bad pixel effect..."
	read keypress

	restore_with_vinvout_mode0
	echo;echo;echo -n "Now you can see the dark bad pixel, press enter key to continue..."
	read keypress
}

# Step 10: correct bad pixels
correct_dark_bad_pixel()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 7: Correct dark bad pixel, Verify bad pixel correction."
	echo "============================================"
	echo; echo -n "Press enter key to correct bad pixels......"
	read keypress
	echo "# cali_bad_pixel -c $width,$height -f ${CALIBRATION_FILE_NAME_2}"
	$cali_bad_pix_cmd -c $width,$height -f ${CALIBRATION_FILE_NAME_2}

	echo; echo -n "check the screen if dark bad pixels disappear, Press enter key to continue..."
	read keypress
}

finish_calibration()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 8: Gather all the bad pixels"
	echo "============================================"
	echo "# bitmap_merger -o $CALIBRATION_FILE_NAME_1,$CALIBRATION_FILE_NAME_2,$CALIBRATION_FILE_NAME"
	$bitmap_merger_cmd -o $CALIBRATION_FILE_NAME_1,$CALIBRATION_FILE_NAME_2,$CALIBRATION_FILE_NAME
	rm $CALIBRATION_FILE_NAME_1 $CALIBRATION_FILE_NAME_2 -rf
	echo "# cali_bad_pixel -c $width,$height -f ${CALIBRATION_FILE_NAME}"
	$cali_bad_pix_cmd -c $width,$height -f ${CALIBRATION_FILE_NAME}

	echo; echo "Thanks for using Ambarella calibration tool. "
	echo "For any questions please send mail to ipcam-sdk-support@ambarella.com"
}



#####main
check_cmd
init_setting
init_sensor
sleep 1
init_vinvout_mode2
detect_bright_bad_pixel
verify_bright_bad_pixel
correct_bright_bad_pixel
init_vinvout_mode2
detect_dark_bad_pixel
verify_dark_bad_pixel
correct_dark_bad_pixel
finish_calibration
