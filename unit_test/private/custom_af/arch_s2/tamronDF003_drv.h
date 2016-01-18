
#define FUNC_NOT_SUPPORTED -1
 
/**
 * For PI control
 */
 typedef enum{
	PI_DN = 0,
	PI_EN = 1,
}PI_status;

/**
 * for lens power saving
 */
#define LENS_STDBY_DN	0
#define LENS_STDBY_EN	1

/*
* ZOOM_idx numbers
*/
#define ZOOM_STEP 32



#define R2A30404NP_SPI_DEV_NODE		"/dev/spidev0.0"

/* for Renesas R2A30404NP 7CH motor driver definition  */

/* Define GPIO usage*/
#define	R_RESET				93
#define	F_PI				46
#define	Z_PI				31

#define	EXT_FOCUS			82
#define	EXT_ZOOM			83
#define	ZOOM_TRIG			84
#define	MOTOR_IN5			84
#define	MOTOR_IN6			85
/***************************************/


#define	INIT_PPS_FOCUS		469
#define	INIT_PPS_ZOOM		469
#define	PRR_ZOOM			PRR_02u
#define	PRR_FOCUS			PRR_02u


/* Drive channel	on the motor driver */
#define	CHANNEL12			0x0000		/* Channels 1 and 2 */
#define	CHANNEL34			0x4000		/* Channels 3 and 4 */
#define	CHANNEL56			0x8000		/* Channels 5 and 6 */
#define	COMM				0xC000		/* Common items */

#define	AF					(CHANNEL12)
#define	ZOOM				(CHANNEL34)
/***************************************/


/* Register address definition for channels */
#define	DIRECTION			0x0000		/* Register 0 */
#define	NO_PULSES1			0x0400		/* Register 1 */
#define	NO_PULSES2			0x0800		/* Register 2 */
#define	PULSE_RATE0			0x0C00		/* Register 3 */
#define	PULSE_RATE_RANGE	0x1000		/* Register 4 */
#define	NO_PULSES_ACC_DEC	0x1400		/* Register 5 */
#define	PULSE_RATE1			0x1800		/* Register 6 */
#define	PULSE_RATE2			0x1C00		/* Register 7 */
#define	PULSE_RATE3			0x2000		/* Register 8 */
#define	PULSE_RATE4			0x2400		/* Register 9 */
#define	PRE_EXCIT			0x2800		/* Register A */
#define	POST_EXCIT			0x2C00		/* Register B */
#define	VOLTAGE				0x3400		/* Register D */
#define	OPERATION_CTRL		0x3C00		/* Register F */


/* Register address for common items */
#define	EXT_OUT				0x0400		/* Register 1 */
#define	PI_CTRL				0x0800		/* Register 2 */
#define	PS_CTRL				0x0C00		/* Register 3 */
#define	INPUT_CTRL			0x1400		/* Register 5 */
#define	CHAN6_CTRL			0x1800		/* Register 6 */
#define	CHAN7_CTRL			0x1C00		/* Register 7 */
#define	SW_RST				0x2000		/* Register 8 */
#define	COMM_OP_CTRL		0x3C00		/* Register F */
/***************************************/


/* For Channel 1 ~ 6 */
/* Register 0 */
#define	FORWARD				0x0000
#define	REVERSE				0x0200
#define	MICRO_MODE			0x0000
#define	MICRO				0x0040
#define	NO_ACC_NO_DEC		0x0000
#define	NO_ACC_DEC			0x0010
#define	ACC_NO_DEC			0x0020
#define	ACC_DEC				0x0030
#define	INIT_POS0			0x0000
#define	INIT_POS1			0x0002
#define	INIT_POS2			0x0004
#define	INIT_POS3			0x0006
#define	INIT_POS4			0x0008
#define	INIT_POS5			0x000A
#define	INIT_POS6			0x000C
#define	INIT_POS7			0x000E
#define	INIT_POS_EN			0x0001

/* Register 4 */
#define	PRR_02u				0x0280		/* set pulse rate range	0.2u sec */
#define	PRR_04u				0x0240		/* set pulse rate range	0.4u sec */
#define	PRR_08u				0x0200		/* set pulse rate range	0.8u sec */
#define	PRR_16u				0x01C0		/* set pulse rate range	1.6u sec */
#define	PRR_32u				0x0180		/* set pulse rate range	3.2u sec */
#define	PRR_64u				0x0140		/* set pulse rate range	6.4u sec */
#define	PRR_128u			0x0100		/* set pulse rate range	12.8u sec */
#define	PRR_256u			0x00C0		/* set pulse rate range	25.6u sec */
#define	PRR_512u			0x0080		/* set pulse rate range	51.2u sec */
#define	PRR_1024u			0x0040		/* set pulse rate range	102.4u sec */
#define	PRR_2048u			0x0000		/* set pulse rate range	204.8u sec */

