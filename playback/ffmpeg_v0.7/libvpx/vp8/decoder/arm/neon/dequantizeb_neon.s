
        .arm
        .fpu neon
        .text
        .global vp8_dequantize_b_loop_neon
        .func vp8_dequantize_b_loop_neon

vp8_dequantize_b_loop_neon:
@ r0    short *Q,
@ r1    short *DQC
@ r2    short *DQ

    vld1.16         {q0, q1}, [r0]
    vld1.16         {q2, q3}, [r1]

    vmul.i16        q4, q0, q2
    vmul.i16        q5, q1, q3

    vst1.16         {q4, q5}, [r2]

    bx             lr
    .endfunc
    .end
	

