#!/bin/sh

echo $PATH | grep "/usr/local/bin"
if [ $? -eq 1 ]; then
      export PATH=$PATH:/usr/local/bin
fi

AMBARELLA_CONF=ambarella.conf

[ -r /etc/$AMBARELLA_CONF ] && . /etc/$AMBARELLA_CONF

kernel_ver=$(uname -r)

if [ -d /sys/module/ambarella_config/parameters ]; then
	config_dir=/sys/module/ambarella_config/parameters
fi

if [ -x /tmp/mmcblk0p1/ext-pro.sh ]; then
echo "==============================================="
echo "Exec: /tmp/mmcblk0p1/ext-pro.sh"
echo "==============================================="
/tmp/mmcblk0p1/ext-pro.sh
elif [ -x /home/default/ext-pro.sh ]; then
echo "==============================================="
echo "Exec: /home/default/ext-pro.sh"
echo "==============================================="
/home/default/ext-pro.sh
fi
case "$?" in
    0)
	;;
    *)
  	echo "Exec ext-pro.sh failed!"
	exit 1
	;;
esac

if [ -d /tmp/mmcblk0p1/lib/modules ]; then
echo "==============================================="
echo "Using ext modules: /tmp/mmcblk0p1/lib/modules"
echo "==============================================="
mount --bind /tmp/mmcblk0p1/lib/modules /lib/modules
elif [ -d /home/default/lib/modules ]; then
echo "==============================================="
echo "Using ext modules: /home/default/lib/modules"
echo "==============================================="
mount --bind /home/default/lib/modules /lib/modules
fi
case "$?" in
    0)
	;;
    *)
  	echo "Mount ext modules failed!"
	exit 1
	;;
esac

if [ -d /tmp/mmcblk0p1/lib/firmware ]; then
echo "==============================================="
echo "Using ext firmware: /tmp/mmcblk0p1/lib/firmware"
echo "==============================================="
mount --bind /tmp/mmcblk0p1/lib/firmware /lib/firmware
elif [ -d /home/default/lib/firmware ]; then
echo "==============================================="
echo "Using ext firmware: /home/default/lib/firmware"
echo "==============================================="
mount --bind /home/default/lib/firmware /lib/firmware
elif [ -d /lib/firmware/still ] && [ "$AMBARELLA_CALI" = "1" ]; then
echo "==============================================="
echo "Using stillcap firmware: /lib/firmware/still"
echo "==============================================="
mount --bind /lib/firmware/still /lib/firmware
fi
case "$?" in
    0)
	;;
    *)
  	echo "Mount ext firmware failed!"
	exit 1
	;;
esac

if [ "$SYS_BOARD_BSP" = "a2ipcam" ]; then
	echo "Note: iav module param can be changed to enable slow shutter and disable osd insert"
	modprobe iav disable_osd_insert=0
else
	modprobe iav
fi
case "$?" in
    0)
	;;
    *)
  	echo "Modprobe IAV failed!"
	exit 1
	;;
esac

modprobe ambtve
case "$?" in
    0)
	;;
    *)
  	echo "Modprobe ambtve failed!"
	exit 1
	;;
esac

if [ -r /lib/modules/$kernel_ver/extra/ambdbus.ko ]; then
	modprobe ambdbus
	case "$?" in
	    0)
		;;
	    *)
	  	echo "Modprobe ambdbus failed!"
		;;
	esac
fi

if [ -r /lib/modules/$kernel_ver/extra/ambhdmi.ko ]; then
        modprobe ambhdmi
        case "$?" in
            0)
                ;;
            *)
                echo "Modprobe ambhdmi failed!"
                ;;
        esac
fi

modprobe vin
case "$?" in
    0)
	;;
    *)
  	echo "Modprobe vin failed!"
	exit 1
	;;
esac

default_vin=mt9t002
if [ "$SYS_BOARD_VERSION" = "A3DVR" ]; then
	default_vin=rn6240
fi
if [ "$SYS_BOARD_BSP" = "a2sipcam" ]; then
	default_vin=ov9710
fi
if [ "$SYS_BOARD_BSP" = "a5sbub" ]; then
	default_vin=mt9j001_hispi
fi
if [ "$SYS_BOARD_BSP" = "a5ssdk" ]; then
	default_vin=mt9j001_hispi
fi
if [ "$SYS_BOARD_BSP" = "durian" ]; then
	default_vin=ov5653
fi
if [ "$SYS_BOARD_BSP" = "i1bub" ]; then
	default_vin=ov14810
fi
if [ "$SYS_BOARD_BSP" = "i1evk" ]; then
	default_vin=ov14810
fi

