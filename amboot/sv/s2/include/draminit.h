#ifndef _DRAMINIT_H_
#define _DRAMINIT_H_

#include "usb.h"

#if defined( __MINGW32__ ) && defined ( _USB_ )
#  ifdef _DRAMINIT_DLL_
#    define DLLIMPEXP __declspec(dllexport)
#  else
#    define DLLIMPEXP __declspec(dllimport)
#  endif
#else
#  define DLLIMPEXP
#endif

/***************************************************************
 * Status codes returned by init_dram_controller
 ***************************************************************/
 
typedef enum {
/** No image file name specified **/
DRAM_ENOINFILE=257,
/** Input image file cannot be read **/
DRAM_ENOACCINFLLE=258,
/** Input image size is incorrect **/
DRAM_ESZINFILE=259,
/** No output file name specified **/
DRAM_ENOOUTFILE=260,
/** Output file with given name cannot be created **/
DRAM_ENOACCOUTFILE=261,

/** USB device handle null **/
DRAM_EUSBDEV=270,
/** USB device write error **/
DRAM_EUSBSZWRITE=271,
/** USB device read error **/
DRAM_EUSBSZREAD =272,
/** USB poll timed out **/
DRAM_EUSBTIMEOUT=273,

/** draminit internal error **/
DRAM_INTERNAL1=300,
/** SUCCESS **/
DRAM_SUCCESS = 0
} init_dram_status_t;

/***************************************************************
 * Does configuration of dram controller using read/write calls
 *  to USB driver.
 * Arguments
 *  udev => pointer to an opened usb_dev_handle
 *  infile => name of a dram parameters table image
 * Return Value
 *  0 on Success, 
 *  Non-Zero on ERROR
 ***************************************************************/

DLLIMPEXP init_dram_status_t init_dram_controller(usb_dev_handle *udev, const char *infile);

#endif
