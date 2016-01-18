#!/bin/sh

config_dir=/sys/module/ambarella_config/parameters
board_chip=$(cat $config_dir/board_chip)
board_type=$(cat $config_dir/board_type)
board_rev=$(cat $config_dir/board_rev)

demo_default()
{
	/usr/local/bin/init.sh $1
	/usr/local/bin/test_encode -i0 -V480p --hdmi -X --bsize 720p
}

demo_i1()
{
	case "$board_type" in
		1)
		init_cmd="--ov14810"
		test2_cmd="--idle -vwvga --lcd td043 --disable-csc --fb 0 -V1080p --hdmi --disable-csc --cs rgb --fb 0"
		;;
		2)
		init_cmd="--na"
		test2_cmd="--idle -vD480x800 --lcd 1p3831 --disable-csc --rotate --fb 0 -V1080p --hdmi --disable-csc --cs rgb --fb 0"
		;;
		*)
		echo "Unknown $board_type"
		init_cmd="--na"
		test2_cmd="--idle -V1080p --hdmi --disable-csc --cs rgb --fb 0"
		;;
	esac

	case "$board_rev" in
		*)
		echo $board_rev
		;;
	esac

	/usr/local/bin/init.sh $init_cmd
	/usr/local/bin/test2 $test2_cmd

	demo_filename=/tmp/mmcblk0p1/Ducks.Take.Off.1080p.QHD.CRF25.x264-CtrlHD.mkv

	sd_empty=1
	while [ $sd_empty -ne 0 ]; do
		if [ -r $demo_filename ]; then
			echo "SD ready"
			sd_empty=0
		else
			sleep 2
			echo "SD empty"
		fi
	done

	loopcount=0
	while true; do
		loopcount=`expr $loopcount + 1`
		/usr/local/bin/pbtest $demo_filename > /dev/null 2>&1
		echo "=====Finish loop $loopcount=====" > /dev/ttyS0
		echo 3 > /proc/sys/vm/drop_caches
	done

	#reboot
}

case "$board_chip" in
	7200)
	demo_i1
	;;

	*)
	echo $board_chip
	demo_default
	;;
esac

