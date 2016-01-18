     	.arm
        .fpu neon
        .text
		.global vp8_yv12_copy_frame_func_neon
		.func vp8_yv12_copy_frame_func_neon


@void vp8_yv12_copy_frame_func_neon(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);

vp8_yv12_copy_frame_func_neon:
    push            {r4 - r11, lr}
    vpush           {d8 - d15}

    sub             sp, sp, #16

    @Copy Y plane
	
	ldr             r8, [r0, #28]       @srcptr1
    ldr             r9, [r1, #28]       @srcptr1
    ldr             r10, [r0, #32]      @srcptr1
    ldr             r11, [r1, #32]      @srcptr1

    ldr             r4, [r0, #4]
    ldr             r5, [r0, #0]
    ldr             r6, [r0, #8]
    ldr             r7, [r1, #8]
    ldr             r2, [r0, #24]       @srcptr1
    ldr             r3, [r1, #24]       @dstptr1
	
    str             r8, [sp]
    str             r9, [sp, #4]
    str             r10, [sp, #8]
    str             r11, [sp, #12]

    @ copy two rows at one time
    mov             lr, r4, lsr #1

cp_src_to_dst_height_loop:
    mov             r8, r2
    mov             r9, r3
    add             r10, r2, r6
    add             r11, r3, r7
    mov             r12, r5, lsr #7

cp_src_to_dst_width_loop:
    vld1.8          {q0, q1}, [r8]!
    vld1.8          {q8, q9}, [r10]!
    vld1.8          {q2, q3}, [r8]!
    vld1.8          {q10, q11}, [r10]!
    vld1.8          {q4, q5}, [r8]!
    vld1.8          {q12, q13}, [r10]!
    vld1.8          {q6, q7}, [r8]!
    vld1.8          {q14, q15}, [r10]!

    subs            r12, r12, #1

    vst1.8          {q0, q1}, [r9]!
    vst1.8          {q8, q9}, [r11]!
    vst1.8          {q2, q3}, [r9]!
    vst1.8          {q10, q11}, [r11]!
    vst1.8          {q4, q5}, [r9]!
    vst1.8          {q12, q13}, [r11]!
    vst1.8          {q6, q7}, [r9]!
    vst1.8          {q14, q15}, [r11]!

    bne             cp_src_to_dst_width_loop

    subs            lr, lr, #1
    add             r2, r2, r6, lsl #1
    add             r3, r3, r7, lsl #1

    bne             cp_src_to_dst_height_loop

    ands            r10, r5, #0x7f                  @check to see if extra copy is needed
    sub             r11, r5, r10
	
	ldr             r2, [r0, #24]       @srcptr1
    ldr             r3, [r1, #24]       @dstptr1
	
    bne             extra_cp_src_to_dst_width
end_of_cp_src_to_dst:

@Copy U & V planes
    ldr             r2, [sp]        @srcptr1
    ldr             r3, [sp, #4]        @dstptr1
    mov             r4, r4, lsr #1                  @src uv_height
    mov             r5, r5, lsr #1                  @src uv_width
    mov             r6, r6, lsr #1                  @src uv_stride
    mov             r7, r7, lsr #1                  @dst uv_stride

    mov             r1, #2

cp_uv_loop:

    @copy two rows at one time
    mov             lr, r4, lsr #1

cp_src_to_dst_height_uv_loop:
    mov             r8, r2
    mov             r9, r3
    add             r10, r2, r6
    add             r11, r3, r7
    mov             r12, r5, lsr #6

cp_src_to_dst_width_uv_loop:
    vld1.8          {q0, q1}, [r8]!
    vld1.8          {q8, q9}, [r10]!
    vld1.8          {q2, q3}, [r8]!
    vld1.8          {q10, q11}, [r10]!

    subs            r12, r12, #1

    vst1.8          {q0, q1}, [r9]!
    vst1.8          {q8, q9}, [r11]!
    vst1.8          {q2, q3}, [r9]!
    vst1.8          {q10, q11}, [r11]!

    bne             cp_src_to_dst_width_uv_loop

    subs            lr, lr, #1
    add             r2, r2, r6, lsl #1
    add             r3, r3, r7, lsl #1

    bne             cp_src_to_dst_height_uv_loop

    ands            r10, r5, #0x3f                  @check to see if extra copy is needed
    sub             r11, r5, r10
    ldr             r2, [sp]        @srcptr1
    ldr             r3, [sp, #4]        @dstptr1
    bne             extra_cp_src_to_dst_uv_width
end_of_cp_src_to_dst_uv:

    subs            r1, r1, #1

    addne               sp, sp, #8

    ldrne               r2, [sp]        @srcptr1
    ldrne               r3, [sp, #4]        @dstptr1

    bne             cp_uv_loop

    add             sp, sp, #8

    vpop            {d8 - d15}
    pop             {r4 - r11, pc}

@=============================
extra_cp_src_to_dst_width:
    add             r2, r2, r11
    add             r3, r3, r11
    add             r0, r8, r6
    add             r11, r9, r7

    mov             lr, r4, lsr #1
extra_cp_src_to_dst_height_loop:
    mov             r8, r2
    mov             r9, r3
    add             r0, r8, r6
    add             r11, r9, r7

    mov             r12, r10

extra_cp_src_to_dst_width_loop:
    vld1.8          {q0}, [r8]!
    vld1.8          {q1}, [r0]!

    subs            r12, r12, #16

    vst1.8          {q0}, [r9]!
    vst1.8          {q1}, [r11]!
    bne             extra_cp_src_to_dst_width_loop

    subs            lr, lr, #1

    add             r2, r2, r6, lsl #1
    add             r3, r3, r7, lsl #1

    bne             extra_cp_src_to_dst_height_loop

    b               end_of_cp_src_to_dst

@=================================
extra_cp_src_to_dst_uv_width:
    add             r2, r2, r11
    add             r3, r3, r11
    add             r0, r8, r6
    add             r11, r9, r7

    mov             lr, r4, lsr #1
extra_cp_src_to_dst_height_uv_loop:
    mov             r8, r2
    mov             r9, r3
    add             r0, r8, r6
    add             r11, r9, r7

    mov             r12, r10

extra_cp_src_to_dst_width_uv_loop:
    vld1.8          {d0}, [r8]!
    vld1.8          {d1}, [r0]!

    subs            r12, r12, #8

    vst1.8          {d0}, [r9]!
    vst1.8          {d1}, [r11]!
    bne             extra_cp_src_to_dst_width_uv_loop

    subs            lr, lr, #1

    add             r2, r2, r6, lsl #1
    add             r3, r3, r7, lsl #1

    bne             extra_cp_src_to_dst_height_uv_loop

    b               end_of_cp_src_to_dst_uv

    .endfunc
    .end