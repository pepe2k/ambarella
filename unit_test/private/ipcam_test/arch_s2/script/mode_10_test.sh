#!/bin/sh
#This script is for imx172+Sunex PN DSL 239
trap "echo exiting;killall -9 rtsp_server;killall test_idsp;test_encode -A -s;killall test_bsreader;exit 1" INT TERM
pre_main_w=3584
pre_main_h=1424
main_w=3584
main_h=1344
boundrary=$(($main_w/2))

if [ $# -lt 4 ];then
	echo usage:$0 roi_x roi_y roi_w roi_h
	exit 1
fi

roi_x=$1
roi_y=$2
roi_w=$3
roi_h=$4

if [ $roi_x -gt $main_w ];then
	echo "Error: roi_x exceeds main_w"
	exit 1
fi


if [ $roi_y -gt $main_h ];then
	echo "Error: roi_y exceeds main_h"
	exit 1
fi

if [ $(($roi_x+$roi_w)) -gt $main_w ];then
	roi_w=$(($main_w-$roi_x))
	echo $roi_w
fi


if [ $(($roi_y+$roi_h)) -gt $main_h ];then
	roi_h=$(($main_h-$roi_y))
	echo $roi_h
fi

#init
if [ -z `lsmod |grep iav|head -1|awk '{print $1}'` ];then
	init.sh --imx172
fi

state=`test_encode --show-system-state|grep 'state ='|head -1|awk '{print $4}'`

if [ $state != "[encoding]" ];then

	#enter preview
	test_encode -i 3840x2160 -f 30 -V480p --hdmi --mixer 0 --enc-mode 10 -X --bsize ${main_w}x${main_h} --bmaxsize ${main_w}x${main_h} -J --btype off -K --btype enc --bsize 1920x1088 --bmaxsize 1920x1088 --vout-swap 1 -w 1920 -W 1920 -P --bsize ${pre_main_w}x${pre_main_h} --bins ${pre_main_w}x${pre_main_h} --bino 64x402 -A --smaxsize 1920x1080

	#set wall pano
	test_dewarp -M 0 -F 186 -R 1610 -a0 -s ${main_w}x${main_h} -o0x0 -h 180 -m2

	#rtsp
	killall rtsp_server
	sleep 1
	rtsp_server &>/dev/null &

	#3A
	killall test_idsp
	sleep 1
	test_idsp -a &>/dev/null &

	#set right SAR
	test_warp -b3 -a0 --din 1792x1080 --din-offset 0x132 --dout 1792x1080 --dout-offset 0x0 -a1 --din 128x1080 --din-offset 0x132 --dout 128x1080 --dout-offset 1792x0
	
	#bsreader
	#killall test_bsreader
	#sleep 1
	#test_bsreader -t &>/dev/null &

	sleep 2

	#start encode
	test_encode -A -h1920x1080 -b3 --offset 0x0 --frame-factor 30/30 -e

fi

flag=0
zoom=1
zoom_direct=0
while [ 1 ];do

	if [ $(($roi_x+$roi_w)) -le $boundrary ];then
		echo "ROI in area 0!"
		test_warp -b3 -a0 --din ${roi_w}x${roi_h} --din-offset ${roi_x}x${roi_y} --dout 1920x1080 --dout-offset 0x0 -a1 --din 0x0 --din-offset 0x0 --dout 0x0 --dout-offset 0x0
		exit 0

	fi

	if [ ${roi_x} -gt $boundrary ];then
		echo "ROI in area 1!"
		test_warp -b3 -a0 --din 0x0 --din-offset 0x0 --dout 0x0 --dout-offset 0x0 -a1 --din ${roi_w}x${roi_h} --din-offset $(($roi_x-$boundrary))x${roi_y} --dout 1920x1080 --dout-offset 0x0
		exit 0
	fi


	echo "ROI in both area 0 and 1!"
	boundrary_4th=$((($boundrary-$roi_x)*1920/$roi_w/4*4))
	#if [ $(($boundrary_4th%2)) -eq 0 ];then
	test_warp -b3 -a0 --din $(($boundrary-$roi_x))x${roi_h} --din-offset ${roi_x}x${roi_y} --dout ${boundrary_4th}x1080 --dout-offset 0x0 -a1 --din $(($roi_w -($boundrary-$roi_x)))x${roi_h} --din-offset 0x${roi_y} --dout $((1920-$boundrary_4th))x1080 --dout-offset ${boundrary_4th}x0
	#fi
	sleep 0.1
	if [ $flag -eq 0 ];then
		roi_x=$(($roi_x+32))
	else
		roi_x=$(($roi_x-32))
	fi

	if [ $(($roi_x+$roi_w)) -gt $((main_w-32)) ];then
		flag=1
	fi
	
	if [ $roi_x -le $((($boundrary)-($roi_w/2))) ] && [ $flag -eq 1 ] && [ $zoom -eq 1 ];then

		#zoom in
		x=597
		y=0
		w=1194
		h=1344
		time=0
		while [ $time -le 21 ];do
		test_warp -b3 -a0 --din ${w}x$h --din-offset ${x}x${y} --dout 960x1080 --dout-offset 0x0 -a1 --din ${w}x$h --din-offset 0x$y --dout 960x1080 --dout-offset 960x0
		w=$(($w-32))
		h=$(($h-36))
		x=$((x+32))
		y=$(($y+18))
		time=$(($time+1))
		sleep 0.1
		done

		#zoom out
                x=1270
                y=378
                w=522
                h=588
                time=0
                while [ $time -le 21 ];do
                test_warp -b3 -a0 --din ${w}x$h --din-offset ${x}x${y} --dout 960x1080 --dout-offset 0x0 -a1 --din ${w}x$h --din-offset 0x$y --dout 960x1080 --dout-offset 960x0
                w=$(($w+32))
                h=$(($h+36))
                x=$((x-32))
                y=$(($y-18))
                time=$(($time+1))
                sleep 0.1
                done
		#test_warp -b3 -a0 --din $(($boundrary-$roi_x))x${roi_h} --din-offset ${roi_x}x${roi_y} --dout ${boundrary_4th}x1080 --dout-offset 0x0 -a1 --din $(($roi_w -($boundrary-$roi_x)))x${roi_h} --din-offset 0x${roi_y} --dout $((1920-$boundrary_4th))x1080 --dout-offset ${boundrary_4th}x0

                x=597
                y=0
                w=1194
                h=1344
		zoom=0
		test_warp -b3 -a0 --din ${w}x$h --din-offset ${x}x${y} --dout 960x1080 --dout-offset 0x0 -a1 --din ${w}x$h --din-offset 0x$y --dout 960x1080 --dout-offset 960x0

	fi

	if [ $roi_x -lt 0 ];then
                flag=0
		zoom=1
		roi_x=0
        fi
done
echo "==========================================="
echo "Test done.You can exit by press control+c !"
echo "==========================================="
while [ 1 ];do
	sleep 1
done

exit 0
