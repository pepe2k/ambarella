/*
 * ambhw/intvec.h
 *
 * History:
 *	2007/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW_INTVEC_H__
#define __AMBHW_INTVEC_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/* The following IRQ numbers are common for all ARCH */
#if (CHIP_REV == I1)
#define MAX_IRQ_NUMBER		288 /* VIC1:32 + VIC2:32 + VIC3:32
				       + GPIO: 192 = 288 */
#elif (CHIP_REV == A7)
#define MAX_IRQ_NUMBER		224 /* VIC1:32 + VIC2:32 + GPIO: 160 = 224 */
#elif (CHIP_REV == A5) || (CHIP_REV == A6) || (CHIP_REV == A7L)
#define MAX_IRQ_NUMBER		192 /* VIC1:32 + VIC2:32 + GPIO: 128 = 192 */
#elif (CHIP_REV == S2)
#define MAX_IRQ_NUMBER		234 /* VIC1:32 + VIC2:32 + VIC3:32
				       + GPIO: 138 = 178 */
#elif (CHIP_REV == A3) || (CHIP_REV == A5S) || (CHIP_REV == A5L)
#define MAX_IRQ_NUMBER		160 /* VIC1:32 + VIC2:32 + GPIO: 96 = 160 */
#elif (CHIP_REV == A8)
#define MAX_IRQ_NUMBER		112 /* VIC1:32 + VIC2:32 + VIC3:32
				       + GPIO: 16 = 112 */
#else
#define MAX_IRQ_NUMBER		128
#endif

#if (CHIP_REV == A1) || (CHIP_REV == A2)
#define GPIO_VICINT_INSTANCES		2
#elif (CHIP_REV == A3) || (CHIP_REV == A2S) || (CHIP_REV == A2M) ||	\
      (CHIP_REV == A2Q) || (CHIP_REV == A5S) || (CHIP_REV == A5L) ||	\
      (CHIP_REV == S2)
#define GPIO_VICINT_INSTANCES		3
#elif (CHIP_REV == A7)
#define GPIO_VICINT_INSTANCES		5
#elif (CHIP_REV == I1)
#define GPIO_VICINT_INSTANCES		6
#elif (CHIP_REV == A8)
#define GPIO_VICINT_INSTANCES		1
#else
#define GPIO_VICINT_INSTANCES		4
#endif

/* The following are ARCH specific IRQ numbers */

#define USBVBUS_IRQ		0
#define VOUT_IRQ		1
#if (CHIP_REV == A7) || (CHIP_REV == A6) || (CHIP_REV == S2)
#define ORC_VOUT0_IRQ		1
#define ORC_VOUT1_IRQ		(32 + 14)
#elif (CHIP_REV == I1) || (CHIP_REV == A8)
#define ORC_VOUT0_IRQ		1
#define ORC_VOUT1_IRQ		(32 + 12)
#define ENCODE_ORC_VOUT0_IRQ	1		/* A8 */
#define ENCODE_ORC_VOUT1_IRQ	(32 + 12)	/* A8 */
#else
#define ORC_VOUT1_IRQ		1	/* A5S A7L*/
#define ORC_VOUT0_IRQ		(32 + 12)
#endif
#define VIN_IRQ			2
#define ORC_VIN_IRQ		2
#define ENCODE_ORC_VIN_IRQ	2	/* A8 */
#if (CHIP_REV == A7L)
#define VDSP_IRQ		6
#define ORC_VDSP_IRQ		6
#else
#define VDSP_IRQ		3
#define ORC_VDSP_IRQ		3
#endif
#define VOUT_TV_IRQ		3
#define VDIRECT_IRQ		3
#if (CHIP_REV == A8)
#define I2S2TX_IRQ		3
#define I2S2RX_IRQ		4
#else
#define I2S2TX_IRQ		(64 + 7)	/* S2 */
#define I2S2RX_IRQ		(64 + 8)	/* S2 */
#endif
#define USBC_IRQ		4
#define USB_CHARGE_DET_IRQ	5	/* S2 */

#if (CHIP_REV == A1) || (CHIP_REV == A2) || (CHIP_REV == A3) || \
    (CHIP_REV == A5) ||	(CHIP_REV == A6) || (CHIP_REV == A8)
#define HOSTTX_IRQ		5
#define HOSTRX_IRQ		6
#endif

#define I2STX_IRQ		7
#define I2SRX_IRQ		8
#define UART0_IRQ		9
#define GPIO0_IRQ		10
#define GPIO1_IRQ		11
#define TIMER1_IRQ		12
#define TIMER2_IRQ		13
#define TIMER3_IRQ		14
#define DMA_IRQ			15
#define FIOCMD_IRQ		16
#define FIODMA_IRQ		17
#define SD_IRQ			18
#define IDC_IRQ			19
#define SSI_IRQ			20
#define WDT_IRQ			21
#define IRIF_IRQ		22
#define CFCD1_IRQ		23
#define SD2CD_IRQ		23	/* Shared with CFCD1_IRQ */
#define XDCD_IRQ		24
#define SD1CD_IRQ		24	/* SD1 SD card card detection. */

#if (CHIP_REV == A2) || (CHIP_REV == A2S) || (CHIP_REV == A2M) ||	\
    (CHIP_REV == A2Q) || (CHIP_REV == A5L) || (CHIP_REV == A6)
#define SD2_IRQ			18	/* Shared with SD_IRQ */
#else
#define SD2_IRQ			25
#endif
#define ETH0_IRQ		25

#define UART1_IRQ		25
#define SDCD_IRQ		26
#define VIN_ORC_VIN_IRQ		26	/* A8 */
#if (CHIP_REV == A7) || (CHIP_REV == S2)
#define GPIO3_IRQ		29		/* A7 */
#elif (CHIP_REV == I1) || (CHIP_REV == A7L)
#define GPIO3_IRQ		(32 + 19)	/* I1 */
#else
#define GPIO3_IRQ		26		/* A5 */
#endif

