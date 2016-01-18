/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

.global __atomic_cmpxchg
.type __atomic_cmpxchg, %function
.global __atomic_swap
.type __atomic_swap, %function
.global __atomic_dec
.type __atomic_dec, %function
.global __atomic_inc
.type __atomic_inc, %function

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

#ifdef __thumb2__
.syntax unified
.code   16
#endif

/*
 * ===========================================================================
 *      ARMv6+ implementation
 * ===========================================================================
 */

/* r0(addr) -> r0(old) */
__atomic_dec:
    .fnstart
    mov     r1, r0                      @ copy addr so we don't clobber it
1:  ldrex   r0, [r1]                    @ load current value into r0
    sub     r2, r0, #1                  @ generate new value into r2
    strex   r3, r2, [r1]                @ try to store new value; result in r3
    cmp     r3, #0                      @ success?
    bxeq    lr                          @ yes, return
    b       1b                          @ no, retry
    .fnend

/* r0(addr) -> r0(old) */
__atomic_inc:
    .fnstart
    mov     r1, r0
1:  ldrex   r0, [r1]
    add     r2, r0, #1
    strex   r3, r2, [r1]
    cmp     r3, #0
    bxeq    lr
    b       1b
    .fnend

/* r0(old) r1(new) r2(addr) -> r0(zero_if_succeeded) */
__atomic_cmpxchg:
    .fnstart
1:  mov     ip, #2                      @ ip=2 means "new != old"
    ldrex   r3, [r2]                    @ load current value into r3
    teq     r0, r3                      @ new == old?
    strexeq ip, r1, [r2]                @ yes, try store, set ip to 0 or 1
    teq     ip, #1                      @ strex failure?
    beq     1b                          @ yes, retry
    mov     r0, ip                      @ return 0 on success, 2 on failure
    bx      lr
    .fnend

/* r0(new) r1(addr) -> r0(old) */
__atomic_swap:
    .fnstart
1:  ldrex   r2, [r1]
    strex   r3, r0, [r1]
    teq     r3, #0
    bne     1b
    mov     r0, r2
    bx      lr
    .fnend

