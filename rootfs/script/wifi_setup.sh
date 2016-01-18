#!/bin/sh
##########################
# History:
#	2014/04/01 - [Tao Wu] modify file
#
# Copyright (C) 2004-2014, Ambarella, Inc.
##########################
mode=$1
driver=$2
ssid=$3
passwd=$4
encryption=$5
channel=$6

DEVICE=wlan0

############# WPA CONFIG ###############
CTRL_INTERFACE=/var/run/wpa_supplicant
CONFIG=/tmp/wpa_supplicant.conf
WPA_LOG=/var/log/wpa_supplicant.log
#the max times to kill app
KILL_NUM_MAX=5
#the max timies to connect AP
CONNECT_NUM_MAX=5

############# HOSTAP CONFIG ###############
HOSTAP_CTRL_INTERFACE=/var/run/hostapd
HOST_CONFIG=/tmp/hostapd.conf
DEFAULT_CHANNEL=1
HOST_MAX_STA=5

AP_IP_ADDRESS=192.168.42.1
AP_NETMASK=255.255.255.0
AP_DHCP_START=192.168.42.2
AP_DHCP_END=192.168.42.254

############### Module ID ################
MODULE_ID=AR6003
if [ -e /sys/module/8189es ]; then
	MODULE_ID=RTL8189ES
fi

############### Function ################

# usage
usages()
{
	echo "usage: $0 [sta|ap] [wext|nl80211]  <SSID> <Password> [open|wpa] <Channel>"
	echo "Example:"
	echo "Connect To AP: 		$0 sta nl80211 <SSID>"
	echo "Connect To security AP: $0 sta nl80211 <SSID> <Password>"
	echo "Setup AP[Open]: 	$0 ap nl80211 <SSID> 0 open <Channel>"
	echo "Setup AP[WPA]: 		$0 ap nl80211 <SSID> <Password> wpa <Channel>"
	echo "Stop all APP(STA, AP): 	$0 stop"
	echo "Notice: If you setup AP mode, WPS is enable by default. #hostapd_cli -i<interface> [wps_pbc |wps_pin any <Pin Code> ]"
}