#if (CHIP_REV == A7L)
#define SSI_SLAVE_IRQ		(32 + 9)	/* A7L */
#else
#define SSI_SLAVE_IRQ		26
#endif

#if (CHIP_REV == A2)
#define ETH_IRQ			 4	/* Shared with USBC_IRQ */
#else
#define ETH_IRQ			27
#endif

#define VOUT_LCD_IRQ		27

#define IDSP_ERROR_IRQ		28
#define IDSP_SOFT_IRQ		28	/* iONE */
#define VIN_ORC_VOUT0_IRQ	28	/* A8 */

#define VOUT_SYNC_MISSED_IRQ	29
#define IDCS_IRQ		29    	/* IDC slave */
#define ETH_PWR_IRQ		29	/* iONE, A8 */

#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1) || \
    (CHIP_REV == A8)
#define HIF_ARM1_IRQ		5
#define HIF_ARM2_IRQ		6
#elif (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
    (CHIP_REV == A5L)
#define HIF_ARM1_IRQ		6	/* A2S, A2M */
#define HIF_ARM2_IRQ		29	/* A2S, A2M */
#elif (CHIP_REV == A6)
#define HIF_ARM1_IRQ		(32 + 13)
#define HIF_ARM2_IRQ		(32 + 12)
#endif

#if (CHIP_REV == A2)
#define GPIO2_IRQ		11	/* Shared with GPIO1_IRQ */
#else
#define GPIO2_IRQ		30
#endif

#define VIN_ORC_VOUT1_IRQ	30	/* A8 */

#define CFCD2_IRQ		31
#define CD2ND_BIT_CD_IRQ	31
#define PWC_IRQ			31	/* A5L */

#define AUDIO_ORC_IRQ		(32 + 0)
#define DRAM_AXI_ERR_IRQ	(32 + 0)	/* iONE */
#define DMA_FIOS_IRQ		(32 + 1)

#if (CHIP_REV == A5L)
#define ADC_LEVEL_IRQ		6
#else
#define ADC_LEVEL_IRQ		(32 + 2)
#endif

#define VOUT1_SYNC_MISSED_IRQ	(32 + 3)

#if (CHIP_REV == I1)
#define SSI3_IRQ		(32 + 3)
#else
#define SSI3_IRQ		(32 + 21)
#endif

#if (CHIP_REV == A7L)
#define IDC2_IRQ		26
#else
#define IDC2_IRQ		(32 + 4)
#endif
#define IDC3_IRQ		(32 + 3)	/* A8 */

#define IDSP_VIN_SW_IRQ		(32 + 3)	/* S2 */

#define IDSP_LAST_PIXEL_IRQ	(32 + 5)
#define IDSP_VSYNC_IRQ		(32 + 6)
#define IDSP_SENSOR_VSYNC_IRQ	(32 + 7)

#define VIN_ORC_VDSP0_IRQ	(32 + 0)	/* A8 */
#define VIN_ORC_VDSP1_IRQ	(32 + 7)	/* A8 */
#define VIN_ORC_VDSP2_IRQ	(32 + 9)	/* A8 */
#define VIN_ORC_VDSP3_IRQ	(32 + 11)	/* A8 */

#if (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2M) || \
    (CHIP_REV == A5L)
#define HDMI_IRQ		26
#else
#define HDMI_IRQ		(32 + 8)
#endif

#define FIO_ECC_IRQ		(32 + 9)	/* S2 */

#define SSI2_IRQ		(32 + 9)

#if (CHIP_REV == A5L)
#define VOUT_TV_SYNC_IRQ	5
#define VOUT_LCD_SYNC_IRQ	23
#else
#define VOUT_TV_SYNC_IRQ	(32 + 10)
#define VOUT_LCD_SYNC_IRQ	(32 + 11)
#endif

#if (CHIP_REV == A7) || (CHIP_REV == S2)
#define AES_IRQ			(32 + 12)
#define DES_IRQ			(32 + 13)
#else
#define AES_IRQ			(32 + 13)
#define DES_IRQ			(32 + 14)
#endif

#if (CHIP_REV == I1)
#define CODE_VDSP_3_IRQ		(64 + 31)
#define CODE_VDSP_2_IRQ		(64 + 30)
#define CODE_VDSP_1_IRQ		(64 + 29)
#define CODE_VDSP_0_IRQ		(64 + 28)
#elif (CHIP_REV == A7) || (CHIP_REV == S2)
#define CODE_VDSP_3_IRQ		(32 + 22)
#define CODE_VDSP_2_IRQ		(32 + 23)
#define CODE_VDSP_1_IRQ		(32 + 24)
#define CODE_VDSP_0_IRQ		ORC_VDSP_IRQ
#else
#define CODE_VDSP_3_IRQ		(32 + 15)
#define CODE_VDSP_2_IRQ		(32 + 16)
#define CODE_VDSP_1_IRQ		(32 + 17)
#define CODE_VDSP_0_IRQ		ORC_VDSP_IRQ
#endif

#if (CHIP_REV == S2)
#define USB_EHCI_IRQ		(64 + 9)
#else
#define USB_EHCI_IRQ		(32 + 16)	/* iONE */
#endif

#if (CHIP_REV == I1) || (CHIP_REV == A8)
#define TS_CH0_TX_IRQ		(64 + 9)
#define TS_CH1_TX_IRQ		(64 + 8)
#define TS_CH0_RX_IRQ		(64 + 7)
#define TS_CH1_RX_IRQ		(64 + 6)
#else
#define TS_CH0_TX_IRQ		(32 + 18)
#define TS_CH1_TX_IRQ		(32 + 19)
#define TS_CH0_RX_IRQ		(32 + 20)
#define TS_CH1_RX_IRQ		(32 + 21)
#endif

