##
## rootfs/make.inc
##
## History:
##    2012/05/29 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_MKFS), y)

ROOTFS_DIR		:= $(call my-dir)
PRE_BUSYBOX_DIR		:= $(AMB_TOPDIR)/prebuild/third-party/$(AMBARELLA_CPU_ARCH)/busybox
EXT_BUSYBOX_DIR		:= $(AMB_TOPDIR)/../external/busybox

####### Basic Filesystem #######

include $(ROOTFS_DIR)/ext_bb/make.inc
include $(ROOTFS_DIR)/misc/ambarella_keymap/make.inc

.PHONY: basic_fs

ifeq ($(CONFIG_AMBARELLA_BUSYBOX_BUILD), y)
basic_fs: build_extbb
else
basic_fs: prebuild_bb
endif

basic_fs: $(DOT_CONFIG)
ifeq ($(CONFIG_CPU_ARM1136JS), y)
	@cp -dpRf $(SYS_LIB_DIR_ARMV6)/*.so $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(SYS_LIB_DIR_ARMV6)/*.so.* $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(GCC_LIB_DIR_ARMV6)/libgcc*.so* $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(CPP_LIB_DIR_ARMV6)/libstdc++.so* $(FAKEROOT_DIR)/usr/lib/
else ifeq ($(CONFIG_CPU_CORTEXA9), y)
	@cp -dpRf $(SYS_LIB_DIR_ARMV7_SF)/*.so $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(SYS_LIB_DIR_ARMV7_SF)/*.so.* $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(GCC_LIB_DIR_ARMV7_SF)/libgcc*.so* $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(CPP_LIB_DIR_ARMV7_SF)/libstdc++.so* $(FAKEROOT_DIR)/usr/lib/
else ifeq ($(CONFIG_CPU_CORTEXA9_HF), y)
	@cp -dpRf $(SYS_LIB_DIR_ARMV7)/*.so $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(SYS_LIB_DIR_ARMV7)/*.so.* $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(GCC_LIB_DIR_ARMV7)/libgcc*.so* $(FAKEROOT_DIR)/lib/
	@cp -dpRf $(CPP_LIB_DIR_ARMV7)/libstdc++.so* $(FAKEROOT_DIR)/usr/lib/
endif
	@find $(FAKEROOT_DIR) -name "libgcc_s.so" | xargs rm -f
	@find $(FAKEROOT_DIR) -name "libstdc++.so.*.py" | xargs rm -f
	@rm -rf $(FAKEROOT_DIR)/lib/*.*a*
	@echo "export AMBARELLA_ARCH=$(AMBARELLA_ARCH)" > $(FAKEROOT_DIR)/etc/ambarella.conf
	@echo "export SYS_BOARD_BSP=$(AMB_BOARD)" >> $(FAKEROOT_DIR)/etc/ambarella.conf
	@echo "export SYS_BOARD_VERSION=$(BOARD_VERSION_STR)" >> $(FAKEROOT_DIR)/etc/ambarella.conf
	@chmod 644 $(FAKEROOT_DIR)/etc/ambarella.conf
	@echo "Prepare BasicFS Done"

$(call add-target-into-build, basic_fs)


####### Prepare to add root password  #######
ifneq ($(CONFIG_AMBARELLA_ROOT_PASSWORD),"")
.PHONY: add_root_password
add_root_password: basic_fs
	@python $(ROOTFS_DIR)/script/create_rootpass.py $(CONFIG_AMBARELLA_ROOT_PASSWORD) $(FAKEROOT_DIR);

$(call add-target-into-build, add_root_password)
endif

####### Prepare to add default user with password  #######
ifeq ($(CONFIG_AMBARELLA_NORMAL_USER), y)
.PHONY: add_normal_user_password
add_normal_user_password: basic_fs
	@python $(ROOTFS_DIR)/script/add_normal_user.py $(CONFIG_AMBARELLA_NORMAL_USER_NAME) \
	$(CONFIG_AMBARELLA_NORMAL_USER_PASSWORD) $(FAKEROOT_DIR);
$(call add-target-into-build, add_normal_user_password)
endif

####### Update /etc/inittab Configuration #######

.PHONY: configure_inittab

configure_inittab: basic_fs
	@cp -dpRf $(PRE_BUSYBOX_DIR)/etc/inittab $(FAKEROOT_DIR)/etc/inittab
	@if [ "$(CONFIG_AMBARELLA_AUTO_TELNETD)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Start telnetd" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/sbin/telnetd" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_TFTPD_SUPPORT)" = "y" ]; then \
		echo "21 stream tcp nowait root ftpd ftpd -w /" > $(FAKEROOT_DIR)/etc/inetd.conf; \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Start inetd" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/sbin/inetd" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_SERIAL_PORT0_GETTY)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Put a getty on the serial port 0" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS0::respawn:/sbin/getty -L ttyS0 115200 vt100" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_SERIAL_PORT1_GETTY)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Put a getty on the serial port 1" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS1::respawn:/sbin/getty -L ttyS1 115200 vt100" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_ASKFIRST_CONSOLE)" = "y" ]; then \
		echo "::askfirst:-/bin/sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_SERIAL_PORT_USB0)" = "y" ]; then \
		echo "export SYS_USB_G_TYPE=serial" >> $(FAKEROOT_DIR)/etc/ambarella.conf; \
		echo "export SYS_USB_G_PARAMETER=use_acm=1" >> $(FAKEROOT_DIR)/etc/ambarella.conf; \
		echo "ttyGS0::respawn:/sbin/getty -L ttyGS0 115200 vt100" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_START_WATCHDOG)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Start watchdog" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/sbin/watchdog -t 5 /dev/watchdog" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_ADD_PARTITION)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Add partition" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/local/bin/add_partition.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_AUTO_DEMO)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run auto demo" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/local/bin/demo.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_AUTO_DEMO_PLAYBACK_4K)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run auto playback 4k demo" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/local/bin/playback_4k.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_BSP_BOARD_S2ATB)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run s2atb auto test" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS0::once:/usr/local/bin/s2atb.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_SHMOO_TEST)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run shmoo test" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS0::once:/usr/local/bin/shmoo.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_MEDIASERVER)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run mediaserver demo" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS0::once:/usr/local/bin/mediaserver.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_DEWARP)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run fisheye dewarp demo" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS0::once:/usr/local/bin/fisheye_dewarp.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_RUN_IVAAPP)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run ivaapp" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "ttyS0::once:/usr/local/bin/ivaapp.sh" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(BUILD_AMBARELLA_APP_IPCAM_WEB_CGI_PARTNER_Y)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run FastCGI" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/bin/spawn-fcgi -f /webSvr/web/cgi-bin/v1/api_router.cgi -s /tmp/fcgi.socket" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@if [ "$(CONFIG_AMBARELLA_WEB_SERVICE_SUPPORT)" = "y" ]; then \
		echo "" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "# Run Lighttpd" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::sysinit:/bin/mkdir /tmp/weblog" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::sysinit:/bin/mkdir /tmp/cgiupload" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::sysinit:/bin/mount --bind /tmp/weblog /webSvr/logs" >> $(FAKEROOT_DIR)/etc/inittab; \
		echo "null::once:/usr/sbin/lighttpd -f /etc/lighttpd.conf -m /usr/lib/" >> $(FAKEROOT_DIR)/etc/inittab; \
	fi
	@echo "Configure inittab Done"

$(call add-target-into-build, configure_inittab)


####### Update /etc/fstab Configuration #######

.PHONY: configure_fstab

configure_fstab: basic_fs
	@cp -dpRf $(PRE_BUSYBOX_DIR)/etc/fstab $(FAKEROOT_DIR)/etc/fstab
	@if [ -n "$(CONFIG_AMBARELLA_MOUNT_MNT)" ]; then \
		echo "# NFS configuration for ttyS0" >> $(FAKEROOT_DIR)/etc/fstab; \
		echo "$(CONFIG_AMBARELLA_MOUNT_MNT)" >> $(FAKEROOT_DIR)/etc/fstab; \
	fi
	@echo "Configure fstab Done"

$(call add-target-into-build, configure_fstab)


####### Update /etc/profile Configuration #######

.PHONY: configure_profile

configure_profile: basic_fs
	@cp -dpRf $(PRE_BUSYBOX_DIR)/etc/profile $(FAKEROOT_DIR)/etc/profile
	@if [ "$(CONFIG_AMBARELLA_DBUS_SUPPORT)" = "y" ]; then \
		echo "if [ -f /tmp/dbus/dbus-session-bus-address ]; then . /tmp/dbus/dbus-session-bus-address; fi" \
			>> $(FAKEROOT_DIR)/etc/profile; \
	fi
	@if [ "$(CONFIG_AMBARELLA_QT_SUPPORT)" = "y" ]; then \
		echo "export QWS_MOUSE_PROTO=Tslib:/dev/input/event1" >> $(FAKEROOT_DIR)/etc/profile; \
		echo "export QWS_KEYBOARD=linuxinput:/dev/input/event0:keymap=/usr/share/qt4/keymap/A5s.qmap" \
			>> $(FAKEROOT_DIR)/etc/profile; \
		echo "export QWS_DISPLAY=ambafb:csc:/dev/fb0" >> $(FAKEROOT_DIR)/etc/profile; \
		echo "export TSLIB_TSDEVICE=/dev/input/event1" >> $(FAKEROOT_DIR)/etc/profile; \
		echo "export TSLIB_CONFFILE=/etc/ts.conf" >> $(FAKEROOT_DIR)/etc/profile; \
		echo "export TSLIB_PLUGINDIR=/usr/lib/ts" >> $(FAKEROOT_DIR)/etc/profile; \
		echo "export TSLIB_CALIBFILE=/etc/pointercal" >> $(FAKEROOT_DIR)/etc/profile; \
	fi
	@echo "if [ -f /etc/TZ ]; then export TZ=\`cat /etc/TZ\`; fi" >> $(FAKEROOT_DIR)/etc/profile
	@if [ "$(CONFIG_AMBARELLA_INIT_SYSTEMD)" = "y" ]; then \
		echo "if [ -f /etc/ambarella.conf ]; then . /etc/ambarella.conf; fi" >> $(FAKEROOT_DIR)/etc/profile; \
	fi
	@echo "Configure profile Done"

$(call add-target-into-build, configure_profile)


####### Configure Network #######

.PHONY: configure_network

configure_network: add_board_rootfs
	@NETCONF=$(FAKEROOT_DIR)/etc/network/interfaces; \
	cp -dpRf $(PRE_BUSYBOX_DIR)/etc/network/interfaces $(FAKEROOT_DIR)/etc/network/interfaces; \
	if [ "$(CONFIG_AMBARELLA_ETH0)" = "y" ]; then \
		echo "" >> $$NETCONF; \
		echo "auto eth0" >> $$NETCONF; \
		if [ "$(CONFIG_AMBARELLA_ETH0_DHCP)" = "y" ]; then \
			echo "iface eth0 inet dhcp" >> $$NETCONF; \
		else \
			if [ -n "$(CONFIG_AMBARELLA_ETH0_ADDRESS)" -a -n "$(CONFIG_AMBARELLA_ETH0_IP_MASK)" ]; then \
				echo "iface eth0 inet static" >> $$NETCONF; \
				echo "address $(CONFIG_AMBARELLA_ETH0_ADDRESS)" >> $$NETCONF; \
				echo "netmask $(CONFIG_AMBARELLA_ETH0_IP_MASK)" >> $$NETCONF; \
				if [ -n $(CONFIG_AMBARELLA_ETH0_GW_ADDRESS) ]; then \
					echo "gateway $(CONFIG_AMBARELLA_ETH0_GW_ADDRESS)" >> $$NETCONF; \
				fi \
			fi \
		fi \
	fi; \
	if [ "$(CONFIG_AMBARELLA_ETH1)" = "y" ]; then \
		echo "" >> $$NETCONF; \
		echo "auto eth1" >> $$NETCONF; \
		if [ "$(CONFIG_AMBARELLA_ETH1_DHCP)" = "y" ]; then \
			echo "iface eth1 inet dhcp" >> $$NETCONF; \
		else \
			if [ -n "$(CONFIG_AMBARELLA_ETH1_ADDRESS)" -a -n "$(CONFIG_AMBARELLA_ETH1_IP_MASK)" ]; then \
				echo "iface eth1 inet static" >> $$NETCONF; \
				echo "address $(CONFIG_AMBARELLA_ETH1_ADDRESS)" >> $$NETCONF; \
				echo "netmask $(CONFIG_AMBARELLA_ETH1_IP_MASK)" >> $$NETCONF; \
				if [ -n $(CONFIG_AMBARELLA_ETH1_GW_ADDRESS) ]; then \
					echo "gateway $(CONFIG_AMBARELLA_ETH1_GW_ADDRESS)" >> $$NETCONF; \
				fi \
			fi \
		fi \
	fi; \
	if [ "$(CONFIG_AMBARELLA_ETH0)" != "y" -a "$(CONFIG_AMBARELLA_ETH1)" != "y" -o "$(CONFIG_AMBARELLA_NETWORKMANAGER_SUPPORT)" = "y" ]; then \
		rm -rf $(FAKEROOT_DIR)/etc/init.d/S40network; \
	fi
	@echo "Configure network Done"

$(call add-target-into-build, configure_network)


ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC), y)
####### Prepare initd Files for Specific Board  #######

.PHONY: add_initd

add_initd: basic_fs
	@if [ "$(CONFIG_AMBARELLA_SHARED_INITD_SUPPORT)" = "y" ]; then \
		cp -dpRf $(ROOTFS_DIR)/init.d/S* $(FAKEROOT_DIR)/etc/init.d/; \
		chmod +x $(FAKEROOT_DIR)/etc/init.d/*; \
	fi
	@echo "Add init.d Done"

$(call add-target-into-build, add_initd)

endif

####### board default rootfs #######

.PHONY: add_board_rootfs

ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC), y)
add_board_rootfs: add_initd prebuild_udev
endif
ifeq ($(CONFIG_AMBARELLA_INIT_SYSTEMD), y)
add_board_rootfs: basic_fs prebuild_systemd
endif
add_board_rootfs: prebuild_alsa
add_board_rootfs:
	@cp -dpRf $(BOARD_ROOTFS_DIR)/* $(FAKEROOT_DIR)/ && chmod +x $(FAKEROOT_DIR)/etc/init.d/S*
ifeq ($(CONFIG_AMBARELLA_INIT_SYSTEMD), y)
	@rm -rf $(FAKEROOT_DIR)/etc/init.d/S*
endif
ifeq ($(CONFIG_AMBARELLA_INIT_LINUXRC), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/systemd
	@rm -rf $(FAKEROOT_DIR)/etc/systemd
endif

$(call add-target-into-build, add_board_rootfs)

####### Add scripts #######

.PHONY: add_scripts

add_scripts:
	@if [ "$(CONFIG_AMBARELLA_SCRIPT_SUPPORT)" = "y" ]; then \
		mkdir -p $(FAKEROOT_DIR)/usr/local/bin/; \
		cp -dpRf $(ROOTFS_DIR)/script/* $(FAKEROOT_DIR)/usr/local/bin/; \
		chmod +x $(FAKEROOT_DIR)/usr/local/bin/*; \
		mkdir -p $(FAKEROOT_DIR)/lib/firmware/; \
		cp -dpRf $(ROOTFS_DIR)/misc/EDID.bin $(FAKEROOT_DIR)/lib/firmware/; \
	fi
$(call add-target-into-build, add_scripts)

####### Remove init.sh when board type is xman cloudcam peard #######

.PHONY: remove_config

remove_config: add_board_rootfs
	@if [ "$(CONFIG_BSP_BOARD_CLOUDCAM)" = "y" ]; then \
		rm -f $(FAKEROOT_DIR)/usr/local/bin/init.sh; \
	fi
	@if [ "$(CONFIG_BSP_BOARD_XMAN)" = "y" ]; then \
		rm -f $(FAKEROOT_DIR)/usr/local/bin/init.sh; \
	fi
	@if [ "$(CONFIG_BSP_BOARD_PEAR)" = "y" ]; then \
		rm -f $(FAKEROOT_DIR)/usr/local/bin/init.sh; \
	fi

$(call add-target-into-build, remove_initsh)

####### Rules to Build Rootfs Image  #######

# Nand Page/Block Size
MTD_PAGESIZE		:= 0x0200
MTD_PEBSIZE		:= 0x4000
UBI_LEBSIZE		:= 0x3C00
UBI_SUBPAGESIZE	:= 0x0200
UBIFSOUTFLAG		:= -F -o

ifeq ($(CONFIG_NAND_P512_E16K), y)
MTD_PAGESIZE		:= 0x0200
MTD_PEBSIZE		:= 0x4000
UBI_LEBSIZE		:= 0x3C00
endif
ifeq ($(CONFIG_NAND_P2K_E64K), y)
MTD_PAGESIZE		:= 0x00800
MTD_PEBSIZE		:= 0x10000
UBI_LEBSIZE		:= 0x0F000
UBI_SUBPAGESIZE	:= 0x0800
endif
ifeq ($(CONFIG_NAND_P2K_E128K), y)
MTD_PAGESIZE		:= 0x00800
MTD_PEBSIZE		:= 0x20000
UBI_LEBSIZE		:= 0x1F000
UBI_SUBPAGESIZE	:= 0x0800
endif
ifeq ($(CONFIG_NAND_P4K_E64K), y)
MTD_PAGESIZE		:= 0x01000
MTD_PEBSIZE		:= 0x10000
UBI_LEBSIZE		:= 0x0E000
UBI_SUBPAGESIZE	:= 0x01000
endif
ifeq ($(CONFIG_NAND_P4K_E128K), y)
MTD_PAGESIZE		:= 0x01000
MTD_PEBSIZE		:= 0x20000
UBI_LEBSIZE		:= 0x1E000
UBI_SUBPAGESIZE	:= 0x01000
endif


# Build Image

.PHONY: build_fsimage_common build_fsimage

build_fsimage:

build_fsimage_common: $(DOT_CONFIG)
	@echo "  Build $(PRIVATE_FS_TYPE) ..."
	@mkdir -p $(ROOTFS_OUT_DIR)
	@if [ -n "$(CONFIG_AMBARELLA_EXT_CMD_FILE)" ]; then \
		if [ -n "" ]; then \
			cp -dpRf $(CONFIG_AMBARELLA_EXT_CMD_FILE) $(ROOTFS_OUT_DIR)/.tmp_ext_cmd; \
			sed 's/$$(AMBABUILD_TOPDIR)/$(AMBABUILD_STR)/g' $(ROOTFS_OUT_DIR)/.tmp_ext_cmd \
					> $(ROOTFS_OUT_DIR)/.ext_cmd; \
			sed 's/$$(FAKEROOT_DIR)/$(FAKEROOT_STR)/g' $(ROOTFS_OUT_DIR)/.ext_cmd \
					> $(ROOTFS_OUT_DIR)/.tmp_ext_cmd; \
			sed 's/$$(ROOTFS_DIR)/$(ROOTFS_STR)/g' $(ROOTFS_OUT_DIR)/.tmp_ext_cmd \
					> $(ROOTFS_OUT_DIR)/.ext_cmd; \
			sed 's/$$(BSP_DIR)/$(AMB_BOARD)/g' $(ROOTFS_OUT_DIR)/.ext_cmd \
					> $(ROOTFS_OUT_DIR)/.tmp_ext_cmd; \
			sed 's/$$(BOARD_VERSION)/$(BOARD_VERSION_STR)/g' $(ROOTFS_OUT_DIR)/.tmp_ext_cmd \
					> $(ROOTFS_OUT_DIR)/.ext_cmd; \
			chmod +x $(ROOTFS_OUT_DIR)/.ext_cmd; \
			sh $(ROOTFS_OUT_DIR)/.ext_cmd; \
		fi; \
	fi
	@find $(FAKEROOT_DIR) -type d | xargs chmod 755
	@find $(FAKEROOT_DIR) -type f -name *.so | xargs chmod 755
	@find $(FAKEROOT_DIR) -type f -name *.so.* | xargs chmod 755
ifneq ($(BUILD_AMBARELLA_APP_DEBUG), y)
	@find $(FAKEROOT_DIR) -type f -executable -exec sh -c "file -i '{}' | grep -q 'x-sharedlib; charset=binary'" \; -print | xargs $(CROSS_COMPILE)strip -s
	@find $(FAKEROOT_DIR) -type f -executable -exec sh -c "file -i '{}' | grep -q 'x-executable; charset=binary'" \; -print | grep -v gdb | xargs $(CROSS_COMPILE)strip -s
endif
	@rm -rf $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@touch $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "chown -R 0:0 $(FAKEROOT_DIR)" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "$(MAKEDEVS) -d $(ROOTFS_DIR)/device_table.txt $(FAKEROOT_DIR)" \
			>> $(ROOTFS_OUT_DIR)/_fakeroot.sh;


ifeq ($(CONFIG_AMBARELLA_ROOTFS_UBIFS), y)
.PHONY: build_ubifs
build_ubifs: PRIVATE_FS_TYPE:=UBIFS
build_ubifs: build_fsimage_common
	@echo "cp -dpRf $(ROOTFS_DIR)/ubicfg.ini $(ROOTFS_OUT_DIR)/ubicfg.ini " \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "$(MKUBIFS) -r $(FAKEROOT_DIR) \
		-m $(MTD_PAGESIZE) -e $(UBI_LEBSIZE) -c 2047 $(UBIFSOUTFLAG) $(ROOTFS_OUT_DIR)/ubifs.img" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "$(UBINIZE) -o $(ROOTFS_OUT_DIR)/ubifs \
		-p $(MTD_PEBSIZE) -m $(MTD_PAGESIZE) -s $(UBI_SUBPAGESIZE) $(ROOTFS_OUT_DIR)/ubicfg.ini" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_ubifs
endif

ifeq ($(CONFIG_AMBARELLA_ROOTFS_SQUASHFS), y)
.PHONY: build_squashfs

bline:=$(shell grep -nr "^stop()" $(FAKEROOT_DIR)/etc/init.d/S11init | cut -d':' -f1)
cline:=$(shell echo "$(bline)-2"|bc)
build_squashfs: PRIVATE_FS_TYPE:=SQUASHFS
build_squashfs: build_fsimage_common
	@sed -i "$(cline)i makedevs -d /etc/squashfs_table.txt /" $(FAKEROOT_DIR)/etc/init.d/S11init
	@cp -rf $(ROOTFS_DIR)/squashfs_table.txt $(FAKEROOT_DIR)/etc
	@echo "udev /dev devtmpfs rw,mode=0755 0 0" >> $(FAKEROOT_DIR)/etc/fstab
	@echo "if [ -e $(ROOTFS_OUT_DIR)/squashfs ];then rm -f $(ROOTFS_OUT_DIR)/squashfs; fi" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "$(MKSQUASHFS)  $(FAKEROOT_DIR) $(ROOTFS_OUT_DIR)/squashfs -comp xz" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@sed -i "/makedevs/d" $(FAKEROOT_DIR)/etc/init.d/S11init
	@rm -f $(FAKEROOT_DIR)/etc/squashfs_table.txt
	@sed -i "/devtmpfs/d" $(FAKEROOT_DIR)/etc/fstab

build_fsimage: build_squashfs
endif

ifeq ($(CONFIG_AMBARELLA_ROOTFS_JFFS2), y)

.PHONY: build_jffs2

build_jffs2: PRIVATE_FS_TYPE:=JFFS2
build_jffs2: build_fsimage_common
	@echo "mkfs.jffs2 -p --root=$(FAKEROOT_DIR) \
		-e $(MTD_PEBSIZE) -s $(MTD_PAGESIZE) -o $(ROOTFS_OUT_DIR)/jffs2 -l -n -m priority" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_jffs2
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_YAFFS), y)

.PHONY: build_yaffs2

build_yaffs2: PRIVATE_FS_TYPE:=YAFFS2
build_yaffs2: build_fsimage_common
	echo "shfos"
	@echo "$(MKYAFFS2) $(FAKEROOT_DIR) $(ROOTFS_OUT_DIR)/yaffs2" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_yaffs2
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_CPIO), y)

.PHONY: build_cpio

build_cpio: PRIVATE_FS_TYPE:=CPIO
build_cpio: build_fsimage_common
	@echo "cd $(FAKEROOT_DIR) && find . | cpio --quiet -o -H newc \
		> $(ROOTFS_OUT_DIR)/rootfs.cpio" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@ln -s -f sbin/init $(FAKEROOT_DIR)/init
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@if [ "$(CONFIG_AMBARELLA_COMPRESS_RAMDISK)" = "y" ]; then \
		gzip -f -9 $(ROOTFS_OUT_DIR)/rootfs.cpio; \
	else \
		mv $(ROOTFS_OUT_DIR)/rootfs.cpio $(ROOTFS_OUT_DIR)/rootfs.cpio.gz; \
	fi
	@$(MAKE) $(AMBA_MAKE_PARA) -C $(LINUX_SRC_DIR) O=$(LINUX_OUT_DIR) \
		CONFIG_INITRAMFS_SOURCE="$(ROOTFS_OUT_DIR)/rootfs.cpio.gz" \
		CONFIG_INITRAMFS_ROOT_UID=0 CONFIG_INITRAMFS_ROOT_GID=0 all
	@cp -dpRf $(LINUX_OUT_DIR)/arch/arm/boot/Image $(KERNEL_OUT_DIR)/Image

build_fsimage: build_cpio
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_RAM), y)

.PHONY: build_ramdisk

build_ramdisk: PRIVATE_FS_TYPE:= RAM DISK
build_ramdisk: build_fsimage_common
	@echo "genext2fs -b $(CONFIG_AMBARELLA_ROOTFS_SIZE) \
		-N $(CONFIG_AMBARELLA_ROOTFS_INODES) -d $(FAKEROOT_DIR) \
		-U -m 0 $(ROOTFS_OUT_DIR)/ramdisk" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@if [ "$(CONFIG_AMBARELLA_COMPRESS_RAMDISK)" = "y" ]; then \
		gzip -f -9 $(ROOTFS_OUT_DIR)/ramdisk; \
	else \
		mv $(ROOTFS_OUT_DIR)/ramdisk $(ROOTFS_OUT_DIR)/ramdisk.gz; \
	fi

build_fsimage: build_ramdisk
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_EXT2), y)

.PHONY: build_ext2

build_ext2: PRIVATE_FS_TYPE:=EXT2
build_ext2: build_fsimage_common
	@echo "genext2fs -b $(CONFIG_AMBARELLA_ROOTFS_SIZE) -d $(FAKEROOT_DIR) \
		-N $(CONFIG_AMBARELLA_ROOTFS_INODES) $(ROOTFS_OUT_DIR)/ext2 " \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "fsck -t ext2 -y $(ROOTFS_OUT_DIR)/ext2" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_ext2
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_EXT3), y)

.PHONY: build_ext3

build_ext3: PRIVATE_FS_TYPE:=EXT3
build_ext3: build_fsimage_common
	@echo "genext2fs -b $(CONFIG_AMBARELLA_ROOTFS_SIZE) -d $(FAKEROOT_DIR) \
		-N $(CONFIG_AMBARELLA_ROOTFS_INODES) $(ROOTFS_OUT_DIR)/ext3 " \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "tune2fs -j $(ROOTFS_OUT_DIR)/ext3" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "fsck -t ext3 -y $(ROOTFS_OUT_DIR)/ext3" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_ext3
endif


ifeq ($(CONFIG_AMBARELLA_ROOTFS_EXT4), y)

.PHONY: build_ext4

build_ext4: PRIVATE_FS_TYPE:=EXT4
build_ext4: build_fsimage_common
	@echo "genext2fs -b $(CONFIG_AMBARELLA_ROOTFS_SIZE) -d $(FAKEROOT_DIR) \
		-N $(CONFIG_AMBARELLA_ROOTFS_INODES) $(ROOTFS_OUT_DIR)/ext4 " \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "tune2fs -j -O extents,dir_index $(ROOTFS_OUT_DIR)/ext4" \
		>> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@echo "fsck -t ext4 -y $(ROOTFS_OUT_DIR)/ext4" >> $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@chmod +x $(ROOTFS_OUT_DIR)/_fakeroot.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/_fakeroot.sh

build_fsimage: build_ext4
endif


ifeq ($(CONFIG_BACKUP_CPIO),y)
.PHONY: build_backup_cpio

build_backup_cpio:
	@echo "Build backup cpio"
	@rm -f $(ROOTFS_OUT_DIR)/backup_cpio.sh $(ROOTFS_OUT_DIR)/backup_cpio.gz && touch $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@echo "tar -xf $(ROOTFS_DIR)/mini_fake.bz2 -C $(ROOTFS_OUT_DIR)" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@echo "cp -dp $(PRE_BUSYBOX_DIR)/usr/share/udhcpc/default.script $(ROOTFS_OUT_DIR)/mini_fake/usr/share/udhcpc/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@echo "cp -dp $(FAKEROOT_DIR)/usr/local/bin/cloudcam_pba_upgrade $(ROOTFS_OUT_DIR)/mini_fake/usr/bin/" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@echo "chown -R 0:0 $(ROOTFS_OUT_DIR)/mini_fake" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@echo "$(MAKEDEVS) -d $(ROOTFS_DIR)/device_table.txt $(ROOTFS_OUT_DIR)/mini_fake" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@echo "cd $(ROOTFS_OUT_DIR)/mini_fake && find . | cpio --quiet -o -H newc > $(ROOTFS_OUT_DIR)/backup.cpio" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@echo "gzip -f -9 $(ROOTFS_OUT_DIR)/backup.cpio" >> $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@chmod u+x $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@cd $(ROOTFS_OUT_DIR); fakeroot -- $(ROOTFS_OUT_DIR)/backup_cpio.sh
	@cp  $(LINUX_OUT_DIR)/arch/arm/boot/Image $(LINUX_OUT_DIR)/arch/arm/boot/Image_ori
	@$(MAKE)  -C $(LINUX_SRC_DIR) O=$(LINUX_OUT_DIR) \
	                CONFIG_INITRAMFS_SOURCE="$(ROOTFS_OUT_DIR)/backup.cpio.gz" \
			CONFIG_INITRAMFS_ROOT_UID=0 CONFIG_INITRAMFS_ROOT_GID=0 CONFIG_MMC=y CONFIG_MMC_AMBARELLA=y CONFIG_MMC_BLOCK=y all
	@cp $(LINUX_OUT_DIR)/arch/arm/boot/Image_ori  $(LINUX_OUT_DIR)/arch/arm/boot/Image
	@cp -dpRf $(LINUX_OUT_DIR)/arch/arm/boot/zImage $(KERNEL_OUT_DIR)/zImage

build_fsimage: build_backup_cpio
endif


endif