if [ "$SYS_BOARD_BSP" = "s2ipcam" ]; then
	#export ext gpio(10) to use vin gpio reset
	echo 168 > /sys/class/gpio/export
fi

case $1 in
    --mt9p031 ) insmod /lib/modules/$kernel_ver/extra/mt9p031.ko ;;
    --mt9p001 ) insmod /lib/modules/$kernel_ver/extra/mt9p001.ko ;;
    --mt9p401 ) insmod /lib/modules/$kernel_ver/extra/mt9p401.ko ;;
    --mt9p006 ) insmod /lib/modules/$kernel_ver/extra/mt9p006.ko ;;
    --mt9p015 ) insmod /lib/modules/$kernel_ver/extra/mt9p015.ko ;;
    --mt9v136 ) insmod /lib/modules/$kernel_ver/extra/mt9v136.ko ;;
    --mt9v136_pal ) insmod /lib/modules/$kernel_ver/extra/mt9v136.ko config_pal=1;;
    --mt9j001_hispi )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/$kernel_ver/extra/mt9j001_hispi.ko ;;
    --mt9j001_parallel ) insmod /lib/modules/$kernel_ver/extra/mt9j001_parallel.ko ;;
    --m3135 ) insmod /lib/modules/$kernel_ver/extra/mt9t002.ko ;;
    --mt9t002 ) insmod /lib/modules/$kernel_ver/extra/mt9t002.ko ;;
    --mt9t002_mipi ) insmod /lib/modules/$kernel_ver/extra/mt9t002_mipi.ko adapter_id=1;;
    --ar0330 )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/$kernel_ver/extra/mt9t002.ko ;;
    --mt9t002_s3d )
	insmod /lib/modules/$kernel_ver/extra/sbrg.ko
	insmod /lib/modules/$kernel_ver/extra/mt9t002_s3d.ko
	;;
    --ar0330cs ) insmod /lib/modules/$kernel_ver/extra/mt9t002_parallel.ko ;;
    --ar0330cs_mipi ) insmod /lib/modules/$kernel_ver/extra/mt9t002_mipi.ko ;;
    --ar0331 ) insmod /lib/modules/$kernel_ver/extra/ar0331.ko hdr_mode=1;;
    --ar0130 ) insmod /lib/modules/$kernel_ver/extra/ar0130.ko ;;
    --ar0835hs ) insmod /lib/modules/$kernel_ver/extra/ar0835hs.ko ;;
    --mt9m033 ) insmod /lib/modules/$kernel_ver/extra/mt9m033.ko ;;
    --mt9m034 ) insmod /lib/modules/$kernel_ver/extra/mt9m033.ko ;;
    --imx035 ) insmod /lib/modules/$kernel_ver/extra/imx035.ko ;;
    --imx036 ) insmod /lib/modules/$kernel_ver/extra/imx036.ko ;;
    --imx072 ) insmod /lib/modules/$kernel_ver/extra/imx072.ko ;;
    --imx078 ) insmod /lib/modules/$kernel_ver/extra/imx078.ko ;;
    --imx121 ) insmod /lib/modules/$kernel_ver/extra/imx121.ko ;;
    --imx122 ) insmod /lib/modules/$kernel_ver/extra/imx122.ko ;;
    --imx104 ) insmod /lib/modules/$kernel_ver/extra/imx104.ko ;;
    --imx136 ) insmod /lib/modules/$kernel_ver/extra/imx136_lvds.ko ;;
    --imx136_wdr ) insmod /lib/modules/$kernel_ver/extra/imx136_parallel.ko ;;
    --imx136_120fps ) insmod /lib/modules/$kernel_ver/extra/imx136_plvds.ko ;;
    --imx172 ) insmod /lib/modules/$kernel_ver/extra/imx172.ko ;;
    --imx138 ) insmod /lib/modules/$kernel_ver/extra/imx104.ko ;;
    --imx178 ) insmod /lib/modules/$kernel_ver/extra/imx178.ko ;;
    --imx185 ) insmod /lib/modules/$kernel_ver/extra/imx185.ko ;;
    --imx185_120fps ) insmod /lib/modules/$kernel_ver/extra/imx185_plvds.ko ;;
    --imx226 ) insmod /lib/modules/$kernel_ver/extra/imx226.ko ;;
    --imx123 ) insmod /lib/modules/$kernel_ver/extra/imx123.ko ;;
    --s5k4 ) insmod /lib/modules/$kernel_ver/extra/s5k4awfx.ko ;;
    --s5k3 ) insmod /lib/modules/$kernel_ver/extra/s5k3e2fx.ko ;;
    --s5k5 ) insmod /lib/modules/$kernel_ver/extra/s5k5b3gx.ko ;;
    --ov7720 ) insmod /lib/modules/$kernel_ver/extra/ov7720.ko ;;
    --ov7725 ) insmod /lib/modules/$kernel_ver/extra/ov7725.ko ;;
    --ov7740 ) insmod /lib/modules/$kernel_ver/extra/ov7740.ko ;;
    --ov2710 ) insmod /lib/modules/$kernel_ver/extra/ov2710_parallel.ko ;;
    --ov2710_mipi ) insmod /lib/modules/$kernel_ver/extra/ov2710_mipi.ko ;;
    --ov2715 ) insmod /lib/modules/$kernel_ver/extra/ov2710_parallel.ko ;;
    --ov2715_mipi ) insmod /lib/modules/$kernel_ver/extra/ov2710_mipi.ko ;;
    --ov5653 ) insmod /lib/modules/$kernel_ver/extra/ov5653.ko ;;
    --ov9710 ) insmod /lib/modules/$kernel_ver/extra/ov9710.ko ;;
    --ov9715 ) insmod /lib/modules/$kernel_ver/extra/ov9710.ko ;;
		--altera_fpga ) insmod /lib/modules/$kernel_ver/extra/altera_fpga.ko ;;
    --gs2970 )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/$kernel_ver/extra/gs2970.ko ;;
    --ov10620 ) insmod /lib/modules/$kernel_ver/extra/ov10620.ko ;;
    --ov10630_yuv ) insmod /lib/modules/$kernel_ver/extra/ov10630_yuv.ko ;;
    --ov10633_yuv ) insmod /lib/modules/$kernel_ver/extra/ov10633_yuv.ko ;;
    --ov10630_rgb ) insmod /lib/modules/$kernel_ver/extra/ov10630_rgb.ko ;;
    --ov14810 )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/$kernel_ver/extra/ov14810.ko ;;
    --ov4689 ) insmod /lib/modules/$kernel_ver/extra/ov4689_mipi.ko ;;
    --ov4689_2lane ) insmod /lib/modules/$kernel_ver/extra/ov4689_mipi.ko lane=2 ;;
	--mn34041pl ) insmod /lib/modules/$kernel_ver/extra/mn34041pl.ko ;;
    --mn34031pl ) insmod /lib/modules/$kernel_ver/extra/mn34031pl.ko ;;
    --mn34210pl ) insmod /lib/modules/$kernel_ver/extra/mn34210pl.ko ;;
    --mn34220pl ) insmod /lib/modules/$kernel_ver/extra/mn34220pl.ko ;;
    --gt2005 ) insmod /lib/modules/$kernel_ver/extra/gt2005.ko ;;
    --tw9910 )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/$kernel_ver/extra/tw9910.ko ;;
    --adv7403 )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/$kernel_ver/extra/adv7403.ko ;;
    --adv7443 )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/$kernel_ver/extra/adv7443.ko ;;
    --adv7441 )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	if [  -e /lib/firmware/EDID.bin ]; then
		insmod /lib/modules/$kernel_ver/extra/adv7443.ko edid_data=EDID.bin
	else
		insmod /lib/modules/$kernel_ver/extra/adv7443.ko
	fi
	;;
    --adv7441a )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	if [  -e /lib/firmware/EDID.bin ]; then
		insmod /lib/modules/$kernel_ver/extra/adv7441a.ko edid_data=EDID.bin
	else
		insmod /lib/modules/$kernel_ver/extra/adv7441a.ko
	fi
	;;
    --adv7619 )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	if [  -e /lib/firmware/EDID.bin ]; then
		insmod /lib/modules/$kernel_ver/extra/adv7619.ko edid_data=EDID.bin
	else
		insmod /lib/modules/$kernel_ver/extra/adv7619.ko
	fi
	;;
    --tvp5150 )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/$kernel_ver/extra/tvp5150.ko ;;
    --rn6240 )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/$kernel_ver/extra/rn6240.ko ;;
    --tw2864 )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	insmod /lib/modules/$kernel_ver/extra/tw2864.ko ;;

    --yuv480i ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_format=1 video_bits=8 video_mode=65520 cap_cap_w=720 cap_cap_h=480 video_phase=3 video_fps=8541875 video_pixclk=27000000 video_yuv_mode=1;;
    --yuv480p ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=65522 cap_cap_w=720 cap_cap_h=480 video_phase=3 video_fps=8541875 video_pixclk=27000000 video_yuv_mode=3;;
    --yuv576i ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_format=1 video_bits=8 video_mode=65521 cap_cap_w=720 cap_cap_h=576 video_phase=3 video_fps=10240000 video_pixclk=27000000 video_yuv_mode=1;;
    --yuv576p ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=65523 cap_cap_w=720 cap_cap_h=576 video_phase=3 video_fps=10240000 video_pixclk=27000000 video_yuv_mode=3;;
    --yuv720p ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=65524 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=8541875 video_yuv_mode=3;;
    --yuv720p50 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=65525 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=10240000 video_yuv_mode=3;;
    --yuv720p30 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=40 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=17066667 video_yuv_mode=3;;
    --yuv720p25 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=39 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=20480000 video_yuv_mode=3;;
    --yuv720p24 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=38 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=21333333 video_yuv_mode=3;;
    --yuv1080i ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_format=1 video_type=6 video_mode=65526 cap_cap_w=1920 cap_cap_h=1080 video_phase=2 video_yuv_mode=3 video_fps=8541875 ;;
    --yuv1080i50 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=6 video_format=1 video_bits=16 video_mode=65527 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_fps=10240000 video_yuv_mode=3;;
    --yuv1080p30 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_mode=65533 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=17066667 ;;
    --yuv1080p25 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=6 video_format=2 video_bits=16 video_mode=65532 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_fps=20480000 video_yuv_mode=3;;
    --yuv1080p ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=6 video_mode=65528 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=17083750 video_pixclk=148500000;;
    --yuv1080p50 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=6 video_mode=65529 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=10240000 video_pixclk=148500000;;
    --yuv1600p30 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=6 video_mode=12 cap_cap_w=1600 cap_cap_h=1200 video_phase=3 video_yuv_mode=3 video_fps=17083750 video_pixclk=27000000;;
    --mdin380 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=6 video_mode=12 cap_cap_w=1600 cap_cap_h=1200 video_phase=3 video_yuv_mode=3 video_fps=17083750 ;;
    --yuv1280_960p30 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=16 cap_cap_w=1280 cap_cap_h=960 video_phase=3 video_fps=17066667 video_yuv_mode=3;;
    --yuv1280_960p25 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=2 video_format=2 video_bits=16 video_mode=16 cap_cap_w=1280 cap_cap_h=960 video_phase=3 video_fps=20480000 video_yuv_mode=3;;

    --yuv480i_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_format=1 video_bits=8 video_mode=65520 cap_start_x=64 cap_start_y=18 cap_cap_w=720 cap_cap_h=480 video_phase=0 video_fps=8541875 video_pixclk=27000000 video_yuv_mode=0;;
    --yuv480p_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=65522 cap_start_x=60 cap_start_y=36 cap_cap_w=720 cap_cap_h=480 video_phase=0 video_fps=8541875 video_pixclk=27000000 video_yuv_mode=3;;
    --yuv576i_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_format=1 video_bits=8 video_mode=65521 cap_start_x=60 cap_start_y=22 cap_cap_w=720 cap_cap_h=576 video_phase=0 video_fps=10240000 video_pixclk=27000000 video_yuv_mode=0;;
    --yuv576p_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=65523 cap_start_x=66 cap_start_y=44 cap_cap_w=720 cap_cap_h=576 video_phase=0 video_fps=10240000 video_pixclk=27000000 video_yuv_mode=3;;
    --yuv720p_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=65524 cap_start_x=220 cap_start_y=25 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=8541875 video_yuv_mode=3;;
    --yuv720p50_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=65525 cap_start_x=220 cap_start_y=25 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=10240000 video_yuv_mode=3;;
    --yuv720p30_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=40 cap_start_x=220 cap_start_y=25 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=17066667 video_yuv_mode=3;;
    --yuv720p25_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=39 cap_start_x=220 cap_start_y=25 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=20480000 video_yuv_mode=3;;
    --yuv720p24_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=38 cap_start_x=220 cap_start_y=25 cap_cap_w=1280 cap_cap_h=720 video_phase=3 video_fps=21333333 video_yuv_mode=3;;
    --yuv1080i_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_format=1 video_type=1 video_mode=65526 cap_start_x=192 cap_start_y=20 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=8541875 ;;
    --yuv1080i50_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_format=1 video_bits=16 video_mode=65527 cap_start_x=192 cap_start_y=20 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_fps=10240000 video_yuv_mode=3;;
    --yuv1080p30_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_mode=65533 cap_start_x=192 cap_start_y=41 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=17066667 ;;
    --yuv1080p25_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_format=2 video_bits=16 video_mode=65532 cap_start_x=192 cap_start_y=41 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_fps=20480000 video_yuv_mode=3;;
    --yuv1080p_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_mode=65528 cap_start_x=192 cap_start_y=41 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=17083750 video_pixclk=148500000;;
    --yuv1080p50_601 ) insmod /lib/modules/$kernel_ver/extra/ambdd.ko video_type=1 video_mode=65529 cap_start_x=192 cap_start_y=41 cap_cap_w=1920 cap_cap_h=1080 video_phase=3 video_yuv_mode=3 video_fps=10240000 video_pixclk=148500000;;

    --na )
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	echo "Init without VIN drivers..." ;;
    --gps )
    echo " Load default Touch Screen Driver "
    insmod /lib/modules/$kernel_ver/kernel/drivers/input/touchscreen/ak4183.ko
	insmod /lib/modules/$kernel_ver/extra/$default_vin.ko
    ;;
    --ak4183 )
    echo " Load ak4183 Touch Screen Driver "
    insmod /lib/modules/$kernel_ver/kernel/drivers/input/touchscreen/ak4183.ko
	insmod /lib/modules/$kernel_ver/extra/$default_vin.ko
    ;;
    --chacha_mt4d )
    echo " Load chacha_mt4d Touch Screen driver "
    insmod /lib/modules/$kernel_ver/kernel/drivers/input/touchscreen/chacha_mt4d.ko
	insmod /lib/modules/$kernel_ver/extra/$default_vin.ko
    ;;
    --nvr_demo )
    echo " Load NVR demo Touch Screen driver "
    insmod /lib/modules/$kernel_ver/kernel/drivers/input/touchscreen/tm1726.ko
    ;;
    *)
	echo -1 > $config_dir/board_vin0_vsync_irq_gpio
	echo "Default method to init without VIN drivers..."
	#echo "Use $default_vin as default."
	echo "Cat this file to find out what VIN can be supported."
	#insmod /lib/modules/$kernel_ver/extra/$default_vin.ko
	;;
