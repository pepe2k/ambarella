#!/bin/sh

## History:
##    2013/08/09 [Lei Hong] created file
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
cali_center_cmd=`which cali_piris`
test_image_cmd=`which test_image`
exp_level=200

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
	echo "Start to do piris calibration";
	echo "============================================"
	echo;
	echo "input exposure level(range 25 ~ 400):"
	read exp_level
}


print_piris_luma()
{
	echo;echo;echo;
	echo "============================================"
	echo "print piris luma and piris position"
	echo "============================================"
	echo;
	cali_piris -p 1 -e $exp_level
	check_feedback
}


finish_calibration()
{
	echo;echo;
	echo "============================================"
	echo "Finish."
	echo "============================================"
}

#####main
check_cmd
init_setting
sleep 1
print_piris_luma
finish_calibration

