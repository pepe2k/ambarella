#!/bin/sh

filename=/ambarella/calibration/cali_awb.txt
if [ -e $filename ]
then
	rm -f $filename
fi

cali_awb_cmd=`which cali_awb`
test_encode_cmd=`which test_encode`

if [ "x$cali_awb_cmd" == "x" ]
then
	echo "Cannot find command cali_awb, abort!!!"
	exit 1
fi

if [ "x$test_encode_cmd" == "x" ]
then
	echo "Cannot find command test_encode, abort!!!"
	exit 1
fi

mount -o remount,rw /
mkdir -p /ambarella/calibration

kill_3a()
{
	HOLD=/tmp/hold.$$
	for proc in cali_awb test_image test_idsp
	do
		ps x| grep $proc | grep -v grep | awk '{print $1}' >> $HOLD
	done
	if [ -s $HOLD ]
	then
		while read LOOP
		do
			kill -9 $LOOP
		done < $HOLD
		rm /tmp/*.$$
	fi
}


# Select sensor and initialize VIN & VOUT
init_vin_vout()
{
	accept=0
	while [ $accept -ne 1 ]
	do
		echo "1:	AR0331		sensor"
		echo "2:	MT9T002		sensor"
		echo "3:	MN34041PL	sensor"
		echo "4:	MN34210PL	sensor"
		echo "5:	MN34220PL	sensor"
		echo "6:	IMX172	sensor"
		echo "7:	IMX178	sensor"
		echo "8:	IMX121	sensor"
		echo "9:	IMX123	sensor"
		echo "a:	IMX104	sensor"
		echo "b:	IMX136	sensor"
		echo "c:	OV2710	sensor"
		echo "d:	IMX185	sensor"
		echo "e:	IMX226	sensor"
		echo "f:	OV5658	sensor"
		echo "g:	OV4689	sensor"
		echo;echo -n "choose sensor type:"
		read sensor_type
		case $sensor_type in
			1|2|3|4|5|6|7|8|9|a|b|c|d|e|f|g)
				accept=1
				;;
			*)
				echo;echo "Please input valid number, without any other keypress(no backspace or delete)!"
				accept=0
				;;
		esac
	done

	accept=0
	while [ $accept -ne 1 ]
	do
		echo "1:	1080p"
		echo "2:	720p"
		echo; echo -n "choose vin mode:"
		read vin_mode
		case $vin_mode in
			1 | 2)
				accept=1
				;;
			*)
				echo;echo "Please input valid number, without any other keypress(no backspace or delete)!"
				;;
		esac
	done

	case "$vin_mode" in
		1	)
			width="1920"
			height="1080"
			;;
		2	)
			width="1280"
			height="720"
			;;
		*	)
			echo "vin mode unrecognized!"
			exit 1
			;;
	esac

	case "$sensor_type" in
		1	)
			init.sh --ar0331
			;;
		2	)
			init.sh --mt9t002
			;;
		3	)
			init.sh --mn34041pl
			;;
		4	)
			init.sh --mn34210pl
			;;
		5	)
			init.sh --mn34220pl
			;;
		6	)
			init.sh --imx172
			;;
		7	)
			init.sh --imx178
			;;
		8	)
			init.sh --imx121
			;;
		9	)
			init.sh --imx123
			;;
		a	)
			init.sh --imx104
			;;
		b	)
			init.sh --imx136
			;;
		c	)
			init.sh --ov2710
			;;
		d	)
			init.sh --imx185
			;;
		e	)
			init.sh --imx226
			;;
		f	)
			init.sh --ov5658
			;;
		g	)
			init.sh --ov4689
			;;
		*	)
			echo "sensor type unrecognized!"
			exit 1
			;;
	esac
	echo;
	echo "# test_encode -i"$width"x"$height" -V480p --hdmi -X --bsize "$width"x"$height""
	$test_encode_cmd -i"$width"x"$height" -V480p --hdmi -X --bsize "$width"x"$height"
}


# Get current AWB gain
get_current_awb_gain()
{
	kill_3a
	echo -n "(1) Adjust the environment light to LOW Color Temperature. Press Enter key to continue..."
	read keypress
	echo "# cali_awb -d -f $filename"
	$cali_awb_cmd -d -f $filename
	orig_low_r=`cat $filename | awk -F: 'NR==1{print $1}'`
	orig_low_g=`cat $filename | awk -F: 'NR==1{print $2}'`
	orig_low_b=`cat $filename | awk -F: 'NR==1{print $3}'`

	echo
	echo -n "(2) Adjust the environment light to HIGH Color Temperature. Press Enter key to continue..."
	read keypress
	echo "# cali_awb -d -f $filename"
	$cali_awb_cmd -d -f $filename
	orig_high_r=`cat $filename | awk -F: 'NR==2{print $1}'`
	orig_high_g=`cat $filename | awk -F: 'NR==2{print $2}'`
	orig_high_b=`cat $filename | awk -F: 'NR==2{print $3}'`
}

# Set the target AWB gain
set_target_awb_gain()
{
	case "$1" in
		d | D	)
			target_low_r=985
			target_low_g=1024
			target_low_b=3020
			target_high_r=1800
			target_high_g=1024
			target_high_b=1600
			;;
		*	)
			echo;
			echo "(1) Input the target Gain in LOW Color Temperature"
			echo -n "		Gain for red	:"
			read target_low_r
			echo -n "		Gain for green:"
			read target_low_g
			echo -n "		Gain for blue :"
			read target_low_b
			echo "(2) Input the target Gain in HIGH Color Temperature"
			echo -n "		Gain for red	:"
			read target_high_r
			echo -n "		Gain for green:"
			read target_high_g
			echo -n "		Gain for blue :"
			read target_high_b
			;;
	esac

	echo -e "$target_low_r:$target_low_g:$target_low_b" >> $filename
	echo -e "$target_high_r:$target_high_g:$target_high_b" >> $filename

	echo;
	echo "Target WB Gain"
	echo "	Low	Color Temp: R $target_low_r, G $target_low_g, B $target_low_b"
	echo "	High Color Temp: R $target_high_r, G $target_high_g, B $target_high_b"
	echo
}

# Implement AWB calibration
run_awb_calibration()
{
	kill_3a
	echo
	echo "		Original WB Gain"
	echo "				 Low Color Temp: R $orig_low_r, G $orig_low_g, B $orig_low_b"
	echo "				 High Color Temp: R $orig_high_r, G $orig_high_g, B $orig_high_b"
	echo "		Target WB Gain"
	echo "				 Low Color Temp: R $target_low_r, G $target_low_g, B $target_low_b"
	echo "				 High Color Temp: R $target_high_r, G $target_high_g, B $target_high_b"
	echo
	echo "# $cali_awb_cmd -l $filename &"
	$cali_awb_cmd -l $filename &
}

# Reset AWB gain
reset_awb_gain()
{
	kill_3a
	echo "# cali_awb -r"
	$cali_awb_cmd -r &
}

#_MAIN_
echo;
echo "============================================"
echo "Start to do AWB calibration";
echo "============================================"
init_vin_vout
echo

echo;echo;echo;
echo "============================================"
echo "Step 1: Get the current AWB gain."
echo "============================================"
echo -n "Please put the grey card in front of the camera and press Enter key to continue..."
read keypress
echo
get_current_awb_gain

echo;echo;echo;
echo "============================================"
echo "Step 2: Set the target White Balance gain."
echo "============================================"
echo -n "Press D (use the default values for example, 2800k & 6500k) or Enter key (input target WB gain)..."
read -n 1 select
set_target_awb_gain $select
sleep 1

echo;echo;echo;

echo "============================================"
echo "Step 3: Run awb_calibration."
echo "============================================"
echo -n "Press Enter key to continue..."
read keypress
run_awb_calibration
sleep 1


echo;echo;echo;
echo "============================================"
echo "Step 4: Verify AWB calibration effect."
echo "============================================"
echo -n "Press Enter key to reset AWB..."
read keypress
reset_awb_gain
sleep 1
echo
echo -n "Now you see the scene in orignal AWB. Press Enter key to run AWB Calibration..."
read keypress
run_awb_calibration
sleep 1

echo; echo "Thanks for using Ambarella calibration tool. "
echo "For any questions please send mail to ipcam-sdk-support@ambarella.com"


