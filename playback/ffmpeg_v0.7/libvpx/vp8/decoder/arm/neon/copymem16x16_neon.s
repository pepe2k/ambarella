		.arm
        .fpu neon
        .text
		.global vp8_copy_mem16x16_neon
		.func vp8_copy_mem16x16_neon

@void copy_mem16x16_neon( unsigned char *src, int src_stride, unsigned char *dst, int dst_stride)

vp8_copy_mem16x16_neon:

    vld1.u8     {q0}, [r0], r1
    vld1.u8     {q1}, [r0], r1
    vld1.u8     {q2}, [r0], r1
    vst1.u8     {q0}, [r2], r3
    vld1.u8     {q3}, [r0], r1
    vst1.u8     {q1}, [r2], r3
    vld1.u8     {q4}, [r0], r1
    vst1.u8     {q2}, [r2], r3
    vld1.u8     {q5}, [r0], r1
    vst1.u8     {q3}, [r2], r3
    vld1.u8     {q6}, [r0], r1
    vst1.u8     {q4}, [r2], r3
    vld1.u8     {q7}, [r0], r1
    vst1.u8     {q5}, [r2], r3
    vld1.u8     {q8}, [r0], r1
    vst1.u8     {q6}, [r2], r3
    vld1.u8     {q9}, [r0], r1
    vst1.u8     {q7}, [r2], r3
    vld1.u8     {q10}, [r0], r1
    vst1.u8     {q8}, [r2], r3
    vld1.u8     {q11}, [r0], r1
    vst1.u8     {q9}, [r2], r3
    vld1.u8     {q12}, [r0], r1
    vst1.u8     {q10}, [r2], r3
    vld1.u8     {q13}, [r0], r1
    vst1.u8     {q11}, [r2], r3
    vld1.u8     {q14}, [r0], r1
    vst1.u8     {q12}, [r2], r3
    vld1.u8     {q15}, [r0], r1
    vst1.u8     {q13}, [r2], r3
    vst1.u8     {q14}, [r2], r3
    vst1.u8     {q15}, [r2], r3

    mov     pc, lr

    .endfunc

    .end
