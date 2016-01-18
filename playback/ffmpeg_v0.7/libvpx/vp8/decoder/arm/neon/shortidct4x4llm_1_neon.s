     	.arm
        .fpu neon
        .text
		.global vp8_short_idct4x4llm_1_neon
		.global vp8_dc_only_idct_neon
		

@void vp8_short_idct4x4llm_1_c(short *input, short *output, int pitch);
@r0    short *input;
@r1    short *output;
@r2    int pitch;
        
		.func vp8_short_idct4x4llm_1_neon	
		
vp8_short_idct4x4llm_1_neon:
    vld1.16         {d0[]}, [r0]            @load input[0]

    add             r3, r1, r2
    add             r12, r3, r2

    vrshr.s16       d0, d0, #3

    add             r0, r12, r2

    vst1.16         {d0}, [r1]
    vst1.16         {d0}, [r3]
    vst1.16         {d0}, [r12]
    vst1.16         {d0}, [r0]

    bx             lr
    .endfunc


@void vp8_dc_only_idct_c(short input_dc, short *output, int pitch);
@ r0    short input_dc;
@ r1    short *output;
@ r2    int pitch;

        .func vp8_dc_only_idct_neon
		
vp8_dc_only_idct_neon:
    vdup.16         d0, r0

    add             r3, r1, r2
    add             r12, r3, r2

    vrshr.s16       d0, d0, #3

    add             r0, r12, r2

    vst1.16         {d0}, [r1]
    vst1.16         {d0}, [r3]
    vst1.16         {d0}, [r12]
    vst1.16         {d0}, [r0]

    bx             lr

    .endfunc
    .end
