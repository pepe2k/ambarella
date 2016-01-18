Unit chip test readme

test steps:
1: config BUILD_AMBARELLA_UNIT_TESTS_CHIP, built test_chip
2: move test_chip_cfg file into sdcard. Insert this sdcard into lower sdcard slot(mmcblk0p1 after reboot). insert another card into upper sdcard slot(mmcblk1p1 after reboot).
   Note: because udev auto link /tmp/mmcblk0p1 and /mmcblk1p1 with /sdcard, two sdcard slots can not be used in the same time, we need to remount these two cards.
	command :# rm /tmp/mmcblk0p1
		 # rm /tmp/mmcblk1p1
		 # mkdir /tmp/mmcblk0p1
		 # mkdir /tmp/mmcblk1p1
		 # mount /dev/mmcblk0p1 /tmp/mmcblk0p1
		 # mount /dev/mmcblk1p1 /tmp/mmcblk1p1

	This issue will be fix in the feature if we remount two card in test_chip code or modify 11-sd-cards-auto-mount.rules.

3: you can see "test_chip -h" to choose module you want to test.
   Note:  
	1) watchdog test will reboot system 
	2) keypad test need to press key, it can not auto run.
	3) USBhost test need to move test_chip_cfg file into a sdcard, connect this card with S2 USBhost by USB hub.

4: if you want to save log in sdcard, use "--savelog", it will save some part of test log in card in mmcblk1p1. 

5: Sensor is mt9t002 default.