#!/bin/sh
trap "echo exiting;killall -9 rtsp_server;killall test_image;test_encode -A -s;killall test_bsreader;exit 1" INT TERM

if [ $# -lt 1 ];then
	echo usage:$0 "[DoublePano|360Pano]"
	exit 1
fi

#init
if [ -z `lsmod |grep iav|head -1|awk '{print $1}'` ];then
	init.sh --imx178
fi

state=`test_encode --show-system-state|grep 'state ='|head -1|awk '{print $4}'`

if [ $state != "[encoding]" ];then

	#enter preview
	if [ $1 == "DoublePano" ];then
    test_encode -i 3072x2048 -f 25 -V480p --hdmi --mixer 0 --enc-mode 1 -X --bsize 3072x2048 --bmaxsize 3072x2048 -J --btype off -K --btype prev --bsize 480p --vout-swap 1 -w 1968 -W 2048 -P --bsize 1968x1968 --bins 1968x1968 --bino 556x40 -A --smaxsize 2048x2048
	elif [ $1 == "360Pano" ];then
    test_encode -i 3072x2048 -f 25 -V480p --hdmi --mixer 0 --enc-mode 1 -X --bsize 5120x1024 --bmaxsize 5120x1024 -J --btype off -K --btype prev --bsize 480p --vout-swap 1 -w 1968 -W 2048 -P --bsize 1968x1968 --bins 1968x1968 --bino 556x40 -A --smaxsize 4096x1024
  fi

	#set ceiling pano
	if [ $1 == "DoublePano" ];then
    test_dewarp -M 1 -F 185 -R 1024 --wm 1 -a0 -s1024x1024 -N -h 90 -z3/2 -m2 -a1 -s1024x1024 -o 1024x0 -E -h 90 -z3/2 -m2 -a2 -s1024x1024 -o 0x1024 -S -h 90 -z3/2 -m2 -a3 -s1024x1024 -o 1024x1024 -W -h 90 -z3/2 -m2 -a4 -m4 --wc-src 2 --wc-offset 2048x0  -a5 -m4 --wc-src 0 --wc-offset 2048x1024
	elif [ $1 == "360Pano" ];then
    test_dewarp -M 1 -F 185 -R 1024 --wm 1 -a0 -s1024x1024 -N -h 90 -z3/2 -m2 -a1 -s1024x1024 -o 1024x0 -E -h 90 -z3/2 -m2 -a2 -s1024x1024 -o 2048x0 -S -h 90 -z3/2 -m2 -a3 -s1024x1024 -o 3072x0 -W -h 90 -z3/2 -m2 -a4 -m4 --wc-src 0 --wc-offset 4096x0
	fi

	#rtsp
	killall rtsp_server
	sleep 1
	rtsp_server &>/dev/null &

	#3A
	killall test_image
	sleep 1
	test_image -i 0 &

	#bsreader
	#killall test_bsreader
	#sleep 1
	#test_bsreader -t &>/dev/null &

	sleep 2

	#start encode
	if [ $1 == "DoublePano" ];then
    test_encode -A -h2048x2048 --offset 0x0 --frame-factor 30/30 -e
  elif [ $1 == "360Pano" ];then
    test_encode -A -h4096x1024 --offset 0x0 --frame-factor 30/30 -e
  fi

fi

offset=0
step=32
flag=1
count=0

while [ 1 ];do
  test_encode -A --offset ${offset}x0
  offset=$(($offset+$flag*$step))
  count=$(($count+1))

  if [ $count -ge 32 ];then
    flag=$(($flag*-1))
    count=0
  fi

  sleep 0.1
done

echo "==========================================="
echo "Test done.You can exit by press control+c !"
echo "==========================================="
while [ 1 ];do
	sleep 1
done

exit 0
