		.arm
        .fpu neon
        .text
		.global vp8_copy_mem8x8_neon
		.func vp8_copy_mem8x8_neon


@void copy_mem8x8_neon( unsigned char *src, int src_stride, unsigned char *dst, int dst_stride)

vp8_copy_mem8x8_neon:

    vld1.u8     {d0}, [r0], r1
    vld1.u8     {d1}, [r0], r1
    vst1.u8     {d0}, [r2], r3
    vld1.u8     {d2}, [r0], r1
    vst1.u8     {d1}, [r2], r3
    vld1.u8     {d3}, [r0], r1
    vst1.u8     {d2}, [r2], r3
    vld1.u8     {d4}, [r0], r1
    vst1.u8     {d3}, [r2], r3
    vld1.u8     {d5}, [r0], r1
    vst1.u8     {d4}, [r2], r3
    vld1.u8     {d6}, [r0], r1
    vst1.u8     {d5}, [r2], r3
    vld1.u8     {d7}, [r0], r1
    vst1.u8     {d6}, [r2], r3
    vst1.u8     {d7}, [r2], r3

    mov     pc, lr

    .endfunc

    .end
