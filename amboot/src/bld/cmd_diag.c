/**
 * system/src/bld/cmd_diag.c
 *
 * History:
 *    2005/09/09 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>

extern void diag_adc(void);
extern void diag_adc_gyro(void);
extern void diag_gpio_set(int gpio, int on);
extern void diag_gpio_get(int gpio);
extern void diag_gpio_hw(int gpio);
extern void diag_gpio_pull(int gpio, int val);
extern void diag_ir(void);
#if defined(ENABLE_FLASH)
extern void diag_nand_read(void);
extern void diag_nand_read_speed(void);
extern void diag_nand_prog(void);
extern void diag_nand_erase(int argc, char *argv[]);
extern u32 diag_nand_verify(int argc, char *argv[]);
extern void diag_nand_rclm_bb(int argc, char *argv[]);
extern void diag_nand_dump(int argc, char *argv[]);
extern void diag_nand_write(int argc, char *argv[]);
extern void diag_nand_partition(char *s ,u32 times);
extern int diag_onenand_read_reg(int argc, char *argv[]);
extern int diag_onenand_erase(int argc, char *argv[]);
extern int diag_onenand_write_reg(int argc, char *argv[]);
extern int diag_onenand_read_pages(int argc, char *argv[]);
extern int diag_onenand_prog(int argc, char *argv[]);
extern void diag_nor_verify(int argc, char *argv[]);
extern int diag_snor_verify(int argc, char *argv[]);
#endif
#if defined(ENABLE_SD)
extern void diag_sd_read(char *device_type);
extern void diag_sd_read_speed(char *device_type);
extern void diag_sd_write(char *device_type);
extern void diag_sd_write_speed(char *device_type);
extern void diag_sd_verify(char *device_type);
#endif

extern  int crypto_diag(void);

static char help_diag[] =
	"diag [command]\r\n"
	"Run diagnostic code to test various system hardware.\r\n"
	"The available commands are:\r\n"
	"\tdiag adc  - Test ADC.\r\n"
	"\tdiag gpio [set|clr|get|hw|pup|pdown|poff] id - Test GPIO.\r\n"
	"\tdiag ir   - Test IR.\r\n"
#if defined(ENABLE_FLASH)
	"\tdiag nand erase [end_block] - Test NAND erase.\r\n"
	"\tdiag nand read - Test NAND read.\r\n"
	"\tdiag nand prog - Test NAND prog.\r\n"
	"\tdiag nand rclm [init|late|other|all]- "
	   "Reclaim nand bad blocks.\r\n"
	"\tdiag nand verify [boot|storage|all]- "
	   "Test NAND read write verify.\r\n"
	"\tdiag nand dump page pages [no_ecc]- "
	   "Dump nand contents in given page address.\r\n"
	"\tdiag nand write page pages [no_ecc] [BITFLIP] - "
	   "Program NAND page with/without ecc and for bitflip test.\r\n"
	"\tdiag nand part [partition|all] [times] -\r\n"
	   "\t\tWhere partition is bst|bld|pri|bak|rmd|rom|dsp\r\n"
	   "\t\tCheck CRC32 for every partition.\r\n"
	"\tdiag nand markbad block - mark bad block in given block address.\r\n"
	"\tdiag nor verify - Test NOR read write verify.\r\n"
	"\tLoop command: \r\n"
	"\tdiag nand erase loop - Test NAND erase in forever loop.\r\n"
	"\tdiag nand read loop - Test NAND read in forever loop.\r\n"
	"\tdiag nand prog loop - Test NAND prog in forever loop.\r\n"
	"\tdiag nand prog verify - Test NAND verify in forever loop.\r\n"
	"\r\n"
#if !defined(CONFIG_ONENAND_NONE)
	"\tdiag onand r_reg [bank] [addr] - Read register on ONENAND.\r\n"
	"\tdiag onand w_reg [bank] [addr] - Write register on ONENAND.\r\n"
#endif
#if !defined(CONFIG_SNOR_NONE)
	"\tdiag snor verify [boot|storage|all] [loop]-"
	" Test SNOR read write verify.\r\n"
#endif
#endif /* ENABLE_FLASH */
#if defined(ENABLE_SD)
	"\t\t[slot]: sd/sdio/sd2\r\n"
	"\t\t[type]: sd/sdhc/mmc/MoviNAND\r\n"
	"\tdiag sd read slot type - Test read.\r\n"
	"\tdiag sd rst slot type - Test read speed.\r\n"
	"\tdiag sd write slot type - Test write\r\n"
	"\tdiag sd wst slot type - Test write speed\r\n"
	"\tdiag sd verify slot type - Test read write verify.\r\n"
#endif
#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1) || (CHIP_REV == S2)
	"\tdiag crypto - auto test crypt module,support a5s,a7,i1,s2\r\n"
#endif
;