#if (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q)
#define MS_IRQ			5	/* A2S, A2M */
#elif (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1)
#define MS_IRQ			(32 + 15)
#else
#define MS_IRQ			(32 + 22)
#endif


#if (CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
    (CHIP_REV == A5L)
#define MOTOR_IRQ		25	/* A2S, A2M */
#elif (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1) || \
      (CHIP_REV == S2)
#define MOTOR_IRQ		(32 + 17)
#else
#define MOTOR_IRQ		(32 + 23)
#endif

#if (CHIP_REV == A7) || (CHIP_REV == S2)
#define GDMA_IRQ		(32 + 18)
#elif (CHIP_REV == A8)
#define GDMA_IRQ		31
#else
#define GDMA_IRQ		(32 + 16)
#endif

#if (CHIP_REV == I1)
#define GPIO4_IRQ		(32 + 20)
#else
#define GPIO4_IRQ		(32 + 16)
#endif

#if (CHIP_REV == I1) || (CHIP_REV == A8)
#define MD5_SHA1_IRQ		(32 + 18)
#else
#define MD5_SHA1_IRQ		(32 + 25)
#endif

#if (CHIP_REV == A8)
#define FDET_IRQ		(32 + 15)
#else
#define FDET_IRQ		(32 + 19)
#endif

#define DECODE_ORC_VIN_IRQ	(32 + 19)	/* A8 */
#define DECODE_ORC_VOUT0_IRQ	(32 + 20)	/* A8 */
#define SDIO_IRQ		(32 + 20)	/* S2 */
#define ETH2_IRQ		(32 + 20)
#define GPIO5_IRQ		(32 + 21)
#define DECODE_ORC_VOUT1_IRQ	(32 + 21)	/* A8 */
#define DECODE_ORC_VDSP0_IRQ	(32 + 22)	/* A8 */
#define SATA_IRQ		(32 + 22)
#define DRAM_ERR_IRQ		(32 + 23)	/* iONE */
#define SDXC_IRQ		(32 + 24)	/* iONE */
#define DECODE_ORC_VDSP1_IRQ	(32 + 24)	/* A8 */
#define UART2_IRQ		(32 + 25)	/* iONE A8 */
#define UART3_IRQ		(32 + 26)	/* iONE A8 */
#define IDSP_PROGRAMMABLE_IRQ	(32 + 26)	/* S2  */
#define TIMER4_IRQ		(32 + 27)	/* iONE A8 */
#define TIMER5_IRQ		(32 + 28)	/* iONE A8 */
#define TIMER6_IRQ		(32 + 29)	/* iONE A8 */
#define TIMER7_IRQ		(32 + 30)	/* iONE A8 */
#define TIMER8_IRQ		(32 + 31)	/* iONE A8 */

#if (CHIP_REV == I1)
#define ROLLING_SHUTTER_IRQ	3		/* iONE */
#elif (CHIP_REV == S2)
#define ROLLING_SHUTTER_IRQ	(32 + 0)
#else
#define ROLLING_SHUTTER_IRQ	(32 + 26)
#endif

#define SSI4_IRQ		(64 + 1)	/* iONE */
#define RNG_IRQ			(64 + 2)	/* iONE */
#if (CHIP_REV == A7L)
#define SDXC_CD_IRQ		5
#else
#define SDXC_CD_IRQ		(64 + 3)	/* iONE */
#endif
#define DECODE_ORC_VDSP2_IRQ	(64 + 3)	/* A8 */
#define CORTEX_CORE0_IRQ	(64 + 4)	/* iONE */
#define CORTEX_CORE1_IRQ	(64 + 5)	/* iONE */
#define SOFTWARE_IRQ(x)		(64 + 10 + (x))	/* iONE */
#define CORTEX_WDT_IRQ		(64 + 24)	/* iONE */
#define USB_OHCI_IRQ		(64 + 25)	/* iONE */
#define SPDIF_IRQ		(64 + 26)	/* iONE */

#if (CHIP_REV == S2)
#define SSI_AHB_IRQ		(32 + 21)
#else
#define SSI_AHB_IRQ		(64 + 27)	/* iONE */
#endif

#define IDSP_PIP_CODING_IRQ	(64 + 0)
#define IDSP_PIP_SVSYNC_IRQ	(64 + 1)
#define IDSP_PIP_PRAM_IRQ	(64 + 2)
#define IDSP_PIP_LAST_PIXEL_IRQ	(64 + 3)

#define SIF_ARM0_IRQ		(64 + 0)	/* A8 */
#define SIF_ARM1_IRQ		(64 + 1)	/* A8 */

#define CORTEX0_CORE0_AMBA_IRQ	CORTEX_CORE0_IRQ
#define CORTEX0_CORE1_AMBA_IRQ	CORTEX_CORE1_IRQ

#define CORTEX1_CORE0_AMBA_IRQ	(64 + 25)
#define CORTEX1_CORE1_AMBA_IRQ	(64 + 26)

#define DECODE_ORC_VDSP3_IRQ	(64 + 27)	/* A8 */
#define ENCODE_ORC_VDSP0_IRQ	(64 + 28)	/* A8 */
#define ENCODE_ORC_VDSP1_IRQ	(64 + 29)	/* A8 */
#define ENCODE_ORC_VDSP2_IRQ	(64 + 30)	/* A8 */
#define ENCODE_ORC_VDSP3_IRQ	(64 + 31)	/* A8 */
#define GPIO_IRQ(x)		((x) + VIC_INSTANCES * 32)