esac
case "$?" in
    0)
	;;
    *)
  	echo "Insmod $1 failed!"
	exit 1
	;;
esac

if [ -r /lib/modules/$kernel_ver/extra/ambad.ko ]; then
	modprobe ambad
fi

if [ -r /lib/modules/$kernel_ver/extra/fdet.ko ]; then
	modprobe fdet
fi

if [ -r /lib/modules/$kernel_ver/extra/mpu6000.ko ]; then
	modprobe mpu6000
fi

if [ -r /lib/modules/$kernel_ver/extra/eis.ko ]; then
	modprobe eis
fi

case $2 in
	--high )
	/usr/local/bin/amba_debug -w 0x701700e4 -d 0x30110100 && /usr/local/bin/amba_debug -w 0x701700e4 -d 0x30110101 && /usr/local/bin/amba_debug -w 0x701700e4 -d 0x30110100
	echo "Change iDSP to 147 MHz"
	/usr/local/bin/amba_debug -w 0x701700dc -d 0x1c111000 && /usr/local/bin/amba_debug -w 0x701700dc -d 0x1c111001 && /usr/local/bin/amba_debug -w 0x701700dc -d 0x1c111000
	echo "Change DDR2 to 348 MHz"
	;;
	*)
	echo "Use default settings"
	;;
