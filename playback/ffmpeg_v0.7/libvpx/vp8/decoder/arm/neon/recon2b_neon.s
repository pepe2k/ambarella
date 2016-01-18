		.arm
        .fpu neon
        .text
		.global vp8_recon2b_neon
		.func vp8_recon2b_neon


@ r0    unsigned char  *pred_ptr,
@ r1    short *diff_ptr,
@ r2    unsigned char *dst_ptr,
@ r3    int stride

vp8_recon2b_neon:
    vld1.u8         {q8, q9}, [r0]      @load data from pred_ptr
    vld1.16         {q4, q5}, [r1]!     @load data from diff_ptr

    vmovl.u8        q0, d16             @modify Pred data from 8 bits to 16 bits
    vld1.16         {q6, q7}, [r1]!
    vmovl.u8        q1, d17
    vmovl.u8        q2, d18
    vmovl.u8        q3, d19

    vadd.s16        q0, q0, q4          @add Diff data and Pred data together
    vadd.s16        q1, q1, q5
    vadd.s16        q2, q2, q6
    vadd.s16        q3, q3, q7

    vqmovun.s16     d0, q0              @CLAMP() saturation
    vqmovun.s16     d1, q1
    vqmovun.s16     d2, q2
    vqmovun.s16     d3, q3
    add             r0, r2, r3

    vst1.u8         {d0}, [r2]          @store result
    vst1.u8         {d1}, [r0], r3
    add             r2, r0, r3
    vst1.u8         {d2}, [r0]
    vst1.u8         {d3}, [r2], r3

    bx             lr

    .endfunc
    .end