#define ARM11_SW_GINT_IRQ(x)	SOFTWARE_IRQ(x)

/*
 * The number of IRQs depends on the number of VIC and GPIO controllers
 * on the Ambarella Processor architecture
 */
#define NR_VIC_IRQS		(VIC_INSTANCES * 32)
#define NR_GPIO_IRQS		(GPIO_INSTANCES * 32)

#define VIC_INT_VEC_OFFSET	0
#define VIC2_INT_VEC_OFFSET	32
#define VIC3_INT_VEC_OFFSET	64

#define GPIO_INT_VEC_OFFSET	(VIC_INSTANCES * 32)
#define GPIO0_INT_VEC_OFFSET	(GPIO_INT_VEC_OFFSET + 0)
#define GPIO1_INT_VEC_OFFSET	(GPIO_INT_VEC_OFFSET + 32)
#define GPIO2_INT_VEC_OFFSET	(GPIO_INT_VEC_OFFSET + 64)
#define GPIO3_INT_VEC_OFFSET	(GPIO_INT_VEC_OFFSET + 96)
#define GPIO4_INT_VEC_OFFSET	(GPIO_INT_VEC_OFFSET + 128)
#define GPIO5_INT_VEC_OFFSET	(GPIO_INT_VEC_OFFSET + 160)

#define NR_IRQS			(NR_VIC_IRQS + NR_GPIO_IRQS)
#define FIQ_START		NR_IRQS

/******************************************/
/* Interrupt numbers of the uITRON kernel */
/******************************************/

#define VIC2_INT_VEC(x)			((x) + VIC2_INT_VEC_OFFSET)
#define VIC3_INT_VEC(x)			((x) + VIC3_INT_VEC_OFFSET)

/* ORC */
#define VOUT_INT_VEC			VOUT_IRQ
#define ORC_VOUT1_INT_VEC		ORC_VOUT1_IRQ
#define VIN_INT_VEC			VIN_IRQ
#define ORC_VIN_INT_VEC			ORC_VIN_IRQ
#define VDSP_INT_VEC			VDSP_IRQ
#define ORC_VDSP_INT_VEC		ORC_VDSP_IRQ
#define AUDIO_ORC_INT_VEC		AUDIO_ORC_IRQ
#define CODE_VOUT_B_INT_VEC		CODE_VDSP_B_IRQ
#define CODE_VDSP_3_INT_VEC		CODE_VDSP_3_IRQ
#define CODE_VDSP_2_INT_VEC		CODE_VDSP_2_IRQ
#define CODE_VDSP_1_INT_VEC		CODE_VDSP_1_IRQ
#define CODE_VDSP_0_INT_VEC		CODE_VDSP_0_IRQ
#define ORC_VOUT0_INT_VEC		ORC_VOUT0_IRQ
#define VOUT_LCD_INT_VEC		VOUT_LCD_IRQ
#define VOUT_TV_INT_VEC			VOUT_TV_IRQ

#define ENCODE_ORC_VIN_VEC		ENCODE_ORC_VIN_IRQ
#define ENCODE_ORC_VOUT0_VEC		ENCODE_ORC_VOUT0_IRQ
#define ENCODE_ORC_VOUT1_VEC		ENCODE_ORC_VOUT1_IRQ
#define ENCODE_ORC_VDSP0_VEC		ENCODE_ORC_VDSP0_IRQ
#define ENCODE_ORC_VDSP1_VEC		ENCODE_ORC_VDSP1_IRQ
#define ENCODE_ORC_VDSP2_VEC		ENCODE_ORC_VDSP2_IRQ
#define ENCODE_ORC_VDSP3_VEC		ENCODE_ORC_VDSP3_IRQ
#define DECODE_ORC_VIN_VEC		DECODE_ORC_VIN_IRQ
#define DECODE_ORC_VOUT0_VEC		DECODE_ORC_VOUT0_IRQ
#define DECODE_ORC_VOUT1_VEC		DECODE_ORC_VOUT1_IRQ
#define DECODE_ORC_VDSP0_VEC		DECODE_ORC_VDSP0_IRQ
#define DECODE_ORC_VDSP1_VEC		DECODE_ORC_VDSP1_IRQ
#define DECODE_ORC_VDSP2_VEC		DECODE_ORC_VDSP2_IRQ
#define DECODE_ORC_VDSP3_VEC		DECODE_ORC_VDSP3_IRQ
#define VIN_ORC_VIN_VEC			VIN_ORC_VIN_IRQ
#define VIN_ORC_VOUT0_VEC		VIN_ORC_VOUT0_IRQ
#define VIN_ORC_VOUT1_VEC		VIN_ORC_VOUT1_IRQ
#define VIN_ORC_VDSP0_VEC		VIN_ORC_VDSP0_IRQ
#define VIN_ORC_VDSP1_VEC		VIN_ORC_VDSP1_IRQ
#define VIN_ORC_VDSP2_VEC		VIN_ORC_VDSP2_IRQ
#define VIN_ORC_VDSP3_VEC		VIN_ORC_VDSP3_IRQ

/* USB */
#define USBVBUS_INT_VEC			USBVBUS_IRQ
#define USBC_INT_VEC			USBC_IRQ
#define USB_ECHI_VEC			USB_EHCI_IRQ
#define USB_OCHI_VEC			USB_OHCI_IRQ

/* Misc */
#define IRIF_INT_VEC			IRIF_IRQ
#define ADC_LEVEL_INT_VEC		ADC_LEVEL_IRQ
#define RNG_INT_VEC			RNG_IRQ
#define WDT_INT_VEC			WDT_IRQ
#define ROLLING_SHUTTER_INT_VEC		ROLLING_SHUTTER_IRQ

