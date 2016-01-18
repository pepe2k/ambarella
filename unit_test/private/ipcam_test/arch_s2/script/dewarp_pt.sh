#!/bin/sh

step=1
interval=0.03

pantilt_wall()
{
	t=-90
	p=0
	while [ $t -lt 91 ]
	do
		echo "tilt $t, pan $p";
		/usr/local/bin/test_dewarp -M0 -F185 -R1024 -a0 -s1024x1024 -p$p -t$t -m3 -l0 --time
		t=`expr $t + $step`
		sleep $interval
	done

	p=-90
	t=0
	while [ $p -lt 91 ]
	do
		echo "tilt $t, pan $p";
		/usr/local/bin/test_dewarp -M0 -F185 -R1024 -a0 -s1024x1024 -p$p -t$t -m3 -l0 --time
		p=`expr $p + $step`
		sleep $interval
	done
}

pantilt_ceiling()
{
	t=-90
	p=0;
	while [ $t -lt 1 ]
	do
		echo "tilt $t, pan $p";
		/usr/local/bin/test_dewarp -M1 -F185 -R1024 -a0 -s1024x1024 -p$p -t$t -m3 -l0 --time
		t=`expr $t + $step`
		sleep $interval
	done

	t=-90
	while [ $t -lt 1 ]
	do
		p=180
		while [ $p -gt -181 ]
		do
			echo "tilt $t, pan $p";
			/usr/local/bin/test_dewarp -M1 -F185 -R1024 -a0 -s1024x1024 -p$p -t$t -m3 -l0 --time
			p=`expr $p - $step`
		sleep $interval
		done
		t=`expr $t + $step`
	done
}

pantilt_desktop()
{
	t=90
	p=0;
	while [ $t -gt -1 ]
	do
		echo "tilt $t, pan $p";
		/usr/local/bin/test_dewarp -M2 -F185 -R1024 -a0 -s1024x1024 -p$p -t$t -m3 -l0 --time
		t=`expr $t - $step`
		sleep $interval
	done

	t=90
	while [ $t -lt 1 ]
	do
		p=180
		while [ $p -gt -181 ]; do
			echo "tilt $t, pan $p";
			/usr/local/bin/test_dewarp -M2 -F185 -R1024 -a0 -s1024x1024 -p$p -t$t -m3 -l0 --time
			p=`expr $p - $step`
		sleep $interval
		done
		t=`expr $t + $step`
	done
}

if [ $# -lt 1 ]
then
	echo "Usage: $0 [0|1|2].  0: wall, 1: ceiling, 2: desktop"
	exit
fi

if [ $1 -eq 0 ]
then
	pantilt_wall
elif [ $1 -eq 1 ]
then
	pantilt_ceiling
elif [ $1 -eq 2 ]
then
	pantilt_desktop
fi
exit

