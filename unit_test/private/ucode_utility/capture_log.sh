#!/bin/sh

filename=dsplog

interrupt_name="vin0_idsp vin0_idsp_last_pixel vdsp";

dead_flag="dead"
exec_capture_log=`which capture_log`

get_value()
{
	newname=${1}${2};
	eval "echo $`echo $newname`";
}


read_interrupt()
{
	cat /proc/interrupts | grep -w $1 | awk '{print $2+$3}'
}

get_group_interrupts()
{
	for name in $interrupt_name
	do
		eval $name$1=`read_interrupt $name`;
	done

}

check_group_interrupts()
{
	for name in $interrupt_name
	do
		value1=`get_value $name $1`
		value2=`get_value $name $2`
		if [ $value1 -eq $value2 ]
		then
			exit_flag=1
		fi
	done
}


exit_flag=0
cnt=0
idx1=1
idx2=2
while [ $exit_flag -eq 0 ]
do
	i=`expr $cnt % 3`;
	eval fname=$filename"_"$i".bin";
	kill `pidof capture_log`;
	cmd=$exec_capture_log" "$fname;
	echo $cmd;
	$cmd &
	sleep 20;
	get_group_interrupts $idx1;
	sleep 2;
	get_group_interrupts $idx2;
	check_group_interrupts $idx1 $idx2;
	cnt=`expr $cnt + 1`;
done