/* I2S and HOsT*/
#define HOSTTX_INT_VEC			HOSTTX_IRQ
#define HOSTRX_INT_VEC			HOSTRX_IRQ
#define I2STX_INT_VEC			I2STX_IRQ
#define I2SRX_INT_VEC			I2SRX_IRQ
#define I2S2TX_INT_VEC			I2S2TX_IRQ
#define I2S2RX_INT_VEC			I2S2RX_IRQ

/* Timer */
#define TIMER1_INT_VEC			TIMER1_IRQ
#define TIMER2_INT_VEC			TIMER2_IRQ
#define TIMER3_INT_VEC			TIMER3_IRQ
#define TIMER4_INT_VEC			TIMER4_IRQ
#define TIMER5_INT_VEC			TIMER5_IRQ
#define TIMER6_INT_VEC			TIMER6_IRQ
#define TIMER7_INT_VEC			TIMER7_IRQ
#define TIMER8_INT_VEC			TIMER8_IRQ

/* Ethernet */
#define ETH_INT_VEC			ETH_IRQ
#define ETH0_INT_VEC			ETH0_IRQ
#define ETH2_INT_VEC			ETH2_IRQ
#define ETH_PWR_INT_VEC			ETH_PWR_IRQ

/* UART */
#define UART0_INT_VEC			UART0_IRQ
#define UART1_INT_VEC			UART1_IRQ
#define UART2_INT_VEC			UART2_IRQ
#define UART3_INT_VEC			UART3_IRQ

/* FIO */
#define FIOCMD_INT_VEC			FIOCMD_IRQ
#define FIODMA_INT_VEC			FIODMA_IRQ
#define DMA_FIOS_INT_VEC		DMA_FIOS_IRQ
#define FIO_ECC_INT_VEC			FIO_ECC_IRQ	/* S2 */

/* DMA */
#define DMA_INT_VEC			DMA_IRQ
#define GDMA_INT_VEC			GDMA_IRQ

/* Cards */
#define SD_INT_VEC			SD_IRQ
#define CFCD1_INT_VEC			CFCD1_IRQ
#define SD2CD_INT_VEC			SD2CD_IRQ	/* Shared with CFCD1_INT_VEC */
#define XDCD_INT_VEC			XDCD_IRQ
#define SD1CD_INT_VEC			SD1CD_IRQ
#define SDCD_INT_VEC			SDCD_IRQ
#define CD2ND_BIT_CD_INT_VEC		CD2ND_BIT_CD_IRQ
#define CFCD2_INT_VEC			CFCD2_IRQ
#define SD2_INT_VEC			SD2_IRQ
#define SDXC_INT_VEC			SDXC_IRQ	/* iONE */
#define SDXC_CD_INT_VEC			SDXC_CD_IRQ	/* iONE */
#define SDIO_INT_VEC			SDIO_IRQ

/* IDC */
#define IDC_INT_VEC			IDC_IRQ
#define IDC2_INT_VEC			IDC2_IRQ
#define IDC3_INT_VEC			IDC3_IRQ
#define IDCS_INT_VEC			IDCS_IRQ


/* VIN and IDSP */
#define IDSP_LAST_PIXEL_INT_VEC		IDSP_LAST_PIXEL_IRQ
#define IDSP_VSYNC_INT_VEC		IDSP_VSYNC_IRQ
#define IDSP_SENSOR_VSYNC_INT_VEC	IDSP_SENSOR_VSYNC_IRQ
#define IDSP_ERROR_INT_VEC		IDSP_ERROR_IRQ
#define IDSP_SOFT_INT_VEC		IDSP_SOFT_IRQ
#define IDSP_VIN_SW_INT_VEC		IDSP_VIN_SW_IRQ /* S2 */
#define IDSP_PIP_CODING_INT_VEC		IDSP_PIP_CODING_IRQ
#define IDSP_PIP_SVSYNC_INT_VEC		IDSP_PIP_SVSYNC_IRQ
#define IDSP_PIP_PRAM_INT_VEC		IDSP_PIP_PRAM_IRQ
#define IDSP_PIP_LAST_PIXEL_INT_VEC	IDSP_PIP_LAST_PIXEL_IRQ
#define IDSP_PROGRAMMABLE_INT_VEC	IDSP_PROGRAMMABLE_IRQ /* S2 */


/* SSI */
#define SSI_INT_VEC			SSI_IRQ
#define SSI2_INT_VEC			SSI2_IRQ
#define SSI3_INT_VEC			SSI3_IRQ
#define SSI4_INT_VEC			SSI4_IRQ
#define SSI_SLAVE_INT_VEC		SSI_SLAVE_IRQ
#define SSI_AHB_INT_VEC			SSI_AHB_IRQ

/* VOUT */
#define HDMI_INT_VEC			HDMI_IRQ
#define VOUT_TV_SYNC_INT_VEC		VOUT_TV_SYNC_IRQ
#define VOUT_LCD_SYNC_INT_VEC		VOUT_LCD_SYNC_IRQ
#define VOUT1_SYNC_MISSED_INT_VEC	VOUT1_SYNC_MISSED_IRQ
#define VOUT_SYNC_MISSED_INT_VEC	VOUT_SYNC_MISSED_IRQ

/* TS */
#define TS_CH0_TX_INT_VEC		TS_CH0_TX_IRQ
#define TS_CH1_TX_INT_VEC		TS_CH1_TX_IRQ
#define TS_CH0_RX_INT_VEC		TS_CH0_RX_IRQ
#define TS_CH1_RX_INT_VEC		TS_CH1_RX_IRQ

/* Crypto */
#define AES_INT_VEC			AES_IRQ
#define DES_INT_VEC			DES_IRQ
#define MD5_SHA1_INT_VEC		MD5_SHA1_IRQ

