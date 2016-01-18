#ifndef _SENSORS_H
#define _SENSORS_H

// SENSOR IDs
#define GENERIC_SENSOR			(127)	//changed to 127 in new DSP firmware
#define MICRON_MT9T001			(1)
#define ALTASENS_PROCAMHD		(2)
#define SONY_ICX495CCD			(3)
#define MICRON_MT9P001			(4)	//micron 5meg, same for MT9P031
#define OV5260				(5)
#define MICRON_MT9M002			(6)	// micron 1.6meg
#define MICRON_MT9P401			(7)
#define Altasens_ALTA3372		(8)
#define Altasens_ALTA5262		(9)
#define SONY_IMX017			(10)
#define MICRON_MT9N001			(11)	// micron 9meg
#define SONY_IMX039			(12)
#define MICRON_MT9J001			(13)	// micron 10meg
#define SEMCOYHD			(14)
#define SONYIMX044			(15)
#define OV5633				(16)
#define OV9710				(17)
#define OV9715				(17)
#define MICRON_MT9V136			(18)
#define OV7720				(19)
#define OV7725				(19)
#define OV7740				(20)
#define OV10620				(21)
#define OV10630				(21)
#define MICRON_MT9P031			(22)	// variant of micron5m
#define OV9810				(23)
#define OV8810				(24)
#define SONYIMX032			(25)
#define MICRON_MT9P014			(26)
#define OV2710				(27)
#define OV2715				(27)
#define OV5653				(28)
#define SONYIMX035			(29)
#define MICRON_MT9M033			(30)
#define OV7962				(31)
#define SEC_S5K4AWFX			(32)	//Samsung
#define OV14810				(29)
#define SONYIMX072			(33)
#define SONYIMX122			(34)
#define SONYIMX121			(35)
#define SONYIMX172			(36)
#define SONYIMX078			(37)
#define SONYIMX226			(38)

#define MICRON_MT9T002			(32)	// copy from uItron iOne
#define MICRON_AR0331			(42)
#define SAMSUNG_S5K5B3GX		(32)
#define ALTERA_FPGAVIN				(43)

// SENSOR READ-OUT MODES
#define SONYICX495_FRAME_READ_OUT				(0x0)
#define SONYICX495_4_16_HORI					(0x1)
#define SONYICX495_4_16_NO_HORI					(0x2)
#define SONYICX495_4_8_HORI					(0x3)
#define SONYICX495_4_8_NO_HORI					(0x4)
#define SONYICX495_AF_MODE					(0x5)

#define MICRON_MP9T001_2xV_2xH_Binning				(0x1)
#define MICRON_MP9T001_CFA_PHHASE_CORRECTION_VERTICAL_ONLY	(0x2)
#define MICRON_MP9T001_CFA_PHHASE_CORRECTION_HORIZONTAL_ONLY	(0x3)

#endif

