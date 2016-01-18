#ifndef VERSION
#define VERSION	0
#endif

#ifndef CHIPNAME
#define CHIPNAME	a5l
#endif

#define _QUOTEME(x)	#x
#define QUOTEME(x)	_QUOTEME(x)

/** Timing values below are assuming clock of 2ns and loop length = 1 clock */
#define TBL_200US	0x20000 /* 200 us */
#define TBL_400NS	0x100  /*400 ns */
#define TBL_75NS	0x28  /* 75 ns */
#define TBL_500US	0x50000 /* 500 us */
#define	TBL_150NS	0x50  /*150 ns */
#define	TBL_100NS	0x40	/*100 ns */

#define DLL0_REG_VAL	0x201020
#define DLL1_REG_VAL	0x201020
#define DLL_CTRL_SEL_MISC_VAL 0x7ee5

/*
 * dram_delay $reg
 *
 * count value in $reg, delay count: 1 sec delay is approch to (core_frequence_Hz / 5)
 *
 */
#ifndef  _C_ 
.macro dram_delay label, reg=r0
\label:
		sub		\reg, \reg, #1
		bne		\label
.endm

.macro load_from_table, param=p, reg=r1
#ifdef _SPIBOOT_
	mov		\reg, #(\param-dram_params_table+0x8)
	blx		r4  /* r4 has address of spi_readlx */
	
#else
	ldr		\reg, [sp, #(\param-dram_params_table)]
#endif
.endm
#endif
	
	