check_param()
{
	if [ ${mode} != "sta" ] && [ ${mode} != "ap" ] && [ ${mode} != "stop" ]; then
		echo "Please Select Mode [sta|ap] or stop"
		exit 1;
	fi

	if [ ${driver} != "wext" ] && [ ${driver} != "nl80211" ]; then
		echo "Please Select Driver [wext|nl80211]"
		exit 1;
	fi

	if [ ${mode} == "ap" ]; then
		if [ ${encryption} != "open" ] && [ ${encryption} != "wpa" ]; then
			echo "Please Select Encryption [open|wpa]"
			exit 1;
		fi
		if [ ${#channel} -gt 0 ]; then
			if [ ${channel} -gt 14 ] || [ ${channel}  -lt 1 ]; then
				echo "Your Channel is wrong(1 ~ 13), using Channl ${DEFAULT_CHANNEL} by default."
				channel=$DEFAULT_CHANNEL
			fi
		else
			echo "Using Channl ${DEFAULT_CHANNEL} by default."
			channel=$DEFAULT_CHANNEL
		fi
	fi

	rm -rf ${CONFIG}
	rm -rf ${HOST_CONFIG}
	mkdir -p /var/lib/misc
}

# kill wpa_supplicant process
kill_apps()
{
	for app in "$@"
	do
		kill_num=0
		while [ "$(pgrep ${app})" != "" ]
		do
			if [ $kill_num -eq $KILL_NUM_MAX ]; then
				echo "Please try execute \"killall ${app}\" by yourself"
				exit 1
			else
				killall -9 ${app}
				sleep 1
			fi
			kill_num=$((kill_num+1));
		done
	done
}

# configurate wap_supplicant config
WPA_SCAN()
{
	wpa_supplicant -D${driver} -i${DEVICE} -C${CTRL_INTERFACE} -B
	wpa_cli -i${DEVICE} scan
	sleep 3
	scan_result=`wpa_cli -i${DEVICE} scan_result`

	kill_apps wpa_supplicant
	echo "=============================================="
	echo "${scan_result}"
	echo "=============================================="
}

generate_wpa_conf()
{
	WPA_SCAN
	scan_entry=`echo "${scan_result}" | tr '\t' ' ' | grep " ${ssid}$" | tail -n 1`

	if [ "${scan_entry}" == "" ]; then
		echo "failed to detect SSID ${ssid}, please try to get close to the AP"
		exit 1
	fi

	echo "ctrl_interface=${CTRL_INTERFACE}" > ${CONFIG}
	echo "network={" >> ${CONFIG}
	echo "ssid=\"${ssid}\"" >> ${CONFIG}

	WEP=`echo "${scan_entry}" | grep WEP`
	WPA=`echo "${scan_entry}" | grep WPA`
	WPA2=`echo "${scan_entry}" | grep WPA2`
	CCMP=`echo "${scan_entry}" | grep CCMP`
	TKIP=`echo "${scan_entry}" | grep TKIP`

	if [ "${WPA}" != "" ]; then
		#WPA2-PSK-CCMP	(11n requirement)
		#WPA-PSK-CCMP
		#WPA2-PSK-TKIP
		#WPA-PSK-TKIP
		echo "key_mgmt=WPA-PSK" >> ${CONFIG}

		if [ "${WPA2}" != "" ]; then
			echo "proto=WPA2" >> ${CONFIG}
		else
			echo "proto=WPA" >> ${CONFIG}
		fi

		if [ "${CCMP}" != "" ]; then
			echo "pairwise=CCMP" >> ${CONFIG}
		else
			echo "pairwise=TKIP" >> ${CONFIG}
		fi
		echo "psk=\"${passwd}\"" >> ${CONFIG}
	fi

	if [ "${WEP}" != "" ] && [ "${WPA}" == "" ]; then
		echo "key_mgmt=NONE" >> ${CONFIG}
	    echo "wep_key0=${passwd}" >> ${CONFIG}
	    echo "wep_tx_keyidx=0" >> ${CONFIG}
	fi

	if [ "${WEP}" == "" ] && [ "${WPA}" == "" ]; then
		echo "key_mgmt=NONE" >> ${CONFIG}
	fi

	echo "}" >> ${CONFIG}
}

# start wpa_supplicant
start_wpa_supplicant()
{
	ifconfig ${DEVICE} up
	wpa_supplicant -D${driver} -i${DEVICE} -c${CONFIG} -f${WPA_LOG} -dd -B
	sleep 3
}

# check wpa status
check_wpa_status()
{
	connect_num=0
	until [ $connect_num -gt $CONNECT_NUM_MAX ]
	do
		WPA_STATUS_CMD=`wpa_cli -i${DEVICE} status`
		WPA_STATUS=${WPA_STATUS_CMD##*wpa_state=}
		WPA_STATUS=${WPA_STATUS:0:9}
		echo "wpa_supplicant status=${WPA_STATUS}"
		if [ "$WPA_STATUS" == "COMPLETED" ]; then
			if [ "$(pgrep wpa_supplicant)" == "" ]; then
				echo ">>>wpa_supplicant isn't running<<<"
				exit 1
			fi
			udhcpc -i${DEVICE}
			echo ">>>wifi_setup OK<<<"
			exit 0
		fi
		sleep 1
		connect_num=$((connect_num+1));
	done

	kill_apps wpa_supplicant
	echo ">>>wifi_setup failed, password may be incorrect or ap not in range<<<"
}

################  AP #####################

generate_hostapd_conf()
{
	#generate hostapd.conf
	echo "interface=wlan0" > ${HOST_CONFIG}
	echo "ctrl_interface=/var/run/hostapd" >> ${HOST_CONFIG}
	echo "beacon_int=100" >> ${HOST_CONFIG}
#	echo "dtim_period=1" >> ${HOST_CONFIG}
	echo "preamble=0" >> ${HOST_CONFIG}
	#WPS support
	echo "wps_state=2" >> ${HOST_CONFIG}
	echo "eap_server=1" >> ${HOST_CONFIG}
	echo "ap_pin=12345670" >> ${HOST_CONFIG}
	echo "config_methods=label display push_button keypad ethernet" >> ${HOST_CONFIG}
	echo "wps_pin_requests=/var/run/hostapd.pin-req" >> ${HOST_CONFIG}
	#general
	echo "ssid=${ssid}" >> ${HOST_CONFIG}
	echo "max_num_sta=${HOST_MAX_STA}" >> ${HOST_CONFIG}
	echo "channel=${channel}" >> ${HOST_CONFIG}

	case ${encryption} in
		open)
			;;
		wpa)
			echo "wpa=2" >> ${HOST_CONFIG}
			echo "wpa_pairwise=CCMP" >> ${HOST_CONFIG}
			echo "wpa_passphrase=${passwd}" >> ${HOST_CONFIG}
			echo "wpa_key_mgmt=WPA-PSK" >> ${HOST_CONFIG}
			;;
		*)
			;;
	esac

	#Realtek rtl8189es
	if [ ${MODULE_ID} == "RTL8189ES" ]; then
		echo "driver=nl80211" >> ${HOST_CONFIG}
		echo "hw_mode=g" >> ${HOST_CONFIG}
		echo "ieee80211n=1" >> ${HOST_CONFIG}
		echo "ht_capab=[SHORT-GI-20][SHORT-GI-40][HT40]" >> ${HOST_CONFIG}
		#echo "wme_enabled=1" >> /tmp/hostapd.conf
		#echo "wpa_group_rekey=86400" >> /tmp/hostapd.conf
	fi
}

start_hostapd_ap()
{
## Setup interface and set IP,gateway##
	ifconfig ${DEVICE} up
	ifconfig ${DEVICE} ${AP_IP_ADDRESS}
	route add default netmask ${AP_NETMASK} gw ${AP_IP_ADDRESS}
## Start Hostapd ##
	hostapd ${HOST_CONFIG} -B
	sleep 3

## Start DHCP Server ##
	dnsmasq --no-daemon --no-resolv --no-poll --dhcp-range=${AP_DHCP_START},${AP_DHCP_END},1h &
	echo "HostAP Setup Finished."
}

################   Main  ###################

if [ $# -eq 1 ] && [ ${mode} == "stop" ]; then
	if [ -x /etc/init.d/S45network-manager ] && [ "$(pidof NetworkManager)" != "" ]; then
		/etc/init.d/S45network-manager stop
	fi
	kill_apps wpa_supplicant udhcpc hostapd dnsmasq
	ifconfig ${DEVICE} down
	exit 0
fi

if [ $# -lt 3 ]; then
	usages
	exit 1
fi

check_param
if [ -x /etc/init.d/S45network-manager ] && [ "$(pidof NetworkManager)" != "" ]; then
	/etc/init.d/S45network-manager stop
fi
kill_apps wpa_supplicant udhcpc hostapd dnsmasq
if [ ${mode} == "sta" ]; then
	generate_wpa_conf
	start_wpa_supplicant
	check_wpa_status
elif [ ${mode} == "ap" ]; then
	freq=$(($channel*5 +2407))
	generate_hostapd_conf
	start_hostapd_ap
else
	usage
fi

########################################