/* GPIO */
#define GPIO0_INT_VEC			GPIO0_IRQ
#define GPIO1_INT_VEC			GPIO1_IRQ
#define GPIO2_INT_VEC			GPIO2_IRQ
#define GPIO3_INT_VEC			GPIO3_IRQ
#define GPIO4_INT_VEC			GPIO4_IRQ
#define GPIO5_INT_VEC			GPIO5_IRQ

/* MS */
#define MS_INT_VEC			MS_IRQ

/* Motor */
#define MOTOR_INT_VEC			MOTOR_IRQ

/* Face detection */
#define FDET_INT_VEC			FDET_IRQ

/* HIF */
#define HIF_ARM1_INT_VEC		HIF_ARM1_IRQ
#define HIF_ARM2_INT_VEC		HIF_ARM2_IRQ

/* SIF */
#define SIF_ARM1_INT_VEC		SIF_ARM1_IRQ
#define SIF_ARM2_INT_VEC		SIF_ARM2_IRQ

/* DRAM */
#define DRAM_ERR_INT_VEC		DRAM_ERR_IRQ		/* iONE */
#define DRAM_AXI_ERR_INT_VEC		DRAM_AXI_ERR_IRQ	/* iONE */

/* SPDIF */
#define SPDIF_INT_VEC			SPDIF_IRQ

/* Cortex */
#define CORTEX_CORE0_INT_VEC		CORTEX_CORE0_IRQ	/* iONE */
#define CORTEX_CORE1_INT_VEC		CORTEX_CORE1_IRQ	/* iONE */
#define CORTEX_WDT_INT_VEC		CORTEX_WDT_IRQ		/* iONE */

#define CORTEX0_CORE0_AMBA_INT_VEC	CORTEX0_CORE0_AMBA_IRQ
#define CORTEX0_CORE1_AMBA_INT_VEC	CORTEX0_CORE1_AMBA_IRQ
#define CORTEX1_CORE0_AMBA_INT_VEC	CORTEX1_CORE0_AMBA_IRQ
#define CORTEX1_CORE1_AMBA_INT_VEC	CORTEX1_CORE1_AMBA_IRQ

/* Software */
#define SOFTWARE_INT_VEC(x)		SOFTWARE_IRQ(x)		/* iONE */
#define ARM11_SW_GINT_INT_VEC(x)	ARM11_SW_GINT_IRQ(x)

/* SATA */
#define SATA_INT_VEC			SATA_IRQ		/* iONE */

#define GPIO_INT_VEC(x)			((x) + GPIO_INT_VEC_OFFSET)
#define GPIO00_INT_VEC			GPIO_INT_VEC(0)
#define GPIO01_INT_VEC			GPIO_INT_VEC(1)
#define GPIO02_INT_VEC			GPIO_INT_VEC(2)
#define GPIO03_INT_VEC			GPIO_INT_VEC(3)
#define GPIO04_INT_VEC			GPIO_INT_VEC(4)
#define GPIO05_INT_VEC			GPIO_INT_VEC(5)
#define GPIO06_INT_VEC			GPIO_INT_VEC(6)
#define GPIO07_INT_VEC			GPIO_INT_VEC(7)
#define GPIO08_INT_VEC			GPIO_INT_VEC(8)
#define GPIO09_INT_VEC			GPIO_INT_VEC(9)
#define GPIO10_INT_VEC			GPIO_INT_VEC(10)
#define GPIO11_INT_VEC			GPIO_INT_VEC(11)
#define GPIO12_INT_VEC			GPIO_INT_VEC(12)
#define GPIO13_INT_VEC			GPIO_INT_VEC(13)
#define GPIO14_INT_VEC			GPIO_INT_VEC(14)
#define GPIO15_INT_VEC			GPIO_INT_VEC(15)
#define GPIO16_INT_VEC			GPIO_INT_VEC(16)
#define GPIO17_INT_VEC			GPIO_INT_VEC(17)
#define GPIO18_INT_VEC			GPIO_INT_VEC(18)
#define GPIO19_INT_VEC			GPIO_INT_VEC(19)
#define GPIO20_INT_VEC			GPIO_INT_VEC(20)
#define GPIO21_INT_VEC			GPIO_INT_VEC(21)
#define GPIO22_INT_VEC			GPIO_INT_VEC(22)
#define GPIO23_INT_VEC			GPIO_INT_VEC(23)
#define GPIO24_INT_VEC			GPIO_INT_VEC(24)
#define GPIO25_INT_VEC			GPIO_INT_VEC(25)
#define GPIO26_INT_VEC			GPIO_INT_VEC(26)
#define GPIO27_INT_VEC			GPIO_INT_VEC(27)
#define GPIO28_INT_VEC			GPIO_INT_VEC(28)
#define GPIO29_INT_VEC			GPIO_INT_VEC(29)
#define GPIO30_INT_VEC			GPIO_INT_VEC(30)
#define GPIO31_INT_VEC			GPIO_INT_VEC(31)
#define GPIO32_INT_VEC			GPIO_INT_VEC(32)
#define GPIO33_INT_VEC			GPIO_INT_VEC(33)
#define GPIO34_INT_VEC			GPIO_INT_VEC(34)
#define GPIO35_INT_VEC			GPIO_INT_VEC(35)
#define GPIO36_INT_VEC			GPIO_INT_VEC(36)
#define GPIO37_INT_VEC			GPIO_INT_VEC(37)
#define GPIO38_INT_VEC			GPIO_INT_VEC(38)
#define GPIO39_INT_VEC			GPIO_INT_VEC(39)
#define GPIO40_INT_VEC			GPIO_INT_VEC(40)
#define GPIO41_INT_VEC			GPIO_INT_VEC(41)
#define GPIO42_INT_VEC			GPIO_INT_VEC(42)
#define GPIO43_INT_VEC			GPIO_INT_VEC(43)
#define GPIO44_INT_VEC			GPIO_INT_VEC(44)
#define GPIO45_INT_VEC			GPIO_INT_VEC(45)
#define GPIO46_INT_VEC			GPIO_INT_VEC(46)
#define GPIO47_INT_VEC			GPIO_INT_VEC(47)
#define GPIO48_INT_VEC			GPIO_INT_VEC(48)
#define GPIO49_INT_VEC			GPIO_INT_VEC(49)
#define GPIO50_INT_VEC			GPIO_INT_VEC(50)
#define GPIO51_INT_VEC			GPIO_INT_VEC(51)
#define GPIO52_INT_VEC			GPIO_INT_VEC(52)
#define GPIO53_INT_VEC			GPIO_INT_VEC(53)
#define GPIO54_INT_VEC			GPIO_INT_VEC(54)
#define GPIO55_INT_VEC			GPIO_INT_VEC(55)
#define GPIO56_INT_VEC			GPIO_INT_VEC(56)
#define GPIO57_INT_VEC			GPIO_INT_VEC(57)
#define GPIO58_INT_VEC			GPIO_INT_VEC(58)
#define GPIO59_INT_VEC			GPIO_INT_VEC(59)
#define GPIO60_INT_VEC			GPIO_INT_VEC(60)
#define GPIO61_INT_VEC			GPIO_INT_VEC(61)
#define GPIO62_INT_VEC			GPIO_INT_VEC(62)
#define GPIO63_INT_VEC			GPIO_INT_VEC(63)