esac

LOAD_UCODE=`which load_ucode`
if [  -e $LOAD_UCODE ]; then
$LOAD_UCODE /lib/firmware
case "$?" in
	0)
	;;
	*)
  	echo "Load ucode failed!"
	exit 1
	;;
esac
fi

if [  -e /usr/local/bin/iav_server ]; then
echo "Start IAV Server ..."
/usr/local/bin/iav_server &
fi

if [  -e /tmp/mmcblk0p1/ext-post.sh ]; then
echo "==============================================="
echo "Exec: /tmp/mmcblk0p1/ext-post.sh"
echo "==============================================="
/tmp/mmcblk0p1/ext-post.sh
elif [  -e /home/default/ext-post.sh ]; then
echo "==============================================="
echo "Exec: /home/default/ext-post.sh"
echo "==============================================="
/home/default/ext-post.sh
fi
case "$?" in
    0)
	;;
    *)
  	echo "Exec ext-post.sh failed!"
	exit 1
	;;
esac

#add dsp log driver
if [ -r /lib/modules/$kernel_ver/extra/dsplog.ko ]; then
	echo "load dsplog driver "
	modprobe dsplog
fi

if [ -r /usr/local/bin/ImageServerDaemon.sh ]; then
	if [ -x /usr/local/bin/ImageServerDaemon.sh ]; then
		echo "==============================================="
		echo "Exec: /usr/local/bin/ImageServerDaemon.sh"
		echo "==============================================="
		/usr/local/bin/ImageServerDaemon.sh start
	fi
fi

exit $?

