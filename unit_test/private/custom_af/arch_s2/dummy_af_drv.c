/**
 * unit_test/private_custom_af/arch_s2
 * Control APIs of MF Lens
 *
 * Details  : Dummy af control APIs
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <assert.h>

#include "img_struct_arch.h"

#include "dummy_af_drv.h"


int af_print_level = 0;

#define	PRINT(format, args...)		do {		\
							if (af_print_level >= 1) {			\
								printf(format, ##args);			\
							}									\
						} while (0)

#define	AF_PRINT(mylog, LOG_LEVEL, format, args...)		do {		\
							if (mylog >= LOG_LEVEL) {			\
								printf(format, ##args);			\
							}									\
						} while (0)

#define	ERROR_LEVEL      0
#define 	MSG_LEVEL          1

#define MSG(format, args...)	AF_PRINT(af_print_level, MSG_LEVEL, ">>> " format, ##args)

/**
 * Set the shutter speed
 * 	For CCD, the speed decides when to close the mechanical shutter.
 * 	For CMOS, the speed equals the actual length of electronic
 * 	rolling shutter or the time to close the mechanical shutter
 *
 * @Params ex_mode - AA operation mode
 * @Params ex_time - Exposure time in an internal format of
 *	      	     (4-bit INT + 12-bit Fraction)
 * @Returns - 0 success, < 1 failed
 */
static int set_shutter_speed(u8 ex_mode, u16 ex_time) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

/**
 * General APIs
 */

/**
 * Initialize Lens module to the standby stage
 *
 * @param plens_init - the pointer to the data for initialization
 * @returns - 0 success, < 1 failed
 * @See also lens_init_t
 */

static int lens_init(void) {
	MSG("----Custom AF:%s----\n", __func__);
	return 0;
}

/**
 * Park the lens module before power off
 */
static int lens_park(void) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

/**
 * Zoom Control API
 */

/**
 * Zoom in
 *
 * @Param pps - the pulses per second to move
 * @Param distance - the number of step to move
 * @Returns - 0 success, < 1 failed
 */
static int zoom_in(u16 pps, u32 distance) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

/**
 * Zoom out
 *
 * @Param pps - the pulses per second to move
 * @Param distance - the number of step to move
 * @Returns - 0 success, < 1 failed
 */
static int zoom_out(u16 pps, u32 distance) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

/**
 * Zoom stop
 * @Returns - 0 success, < 1 failed
 */
static int zoom_stop(void) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

/**
 * Focus Control API
 */

/**
 * Focus far
 *
 * @Param pps - the pulses per second to move
 * @Param distance - the number of step to move
 * @Returns - 0 success, < 1 failed
 */

static int focus_far(u16 pps, u32 distance) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

/**
 * Focus near
 *
 * @Param pps - the pulses per second to move
 * @Param distance - the number of step to move
 * @Returns - 0 success, < 1 failed
 */
static int focus_near(u16 pps, u32 distance) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}
/**
 * Focus stop
 *
 * @Returns - 0 success, < 1 failed
 */
static int focus_stop(void) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}


 /**
 * Set aperture
 *
 * @Param aperture_idx -  aperture index, 0 means fully opened.
 * @Returns - 0 success, < 1 failed
 */
static int set_aperture(u16 aperture_idx) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

/**
 * Set the mechanical shutter for snapshot
 *
 * @Param me_shutter -  0: close,  1: open
 * @Returns - 0 success, < 1 failed
 */
static int set_mechanical_shutter(u8 me_shutter) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

/**
 * Enable or disable zoom PI
 * wide:PI = 1
 * tele: PI = 0
 */
static void set_zoom_pi(u8 en) {
	MSG("----Custom AF:%s----\n", __func__);
}

 /**
  * Enable or disable focus PI
 * near:PI = 0
 * far: PI = 1
  */
static void set_focus_pi(u8 en) {
	MSG("----Custom AF:%s----\n", __func__);
}

/**
 * lens motors power saving
 */
static int lens_standby(u8 en) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

/* 1 : Open IR-cut, motor IN ; 0 : Close IR-cut */
static int set_IRCut(u8 enable) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

static int isFocusRuning(void) {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

static int isZoomRuning() {
	MSG("----Custom AF:%s----\n", __func__);
	return FUNC_NOT_SUPPORTED;
}

lens_dev_drv_t dummy_af_drv = {
	.set_IRCut = set_IRCut,
	.set_shutter_speed = set_shutter_speed,
	.lens_init = lens_init,
	.lens_park = lens_park,
	.zoom_stop = zoom_stop,
	.focus_near = focus_near,
	.focus_far = focus_far,
	.zoom_in = zoom_in,
	.zoom_out = zoom_out,
	.focus_stop = focus_stop,
	.set_aperture = set_aperture,
	.set_mechanical_shutter = set_mechanical_shutter,
	.set_zoom_pi = set_zoom_pi,
	.set_focus_pi = set_focus_pi,
	.lens_standby = lens_standby,
	.isFocusRuning = isFocusRuning,
	.isZoomRuning = isZoomRuning
};