#if (MAX_IRQ_NUMBER >= 128)
#define GPIO64_INT_VEC			GPIO_INT_VEC(64)
#define GPIO65_INT_VEC			GPIO_INT_VEC(65)
#define GPIO66_INT_VEC			GPIO_INT_VEC(66)
#define GPIO67_INT_VEC			GPIO_INT_VEC(67)
#define GPIO68_INT_VEC			GPIO_INT_VEC(68)
#define GPIO69_INT_VEC			GPIO_INT_VEC(69)
#define GPIO70_INT_VEC			GPIO_INT_VEC(70)
#define GPIO71_INT_VEC			GPIO_INT_VEC(71)
#define GPIO72_INT_VEC			GPIO_INT_VEC(72)
#define GPIO73_INT_VEC			GPIO_INT_VEC(73)
#define GPIO74_INT_VEC			GPIO_INT_VEC(74)
#define GPIO75_INT_VEC			GPIO_INT_VEC(75)
#define GPIO76_INT_VEC			GPIO_INT_VEC(76)
#define GPIO77_INT_VEC			GPIO_INT_VEC(77)
#define GPIO78_INT_VEC			GPIO_INT_VEC(78)
#define GPIO79_INT_VEC			GPIO_INT_VEC(79)
#define GPIO80_INT_VEC			GPIO_INT_VEC(80)
#define GPIO81_INT_VEC			GPIO_INT_VEC(81)
#define GPIO82_INT_VEC			GPIO_INT_VEC(82)
#define GPIO83_INT_VEC			GPIO_INT_VEC(83)
#define GPIO84_INT_VEC			GPIO_INT_VEC(84)
#define GPIO85_INT_VEC			GPIO_INT_VEC(85)
#define GPIO86_INT_VEC			GPIO_INT_VEC(86)
#define GPIO87_INT_VEC			GPIO_INT_VEC(87)
#define GPIO88_INT_VEC			GPIO_INT_VEC(88)
#define GPIO89_INT_VEC			GPIO_INT_VEC(89)
#define GPIO90_INT_VEC			GPIO_INT_VEC(90)
#define GPIO91_INT_VEC			GPIO_INT_VEC(91)
#define GPIO92_INT_VEC			GPIO_INT_VEC(92)
#define GPIO93_INT_VEC			GPIO_INT_VEC(93)
#define GPIO94_INT_VEC			GPIO_INT_VEC(94)
#define GPIO95_INT_VEC			GPIO_INT_VEC(95)
#endif

#if (MAX_IRQ_NUMBER >= 192)
#define GPIO96_INT_VEC			GPIO_INT_VEC(96)
#define GPIO97_INT_VEC			GPIO_INT_VEC(97)
#define GPIO98_INT_VEC			GPIO_INT_VEC(98)
#define GPIO99_INT_VEC			GPIO_INT_VEC(99)
#define GPIO100_INT_VEC			GPIO_INT_VEC(100)
#define GPIO101_INT_VEC			GPIO_INT_VEC(101)
#define GPIO102_INT_VEC			GPIO_INT_VEC(102)
#define GPIO103_INT_VEC			GPIO_INT_VEC(103)
#define GPIO104_INT_VEC			GPIO_INT_VEC(104)
#define GPIO105_INT_VEC			GPIO_INT_VEC(105)
#define GPIO106_INT_VEC			GPIO_INT_VEC(106)
#define GPIO107_INT_VEC			GPIO_INT_VEC(107)
#define GPIO108_INT_VEC			GPIO_INT_VEC(108)
#define GPIO109_INT_VEC			GPIO_INT_VEC(109)
#define GPIO110_INT_VEC			GPIO_INT_VEC(110)
#define GPIO111_INT_VEC			GPIO_INT_VEC(111)
#define GPIO112_INT_VEC			GPIO_INT_VEC(112)
#define GPIO113_INT_VEC			GPIO_INT_VEC(113)
#define GPIO114_INT_VEC			GPIO_INT_VEC(114)
#define GPIO115_INT_VEC			GPIO_INT_VEC(115)
#define GPIO116_INT_VEC			GPIO_INT_VEC(116)
#define GPIO117_INT_VEC			GPIO_INT_VEC(117)
#define GPIO118_INT_VEC			GPIO_INT_VEC(118)
#define GPIO119_INT_VEC			GPIO_INT_VEC(119)
#define GPIO120_INT_VEC			GPIO_INT_VEC(120)
#define GPIO121_INT_VEC			GPIO_INT_VEC(121)
#define GPIO122_INT_VEC			GPIO_INT_VEC(122)
#define GPIO123_INT_VEC			GPIO_INT_VEC(123)
#define GPIO124_INT_VEC			GPIO_INT_VEC(124)
#define GPIO125_INT_VEC			GPIO_INT_VEC(125)
#define GPIO126_INT_VEC			GPIO_INT_VEC(126)
#define GPIO127_INT_VEC			GPIO_INT_VEC(127)
#endif