#define	MICRO64				0x0030		/* 64 steps */
#define	MICRO128			0x0020		/* 128 steps */
#define	MICRO256			0x0010		/* 256 steps */
#define	MICRO512			0x0000		/* 512 steps */

/* Register A */
#define	EXCITIME_0			0x0000		/* 0ms */
#define	EXCITIME_1			0x0010		/* 0.8ms */
#define	EXCITIME_2			0x0020		/* 1.6ms */
#define	EXCITIME_3			0x0030		/* 2.5ms */
#define	EXCITIME_4			0x0040		/* 3.3ms */
#define	EXCITIME_5			0x0050		/* 4.0ms */
#define	EXCITIME_6			0x0060		/* 5.0ms */
#define	EXCITIME_7			0x0070		/* 5.7ms */
#define	EXCITIME_8			0x0080		/* 6.5ms */
#define	EXCITIME_CONTINUE	0x03F0		/* continuous */

/* Register D */
#define	VOLT12_34			0x00C0
#define	VOLT12_36			0x0080
#define	VOLT34_34			0x00C6
#define	VOLT34_36			0x0084
#define	CARRFEQ0			0x0000		/* 230khz */
#define	CARRFEQ1			0x0020		/* 400khz */

/* Register F */
#define	ACTIVE				0x0200
#define	STOP				0x0000
#define	HOLD_ON				0x0100
#define	HOLD_OFF			0x0000
#define	EXCIT_ON			0x0080
#define	EXCIT_OFF			0x0000
/***************************************/


/* For COMM chanel */
/* Register 1 EXT output */
#define	EXT_EXCIT			0x0000
#define	EXT_PULSE			0x0100
#define	EXT12				0x0000
#define	EXT34				0x0000
#define	EXT56				0x0080
#define	MOB34				0x0000

/* Register 2 PI output */
#define	PI1_ON				0x0200
#define	PI1_OFF				0x0000
#define	PI2_ON				0x0100
#define	PI2_OFF				0x0000
#define	PI3_ON				0x0080
#define	PI3_OFF				0x0000

/* Register 3 PS control */
#define	PS_ON				0x0200
#define	PS12_ON			0x0100
#define	PS3_ON				0x0080
#define	PS4_ON				0x0040
#define	PS_OFF				0x0000

/* Register 5 Input pin control */
#define	IN_CTRL				0x0000
#define	EDGE_HIGH			0x0000
#define	EDGE_LOW			0x0080
#define	CH5_SERI_WIRE		0x0000
#define	CH5_TWO_WIRE		0x0040
#define	CH5_EN_IN			0x0000
#define	CH5_IN_IN			0x0020
#define	CH6_SERI_WIRE		0x0000
#define	CH6_TWO_WIRE		0x0010
#define	CH6_EN_IN			0x0000
#define	CH6_IN_IN			0x0008
#define	CH7_SERI_WIRE		0x0000
#define	CH7_TWO_WIRE		0x0004
#define	CH7_EN_IN			0x0000
#define	CH7_IN_IN			0x0002

/* Register 6,7 Channel 6,7 control */
#define	OFF_MODE			0x0000
#define	BRAKE_MODE			0x0100

#define	VOLT_310mv			0x0000		/* Constant current value */
#define	VOLT_300mv			0x0010
#define	VOLT_290mv			0x0020
#define	VOLT_280mv			0x0030
#define	VOLT_270mv			0x0040
#define	VOLT_260mv			0x0050
#define	VOLT_250mv			0x0060
#define	VOLT_240mv			0x0070
#define	VOLT_230mv			0x0080
#define	VOLT_220mv			0x0090
#define	VOLT_210mv			0x00A0
#define	VOLT_200mv			0x00B0
#define	VOLT_190mv			0x00C0
#define	VOLT_180mv			0x00D0
#define	VOLT_170mv			0x00E0
#define	VOLT_160mv			0x00F0

/* Register 7 Channel 7 control */
#define	WEAK_EXC_OFF		0x0000
#define	WEAK_EXC_ON			0x0008

#define	WEAK_VOLT_2			0x0000		/* One-half of constant current value */
#define	WEAK_VOLT_3			0x0004		/* One-third of constant current value */
	
/* Register 8 */
#define	ALL_RESET			0x0380
/***************************************/


#define MICROSTEP_ZOOM		6 //??
#define MICROSTEP_FOCUS		6 // ??
#define U_STEP_ZOOM			16 // ??
#define U_STEP_FOCUS		1 // ??
