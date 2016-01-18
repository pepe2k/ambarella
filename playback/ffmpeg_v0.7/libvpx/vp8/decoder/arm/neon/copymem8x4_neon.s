		.arm
        .fpu neon
        .text
		.global vp8_copy_mem8x4_neon
		.func vp8_copy_mem8x4_neon


@void copy_mem8x4_neon( unsigned char *src, int src_stride, unsigned char *dst, int dst_stride)

vp8_copy_mem8x4_neon:
    vld1.u8     {d0}, [r0], r1
    vld1.u8     {d1}, [r0], r1
    vst1.u8     {d0}, [r2], r3
    vld1.u8     {d2}, [r0], r1
    vst1.u8     {d1}, [r2], r3
    vld1.u8     {d3}, [r0], r1
    vst1.u8     {d2}, [r2], r3
    vst1.u8     {d3}, [r2], r3

    mov     pc, lr

    .endfunc  

    .end