#if (MAX_IRQ_NUMBER >= 224)
#define GPIO128_INT_VEC			GPIO_INT_VEC(128)
#define GPIO129_INT_VEC			GPIO_INT_VEC(129)
#define GPIO130_INT_VEC			GPIO_INT_VEC(130)
#define GPIO131_INT_VEC			GPIO_INT_VEC(131)
#define GPIO132_INT_VEC			GPIO_INT_VEC(132)
#define GPIO133_INT_VEC			GPIO_INT_VEC(133)
#define GPIO134_INT_VEC			GPIO_INT_VEC(134)
#define GPIO135_INT_VEC			GPIO_INT_VEC(135)
#define GPIO136_INT_VEC			GPIO_INT_VEC(136)
#define GPIO137_INT_VEC			GPIO_INT_VEC(137)
#define GPIO138_INT_VEC			GPIO_INT_VEC(138)
#define GPIO139_INT_VEC			GPIO_INT_VEC(139)
#define GPIO140_INT_VEC			GPIO_INT_VEC(140)
#define GPIO141_INT_VEC			GPIO_INT_VEC(141)
#define GPIO142_INT_VEC			GPIO_INT_VEC(142)
#define GPIO143_INT_VEC			GPIO_INT_VEC(143)
#define GPIO144_INT_VEC			GPIO_INT_VEC(144)
#define GPIO145_INT_VEC			GPIO_INT_VEC(145)
#define GPIO146_INT_VEC			GPIO_INT_VEC(146)
#define GPIO147_INT_VEC			GPIO_INT_VEC(147)
#define GPIO148_INT_VEC			GPIO_INT_VEC(148)
#define GPIO149_INT_VEC			GPIO_INT_VEC(149)
#define GPIO150_INT_VEC			GPIO_INT_VEC(150)
#define GPIO151_INT_VEC			GPIO_INT_VEC(151)
#define GPIO152_INT_VEC			GPIO_INT_VEC(152)
#define GPIO153_INT_VEC			GPIO_INT_VEC(153)
#define GPIO154_INT_VEC			GPIO_INT_VEC(154)
#define GPIO155_INT_VEC			GPIO_INT_VEC(155)
#define GPIO156_INT_VEC			GPIO_INT_VEC(156)
#define GPIO157_INT_VEC			GPIO_INT_VEC(157)
#define GPIO158_INT_VEC			GPIO_INT_VEC(158)
#define GPIO159_INT_VEC			GPIO_INT_VEC(159)
#endif

#if (MAX_IRQ_NUMBER >= 256)
#define GPIO160_INT_VEC			GPIO_INT_VEC(160)
#define GPIO161_INT_VEC			GPIO_INT_VEC(161)
#define GPIO162_INT_VEC			GPIO_INT_VEC(162)
#define GPIO163_INT_VEC			GPIO_INT_VEC(163)
#define GPIO164_INT_VEC			GPIO_INT_VEC(164)
#define GPIO165_INT_VEC			GPIO_INT_VEC(165)
#define GPIO166_INT_VEC			GPIO_INT_VEC(166)
#define GPIO167_INT_VEC			GPIO_INT_VEC(167)
#define GPIO168_INT_VEC			GPIO_INT_VEC(168)
#define GPIO169_INT_VEC			GPIO_INT_VEC(169)
#define GPIO170_INT_VEC			GPIO_INT_VEC(170)
#define GPIO171_INT_VEC			GPIO_INT_VEC(171)
#define GPIO172_INT_VEC			GPIO_INT_VEC(172)
#define GPIO173_INT_VEC			GPIO_INT_VEC(173)
#define GPIO174_INT_VEC			GPIO_INT_VEC(174)
#define GPIO175_INT_VEC			GPIO_INT_VEC(175)
#define GPIO176_INT_VEC			GPIO_INT_VEC(176)
#define GPIO177_INT_VEC			GPIO_INT_VEC(177)
#define GPIO178_INT_VEC			GPIO_INT_VEC(178)
#define GPIO179_INT_VEC			GPIO_INT_VEC(179)
#define GPIO180_INT_VEC			GPIO_INT_VEC(180)
#define GPIO181_INT_VEC			GPIO_INT_VEC(181)
#define GPIO182_INT_VEC			GPIO_INT_VEC(182)
#define GPIO183_INT_VEC			GPIO_INT_VEC(183)
#define GPIO184_INT_VEC			GPIO_INT_VEC(184)
#define GPIO185_INT_VEC			GPIO_INT_VEC(185)
#define GPIO186_INT_VEC			GPIO_INT_VEC(186)
#define GPIO187_INT_VEC			GPIO_INT_VEC(187)
#define GPIO188_INT_VEC			GPIO_INT_VEC(188)
#define GPIO189_INT_VEC			GPIO_INT_VEC(189)
#define GPIO190_INT_VEC			GPIO_INT_VEC(190)
#define GPIO191_INT_VEC			GPIO_INT_VEC(191)
#endif

#endif