static int cmd_diag(int argc, char *argv[])
{
	if (strcmp(argv[1], "adc") == 0) {
		if (strcmp(argv[2], "gyro") == 0)
			diag_adc_gyro();
		else
			diag_adc();
	} else if (strcmp(argv[1], "gpio") == 0) {
		if (argc == 4) {
			u32			gpio_id = 0xFFFFFFFF;

			strtou32(argv[3], &gpio_id);
			if (strcmp(argv[2], "get") == 0) {
				diag_gpio_get(gpio_id);
			} else if (strcmp(argv[2], "set") == 0) {
				diag_gpio_set(gpio_id, 1);
			} else if (strcmp(argv[2], "clr") == 0) {
				diag_gpio_set(gpio_id, 0);
			} else if (strcmp(argv[2], "poff") == 0) {
				diag_gpio_pull(gpio_id, 0);
			} else if (strcmp(argv[2], "pdown") == 0) {
				diag_gpio_pull(gpio_id, 2);
			} else if (strcmp(argv[2], "pup") == 0) {
				diag_gpio_pull(gpio_id, 3);
			} else if (strcmp(argv[2], "hw") == 0) {
				diag_gpio_hw(gpio_id);
			}
		} else {
			uart_putstr(help_diag);
		}
	} else if (strcmp(argv[1], "ir") == 0) {
		diag_ir();
#if defined(ENABLE_FLASH)
	} else if (strcmp(argv[1], "nand") == 0) {
		if (argc == 3) {
			if (strcmp(argv[2], "erase") == 0)
                                diag_nand_erase(0, &argv[2]);
			else if (strcmp(argv[2], "read") == 0)
				diag_nand_read();
			else if (strcmp(argv[2], "prog") == 0)
				diag_nand_prog();
			else if (strcmp(argv[2], "verify") == 0)
				diag_nand_verify(0, &argv[2]);
			else if (strcmp(argv[2], "speed") == 0)
				diag_nand_read_speed();
			else
				uart_putstr(help_diag);
		} else if (argc == 4 && (strcmp(argv[3], "loop") == 0)) {
			if (strcmp(argv[2], "erase") == 0)
				while (1)
	                                diag_nand_erase(0, &argv[2]);
			else if (strcmp(argv[2], "read") == 0)
				while (1)
					diag_nand_read();
			else if (strcmp(argv[2], "prog") == 0)
				while (1)
					diag_nand_prog();
			else if (strcmp(argv[2], "verify") == 0)
				while (1)
					diag_nand_verify(0, &argv[2]);
			else
				uart_putstr(help_diag);
		} else if (argc >= 4 && argc < 8) {
			if (strcmp(argv[2], "verify") == 0) {
				diag_nand_verify(1, &argv[3]);
			} else if (strcmp(argv[2], "erase") == 0) {
                                diag_nand_erase(1, &argv[3]);
			} else if (strcmp(argv[2], "rclm") == 0) {
                                diag_nand_rclm_bb(1, &argv[3]);
			} else if (strcmp(argv[2], "dump") == 0) {
				diag_nand_dump(argc - 3, &argv[3]);
			} else if (strcmp(argv[2], "write") == 0) {
				diag_nand_write(argc - 3, &argv[3]);
			} else if (strcmp(argv[2], "part") == 0) {
				u32 times;
				strtou32(argv[4], &times);
				diag_nand_partition(argv[3] ,times);
			} else if (strcmp(argv[2], "markbad") == 0) {
				u32 bad_block;
				strtou32(argv[3], &bad_block);
				nand_mark_bad_block(bad_block);
			}
		} else {
			uart_putstr(help_diag);
		}
	} else if (strcmp(argv[1], "nor") == 0) {
		if ((argc == 3) && (strcmp(argv[2], "verify") == 0))
			diag_nor_verify(0, &argv[2]);
#if !defined(CONFIG_ONENAND_NONE)
	} else if ((argc == 5) && strcmp(argv[1], "onenand") == 0) {
		if (strcmp(argv[2], "r_reg") == 0)
			diag_onenand_read_reg(2, &argv[3]);
		else if (strcmp(argv[2], "w_reg") == 0)
			diag_onenand_write_reg(2, &argv[3]);
		else if (strcmp(argv[2], "erase") == 0)
			diag_onenand_erase(2, &argv[3]);
	} else if ((argc == 7) && strcmp(argv[1], "onenand") == 0) {
		if (strcmp(argv[2], "rp") == 0)
			diag_onenand_read_pages(4, &argv[3]);
		else if (strcmp(argv[2], "wp") == 0)
			diag_onenand_prog(4, &argv[3]);
#endif
#if !defined(CONFIG_SNOR_NONE)
	} else if (strcmp(argv[1], "snor") == 0) {
		if ((argc == 3) && (strcmp(argv[2], "verify") == 0))
			diag_snor_verify(argc - 2, &argv[2]);
		if ((argc == 4 || argc == 5) &&
				(strcmp(argv[2], "verify") == 0))
			diag_snor_verify(argc - 3, &argv[3]);
#endif
#endif /* ENABLE FLASH */
#if defined(ENABLE_SD)
	} else if (strcmp(argv[1], "sd") == 0) {
		if (argc == 5) {
			if (strcmp(argv[2], "read") == 0)
				diag_sd_read((char *) &argv[3]);
			else if (strcmp(argv[2], "write") == 0)
				diag_sd_write((char *) &argv[3]);
			else if (strcmp(argv[2], "verify") == 0)
				diag_sd_verify((char *) &argv[3]);
			else if (strcmp(argv[2], "rst") == 0)
				diag_sd_read_speed((char *) &argv[3]);
			else if (strcmp(argv[2], "wst") == 0)
				diag_sd_write_speed((char *) &argv[3]);
			else
				uart_putstr(help_diag);
		} else {
			uart_putstr(help_diag);
		}
#endif
	}
#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1) || (CHIP_REV == S2)
	else if (strcmp(argv[1],"crypto") == 0) {
		crypto_diag();
	}
#endif
	else {
		uart_putstr(help_diag);
		return -1;
	}

	return 0;
}

__CMDLIST(cmd_diag, "diag", help_diag);
