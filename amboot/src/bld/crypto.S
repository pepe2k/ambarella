/**
 * bld/crypto.S
 *
 * Author: cddiao@ambarella.com
 *
 * Copyright (C) 2012-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <config.h>

#if defined(CONFIG_ARCH_A7)

#define CRYPT_UNIT_REG(x)			(0x60014000 + (x))
#define AES_BLOCK_SIZE				(16)
#define DES_BLOCK_SIZE				(8)

.globl	a7_config_aes_256
.globl	a7_do_aes_enc
.globl	a7_do_aes_dec
.globl	a7_config_des_256
.globl	a7_do_des_enc
.globl	a7_do_des_dec
.globl	a7_md5_transform
.globl	a7_sha1_transform


a7_config_aes_256:
	push	{r4-r11, lr}
	mov	r2,r0
	ldr	r0, bin_comp_reg
	mov	r1,#0
	str	r1,[r0]
	mov	r0,r2
	ldr	r1,aes_256_key_reg
	ldmia	r0, {r4-r11}
	stmia	r1, {r4-r11}
	pop	{r4-r11, pc}
a7_do_aes_enc:
	push	{r4-r11, lr}
	ldr	r4, aes_enc_input_reg
	b	a7_do_aes
a7_do_aes_dec:
	push	{r4-r11, lr}
	ldr	r4, aes_dec_input_reg
	b	a7_do_aes
a7_do_aes:
	ldr	r5, aes_output_ready_reg
	ldr	r6, aes_output_reg
loop_a7_do_aes:
	cmp	r2, #AES_BLOCK_SIZE
	blo	exit_a7_do_aes
	sub	r2, r2, #AES_BLOCK_SIZE
	ldmia	r0!, {r8-r11}
	stmia	r4, {r8-r11}
wait_a7_do_aes:
	ldr	r7, [r5]
	cmp	r7, #1
	bne	wait_a7_do_aes
	ldmia	r6, {r8-r11}
	stmia	r1!, {r8-r11}
	b	loop_a7_do_aes
exit_a7_do_aes:
	pop	{r4-r11, pc}

a7_config_des_256:
	push	{r4-r11, lr}
	mov	r2,r0
	ldr	r0, bin_comp_reg
	mov	r1,#0x0
	str	r1,[r0]
	mov	r0,r2
	ldr	r1, des_key_reg
	ldmia	r0, {r10-r11}
	stmia	r1, {r10-r11}
	pop	{r4-r11, pc}
a7_do_des_enc:
	push	{r4-r11, lr}
	ldr	r4, des_enc_input_reg
	b	a7_do_des
a7_do_des_dec:
	push	{r4-r11, lr}
	ldr	r4, des_dec_input_reg
	b	a7_do_des
a7_do_des:
	ldr	r5, des_output_ready_reg
	ldr	r6, des_output_reg
loop_a7_do_des:
	cmp	r2, #DES_BLOCK_SIZE
	blo	exit_a7_do_des
	sub	r2, r2, #DES_BLOCK_SIZE
	ldmia	r0!, {r10-r11}
	stmia	r4, {r10-r11}
wait_a7_do_des:
	ldr	r7, [r5]
	cmp	r7, #1
	bne	wait_a7_do_des
	ldmia	r6, {r10-r11}
	stmia	r1!, {r10-r11}
	b	loop_a7_do_des
exit_a7_do_des:
	pop	{r4-r11, pc}

a7_md5_transform:
	push	{r4-r11, lr}
	mov	r3,r0
	mov	r4,r1
	ldr	r0, bin_comp_reg
	mov	r1,#0x0
	str	r1,[r0]
	mov	r0,r3
	mov	r1,r4
wait_md5_rdy_input:
	ldr	r4, md5_rdy_input_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_md5_rdy_input
	ldr	r3, md5_init_reg
	ldmia	r0, {r8-r11}
	stmia	r3, {r8-r11}
wait_md5_rdy_input2:
	ldr	r4, md5_rdy_input_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_md5_rdy_input2
//send by 2 steps
	ldr	r3, md5_input_reg_1
	ldmia	r1, {r4-r11}
	stmia	r3, {r4-r11}
	ldr	r3, md5_input_reg_2
	add	r1, r1, #32
	ldmia	r1, {r4-r11}
	stmia	r3, {r4-r11}
wait_md5_output_rdy:
	ldr	r4, md5_rdy_output_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_md5_output_rdy
	ldr	r3, md5_output_reg
	ldmia	r3, {r8-r11}
	stmia	r0!, {r8-r11}
	pop	{r4-r11, pc}

a7_sha1_transform:
	push	{r4-r11, lr}
	mov	r3,r0
	mov	r4,r1
	ldr	r0, bin_comp_reg
	mov	r1,#0x0
	str	r1,[r0]
	mov	r0,r3
	mov	r1,r4
wait_sha1_rdy_input:
	ldr	r4, sha1_rdy_input_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_sha1_rdy_input
	ldr	r3, sha1_init_reg
	ldmia	r0, {r7-r11}
	stmia	r3, {r7-r11}
wait_sha1_rdy_input2:
	ldr	r4, sha1_rdy_input_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_sha1_rdy_input2
//send by 2 steps
	ldr	r3, sha1_input_reg_1
	ldmia	r1, {r4-r11}
	stmia	r3, {r4-r11}
	ldr	r3, sha1_input_reg_2
	add	r1, r1, #32
	ldmia	r1, {r4-r11}
	stmia	r3, {r4-r11}
wait_sha1_output_rdy:
	ldr	r4, sha1_rdy_output_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_sha1_output_rdy
	ldr	r3, sha1_output_reg
	ldmia	r3, {r7-r11}
	stmia	r0!, {r7-r11}
	pop	{r4-r11, pc}


.align 5
bin_comp_reg:		.word	CRYPT_UNIT_REG(0xffc)
aes_256_key_reg:	.word	CRYPT_UNIT_REG(0x200)
aes_enc_input_reg:	.word	CRYPT_UNIT_REG(0x248)
aes_dec_input_reg:	.word	CRYPT_UNIT_REG(0x258)
aes_output_ready_reg:	.word	CRYPT_UNIT_REG(0x278)
aes_output_reg:		.word	CRYPT_UNIT_REG(0x268)
des_key_reg:		.word	CRYPT_UNIT_REG(0x00)
des_enc_input_reg:	.word	CRYPT_UNIT_REG(0x08)
des_dec_input_reg:	.word	CRYPT_UNIT_REG(0x10)
des_output_ready_reg:	.word	CRYPT_UNIT_REG(0x20)
des_output_reg:		.word	CRYPT_UNIT_REG(0x18)
md5_init_reg:		.word	CRYPT_UNIT_REG(0x480)
md5_input_reg_1:	.word	CRYPT_UNIT_REG(0x490)
md5_input_reg_2:	.word	CRYPT_UNIT_REG(0x4b0)
md5_output_reg:		.word	CRYPT_UNIT_REG(0x4d0)
md5_rdy_input_reg:	.word	CRYPT_UNIT_REG(0x4e8)
md5_rdy_output_reg:	.word	CRYPT_UNIT_REG(0x4e0)
sha1_init_reg:		.word	CRYPT_UNIT_REG(0x400)
sha1_input_reg_1:	.word	CRYPT_UNIT_REG(0x414)
sha1_input_reg_2:	.word	CRYPT_UNIT_REG(0x434)
sha1_output_reg:	.word	CRYPT_UNIT_REG(0x454)
sha1_rdy_output_reg:	.word	CRYPT_UNIT_REG(0x468)
sha1_rdy_input_reg:	.word	CRYPT_UNIT_REG(0x4e8)
//end a7

#elif defined(CONFIG_ARCH_I1) || defined(CONFIG_ARCH_S2)

#include <cryptoinit.h>
#define CRYPT_UNIT_REG(x)			(CRYPTO_REMAP_ADDR + (x))
#define AES_BLOCK_SIZE				(16)
#define DES_BLOCK_SIZE				(8)
.globl	i1_config_aes_256
.globl	i1_do_aes_enc
.globl	i1_do_aes_dec
.globl	i1_config_des_256
.globl	i1_do_des_enc
.globl	i1_do_des_dec
.globl	i1_md5_transform
.globl	i1_sha1_transform

i1_config_aes_256:
	push	{r4-r11, lr}
	ldr	r1,aes_256_key_reg
	ldmia	r0, {r4-r11}
	stmia	r1, {r4-r11}
	pop	{r4-r11, pc}
i1_do_aes_enc:
	push	{r4-r11, lr}
	ldr	r4, aes_enc_input_reg
	b	i1_do_aes
i1_do_aes_dec:
	push	{r4-r11, lr}
	ldr	r4, aes_dec_input_reg
	b	i1_do_aes
i1_do_aes:
	ldr	r5, aes_output_ready_reg
	ldr	r6, aes_output_reg
loop_i1_do_aes:
	cmp	r2, #AES_BLOCK_SIZE
	blo	exit_i1_do_aes
	sub	r2, r2, #AES_BLOCK_SIZE
	ldmia	r0!, {r8-r11}
	stmia	r4, {r8-r11}
wait_i1_do_aes:
	ldr	r7, [r5]
	cmp	r7, #1
	bne	wait_i1_do_aes
	ldmia	r6, {r8-r11}
	stmia	r1!, {r8-r11}
	b	loop_i1_do_aes
exit_i1_do_aes:
	pop	{r4-r11, pc}

i1_config_des_256:
	push	{r4-r11, lr}
	ldr	r1, des_key_reg
	ldmia	r0, {r10-r11}
	stmia	r1, {r10-r11}
	pop	{r4-r11, pc}
i1_do_des_enc:
	push	{r4-r11, lr}
	ldr	r4, des_enc_input_reg
	b	i1_do_des
i1_do_des_dec:
	push	{r4-r11, lr}
	ldr	r4, des_dec_input_reg
	b	i1_do_des
i1_do_des:
	ldr	r5, des_output_ready_reg
	ldr	r6, des_output_reg
loop_i1_do_des:
	cmp	r2, #DES_BLOCK_SIZE
	blo	exit_i1_do_des
	sub	r2, r2, #DES_BLOCK_SIZE
	ldmia	r0!, {r10-r11}
	stmia	r4, {r10-r11}
wait_i1_do_des:
	ldr	r7, [r5]
	cmp	r7, #1
	bne	wait_i1_do_des
	ldmia	r6, {r10-r11}
	stmia	r1!, {r10-r11}
	b	loop_i1_do_des
exit_i1_do_des:
	pop	{r4-r11, pc}

i1_md5_transform:
	push	{r4-r11, lr}
wait_md5_rdy_input:
	ldr	r4, md5_rdy_input_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_md5_rdy_input
	ldr	r3, md5_init_reg
	ldmia	r0, {r8-r11}
	stmia	r3, {r8-r11}
wait_md5_rdy_input2:
	ldr	r4, md5_rdy_input_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_md5_rdy_input2
//send by 2 steps
	ldr	r3, md5_input_reg_1
	ldmia	r1, {r4-r11}
	stmia	r3, {r4-r11}
	ldr	r3, md5_input_reg_2
	add	r1, r1, #32
	ldmia	r1, {r4-r11}
	stmia	r3, {r4-r11}
wait_md5_output_rdy:
	ldr	r4, md5_rdy_output_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_md5_output_rdy
	ldr	r3, md5_output_reg
	ldmia	r3, {r8-r11}
	stmia	r0!, {r8-r11}
	pop	{r4-r11, pc}

i1_sha1_transform:
	push	{r4-r11, lr}
wait_sha1_rdy_input:
	ldr	r4, sha1_rdy_input_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_sha1_rdy_input
	ldr	r3, sha1_init_reg
//align at 64bit,
	ldmia	r0, {r6-r11}
	stmia	r3, {r6-r11}
wait_sha1_rdy_input2:
	ldr	r4, sha1_rdy_input_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_sha1_rdy_input2
//send by 2 steps
	ldr	r3, sha1_input_reg_1
	ldmia	r1, {r4-r11}
	stmia	r3, {r4-r11}
	ldr	r3, sha1_input_reg_2
	add	r1, r1, #32
	ldmia	r1, {r4-r11}
	stmia	r3, {r4-r11}
wait_sha1_output_rdy:
	ldr	r4, sha1_rdy_output_reg
	ldr	r3, [r4]
	cmp	r3, #1
	bne	wait_sha1_output_rdy
	ldr	r3, sha1_output_reg
	ldmia	r3, {r6-r11}
	stmia	r0!, {r6-r11}
	pop	{r4-r11, pc}


.align 5
aes_256_key_reg:	.word	CRYPT_UNIT_REG(0x200)
aes_enc_input_reg:	.word	CRYPT_UNIT_REG(0x248)
aes_dec_input_reg:	.word	CRYPT_UNIT_REG(0x258)
aes_output_ready_reg:	.word	CRYPT_UNIT_REG(0x278)
aes_output_reg:		.word	CRYPT_UNIT_REG(0x268)
des_key_reg:		.word	CRYPT_UNIT_REG(0x00)
des_enc_input_reg:	.word	CRYPT_UNIT_REG(0x08)
des_dec_input_reg:	.word	CRYPT_UNIT_REG(0x10)
des_output_ready_reg:	.word	CRYPT_UNIT_REG(0x20)
des_output_reg:		.word	CRYPT_UNIT_REG(0x18)
md5_init_reg:		.word	CRYPT_UNIT_REG(0x480)
md5_input_reg_1:	.word	CRYPT_UNIT_REG(0x490)
md5_input_reg_2:	.word	CRYPT_UNIT_REG(0x4b0)
md5_output_reg:		.word	CRYPT_UNIT_REG(0x4d0)
md5_rdy_input_reg:	.word	CRYPT_UNIT_REG(0x4f0)
md5_rdy_output_reg:	.word	CRYPT_UNIT_REG(0x4e0)
sha1_init_reg:		.word	CRYPT_UNIT_REG(0x400)
sha1_input_reg_1:	.word	CRYPT_UNIT_REG(0x418)
sha1_input_reg_2:	.word	CRYPT_UNIT_REG(0x438)
sha1_output_reg:	.word	CRYPT_UNIT_REG(0x458)
sha1_rdy_output_reg:	.word	CRYPT_UNIT_REG(0x470)
sha1_rdy_input_reg:	.word	CRYPT_UNIT_REG(0x4f0)
//end i1 and s2

#elif defined(CONFIG_ARCH_A5S)

#define CRYPT_UNIT_REG(x)			(0x60014000 + (x))
#define AES_BLOCK_SIZE				(16)
#define DES_BLOCK_SIZE				(8)

.globl	a5s_config_aes_256
.globl	a5s_do_aes
.globl	a5s_config_des
.globl	a5s_do_des

a5s_config_aes_256:
	push	{r4-r11, lr}
	ldr	r3, aes_opcode_reg
	ldr	r2, aes_256_key_reg
	str	r0, [r3]
	ldmia	r1, {r4-r11}
	stmia	r2, {r4-r11}
	pop	{r4-r11, pc}

a5s_do_aes:
	push	{r4-r11, lr}
	ldr	r4, aes_input_reg
	ldr	r5, aes_output_ready_reg
	ldr	r6, aes_output_reg
loop_a5s_do_aes:
	cmp	r2, #AES_BLOCK_SIZE
	blo	exit_a5s_do_aes
	sub	r2, r2, #AES_BLOCK_SIZE
	ldmia	r0!, {r8-r11}
	stmia	r4, {r8-r11}
wait_a5s_do_aes:
	ldr	r7, [r5]
	cmp	r7, #1
	bne	wait_a5s_do_aes
	ldmia	r6, {r8-r11}
	stmia	r1!, {r8-r11}
	b	loop_a5s_do_aes
exit_a5s_do_aes:
	pop	{r4-r11, pc}

a5s_config_des:
	push	{r4-r11, lr}
	ldr	r3, des_opcode_reg
	ldr	r2, des_key_reg
	str	r0, [r3]
	ldmia	r1, {r10-r11}
	stmia	r2, {r10-r11}
	pop	{r4-r11, pc}

a5s_do_des:
	push	{r4-r11, lr}
	ldr	r4, des_input_reg
	ldr	r5, des_output_ready_reg
	ldr	r6, des_output_reg
loop_a5s_do_des:
	cmp	r2, #DES_BLOCK_SIZE
	blo	exit_a5s_do_des
	sub	r2, r2, #DES_BLOCK_SIZE
	ldmia	r0!, {r10-r11}
	stmia	r4, {r10-r11}
wait_a5s_do_des:
	ldr	r7, [r5]
	cmp	r7, #1
	bne	wait_a5s_do_des
	ldmia	r6, {r10-r11}
	stmia	r1!, {r10-r11}
	b	loop_a5s_do_des
exit_a5s_do_des:
	pop	{r4-r11, pc}

.align 5
aes_opcode_reg:		.word	CRYPT_UNIT_REG(0x7c)
aes_256_key_reg:	.word	CRYPT_UNIT_REG(0x4c)
aes_input_reg:		.word	CRYPT_UNIT_REG(0x6c)
aes_output_ready_reg:	.word	CRYPT_UNIT_REG(0x80)
aes_output_reg:		.word	CRYPT_UNIT_REG(0x84)
des_opcode_reg:		.word	CRYPT_UNIT_REG(0x10)
des_key_reg:		.word	CRYPT_UNIT_REG(0x00)
des_input_reg:		.word	CRYPT_UNIT_REG(0x08)
des_output_ready_reg:	.word	CRYPT_UNIT_REG(0x14)
des_output_reg:		.word	CRYPT_UNIT_REG(0x18)
#endif
