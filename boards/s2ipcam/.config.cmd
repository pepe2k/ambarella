deps_config := \
	unit_test/linux/benchmark/AmbaConfig \
	unit_test/AmbaConfig \
	prebuild/third-party/noarch/realtek-wifi/AmbaConfig \
	prebuild/third-party/noarch/broadcom-wifi/AmbaConfig \
	prebuild/third-party/noarch/atheros-wifi-calib/AmbaConfig \
	prebuild/third-party/noarch/atheros-wifi/AmbaConfig \
	prebuild/third-party/AmbaConfig \
	rootfs/misc/ambarella_keymap/AmbaConfig \
	rootfs/AmbaConfig \
	prebuild/AmbaConfig \
	playback/AmbaConfig \
	packages/img_mw/mw/AmbaConfig \
	packages/img_mw/dev/AmbaConfig \
	packages/AmbaConfig \
	kernel/private/lib/AmbaConfig \
	kernel/private/drivers/dsplog/AmbaConfig \
	kernel/private/drivers/p_iris/AmbaConfig \
	kernel/private/drivers/gyro/AmbaConfig \
	kernel/private/drivers/eis/AmbaConfig \
	kernel/private/drivers/fdet/AmbaConfig \
	kernel/private/drivers/crypto/AmbaConfig \
	kernel/private/drivers/debug/AmbaConfig \
	kernel/private/drivers/imgproc/AmbaConfig \
	kernel/private/drivers/iav2/AmbaConfig \
	kernel/private/drivers/iav/AmbaConfig \
	kernel/private/drivers/gpu/sgx/AmbaConfig \
	kernel/private/drivers/gpu/AmbaConfig \
	kernel/private/drivers/dsp/AmbaConfig \
	kernel/private/drivers/lens/AmbaConfig \
	kernel/private/drivers/vout/hdmi/AmbaConfig \
	kernel/private/drivers/vout/digital/AmbaConfig \
	kernel/private/drivers/vout/dve/AmbaConfig \
	kernel/private/drivers/vout/AmbaConfig \
	kernel/private/drivers/vin/sbrg/AmbaConfig \
	kernel/private/drivers/vin/decoders/adv7441a/AmbaConfig \
	kernel/private/drivers/vin/decoders/AmbaConfig \
	kernel/private/drivers/vin/sensors/AmbaConfig \
	kernel/private/drivers/vin/AmbaConfig \
	kernel/private/drivers/hw_timer/AmbaConfig \
	kernel/private/drivers/msg/AmbaConfig \
	kernel/private/drivers/AmbaConfig \
	kernel/external/compat/AmbaConfig \
	kernel/external/atheros/AmbaConfig \
	kernel/AmbaConfig \
	camera/AmbaConfig \
	app/utility/jpg_enc/AmbaConfig \
	app/ipcam/rtsp/AmbaConfig \
	app/ipcam/mediaserver/AmbaConfig \
	app/AmbaConfig \
	amboot/config/amboot.amboot.in \
	amboot/config/amboot.fio.in \
	amboot/config/amboot.mem.in \
	boards/AmbaConfig \
	amboot/AmbaConfig \
	/home/shawn/fly/ambarella/AmbaConfig

.config config.h: \
	$(deps_config)


$(deps_config): ;
