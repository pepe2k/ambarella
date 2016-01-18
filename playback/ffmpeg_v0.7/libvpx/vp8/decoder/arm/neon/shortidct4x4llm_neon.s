     	.arm
        .fpu neon
        .text
		.global vp8_short_idct4x4llm_neon
		.func vp8_short_idct4x4llm_neon

/*************************************************************
void vp8_short_idct4x4llm_c(short *input, short *output, int pitch)
r0 short * input
r1 short * output
r2 int pitch
*************************************************************/


vp8_short_idct4x4llm_neon:
    ldr             r12, =_idct_coeff_
    vld1.16         {q1, q2}, [r0]
    vld1.16         {d0}, [r12]

    vswp            d3, d4                  @q2(vp[4] vp[12])

    vqdmulh.s16     q3, q2, d0[2]
    vqdmulh.s16     q4, q2, d0[0]

    vqadd.s16       d12, d2, d3             @a1
    vqsub.s16       d13, d2, d3             @b1

    vshr.s16        q3, q3, #1
    vshr.s16        q4, q4, #1

    vqadd.s16       q3, q3, q2              @modify since sinpi8sqrt2 > 65536/2 (negtive number)
    vqadd.s16       q4, q4, q2

    @d6 - c1:temp1
    @d7 - d1:temp2
    @d8 - d1:temp1
    @d9 - c1:temp2

    vqsub.s16       d10, d6, d9             @c1
    vqadd.s16       d11, d7, d8             @d1

    vqadd.s16       d2, d12, d11
    vqadd.s16       d3, d13, d10
    vqsub.s16       d4, d13, d10
    vqsub.s16       d5, d12, d11

    vtrn.32         d2, d4
    vtrn.32         d3, d5
    vtrn.16         d2, d3
    vtrn.16         d4, d5

    vswp            d3, d4

    vqdmulh.s16     q3, q2, d0[2]
    vqdmulh.s16     q4, q2, d0[0]

    vqadd.s16       d12, d2, d3             @a1
    vqsub.s16       d13, d2, d3             @b1

    vshr.s16        q3, q3, #1
    vshr.s16        q4, q4, #1

    vqadd.s16       q3, q3, q2              @modify since sinpi8sqrt2 > 65536/2 (negtive number)
    vqadd.s16       q4, q4, q2

    vqsub.s16       d10, d6, d9             @c1
    vqadd.s16       d11, d7, d8             @d1

    vqadd.s16       d2, d12, d11
    vqadd.s16       d3, d13, d10
    vqsub.s16       d4, d13, d10
    vqsub.s16       d5, d12, d11

    vrshr.s16       d2, d2, #3
    vrshr.s16       d3, d3, #3
    vrshr.s16       d4, d4, #3
    vrshr.s16       d5, d5, #3

    add             r3, r1, r2
    add             r12, r3, r2
    add             r0, r12, r2

    vtrn.32         d2, d4
    vtrn.32         d3, d5
    vtrn.16         d2, d3
    vtrn.16         d4, d5

    vst1.16         {d2}, [r1]
    vst1.16         {d3}, [r3]
    vst1.16         {d4}, [r12]
    vst1.16         {d5}, [r0]

    bx             lr

    .endfunc
	
    .section .rodata
_idct_coeff_:	
    .int     0x4e7b4e7b, 0x8a8c8a8c

    .end
