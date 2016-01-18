/*
 * Copyright (c) 2008 Mans Rullgard <mans@mansr.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "asm.S"

        preserve8
        .fpu neon

        .text


function ambadec_rv40_pred4x4_vertical_nv12_neon, export =1
        sub 		        r1, r0, r2
        vld1.64		{d0}, [r1]                  @get source 8byte
        vst1.64		{d0}, [r0], r2              @write back
        vst1.64		{d0}, [r0], r2 
        vst1.64		{d0}, [r0], r2
        vst1.64		{d0}, [r0]       
        bx              lr
        ..endfunc

function ambadec_rv40_pred4x4_horizontal_nv12_neon, export =1
        sub               r1, r0, #2
        vld1.16		d0[], [r1], r2                        @get source
        vst1.64		d0, [r0], r2                           @write back

        vld1.16		d1[], [r1], r2                        @get source
        vst1.64		d1, [r0], r2                           @write back

        vld1.16		d2[], [r1], r2                         @get source
        vst1.64		d2, [r0], r2                           @write back
        
        vld1.16		d3[], [r1]                              @get source
        vst1.64		d3, [r0]                                @write back        
      
        bx              lr
        ..endfunc

function ambadec_rv40_pred4x4_128dc_nv12_neon, export =1
        vmov.u8          d0, #128
        vst1.64            d0, [r0], r2                        @write back
        vst1.64            d0, [r0], r2
        vst1.64            d0, [r0], r2
        vst1.64            d0, [r0]
        bx              lr
        ..endfunc

function ambadec_rv40_pred4x4_topdc_nv12_neon, export =1
        sub 		         r1,  r0,  r2
        pld                  [r1]
        vmov.s32         d1, #0
        vld1.64            d0,  [r1]                 @u v interlace  
        vmov.s32         d5,   #2                 @
        vtrn.8              d0,  d1                  @d0: u,  d1: v 
        vmovn.u16      d2, q0
        
        vpaddl.u8        d3,  d2
        vpaddl.u16      d4,  d3                 @add sum

        vadd.s32         d0,  d5,  d4
        vshr.u32         d1, d0, #2            @average
        vshr.u64          d2, d1, #24 
        vorr                d3, d1, d2            @interlave uv reslut
        vdup.16          d4, d3[0]              @duplicate by s16

        vst1.64            d4, [r0], r2
        vst1.64            d4, [r0], r2
        vst1.64            d4, [r0], r2
        vst1.64            d4, [r0]        
        bx                 lr
        ..endfunc

function ambadec_rv40_pred8x8_128dc_nv12_neon, export =1
        mov                 r2, #128
        vdup.8             q0, r2
        add                  r2, r0, r1
        mov                 r1, r1, LSL #1
        
        vstm                r0, {d0,d1} 
        add                  r0, r0, r1 
        vstm                r2, {d0,d1}
        add                  r2, r2, r1 
            
        vstm                r0, {d0,d1} 
        add                  r0, r0, r1 
        vstm                r2, {d0,d1}
        add                  r2, r2, r1 
        
        vstm                r0, {d0,d1} 
        add                  r0, r0, r1 
        vstm                r2, {d0,d1}
        add                  r2, r2, r1 
            
        vstm                r0, {d0,d1} 
        vstm                r2, {d0,d1}

        bx              lr
        ..endfunc


function ambadec_rv40_pred8x8_vertical_nv12_neon, export =1
        sub 		        r2,  r0, r1
        vldm               r2, {d0,d1}
        add                 r2, r2, r1, LSL #1
        mov                r1, r1, LSL #1

        vstm                r0, {d0,d1} 
        add                  r0, r0, r1 
        vstm                r2, {d0,d1}
        add                  r2, r2, r1 

        vstm                r0, {d0,d1} 
        add                  r0, r0, r1 
        vstm                r2, {d0,d1}
        add                  r2, r2, r1 

        vstm                r0, {d0,d1} 
        add                  r0, r0, r1 
        vstm                r2, {d0,d1}
        add                  r2, r2, r1 

        vstm                r0, {d0,d1} 
        vstm                r2, {d0,d1}
        
        bx              lr
        ..endfunc

function ambadec_rv40_pred8x8_topdc_nv12_neon, export =1
        sub 		        r2,  r0, r1
        vldm               r2, {d0, d1}          @u v interlace
        vtrn.8             d0,  d1                @d0: u,  d1: v 
        mov               r1, r1, LSL #1

        vpaddl.u8        q0,  q0
        vmov.s32        d2,   #4 
        vpaddl.u16      q0,  q0
        vpaddl.u32      q0,  q0                 @add sum

        vmovl.s32       q2,   d2
        vadd.s64         q1,  q0,  q2

        vshr.u64         q3, q1, #3            @average
        vshl.u64          d4, d7, #8 
        vorr                d2, d4, d6            @interlave uv reslut
        add                r2, r2, r1
        vdup.16          q0, d2[0]              @duplicate by s16

        vstm                r0, {d0,d1} 
        add                  r0, r0, r1 
        vstm                r2, {d0,d1}
        add                  r2, r2, r1 

        vstm                r0, {d0,d1} 
        add                  r0, r0, r1 
        vstm                r2, {d0,d1}
        add                  r2, r2, r1

        vstm                r0, {d0,d1} 
        add                  r0, r0, r1 
        vstm                r2, {d0,d1}
        add                  r2, r2, r1
            
        vstm                r0, {d0,d1} 
        vstm                r2, {d0,d1}
            
        bx              lr
        ..endfunc

@ r0, r3 write address
function ambadec_rv40_pred8x8_horizontal_nv12_neon, export =1
        sub                r2,  r0, #2

        pld                 [r2, r1]
        vld1.16           {d0[], d1[]},  [r2],  r1                     @load data and duplicate
        vst1.64          {d0, d1}, [r0],  r1                            @write data

        pld                 [r2, r1]
        vld1.16           {d2[], d3[]},  [r2],  r1                     @load data and duplicate
        vst1.64          {d2, d3}, [r0],  r1                            @write data

        pld                 [r2, r1]
        vld1.16           {d0[], d1[]},  [r2],  r1                     @load data and duplicate
        vst1.64          {d0, d1}, [r0],  r1                            @write data

        pld                 [r2, r1]
        vld1.16           {d2[], d3[]},  [r2],  r1                     @load data and duplicate
        vst1.64          {d2, d3}, [r0],  r1                            @write data

        pld                 [r2, r1]
        vld1.16           {d0[], d1[]},  [r2],  r1                     @load data and duplicate
        vst1.64          {d0, d1}, [r0],  r1                            @write data

        pld                 [r2, r1]
        vld1.16           {d2[], d3[]},  [r2],  r1                     @load data and duplicate
        vst1.64          {d2, d3}, [r0],  r1                            @write data

        pld                 [r2, r1]
        vld1.16           {d0[], d1[]},  [r2],  r1                     @load data and duplicate
        vst1.64          {d0, d1}, [r0],  r1                            @write data

        vld1.16           {d2[], d3[]},  [r2]                            @load data and duplicate
        vst1.64          {d2, d3}, [r0]                                   @write data
        
               
        bx              lr
        ..endfunc



function ambadec_rv40_pred8x8_128dc_neon, export =1
        mov                 r2, #128
        vdup.8             d0, r2                              @prepare data
        
        vst1.64           d0, [r0], r1                       @write
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0]
        
        bx              lr
        ..endfunc

function ambadec_rv40_pred8x8_vertical_neon, export =1
        sub 		         r2,  r0, r1
        vld1.64            d0, [r2]
        
        vst1.64           d0, [r0], r1                       @write
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0], r1
        vst1.64           d0, [r0]
        bx              lr
        ..endfunc

function ambadec_rv40_pred8x8_topdc_neon, export =1
        sub 		        r2,  r0, r1
        vld1.64            d0, [r2]                      @load data
        vmov.s32        d4,   #4          
        vpaddl.u8        d1, d0                   @calculate sum
        vpaddl.u16      d2, d1
        vpaddl.u32      d3, d2
        vadd.i32         d5, d4, d3             @add 4, div by 8
        vshr.u32         d0, d5, #3                     
        vdup.8           d1, d0[0]                     @expand data
        
        vst1.64            d1, [r0], r1                 @write
        vst1.64            d1, [r0], r1
        vst1.64            d1, [r0], r1
        vst1.64            d1, [r0], r1
        vst1.64            d1, [r0], r1
        vst1.64            d1, [r0], r1
        vst1.64            d1, [r0], r1
        vst1.64            d1, [r0]
        
        bx              lr
        ..endfunc

@r2 read address, r0 write address
function ambadec_rv40_pred8x8_horizontal_neon, export =1
        sub                r2,  r0, #1
        pld                 [r2, r1]
        vld1.8             d0[], [r2],  r1
        vst1.64           d0, [r0],  r1

        pld                 [r2, r1]
        vld1.8             d1[], [r2],  r1
        vst1.64           d1,  [r0],  r1

        pld                 [r2, r1]
        vld1.8             d2[], [r2],  r1
        vst1.64           d2,  [r0],  r1

        pld                 [r2, r1]
        vld1.8             d3[],  [r2],  r1
        vst1.64           d3,  [r0],  r1

        pld                 [r2, r1]
        vld1.8             d0[], [r2],  r1
        vst1.64           d0, [r0],  r1

        pld                 [r2, r1]
        vld1.8             d1[], [r2],  r1
        vst1.64           d1,  [r0],  r1

        pld                 [r2, r1]
        vld1.8             d2[], [r2],  r1
        vst1.64           d2,  [r0],  r1

        vld1.8             d3[],  [r2]
        vst1.64           d3,  [r0]
        
        bx              lr
        ..endfunc


function ambadec_rv40_pred16x16_128dc_neon, export =1
        mov                 r2, #128
        vdup.8             q0, r2
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0]
        bx              lr
        ..endfunc

function ambadec_rv40_pred16x16_vertical_neon, export =1
        sub 		         r2,  r0, r1
        vld1.64            {d0,d1}, [r2]
        
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0]
        bx              lr
        ..endfunc

function ambadec_rv40_pred16x16_topdc_neon, export =1
        sub 		        r2,  r0, r1
        vld1.64            {d0,d1}, [r2]                        @load 16 bytes
        
@        vpaddl.u8        d2, d0                                   @calculate sum of 16 bytes
@        vpaddl.u8        d3, d1
        vpaddl.u8           q1, q0 
        
        vmov.s32        d6, #8                                    @rounding

        vpadd.u16       d4, d3, d2
        vpaddl.u16      d5, d4
        vpaddl.u32      d0, d5
        vadd.u32        d2, d0, d6
        
        vshr.u64         d3, d2, #4                            @average
        vdup.8            q0, d3[0]
        
        vst1.64            {d0,d1}, [r0], r1                 @write
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0], r1
        vst1.64            {d0,d1}, [r0]
        bx              lr
        ..endfunc

function ambadec_rv40_pred16x16_horizontal_neon, export =1
        sub                r2,  r0, #1
        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1        

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        pld                 [r2, r1]
        vld1.8             {d0[], d1[]},  [r2],  r1
        vst1.64           {d0, d1},  [r0],  r1

        vld1.8             {d0[], d1[]},  [r2]
        vst1.64           {d0, d1},  [r0]  
       
        bx              lr
        ..endfunc

@@r0: des_y, r1: des_uv, r2:dct, r3: stride, r5: dctu, r6: des_y 3-4 block, r7: 2-block offset, r8: offset 
function ambadec_rv40_add_residue_nv12_neon, export =1
        pld               [r0]
        pld               [r0, r3]
        pld               [r2]
        pld               [r2, #128]
        pld               [r2, #256]
        pld               [r2, #384]
        pld               [r2, #512]
        pld               [r1]
        pld               [r1, r3]
        pld               [r2, #640]
        
        stmfd sp!,    {r4-r8}
        mov             r4,  #4                                      @r4: loop count
        add              r5, r2, #512                              @r5: dct elememt u
        add              r6, r0, r3, LSL #3                      @r6: des_y, 3-4 block          
        add              r7, #256                                   @dct 2block offset
        mov             r8, #128                                    @dct block offset

        pld              [r6]
        pld              [r6, r3]

1:
        vld1.64         {d0, d1}, [r0]                           @load y: 1x16 first line
        vld1.64         {d16, d17}, [r0, r3]                  @load y: 1x16 second line
        vld1.64         {d4, d5, d6, d7}, [r2]           @dct element y: 2x8, 2x8, first half 
        vld1.64         {d20, d21, d22, d23}, [r2, r8]      @dct element y: 2x8, 2x8, second half
        vld1.64         {d2, d3}, [r1]                           @load uv: 1x16 interlave first line
        vld1.64         {d18, d19}, [r1, r3]                  @load uv: 1x16 interlave second line

        vld1.64         {d10, d11},  [r5, r8]                  @dct element v
        vld1.64         {d8, d9},  [r5]                           @dct element u

        vmovl.u8      q6, d0                                       @expand y to 16
        vmovl.u8      q7, d1
        
        add              r5, r5, #16                                    @update r5
        
        vmovl.u8      q14, d16                                   @expand y to 16
        vmovl.u8      q15, d17
        
        vld1.64         {d24, d25},  [r5]                     @dct element u
        vld1.64         {d26, d27},   [r5, r8]                @dct element v
        
        vadd.s16      q6, q6, q2                                 @add y
        vadd.s16      q7, q7, q10    

        vadd.s16      q14, q14, q3                             @add y
        vadd.s16      q15, q15, q11    

        vtrn.16         q4, q5                                       @interlave u and v dct element
        vtrn.16         q12, q13                                    @interlave u and v dct element

        vqmovun.s16  d0, q6                                     @restrict to 8
        vqmovun.s16  d1, q7
        
        vqmovun.s16  d16, q14                                  @restrict to 8
        vqmovun.s16  d17, q15

        vst1.64         {d0, d1}, [r0], r3                      @write result y
        
        vmovl.u8      q6, d2                                       @expand uv to 16
        vmovl.u8      q7, d3

        vst1.64         {d16, d17}, [r0], r3                  @write result y
                
        vmovl.u8      q14, d18                                    @expand uv to 16
        vmovl.u8      q15, d19

        vadd.s16      q6, q6, q4                                 @add uv
        vadd.s16      q7, q7, q5
        
        vadd.s16      q14, q14, q12                             @add uv
        vadd.s16      q15, q15, q13

        vqmovun.s16  d2, q6                                     @restrict to 8
        vqmovun.s16  d3, q7

@block 3-4:

        vqmovun.s16  d18, q14                                  @restrict to 8
        vqmovun.s16  d19, q15
        
        add                r2, r2, #256                            @block 3, 4

        vld1.64         {d0, d1}, [r6]                           @load y
        vld1.64         {d4, d5, d6, d7}, [r2]           @dct element y: 2x8, sy first half

        vst1.64         {d18, d19}, [r1, r3]                 @write result uv
        
        vld1.64         {d16, d17}, [r6, r3]                   @load y
        vld1.64         {d20, d21, d22, d23}, [r2, r8]       @dct element y
        
        vst1.64         {d2, d3}, [r1], r3                     @write result uv      

        vmovl.u8      q6, d0                                       @expand y to 16
        vmovl.u8      q7, d1

        vmovl.u8      q14, d16                                       @expand y to 16
        vmovl.u8      q15, d17
        
        vadd.s16      q6, q6, q2                                      @add y
        vadd.s16      q7, q7, q10   

        vadd.s16      q14, q14, q3                                 @add y
        vadd.s16      q15, q15, q11 

        vqmovun.s16  d0, q6                                     @restrict to 8
        vqmovun.s16  d1, q7
        
        vqmovun.s16  d16, q14                                   @restrict to 8
        vqmovun.s16  d17, q15
        
        add               r5, #16                                   @update r5        
        sub               r2, r2, #224                            @update r2, -256+32
        
        subs             r4, #1
        
        pld               [r0]
        pld               [r0, r3]
        pld               [r2]
        pld               [r2, #128]
        
        vst1.64         {d16, d17}, [r6, r3]                 @write result y

        pld               [r2, #256]
        pld               [r2, #384]
        pld               [r5]
        pld               [r5, #128]
        
        vst1.64         {d0, d1}, [r6] , r3                    @write result y

        pld               [r1]
        pld               [r1, r3]
        pld               [r6]
        pld               [r6, r3]
        
        bne  1b

        ldmfd sp!, {r4-r8}        
        bx                    lr
        ..endfunc       


@@r0: des_y, r1:dct, r2: stride 
function ambadec_rv40_add_residue_y4x4_neon, export =1
        mov               r3, r0                       @reserve write back address
        pld                 [r0]
        pld                 [r1]
        pld                 [r0, r2]
        pld                 [r1, #32]
        pld                 [r0, r2, LSL #1]
        pld                 [r1, #64]
        pld                 [r1, #96]
        stmfd sp!,      {r4-r8}
        mov              r8, #32
        
        ldr                 r4, [r0], r2                @load data, des_y=r4, r5, r6, r7. dct element= d0, d1, d2, d3
        vld1.64          d0, [r1], r8
        ldr                 r5, [r0], r2
        vld1.64          d1, [r1], r8
        pld                [r0, r2]
        ldr                 r6, [r0], r2
        vld1.64          d2, [r1] ,r8
        ldr                 r7, [r0]
        vld1.64          d3, [r1]

        vmov            d4, r4, r5                 @move des_y to d4, d5
        vmov            d5, r6, r7

        vmovl.u8      q4, d4                       @expand to 16
        vmovl.u8      q5, d5

        vadd.s16      q2, q0, q4                 @add sum
        vadd.s16      q3, q1, q5

        vqmovun.s16  d0, q2                      @restrict to 8
        vqmovun.s16  d1, q3

        vmov           r4, r5, d0                  @move result to r4-r7
        vmov           r6, r7, d1

        str              r4, [r3], r2                 @write result
        str              r5, [r3], r2
        str              r6, [r3], r2
        str              r7, [r3]

        ldmfd sp!,   {r4-r8}
        
        bx                    lr
        ..endfunc  

@@r0: des_y, r1:dct, r2: stride 
function ambadec_rv40_add_residue_uv4x4_neon, export =1
        add              r3,   r2,    r2,  LSL #1                    @r3=stride*3
        
        pld                [r0]
        pld                [r0, r2]
        pld                [r0, r2, LSL #1]
        pld                [r0, r3]
        pld                [r1]
        pld                [r1, #64]
        pld                [r1, #128]
        pld                [r1, #192]

        mov              r3, r0
        
        vld1.64          {d0},    [r3] , r2                           @d0: uv 8x(u8)
        vld1.64          {d8},    [r3] , r2                          @d8: uv 8x(u8)
        vld1.64          {d16},  [r3] , r2                          @d16: uv 8x(u8) 
        vld1.64          {d24},  [r3]                                 @d24: uv 8x(u8)

        mov              r3, #16
                
        vld1.64          {d4},    [r1], r3                          @d4: dctu 4x(s16)
        vld1.64          {d12},  [r1], r3                          @d12: dctu 4x(s16)
        vld1.64          {d20},  [r1], r3                         @d20: dctu 4x(s16)
        vld1.64          {d28},  [r1]                           @d28: dctu 4x(s16)

        add               r1, r1, #80

        vld1.64          {d5},    [r1], r3                       @d5: dctv 4x(s16) 
        vld1.64          {d13},  [r1], r3                        @d13: dctv 4x(s16)
        vld1.64          {d21},  [r1], r3                       @d21: dctv 4x(s16)
        vld1.64          {d29},  [r1]                             @d29: dctv 4x(s16)


        vmovl.u8       q1, d0                                        @q1: uv 8x(s16)
        vmovl.u8       q5, d8                                        @q5: uv 8x(s16)
        vmovl.u8       q9, d16                                      @q9: uv 8x(s16)
        vmovl.u8       q13, d24                                    @q13: uv 8x(s16)

        vmovl.u16     q0, d5                                        @expand to u16
        vmovl.u16     q4, d13                                      @expand to u16
        vmovl.u16     q8, d21                                      @expand to u16
        vmovl.u16     q12, d29                                    @expand to u16

        vmovl.u16     q3, d4                     
        vmovl.u16     q7, d12                     
        vmovl.u16     q11, d20                     
        vmovl.u16     q15, d28                     

        vshl.i64          q0, #16                      
        vshl.i64          q4, #16                      
        vshl.i64          q8, #16                      
        vshl.i64          q12, #16                      

        vorr               q2, q0, q3                               @q2 is uv interlave        
        vorr               q6, q4, q7                               @q6 is uv interlave        
        vorr               q10, q8, q11                           @q10 is uv interlave        
        vorr               q14, q12, q15                         @q14 is uv interlave        

        vqadd.s16      q3, q2, q1                               @add, q3 is result 
        vqadd.s16      q7, q6, q5                               @add, q7 is result 
        vqadd.s16      q11, q10, q9                           @add, q11 is result 
        vqadd.s16      q15, q14, q13                         @add, q15 is result 
        
        vqmovun.s16  d0, q3                                    @put result to 8bit width            
        vqmovun.s16  d8, q7                                    @put result to 8bit width            
        vqmovun.s16  d16, q11                                @put result to 8bit width            
        vqmovun.s16  d24, q15                                @put result to 8bit width 
        
        vst1.64           d0, [r0], r2                             @write back result
        vst1.64           d8, [r0], r2                             @write back result
        vst1.64           d16, [r0], r2                          @write back result
        vst1.64           d24, [r0]                               @write back result

        bx                    lr
        ..endfunc 
        


function ambadec_rv40_invtrans_8_neon, export =1
        vld1.64     d0, [r0]
        vld1.64     d1, [r0, #16]
        vld1.64     d2, [r0, #32]
        vld1.64     d3, [r0, #48]      @d0:a, d1:b, d2:c, d3:d

@row transform

        vaddl.s16  q2, d0, d2
        vsubl.s16  q5, d0, d2

        vmovl.s16  q8, d1                 @q8: b
        vmovl.s16  q9, d3                 @q9: d
        
        vshl.s32    q3, q2, #4
        vshl.s32    q6, q5, #4
        
        vshl.s32     q10, q8, #4          @q10: 16*b
        vshl.s32     q11, q9, #3          @q11: 8*d
        
        vadd.s32   q3, q3, q2
        vadd.s32   q6, q6, q5

        vadd.s32    q12, q10, q8         @q12: 17*b
        vshr.s32     q10, q10, #1         @q10: 8*b        
        vadd.s32    q13, q12, q11       @q13: 17*b+8*d
        
        vshl.s32    q4, q2, #2
        vshl.s32    q7, q5, #2

        vsub.s32    q10, q10, q8          @q10: 7*b
        vshl.s32     q11, q11, #1         @q11: 16*d
        
        vsub.s32   q14, q3, q4           @q14: z0=13*(a+c)
        
        vsub.s32    q10, q10, q11        @q10: 7*b-16*d
        vsub.s32    q13, q13, q9         @q13: z2=17*b+7*d

        vsub.s32   q6, q6, q7            @q6: z1=13*(a-c)

        vsub.s32    q10, q10, q9           @q10: z3=7*b-17*d

@ q14: z0, q6: z1, q13: z2, q10: z3        
        vadd.s32    q1, q6, q13
        vsub.s32    q2, q6, q13       
        vadd.s32    q0, q14, q10
        vsub.s32    q3, q14, q10          
@q0, q1, q2, q3: row result

@transpose matrix
        vswp          d1, d4
        vswp          d3, d16
        vtrn.s32    q0, q1
        vtrn.s32    q2,  q3                

@column transform
        vmov.s32       d18, #0x200
        
        vadd.s32   q15, q0, q2           @q15: a+c
        vsub.s32   q5,  q0, q2             @q5: a-c
        
        vshl.s32     q10, q1, #4          @q10: 16*b
        vshl.s32     q11, q3, #3          @q11: 8*d

        vmovl.s32    q8, d18
        
        vshl.s32    q14, q15, #4          @q14: 16*(a+c)
        vshl.s32    q6, q5, #4               @q6: 16*(a-c)

        vadd.s32    q12, q10, q1         @q12: 17*b
        vshr.s32     q10, q10, #1         @q10: 8*b  
        vadd.s32    q13, q12, q11       @q13: 17*b+8*d
        
        vadd.s32   q14, q14, q15         @q14: 17*(a+c)
        vadd.s32   q6, q6, q5               @q6: 17*(a-c)
        vshl.s32    q4, q15, #2
        
        vsub.s32    q10, q10, q1          @q10: 7*b
        vshl.s32     q11, q11, #1         @q11: 16*d
        vsub.s32    q13, q13, q3         @q13: z2=17*b+7*d
        
        vshl.s32    q7, q5, #2
        vsub.s32   q14, q14, q4           @q14: z0=13*(a+c)

        vsub.s32    q10, q10, q11        @q10: 7*b-16*d

        vsub.s32   q6, q6, q7               @q6: z1=13*(a-c)
        vadd.s32   q14, q14, q8          @add rounding
        
        vsub.s32    q10, q10, q3           @q10: z3=7*b-17*d  

        vadd.s32   q6, q6, q8               @add rounding
        
@ q14: z0, q6: z1, q13: z2, q10: z3 
        vadd.s32    q5, q14, q10           @q5: z0 + z3
        vadd.s32    q7, q6, q13             @q7: z1 + z2
        vsub.s32    q8, q6, q13             @q8: z1 - z2
        vsub.s32    q9, q14, q10           @q9: z0 - z3

        vshr.s32    q14, q5, #10
        vshr.s32    q11, q7, #10
        vshr.s32    q12, q8, #10
        vshr.s32    q13, q9, #10

        vmovn.s32   d0, q14        
        vmovn.s32   d1, q11
        vmovn.s32   d2, q12        
        vmovn.s32   d3, q13        

@transpose back
        vtrn.s32    d0, d2
        vtrn.s32    d1, d3                
        vtrn.s16    d0, d1
        vtrn.s16    d2, d3  
        
@write result
        vst1.64       d0, [r0]
        vst1.64       d1, [r0, #16]
        vst1.64       d2, [r0, #32]
        vst1.64       d3, [r0, #48]
                
        bx                    lr
        ..endfunc  


function ambadec_rv40_invtrans_16_neon, export =1
        vld1.64     d0, [r0]
        vld1.64     d1, [r0, #32]
        vld1.64     d2, [r0, #64]
        vld1.64     d3, [r0, #96]      @d0:a, d1:b, d2:c, d3:d

@row transform

        vaddl.s16  q2, d0, d2
        vsubl.s16  q5, d0, d2

        vmovl.s16  q8, d1                 @q8: b
        vmovl.s16  q9, d3                 @q9: d
        
        vshl.s32    q3, q2, #4
        vshl.s32    q6, q5, #4
        
        vshl.s32     q10, q8, #4          @q10: 16*b
        vshl.s32     q11, q9, #3          @q11: 8*d
        
        vadd.s32   q3, q3, q2
        vadd.s32   q6, q6, q5

        vadd.s32    q12, q10, q8         @q12: 17*b
        vshr.s32     q10, q10, #1         @q10: 8*b        
        vadd.s32    q13, q12, q11       @q13: 17*b+8*d
        
        vshl.s32    q4, q2, #2
        vshl.s32    q7, q5, #2

        vsub.s32    q10, q10, q8          @q10: 7*b
        vshl.s32     q11, q11, #1         @q11: 16*d
        
        vsub.s32   q14, q3, q4           @q14: z0=13*(a+c)
        
        vsub.s32    q10, q10, q11        @q10: 7*b-16*d
        vsub.s32    q13, q13, q9         @q13: z2=17*b+7*d

        vsub.s32   q6, q6, q7            @q6: z1=13*(a-c)

        vsub.s32    q10, q10, q9           @q10: z3=7*b-17*d

@ q14: z0, q6: z1, q13: z2, q10: z3        
        vadd.s32    q1, q6, q13
        vsub.s32    q2, q6, q13       
        vadd.s32    q0, q14, q10
        vsub.s32    q3, q14, q10          
@q0, q1, q2, q3: row result

@transpose matrix
        vswp          d1, d4
        vswp          d3, d16
        vtrn.s32    q0, q1
        vtrn.s32    q2,  q3                

@column transform
        vmov.s32       d18, #0x200
        
        vadd.s32   q15, q0, q2           @q15: a+c
        vsub.s32   q5,  q0, q2             @q5: a-c
        
        vshl.s32     q10, q1, #4          @q10: 16*b
        vshl.s32     q11, q3, #3          @q11: 8*d

        vmovl.s32    q8, d18
        
        vshl.s32    q14, q15, #4          @q14: 16*(a+c)
        vshl.s32    q6, q5, #4               @q6: 16*(a-c)

        vadd.s32    q12, q10, q1         @q12: 17*b
        vshr.s32     q10, q10, #1         @q10: 8*b  
        vadd.s32    q13, q12, q11       @q13: 17*b+8*d
        
        vadd.s32   q14, q14, q15         @q14: 17*(a+c)
        vadd.s32   q6, q6, q5               @q6: 17*(a-c)
        vshl.s32    q4, q15, #2
        
        vsub.s32    q10, q10, q1          @q10: 7*b
        vshl.s32     q11, q11, #1         @q11: 16*d
        vsub.s32    q13, q13, q3         @q13: z2=17*b+7*d
        
        vshl.s32    q7, q5, #2
        vsub.s32   q14, q14, q4           @q14: z0=13*(a+c)

        vsub.s32    q10, q10, q11        @q10: 7*b-16*d

        vsub.s32   q6, q6, q7               @q6: z1=13*(a-c)
        vadd.s32   q14, q14, q8          @add rounding
        
        vsub.s32    q10, q10, q3           @q10: z3=7*b-17*d  

        vadd.s32   q6, q6, q8               @add rounding
        
@ q14: z0, q6: z1, q13: z2, q10: z3 
        vadd.s32    q5, q14, q10           @q5: z0 + z3
        vadd.s32    q7, q6, q13             @q7: z1 + z2
        vsub.s32    q8, q6, q13             @q8: z1 - z2
        vsub.s32    q9, q14, q10           @q9: z0 - z3

        vshr.s32    q14, q5, #10
        vshr.s32    q11, q7, #10
        vshr.s32    q12, q8, #10
        vshr.s32    q13, q9, #10

        vmovn.s32   d0, q14        
        vmovn.s32   d1, q11
        vmovn.s32   d2, q12        
        vmovn.s32   d3, q13        

@transpose back
        vtrn.s32    d0, d2
        vtrn.s32    d1, d3                
        vtrn.s16    d0, d1
        vtrn.s16    d2, d3  
        
@write result
        vst1.64       d0, [r0]
        vst1.64       d1, [r0, #32]
        vst1.64       d2, [r0, #64]
        vst1.64       d3, [r0, #96]
                
        bx                    lr
        ..endfunc  
        

function ambadec_rv40_invtrans_8_dc_neon, export =1
        vld1.64     d0, [r0]
        vld1.64     d1, [r0, #16]
        vld1.64     d2, [r0, #32]
        vld1.64     d3, [r0, #48]      @d0:a, d1:b, d2:c, d3:d

@row transform

        vaddl.s16  q2, d0, d2
        vsubl.s16  q5, d0, d2

        vmovl.s16  q8, d1                 @q8: b
        vmovl.s16  q9, d3                 @q9: d
        
        vshl.s32    q3, q2, #4
        vshl.s32    q6, q5, #4
        
        vshl.s32     q10, q8, #4          @q10: 16*b
        vshl.s32     q11, q9, #3          @q11: 8*d
        
        vadd.s32   q3, q3, q2
        vadd.s32   q6, q6, q5

        vadd.s32    q12, q10, q8         @q12: 17*b
        vshr.s32     q10, q10, #1         @q10: 8*b        
        vadd.s32    q13, q12, q11       @q13: 17*b+8*d
        
        vshl.s32    q4, q2, #2
        vshl.s32    q7, q5, #2

        vsub.s32    q10, q10, q8          @q10: 7*b
        vshl.s32     q11, q11, #1         @q11: 16*d
        
        vsub.s32   q14, q3, q4           @q14: z0=13*(a+c)
        
        vsub.s32    q10, q10, q11        @q10: 7*b-16*d
        vsub.s32    q13, q13, q9         @q13: z2=17*b+7*d

        vsub.s32   q6, q6, q7            @q6: z1=13*(a-c)

        vsub.s32    q10, q10, q9           @q10: z3=7*b-17*d

@ q14: z0, q6: z1, q13: z2, q10: z3        
        vadd.s32    q1, q6, q13
        vsub.s32    q2, q6, q13       
        vadd.s32    q0, q14, q10
        vsub.s32    q3, q14, q10          
@q0, q1, q2, q3: row result

@transpose matrix
        vswp          d1, d4
        vswp          d3, d16
        vtrn.s32    q0, q1
        vtrn.s32    q2,  q3                

@column transform       
        vadd.s32   q15, q0, q2           @q15: a+c
        vsub.s32   q5,  q0, q2             @q5: a-c
        
        vshl.s32     q10, q1, #4          @q10: 16*b
        vshl.s32     q11, q3, #3          @q11: 8*d
        
        vshl.s32    q14, q15, #4          @q14: 16*(a+c)
        vshl.s32    q6, q5, #4               @q6: 16*(a-c)

        vadd.s32    q12, q10, q1         @q12: 17*b
        vshr.s32     q10, q10, #1         @q10: 8*b  
        vadd.s32    q13, q12, q11       @q13: 17*b+8*d
        
        vadd.s32   q14, q14, q15         @q14: 17*(a+c)
        vadd.s32   q6, q6, q5               @q6: 17*(a-c)
        vshl.s32    q4, q15, #2
        
        vsub.s32    q10, q10, q1          @q10: 7*b
        vshl.s32     q11, q11, #1         @q11: 16*d
        vsub.s32    q13, q13, q3         @q13: z2=17*b+7*d
        vsub.s32    q10, q10, q11        @q10: 7*b-16*d
        
        vshl.s32    q7, q5, #2
        vsub.s32   q14, q14, q4           @q14: z0=13*(a+c)
        vsub.s32   q6, q6, q7               @q6: z1=13*(a-c)
        
        vsub.s32    q10, q10, q3           @q10: z3=7*b-17*d  

        
@ q14: z0, q6: z1, q13: z2, q10: z3 
        vadd.s32    q5, q14, q10           @q5: z0 + z3
        vadd.s32    q7, q6, q13             @q7: z1 + z2
        vsub.s32    q8, q6, q13             @q8: z1 - z2
        vsub.s32    q9, q14, q10           @q9: z0 - z3

        vshl.s32     q11, q5, #1
        vshl.s32     q12, q7, #1
        vshl.s32     q15, q8, #1
        vshl.s32     q4,  q9, #1

        vadd.s32    q2, q11, q5
        vadd.s32    q3, q12, q7
        vadd.s32    q5, q15, q8
        vadd.s32    q6, q4, q9

        vshr.s32    q14, q2, #11
        vshr.s32    q11, q3, #11
        vshr.s32    q12, q5, #11
        vshr.s32    q13, q6, #11

        vmovn.s32   d0, q14        
        vmovn.s32   d1, q11
        vmovn.s32   d2, q12        
        vmovn.s32   d3, q13        

@transpose back
        vtrn.s32    d0, d2
        vtrn.s32    d1, d3                
        vtrn.s16    d0, d1
        vtrn.s16    d2, d3  
        
@write result
        vst1.64       d0, [r0]
        vst1.64       d1, [r0, #16]
        vst1.64       d2, [r0, #32]
        vst1.64       d3, [r0, #48]
                
        bx                    lr
        ..endfunc 




function test_1, export =1
        vld1.64            {d0,d1}, [r0], r1
        vmov               q1, q0
        vmov               q2, q0
        vmov               q3, q0
        vuzp.8             d0, d1
        vst1.64            {d0, d1}, [r0], r1                        @write back
        vzip.8               d2, d3
        vst1.64            {d2, d3}, [r0], r1                        @write back     
        vuzp.32            d4, d5
        vst1.64            {d4, d5}, [r0], r1                        @write back  
        vzip.32             d6, d7
        vst1.64            {d6, d7}, [r0]                              @write back 
        bx              lr
        ..endfunc

function test_2, export =1
        vld1.64            {d0,d1}, [r0], r1
        mov                 r2, #0
        movs                 r2, r2
        mov                  r3, #0x5d
        mov                  r2, #0x32
@        vmoveq.u8            q1, #0x1f
@        vmovne.u8            q2, #0x2e
@        vmoveq.u8            q2, #0x3d
@        vmoveq.u8            q3, #0x4c
        streq                 r2, [r0, #4]
        streq                 r3, [r0], r1 
        vst1.64            {d0, d1}, [r0], r1                        @write back
        vst1.64            {d2, d3}, [r0], r1                        @write back
        vst1.64            {d4, d5}, [r0], r1                        @write back
        vst1.64            {d6, d7}, [r0]                            @write back

        
        bx              lr
        ..endfunc        

function _rv40_v_loop_filter_neon_y , export =1
         stmfd sp!,      {r4-r10}
         sub r0, r0, #4   
         pld [r0]   
         pld [r0, r1] @ 
         ldmia r0, {r4, r5} @    /*load data to d0, d1, d2, d3*/
         pld [r0, r1, LSL #1] @ 
         add r0, r1 @ 
         vmov d0, r4, r5 @ 
         ldmia r0, {r4, r5} @ 
         vmovl.u8 q4, d0 @    /* expand data to q4, q5, q6, q7 */
         vmov d1, r4, r5 @ 
         add r0, r1 @ 
         ldmia r0, {r4, r5} @ 
         vmovl.u8 q5, d1 @ 
         vmov d2, r4, r5 @ 
         add r0, r1 @ 
         ldmia r0, {r4, r5} @ 
         vmovl.u8 q6, d2 @ 
         vmov d3, r4, r5 @ 
         vmovl.u8 q7, d3 @ 
         vtrn.32  q4, q6 @      /*transpose, q4([-4], [0]), q5([-3], [1], q6([-2], [2]), q7([-1], [3])  (reserved !!)*/
         vtrn.32  q5, q7 @ 
         ldr        r9, [r2, #16] @     /*r9= beta*/
         vtrn.16  q4, q5 @ 
         vtrn.16  q6, q7 @ 
         
         vsub.s16 d16, d12, d14 @   /*d16: diff_p1p0[i] = ptr[-2*step] - ptr[-1*step] */
         vsub.s16 d17, d11, d9 @     /*d17: diff_q1q0[i] = ptr[ 1*step] - ptr[ 0*step] */
         vpadd.s16 d4, d16, d17 @   /*add sum*/
         ldr        r6, [r2, #20] @     /*r6= beta2*/
         mov      r9, r9, LSL #2 @     /*beta<<=2*/
         vpaddl.s16 d5, d4 @           /*sum_p1p0: d5[0], sum_q1q0:d5[1]*/ 
         vabs.s32 d6, d5 @             /*get absolute value*/
         vmov r4, r5, d6 @             /*r4=sum_p1p0, r5=sum_q1q0*/
         cmp r4, r9 @ 
         ite   lt @ 
         movlt r4, #1 @                 /*r4=filter_p1*/
         movge r4, #0 @         
         cmp r5, r9 @ 
         ite   lt @ 
         movlt r5, #1 @                 /*r5=filter_q1*/
         movge r5, #0 @         
         orrs r6, r5, r4 @ 
         beq 6f @                      /*if(!filter_p1 && !filter_q1) return*/
         vsub.s16 d18, d12, d10 @     /*d18:  diff_p1p2[i] = ptr[-2*step] - ptr[-3*step] */
         vsub.s16 d19, d11, d13 @     /*d19:  diff_q1q2[i] = ptr[ 1*step] - ptr[ 2*step]; */
         vpadd.s16 d6, d18, d19 @   /*add sum*/
         vpaddl.s16 d7, d6 @           /*sum_p1p2: d7[0], sum_q1q2:d7[1]*/ 
         vabs.s32 d4, d7 @             /*get absolute value*/        
         vmov r7, r8, d4 @             /*r7=sum_p1p2, r8=sum_q1q2*/
         cmp r7, r6 @ 
         ite  lt @ 
         movlt r7, #2 @                 /*r7=filter_p2*/
         movge r7, #0 @         
         cmp r8, r6 @ 
         ite  lt @ 
         movlt r8, #2 @                 /*r8=filter_q2*/
         movge r8, #0 @         
         orr r7, r7, r4 @ 
         ldr  r3, [r2, #4] @           /* load */
         ldr  r10, [r2, #8] @ 
         ldr  r6, [r2, #28] @ 
         add     r3, r3, r10 @          /*calculate lims*/
         add     r10, r4, r5 @ 
         add     r10, r10, r3, LSR #1 @         
         orr r8, r8, r5 @ 
         add     r10, r10, #1 @         /*r10: lims (reserved !!)*/
         orr r8, r8, r7 @               
         orrs r8, r8, r6 @               /*r8, flag_strong0 && flag_strong1*/
         beq    4f @ 
        
         ldr    r4, [r2, #12] @          /*start strong filter: r10:lims, r4: alpha*/
         vsub.s16 d17, d9, d14 @     /*d17:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r4 @ 
         vabs.s16 d21, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d21, d0[0] @    
         vshr.u32  q14, q15, #7 @   
         vmov.u16 d29, #0 @        /*d29: 0*/
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vcgt.u16 d26, d17, d29 @ 
         vmov.s16 d31, #1 @ 
         vcgt.u16 d26, d17, d29 @ 
         vcge.s16 d27, d31, d16 @ 
         vand.u16 d17, d26, d27 @   /* d17:t!=0 && sflag<=1 (reserved !!) */

         vmov       r5, r6, d17 @       /*strong filter check*/
         orrs          r7, r5, r6 @ 
         beq          6f @ 
        
         ldr        r5, [r2] @              /*dmode*/
         ldr        r6, [r2, #32] @              /*r6: rv40_dither_l*/
         ldr        r7, [r2, #36] @              /*r6: rv40_dither_r*/
         ldr        r8, [r6, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         ldr        r9, [r7, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         vmov    d31, r8, r9  @ 
         vmovl.u8 q9, d31 @              /*q9: rv40_dither_l[dmode + i] and rv40_dither_r[dmode + i] (reserved !!)*/
         vadd.s16  d24, d12, d14 @     /*src[-2]+ src[-1]*/
         vadd.s16  d23, d9, d11 @      /*src[0]+ src[1]*/
         vadd.s16  d24, d24, d23 @      /* d24:src[0]+ src[1]+src[-2]+src[-1]*/

         vdup.16    d20, r10 @                      /*d20 expand limis (!!reserved)*/
       
         vadd.s16  d0, d24, d10 @     /*cal p0: d3*/
         vshl.u16   d28, d0, #4 @      /*16x*/
         vshl.u16   d29, d0, #3 @      /*8x*/        
         vadd.u16  d29, d28, d29 @      /*24x*/
         vadd.u16  d0, d0, d29 @      /*25x*/
         vadd.u16  d0, d0, d18 @   
         vsub.s16  d0, d0, d11 @ 
         vadd.s16  d0, d24, d0 @    
         vshr.u16   d6, d0, #7 @         /*d6: p0: [-1]*/

         vadd.s16  d7, d24, d13 @      /*cal q0: d4*/
         vshl.u16   d30, d7, #4 @      /*16x*/
         vshl.u16   d31, d7, #3 @      /*8x*/        
         vadd.u16  d31, d30, d31 @      /*24x*/
         vadd.u16  d7, d7, d31 @      /*25x*/
         vadd.u16  d7, d7, d19 @   
         vsub.s16  d7, d7, d12 @ 
         vadd.s16  d7, d24, d6 @  
         vshr.u16   d1, d7, #7 @         /*d1: q0: [0]*/

         vsub.s16   d26, d14, d20 @              /*clip min*/
         vadd.s16   d27, d14, d20 @              /*clip max*/
         vmax.s16  d26, d26, d6 @ 
         vmin.s16  d26, d26, d27 @ 
        
         vsub.s16   d24, d9, d20 @              /*clip min*/
         vadd.s16   d25, d9, d20 @              /*clip max*/
         vmax.s16  d24, d1, d24 @ 
         vmin.s16  d24, d24, d25 @ 
        
         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d22, d26 @             /* merge result */
         vand.u16  d6, d23, d6 @         
         vorr.u16  d6, d6, d26 @                 /*d6: p0: [-1]*/

         vand.u16  d24, d22, d24 @ 
         vand.u16  d1, d23, d1 @         
         vorr.u16  d1, d1, d24 @                /*d1: q0: [0]*/ 

         vadd.s16 d28, d8, d10 @ 
         vadd.s16 d29, d12, d9 @ 
         vadd.s16 d29, d28, d29 @ 
         vadd.s16 d26, d29, d3 @ 
         vshr.u16 d28, d26, #4 @        /*16x*/
         vshr.u16 d29, d26, #3 @        /*8x*/
         vadd.s16 d29, d28, d29 @     /*24x*/        
         vadd.s16 d29, d26, d29 @       /*25x*/
         vadd.s16 d28, d10, d12 @ 
         vadd.s16 d29, d29, d28 @ 
         vadd.s16 d28, d6, d18 @ 
         vadd.s16 d29, d29, d28 @ 
         vshr.u16  d4, d29, #7 @            /*d4: p1 [-2]*/

         vadd.s16 d30, d14, d11 @ 
         vadd.s16 d31, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d27, d31, d4 @ 
         vshr.u16 d30, d27, #4 @        /*16x*/
         vshr.u16 d31, d27, #3 @        /*8x*/
         vadd.s16 d31, d30, d31 @     /*24x*/        
         vadd.s16 d31, d27, d31 @       /*25x*/
         vadd.s16 d30, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d30, d1, d19 @ 
         vadd.s16 d31, d30, d31 @ 
         vshr.u16  d3, d31, #7 @          /* d3: q1 [1]*/

         vsub.s16   d26, d12, d20 @              /*clip min*/
         vadd.s16   d27, d12, d20 @              /*clip max*/
         vmax.s16  d26, d4, d26 @ 
         vmin.s16  d26, d27, d26 @ 
        
         vsub.s16   d24, d11, d20 @              /*clip min*/
         vadd.s16   d25, d11, d20 @              /*clip max*/
         vmax.s16  d24, d3, d24 @ 
         vmin.s16  d24, d24, d25 @ 

         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d26, d22 @             /* merge result */
         vand.u16  d4, d4, d23 @         
         vorr.u16   d4, d26, d4 @                 /*d4: p1 [-2]*/
        
         vand.u16  d24, d24, d22 @ 
         vand.u16  d3, d3, d23 @         
         vorr.u16  d3, d3, d25 @                /* d3: q1 [1]*/
        

         @ldr     r3, [r2, #24] @                      /*chroma*/
         @movs r3, r3 @ 
        
         @beq 1f @ 
         vadd.s16 q14, q4, q5 @ 
         vadd.s16 q15, q6, q7 @   
         vadd.s16 q15, q14, q15 @ 
         vmov.s16 q12, q15 @ 
         vmov.s16 q13, #64 @ 
         vadd.s16 d30, d30, d10 @ 
         vadd.s16 d31, d31, d13 @ 
         vadd.s16 q12, q12, q13 @ 
         vshr.u16 q14, q15, #4 @               /*16x*/
         vshr.u16 q13, q15, #3 @               /*8x*/
         vadd.s16 q15, q15, q14 @            
         vadd.s16 q15, q15, q13 @             /*25x*/
         vsub.s16 d30, d30, d14 @ 
         vsub.s16 d31, d31, d9 @ 
         vadd.s16 d31, d31, d12 @ 
         vshr.u16 d2, d30, #7 @                /*d2: p2, [-3]*/
         vshr.u16 d5, d31, #7 @                /*d5: q2, [2]*/
         @beq 2f @ 
        
         @1:@ 
         @vmov.u16  d2, d10  @ 
         @vmov.u16  d5, d13  @           

         @2:@ 
         vmov.u16  d0, d8  @ 
         vmov.u16  d7, d15  @   

         vmovl.u16 q10, d17 @ 

         vtrn.32  q0, q2 @      /*transpose back, q0([-4], [0]), q1([-3], [1], q2([-2], [2]), q3([-1], [3])  */
         vtrn.32  q1, q3 @ 
         vmov     r4, r5, d20 @ 
         vtrn.16  q0, q1 @ 
         vtrn.16  q2, q3 @ 

         add       r2, r1, r1, LSL #1 @ 
         vmovn.u16  d22, q0 @ 
         vmovn.u16  d23, q1 @ 
         vmovn.u16  d24, q2 @ 
         vmovn.u16  d25, q3 @ 
        
         sub       r0, r0, r2 @        

         vmov     r6, r7, d22 @ 
         movs     r4, r4 @                                   /*write back data*/
         itt    ne @ 
         strne     r7, [r0, #4] @ 
         strne     r6, [r0] @ 
         add       r0, r0, r1 @ 
        
         vmov     r8, r9, d23 @ 
         movs     r5, r5 @ 
          itt    ne @ 
         strne     r9, [r0, #4] @ 
         strne     r8, [r0] @ 
         add       r0, r0, r1 @ 
        
         vmov     r4, r5, d21 @         
         vmov     r6, r7, d24 @ 
         movs     r4, r4 @  
         itt    ne @ 
         strne     r7, [r0, #4] @ 
         strne     r6, [r0] @ 
         add       r0, r0, r1 @ 

         vmov     r8, r9, d25 @ 
         movs     r5, r5 @  
         itt    ne @ 
         strne     r9, [r0, #4] @ 
         strne     r8, [r0] @     

         b 6f @ 
        
         4:@              /*weak-deblock: d16-d19: diff_p1p0[i], diff_q1q0[i], diff_p1p2[i], diff_q1q2[i]*/
         ldr    r3, [r2, #12] @          /*r3: alpha*/

         vsub.s16 d27, d9, d14 @     /*d27:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r3 @ 
         vabs.s16 d25, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d25, d0[0]@    
         vshr.u32  q14, q15, #7 @    
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vmov.u16 d31, #0 @ 
         vcgt.u16 d26, d27, d31 @ 
         mov      r6, #3 @ 
         and       r7, r4, r5 @ 
         sub       r6, r6, r7 @ 
         vdup.16  d31, r6 @ 
         vcgt.u16 d28, d27, d31 @ 
         vcge.s16 d29, d31, d26 @ 
         vand.u16 d20, d28, d29 @   /* d20:t!=0 && sflag<=1 (reserved !!) */

         vmov       r7, r8, d20 @       /*strong filter check*/
         orrs          r7, r8, r7 @ 
         beq          6f @ 

         ands r6, r5, r4 @    /*r4=filter_p1, r5=filter_q1, r10: lims , r9 beta<<2*/
         mov  r9, r9, LSR #2 @      /*r9: beta*/
         ldr    r7, [r2, #4] @          /*r7: lim_q1*/
         ldr    r8, [r2, #8] @          /*r8: lim_p1*/
         ittt     eq @ 
         moveq  r10, r10, LSR #1 @ 
         moveq  r7, r7, LSR #1 @ 
         moveq  r8, r8, LSR #1 @ 

         vshl.u16   d27, d27, #2 @    /*t<<=2*/
         beq 3f @ 
         vsub.s16  d28, d12, d11 @    /*d28: src[-2*step] - src[1*step]*/
         vadd.s16  d27, d27, d28 @ 
         3:@ 
         vmov.s16 d22, #4 @ 
         vadd.s16 d27, d27, d22 @ 
         vshr.u16  d27, #3 @           /*d27: (t + 4) >> 3*/
         vdup.16   d30, r10 @          /*d30: lim_p0q0(lims)*/
         vabs.s16  d31, d30 @         /*max*/
         vneg.s16  d30, d31 @         /*min*/
         vmax.s16  d30, d30, d27 @  
         vmin.s16   d24, d30, d31 @     /*d24: diff=CLIP_SYMM((t + 4) >> 3, lim_p0q0)*/

         vmov.u16  q0, q4 @               /*copy*/
         vmov.u16  q1, q5 @ 
         vmov.u16  q2, q6 @ 
         vmov.u16  q3, q7 @ 
        
         vsub.s16   d1, d8, d24 @        /*d1: src[ 0*step] - diff;*/
         vadd.s16   d6, d14, d24 @      /*d6: src[-1*step] + diff;*/

         vdup.16     d21, r9 @             /*d21: beta*/
         vdup.16     d22, r4 @             /*d22: filter_p1*/
         vdup.16     d23, r5 @             /*d23: filter_q1*/

         vabs.s16   d28, d18 @           /*d28: FFABS(diff_p1p2)*/  
         vabs.s16   d29, d19 @           /*d28: FFABS(diff_q1q2)*/

         vmov.u16   d31, #0 @ 
         vcgt.u16    d22, d22, d31 @     /*condition mask (filter_p1)*/
         vcgt.u16    d23, d23, d31 @     /*condition mask (filter_q1)*/

         vcge.s16    d30, d21, d28 @ 
         vcge.s16    d31, d21, d29 @         
         vand.u16    d22, d22, d30 @    /*condition mask if(FFABS(diff_p1p2) <= beta && filter_p1)*/
         vand.u16    d23, d23, d31 @    /*condition mask if(FFABS(diff_q1q2) <= beta && filter_q1)*/

         vadd.s16    d28, d16, d17 @ 
         vadd.s16    d29, d18, d19 @ 
         vsub.s16    d28, d28, d24 @ 
         vadd.s16    d29, d29, d24 @ 
         vshr.u16     d28, d28, #1 @    /*d28: t = (diff_p1p0 + diff_p1p2 - diff) >> 1;*/
         vshr.u16     d29, d29, #1 @    /*d29: t = (diff_q1q0 + diff_q1q2 + diff) >> 1;*/

         vdup.16     d21, r8 @             /*d21: lim_p1*/
         vdup.16     d25, r7 @             /*d25: lim_q1*/

         vneg.s16    d30, d21 @ 
         vneg.s16    d31, d25 @      

         vmin.s16    d21, d21, d28 @ 
         vmin.s16    d25, d25, d29 @ 
         vmax.s16   d21, d21, d30 @     /*d21: CLIP_SYMM(t, lim_p1)*/
         vmax.s16   d25, d25, d31 @     /*d25: CLIP_SYMM(t, lim_q1)*/

         vsub.s16    d16, d12, d21 @ 
         vsub.s16    d17, d11, d25 @ 

         vbit.u16      d4,  d16, d22 @ 
         vbit.u16      d3,  d17, d23 @     /*insert result according to mask*/

         vmovl.u16 q10, d20 @ 

         vtrn.32  q0, q2 @      /*transpose back, q0([-4], [0]), q1([-3], [1], q2([-2], [2]), q3([-1], [3])  */
         vtrn.32  q1, q3 @ 
         vmov     r4, r5, d20 @ 
         vtrn.16  q0, q1 @ 
         vtrn.16  q2, q3 @ 

         add       r2, r1, r1, LSL #1 @ 
         vmovn.u16  d22, q0 @ 
         vmovn.u16  d23, q1 @ 
         vmovn.u16  d24, q2 @ 
         vmovn.u16  d25, q3 @ 
        
         sub       r0, r0, r2 @ 

         vmov     r6, r7, d22 @ 
         movs     r4, r4 @                                   /*write back data*/
         itt    ne @ 
         strne     r7, [r0, #4] @ 
         strne     r6, [r0] @ 
         add       r0, r0, r1 @ 
        
         vmov     r8, r9, d23 @ 
         movs     r5, r5 @ 
          itt    ne @ 
         strne     r9, [r0, #4] @ 
         strne     r8, [r0] @ 
         add       r0, r0, r1 @ 
        
         vmov     r4, r5, d21 @         
         vmov     r6, r7, d24 @ 
         movs     r4, r4 @  
         itt    ne @ 
         strne     r7, [r0, #4] @ 
         strne     r6, [r0] @ 
         add       r0, r0, r1 @ 

         vmov     r8, r9, d25 @ 
         movs     r5, r5 @  
         itt    ne @ 
         strne     r9, [r0, #4] @ 
         strne     r8, [r0] @    
        
         6:@ 
          ldmfd sp!,      {r4-r10}
            
@        :
@        : r (src),  r (stride),  r (argu) /*,  r (dmode),  r (lim_q1), r (lim_p1), r (alpha), r (beta),  r (beta2),  r (chroma),  r (edge)*/
@        // r0          r1            r2              %3           %4             %5          %6          %7            %8              %9
@        : r3 ,  r4 ,  r5 ,  r6 ,  r7 ,  r8 ,  r9 ,  r10 ,  cc ,  memory 
        bx              lr
        ..endfunc   

function _rv40_v_loop_filter_neon_u , export =1
         stmfd sp!,      {r4-r10}
         sub r0, r0, #8   @
         pld [r0]   @
         pld [r0, r1] @ 
         add      r3, r0, r1, LSL #1 @
         vmov.u16 q8, #0xff @  /*mask*/
         vld1.64  {d8,d9}, [r0]
         pld [r3] @ 
         pld [r3, r1] @
         vorr.u16  q4, q4, q8 @ /*get u from interlave uv*/
         vld1.64  {d10,d11}, [r0, r1] @
         vld1.64  {d12,d13}, [r3] @
         vorr.u16  q5, q5, q8 @ /*get u from interlave uv*/
         vld1.64  {d14, d15}, [r3, r1] @
         vorr.u16  q6, q6, q8 @ /*get u from interlave uv*/
         vorr.u16  q7, q7, q8 @ /*get u from interlave uv*/
         
         vtrn.32  q4, q6 @      /*transpose, q4([-4], [0]), q5([-3], [1], q6([-2], [2]), q7([-1], [3])  (reserved !!)*/
         vtrn.32  q5, q7 @ 
         ldr        r9, [r2, #16] @     /*r9= beta*/
         vtrn.16  q4, q5 @ 
         vtrn.16  q6, q7 @ 
         
         vsub.s16 d16, d12, d14 @   /*d16: diff_p1p0[i] = ptr[-2*step] - ptr[-1*step] */
         vsub.s16 d17, d11, d9 @     /*d17: diff_q1q0[i] = ptr[ 1*step] - ptr[ 0*step] */
         vpadd.s16 d4, d16, d17 @   /*add sum*/
         ldr        r6, [r2, #20] @     /*r6= beta2*/
         mov      r9, r9, LSL #2 @     /*beta<<=2*/
         vpaddl.s16 d5, d4 @           /*sum_p1p0: d5[0], sum_q1q0:d5[1]*/ 
         vabs.s32 d6, d5 @             /*get absolute value*/
         vmov r4, r5, d6 @             /*r4=sum_p1p0, r5=sum_q1q0*/
         cmp r4, r9 @ 
         ite   lt @ 
         movlt r4, #1 @                 /*r4=filter_p1*/
         movge r4, #0 @         
         cmp r5, r9 @ 
         ite   lt @ 
         movlt r5, #1 @                 /*r5=filter_q1*/
         movge r5, #0 @         
         orrs r6, r5, r4 @ 
         beq 6f @                      /*if(!filter_p1 && !filter_q1) return*/
         vsub.s16 d18, d12, d10 @     /*d18:  diff_p1p2[i] = ptr[-2*step] - ptr[-3*step] */
         vsub.s16 d19, d11, d13 @     /*d19:  diff_q1q2[i] = ptr[ 1*step] - ptr[ 2*step]; */
         vpadd.s16 d6, d18, d19 @   /*add sum*/
         vpaddl.s16 d7, d6 @           /*sum_p1p2: d7[0], sum_q1q2:d7[1]*/ 
         vabs.s32 d4, d7 @             /*get absolute value*/        
         vmov r7, r8, d4 @             /*r7=sum_p1p2, r8=sum_q1q2*/
         cmp r7, r6 @ 
         ite  lt @ 
         movlt r7, #2 @                 /*r7=filter_p2*/
         movge r7, #0 @         
         cmp r8, r6 @ 
         ite  lt @ 
         movlt r8, #2 @                 /*r8=filter_q2*/
         movge r8, #0 @         
         orr r7, r7, r4 @ 
         ldr  r3, [r2, #4] @           /* load */
         ldr  r10, [r2, #8] @ 
         ldr  r6, [r2, #28] @ 
         add     r3, r3, r10 @          /*calculate lims*/
         add     r10, r4, r5 @ 
         add     r10, r10, r3, LSR #1 @         
         orr r8, r8, r5 @ 
         add     r10, r10, #1 @         /*r10: lims (reserved !!)*/
         orr r8, r8, r7 @               
         orrs r8, r8, r6 @               /*r8, flag_strong0 && flag_strong1*/
         beq    4f @ 
        
         ldr    r4, [r2, #12] @          /*start strong filter: r10:lims, r4: alpha*/
         vsub.s16 d17, d9, d14 @     /*d17:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r4 @ 
         vabs.s16 d21, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d21, d0[0] @    
         vshr.u32  q14, q15, #7 @   
         vmov.u16 d29, #0 @        /*d29: 0*/
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vcgt.u16 d26, d17, d29 @ 
         vmov.s16 d31, #1 @ 
         vcgt.u16 d26, d17, d29 @ 
         vcge.s16 d27, d31, d16 @ 
         vand.u16 d17, d26, d27 @   /* d17:t!=0 && sflag<=1 (reserved !!) */

         vmov       r5, r6, d17 @       /*strong filter check*/
         orrs          r7, r5, r6 @ 
         beq          6f @ 
        
         ldr        r5, [r2] @              /*dmode*/
         ldr        r6, [r2, #32] @              /*r6: rv40_dither_l*/
         ldr        r7, [r2, #36] @              /*r6: rv40_dither_r*/
         ldr        r8, [r6, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         ldr        r9, [r7, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         vmov    d31, r8, r9  @ 
         vmovl.u8 q9, d31 @              /*q9: rv40_dither_l[dmode + i] and rv40_dither_r[dmode + i] (reserved !!)*/
         vadd.s16  d24, d12, d14 @     /*src[-2]+ src[-1]*/
         vadd.s16  d23, d9, d11 @      /*src[0]+ src[1]*/
         vadd.s16  d24, d24, d23 @      /* d24:src[0]+ src[1]+src[-2]+src[-1]*/

         vdup.16    d20, r10 @                      /*d20 expand limis (!!reserved)*/
       
         vadd.s16  d0, d24, d10 @     /*cal p0: d3*/
         vshl.u16   d28, d0, #4 @      /*16x*/
         vshl.u16   d29, d0, #3 @      /*8x*/        
         vadd.u16  d29, d28, d29 @      /*24x*/
         vadd.u16  d0, d0, d29 @      /*25x*/
         vadd.u16  d0, d0, d18 @   
         vsub.s16  d0, d0, d11 @ 
         vadd.s16  d0, d24, d0 @    
         vshr.u16   d6, d0, #7 @         /*d6: p0: [-1]*/

         vadd.s16  d7, d24, d13 @      /*cal q0: d4*/
         vshl.u16   d30, d7, #4 @      /*16x*/
         vshl.u16   d31, d7, #3 @      /*8x*/        
         vadd.u16  d31, d30, d31 @      /*24x*/
         vadd.u16  d7, d7, d31 @      /*25x*/
         vadd.u16  d7, d7, d19 @   
         vsub.s16  d7, d7, d12 @ 
         vadd.s16  d7, d24, d6 @  
         vshr.u16   d1, d7, #7 @         /*d1: q0: [0]*/

         vsub.s16   d26, d14, d20 @              /*clip min*/
         vadd.s16   d27, d14, d20 @              /*clip max*/
         vmax.s16  d26, d26, d6 @ 
         vmin.s16  d26, d26, d27 @ 
        
         vsub.s16   d24, d9, d20 @              /*clip min*/
         vadd.s16   d25, d9, d20 @              /*clip max*/
         vmax.s16  d24, d1, d24 @ 
         vmin.s16  d24, d24, d25 @ 
        
         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d22, d26 @             /* merge result */
         vand.u16  d6, d23, d6 @         
         vorr.u16  d6, d6, d26 @                 /*d6: p0: [-1]*/

         vand.u16  d24, d22, d24 @ 
         vand.u16  d1, d23, d1 @         
         vorr.u16  d1, d1, d24 @                /*d1: q0: [0]*/ 

         vadd.s16 d28, d8, d10 @ 
         vadd.s16 d29, d12, d9 @ 
         vadd.s16 d29, d28, d29 @ 
         vadd.s16 d26, d29, d3 @ 
         vshr.u16 d28, d26, #4 @        /*16x*/
         vshr.u16 d29, d26, #3 @        /*8x*/
         vadd.s16 d29, d28, d29 @     /*24x*/        
         vadd.s16 d29, d26, d29 @       /*25x*/
         vadd.s16 d28, d10, d12 @ 
         vadd.s16 d29, d29, d28 @ 
         vadd.s16 d28, d6, d18 @ 
         vadd.s16 d29, d29, d28 @ 
         vshr.u16  d4, d29, #7 @            /*d4: p1 [-2]*/

         vadd.s16 d30, d14, d11 @ 
         vadd.s16 d31, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d27, d31, d4 @ 
         vshr.u16 d30, d27, #4 @        /*16x*/
         vshr.u16 d31, d27, #3 @        /*8x*/
         vadd.s16 d31, d30, d31 @     /*24x*/        
         vadd.s16 d31, d27, d31 @       /*25x*/
         vadd.s16 d30, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d30, d1, d19 @ 
         vadd.s16 d31, d30, d31 @ 
         vshr.u16  d3, d31, #7 @          /* d3: q1 [1]*/

         vsub.s16   d26, d12, d20 @              /*clip min*/
         vadd.s16   d27, d12, d20 @              /*clip max*/
         vmax.s16  d26, d4, d26 @ 
         vmin.s16  d26, d27, d26 @ 
        
         vsub.s16   d24, d11, d20 @              /*clip min*/
         vadd.s16   d25, d11, d20 @              /*clip max*/
         vmax.s16  d24, d3, d24 @ 
         vmin.s16  d24, d24, d25 @ 

         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d26, d22 @             /* merge result */
         vand.u16  d4, d4, d23 @         
         vorr.u16   d4, d26, d4 @                 /*d4: p1 [-2]*/
        
         vand.u16  d24, d24, d22 @ 
         vand.u16  d3, d3, d23 @         
         vorr.u16  d3, d3, d25 @                /* d3: q1 [1]*/
        
         #if 0
         ldr     r3, [r2, #24] @                      /*chroma*/
         movs r3, r3 @ 
        
         beq 1f @ 
         vadd.s16 q14, q4, q5 @ 
         vadd.s16 q15, q6, q7 @   
         vadd.s16 q15, q14, q15 @ 
         vmov.s16 q12, q15 @ 
         vmov.s16 q13, #64 @ 
         vadd.s16 d30, d30, d10 @ 
         vadd.s16 d31, d31, d13 @ 
         vadd.s16 q12, q12, q13 @ 
         vshr.u16 q14, q15, #4 @               /*16x*/
         vshr.u16 q13, q15, #3 @               /*8x*/
         vadd.s16 q15, q15, q14 @            
         vadd.s16 q15, q15, q13 @             /*25x*/
         vsub.s16 d30, d30, d14 @ 
         vsub.s16 d31, d31, d9 @ 
         vadd.s16 d31, d31, d12 @ 
         vshr.u16 d2, d30, #7 @                /*d2: p2, [-3]*/
         vshr.u16 d5, d31, #7 @                /*d5: q2, [2]*/
         beq 2f @ 
        
         1:@ 
         #endif
         vmov.u16  d2, d10  @ 
         vmov.u16  d5, d13  @           

         @2:@ 
         vmov.u16  d0, d8  @ 
         vmov.u16  d7, d15  @   

         vmovl.u16 q10, d17 @ 

         vtrn.32  q0, q2 @      /*transpose back, q0([-4], [0]), q1([-3], [1], q2([-2], [2]), q3([-1], [3])  */
         vtrn.32  q1, q3 @ 
         vmov     r4, r5, d20 @ 
         vmov.u16 q15, #0xff @
         add       r3, r0, r1, LSL #1 @
         vtrn.16  q0, q1 @ 
         vtrn.16  q2, q3 @ 

         movs     r4, r4 @ 
         beq  7f @ 
         vld1.64  {d8, d9}, [r0] @
         vbit.u16 q4, q0, q15 @
         vst1.64  {d8, d9}, [r0] @
         7:  @
         movs     r5, r5 @
         vmov     r4, r5, d21 @  
         beq    8f @
         vld1.64  {d10, d11}, [r0, r1] @
         vbit.u16 q5, q1, q15 @
         vst1.64  {d10, d11}, [r0, r1] @
         8:  @
         movs     r4, r4 @ 
         beq    9f @
         vld1.64  {d12, d13}, [r3] @
         vbit.u16 q6, q2, q15 @
         vst1.64  {d12, d13}, [r3] @
         9:  @
         movs     r5, r5 @
         beq    6f @
         vld1.64  {d14, d15}, [r3, r1] @   
         vbit.u16 q7, q3, q15 @
         vst1.64  {d14, d15}, [r3, r1] @
                 
         b 6f @ 
        
         4:@              /*weak-deblock: d16-d19: diff_p1p0[i], diff_q1q0[i], diff_p1p2[i], diff_q1q2[i]*/
         ldr    r3, [r2, #12] @          /*r3: alpha*/

         vsub.s16 d27, d9, d14 @     /*d27:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r3 @ 
         vabs.s16 d25, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d25, d0[0]@    
         vshr.u32  q14, q15, #7 @    
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vmov.u16 d31, #0 @ 
         vcgt.u16 d26, d27, d31 @ 
         mov      r6, #3 @ 
         and       r7, r4, r5 @ 
         sub       r6, r6, r7 @ 
         vdup.16  d31, r6 @ 
         vcgt.u16 d28, d27, d31 @ 
         vcge.s16 d29, d31, d26 @ 
         vand.u16 d20, d28, d29 @   /* d20:t!=0 && sflag<=1 (reserved !!) */

         vmov       r7, r8, d20 @       /*strong filter check*/
         orrs          r7, r8, r7 @ 
         beq          6f @ 

         ands r6, r5, r4 @    /*r4=filter_p1, r5=filter_q1, r10: lims , r9 beta<<2*/
         mov  r9, r9, LSR #2 @      /*r9: beta*/
         ldr    r7, [r2, #4] @          /*r7: lim_q1*/
         ldr    r8, [r2, #8] @          /*r8: lim_p1*/
         ittt     eq @ 
         moveq  r10, r10, LSR #1 @ 
         moveq  r7, r7, LSR #1 @ 
         moveq  r8, r8, LSR #1 @ 

         vshl.u16   d27, d27, #2 @    /*t<<=2*/
         beq 3f @ 
         vsub.s16  d28, d12, d11 @    /*d28: src[-2*step] - src[1*step]*/
         vadd.s16  d27, d27, d28 @ 
         3:@ 
         vmov.s16 d22, #4 @ 
         vadd.s16 d27, d27, d22 @ 
         vshr.u16  d27, #3 @           /*d27: (t + 4) >> 3*/
         vdup.16   d30, r10 @          /*d30: lim_p0q0(lims)*/
         vabs.s16  d31, d30 @         /*max*/
         vneg.s16  d30, d31 @         /*min*/
         vmax.s16  d30, d30, d27 @  
         vmin.s16   d24, d30, d31 @     /*d24: diff=CLIP_SYMM((t + 4) >> 3, lim_p0q0)*/

         vmov.u16  q0, q4 @               /*copy*/
         vmov.u16  q1, q5 @ 
         vmov.u16  q2, q6 @ 
         vmov.u16  q3, q7 @ 
        
         vsub.s16   d1, d8, d24 @        /*d1: src[ 0*step] - diff;*/
         vadd.s16   d6, d14, d24 @      /*d6: src[-1*step] + diff;*/

         vdup.16     d21, r9 @             /*d21: beta*/
         vdup.16     d22, r4 @             /*d22: filter_p1*/
         vdup.16     d23, r5 @             /*d23: filter_q1*/

         vabs.s16   d28, d18 @           /*d28: FFABS(diff_p1p2)*/  
         vabs.s16   d29, d19 @           /*d28: FFABS(diff_q1q2)*/

         vmov.u16   d31, #0 @ 
         vcgt.u16    d22, d22, d31 @     /*condition mask (filter_p1)*/
         vcgt.u16    d23, d23, d31 @     /*condition mask (filter_q1)*/

         vcge.s16    d30, d21, d28 @ 
         vcge.s16    d31, d21, d29 @         
         vand.u16    d22, d22, d30 @    /*condition mask if(FFABS(diff_p1p2) <= beta && filter_p1)*/
         vand.u16    d23, d23, d31 @    /*condition mask if(FFABS(diff_q1q2) <= beta && filter_q1)*/

         vadd.s16    d28, d16, d17 @ 
         vadd.s16    d29, d18, d19 @ 
         vsub.s16    d28, d28, d24 @ 
         vadd.s16    d29, d29, d24 @ 
         vshr.u16     d28, d28, #1 @    /*d28: t = (diff_p1p0 + diff_p1p2 - diff) >> 1;*/
         vshr.u16     d29, d29, #1 @    /*d29: t = (diff_q1q0 + diff_q1q2 + diff) >> 1;*/

         vdup.16     d21, r8 @             /*d21: lim_p1*/
         vdup.16     d25, r7 @             /*d25: lim_q1*/

         vneg.s16    d30, d21 @ 
         vneg.s16    d31, d25 @      

         vmin.s16    d21, d21, d28 @ 
         vmin.s16    d25, d25, d29 @ 
         vmax.s16   d21, d21, d30 @     /*d21: CLIP_SYMM(t, lim_p1)*/
         vmax.s16   d25, d25, d31 @     /*d25: CLIP_SYMM(t, lim_q1)*/

         vsub.s16    d16, d12, d21 @ 
         vsub.s16    d17, d11, d25 @ 

         vbit.u16      d4,  d16, d22 @ 
         vbit.u16      d3,  d17, d23 @     /*insert result according to mask*/

         vmovl.u16 q10, d20 @ 

         vtrn.32  q0, q2 @      /*transpose back, q0([-4], [0]), q1([-3], [1], q2([-2], [2]), q3([-1], [3])  */
         vtrn.32  q1, q3 @ 
         vmov     r4, r5, d20 @ 
         vmov.u16 q15, #0xff @
         add       r3, r0, r1, LSL #1 @
         vtrn.16  q0, q1 @ 
         vtrn.16  q2, q3 @ 

         movs     r4, r4 @ 
         beq  7f @ 
         vld1.64  {d8, d9}, [r0] @
         vbit.u16 q4, q0, q15 @
         vst1.64  {d8, d9}, [r0] @
         7:  @
         movs     r5, r5 @
         vmov     r4, r5, d21 @  
         beq    8f @
         vld1.64  {d10, d11}, [r0, r1] @
         vbit.u16 q5, q1, q15 @
         vst1.64  {d10, d11}, [r0, r1] @
         8:  @
         movs     r4, r4 @ 
         beq    9f @
         vld1.64  {d12, d13}, [r3] @
         vbit.u16 q6, q2, q15 @
         vst1.64  {d12, d13}, [r3] @
         9:  @
         movs     r5, r5 @
         beq    6f @
         vld1.64  {d14, d15}, [r3, r1] @   
         vbit.u16 q7, q3, q15 @
         vst1.64  {d14, d15}, [r3, r1] @
                 
         6:@ 
          ldmfd sp!,      {r4-r10}
            
@        :
@        : r (src),  r (stride),  r (argu) /*,  r (dmode),  r (lim_q1), r (lim_p1), r (alpha), r (beta),  r (beta2),  r (chroma),  r (edge)*/
@        // r0          r1            r2              %3           %4             %5          %6          %7            %8              %9
@        : r3 ,  r4 ,  r5 ,  r6 ,  r7 ,  r8 ,  r9 ,  r10 ,  cc ,  memory 
        bx              lr
        ..endfunc  

function _rv40_v_loop_filter_neon_v , export =1
         stmfd sp!,      {r4-r10}
         sub r0, r0, #8   @
         pld [r0]   @
         pld [r0, r1] @ 
         add      r3, r0, r1, LSL #1 @
         @vmov.u16 q8, #0xff00 @  /*v mask*/
         vld1.64  {d8,d9}, [r0]
         pld [r3] @ 
         pld [r3, r1] @
         @vorr.u16  q4, q4, q8 @ /*get u from interlave uv*/
         vshr.u16 q4, q4, #8 @ 
         vld1.64  {d10,d11}, [r0, r1] @
         
         vld1.64  {d12,d13}, [r3] @
         @vorr.u16  q5, q5, q8 @ /*get u from interlave uv*/
         vshr.u16 q5, q5, #8 @
         vld1.64  {d14, d15}, [r3, r1] @          
         @vorr.u16  q6, q6, q8 @ /*get u from interlave uv*/
         @vorr.u16  q7, q7, q8 @ /*get u from interlave uv*/
         vshr.u16 q6, q6, #8 @ 
         vshr.u16 q7, q7, #8 @ 
         
         vtrn.32  q4, q6 @      /*transpose, q4([-4], [0]), q5([-3], [1], q6([-2], [2]), q7([-1], [3])  (reserved !!)*/
         vtrn.32  q5, q7 @ 
         ldr        r9, [r2, #16] @     /*r9= beta*/
         vtrn.16  q4, q5 @ 
         vtrn.16  q6, q7 @ 
         
         vsub.s16 d16, d12, d14 @   /*d16: diff_p1p0[i] = ptr[-2*step] - ptr[-1*step] */
         vsub.s16 d17, d11, d9 @     /*d17: diff_q1q0[i] = ptr[ 1*step] - ptr[ 0*step] */
         vpadd.s16 d4, d16, d17 @   /*add sum*/
         ldr        r6, [r2, #20] @     /*r6= beta2*/
         mov      r9, r9, LSL #2 @     /*beta<<=2*/
         vpaddl.s16 d5, d4 @           /*sum_p1p0: d5[0], sum_q1q0:d5[1]*/ 
         vabs.s32 d6, d5 @             /*get absolute value*/
         vmov r4, r5, d6 @             /*r4=sum_p1p0, r5=sum_q1q0*/
         cmp r4, r9 @ 
         ite   lt @ 
         movlt r4, #1 @                 /*r4=filter_p1*/
         movge r4, #0 @         
         cmp r5, r9 @ 
         ite   lt @ 
         movlt r5, #1 @                 /*r5=filter_q1*/
         movge r5, #0 @         
         orrs r6, r5, r4 @ 
         beq 6f @                      /*if(!filter_p1 && !filter_q1) return*/
         vsub.s16 d18, d12, d10 @     /*d18:  diff_p1p2[i] = ptr[-2*step] - ptr[-3*step] */
         vsub.s16 d19, d11, d13 @     /*d19:  diff_q1q2[i] = ptr[ 1*step] - ptr[ 2*step]; */
         vpadd.s16 d6, d18, d19 @   /*add sum*/
         vpaddl.s16 d7, d6 @           /*sum_p1p2: d7[0], sum_q1q2:d7[1]*/ 
         vabs.s32 d4, d7 @             /*get absolute value*/        
         vmov r7, r8, d4 @             /*r7=sum_p1p2, r8=sum_q1q2*/
         cmp r7, r6 @ 
         ite  lt @ 
         movlt r7, #2 @                 /*r7=filter_p2*/
         movge r7, #0 @         
         cmp r8, r6 @ 
         ite  lt @ 
         movlt r8, #2 @                 /*r8=filter_q2*/
         movge r8, #0 @         
         orr r7, r7, r4 @ 
         ldr  r3, [r2, #4] @           /* load */
         ldr  r10, [r2, #8] @ 
         ldr  r6, [r2, #28] @ 
         add     r3, r3, r10 @          /*calculate lims*/
         add     r10, r4, r5 @ 
         add     r10, r10, r3, LSR #1 @         
         orr r8, r8, r5 @ 
         add     r10, r10, #1 @         /*r10: lims (reserved !!)*/
         orr r8, r8, r7 @               
         orrs r8, r8, r6 @               /*r8, flag_strong0 && flag_strong1*/
         beq    4f @ 
        
         ldr    r4, [r2, #12] @          /*start strong filter: r10:lims, r4: alpha*/
         vsub.s16 d17, d9, d14 @     /*d17:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r4 @ 
         vabs.s16 d21, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d21, d0[0] @    
         vshr.u32  q14, q15, #7 @   
         vmov.u16 d29, #0 @        /*d29: 0*/
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vcgt.u16 d26, d17, d29 @ 
         vmov.s16 d31, #1 @ 
         vcgt.u16 d26, d17, d29 @ 
         vcge.s16 d27, d31, d16 @ 
         vand.u16 d17, d26, d27 @   /* d17:t!=0 && sflag<=1 (reserved !!) */

         vmov       r5, r6, d17 @       /*strong filter check*/
         orrs          r7, r5, r6 @ 
         beq          6f @ 
        
         ldr        r5, [r2] @              /*dmode*/
         ldr        r6, [r2, #32] @              /*r6: rv40_dither_l*/
         ldr        r7, [r2, #36] @              /*r6: rv40_dither_r*/
         ldr        r8, [r6, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         ldr        r9, [r7, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         vmov    d31, r8, r9  @ 
         vmovl.u8 q9, d31 @              /*q9: rv40_dither_l[dmode + i] and rv40_dither_r[dmode + i] (reserved !!)*/
         vadd.s16  d24, d12, d14 @     /*src[-2]+ src[-1]*/
         vadd.s16  d23, d9, d11 @      /*src[0]+ src[1]*/
         vadd.s16  d24, d24, d23 @      /* d24:src[0]+ src[1]+src[-2]+src[-1]*/

         vdup.16    d20, r10 @                      /*d20 expand limis (!!reserved)*/
       
         vadd.s16  d0, d24, d10 @     /*cal p0: d3*/
         vshl.u16   d28, d0, #4 @      /*16x*/
         vshl.u16   d29, d0, #3 @      /*8x*/        
         vadd.u16  d29, d28, d29 @      /*24x*/
         vadd.u16  d0, d0, d29 @      /*25x*/
         vadd.u16  d0, d0, d18 @   
         vsub.s16  d0, d0, d11 @ 
         vadd.s16  d0, d24, d0 @    
         vshr.u16   d6, d0, #7 @         /*d6: p0: [-1]*/

         vadd.s16  d7, d24, d13 @      /*cal q0: d4*/
         vshl.u16   d30, d7, #4 @      /*16x*/
         vshl.u16   d31, d7, #3 @      /*8x*/        
         vadd.u16  d31, d30, d31 @      /*24x*/
         vadd.u16  d7, d7, d31 @      /*25x*/
         vadd.u16  d7, d7, d19 @   
         vsub.s16  d7, d7, d12 @ 
         vadd.s16  d7, d24, d6 @  
         vshr.u16   d1, d7, #7 @         /*d1: q0: [0]*/

         vsub.s16   d26, d14, d20 @              /*clip min*/
         vadd.s16   d27, d14, d20 @              /*clip max*/
         vmax.s16  d26, d26, d6 @ 
         vmin.s16  d26, d26, d27 @ 
        
         vsub.s16   d24, d9, d20 @              /*clip min*/
         vadd.s16   d25, d9, d20 @              /*clip max*/
         vmax.s16  d24, d1, d24 @ 
         vmin.s16  d24, d24, d25 @ 
        
         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d22, d26 @             /* merge result */
         vand.u16  d6, d23, d6 @         
         vorr.u16  d6, d6, d26 @                 /*d6: p0: [-1]*/

         vand.u16  d24, d22, d24 @ 
         vand.u16  d1, d23, d1 @         
         vorr.u16  d1, d1, d24 @                /*d1: q0: [0]*/ 

         vadd.s16 d28, d8, d10 @ 
         vadd.s16 d29, d12, d9 @ 
         vadd.s16 d29, d28, d29 @ 
         vadd.s16 d26, d29, d3 @ 
         vshr.u16 d28, d26, #4 @        /*16x*/
         vshr.u16 d29, d26, #3 @        /*8x*/
         vadd.s16 d29, d28, d29 @     /*24x*/        
         vadd.s16 d29, d26, d29 @       /*25x*/
         vadd.s16 d28, d10, d12 @ 
         vadd.s16 d29, d29, d28 @ 
         vadd.s16 d28, d6, d18 @ 
         vadd.s16 d29, d29, d28 @ 
         vshr.u16  d4, d29, #7 @            /*d4: p1 [-2]*/

         vadd.s16 d30, d14, d11 @ 
         vadd.s16 d31, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d27, d31, d4 @ 
         vshr.u16 d30, d27, #4 @        /*16x*/
         vshr.u16 d31, d27, #3 @        /*8x*/
         vadd.s16 d31, d30, d31 @     /*24x*/        
         vadd.s16 d31, d27, d31 @       /*25x*/
         vadd.s16 d30, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d30, d1, d19 @ 
         vadd.s16 d31, d30, d31 @ 
         vshr.u16  d3, d31, #7 @          /* d3: q1 [1]*/

         vsub.s16   d26, d12, d20 @              /*clip min*/
         vadd.s16   d27, d12, d20 @              /*clip max*/
         vmax.s16  d26, d4, d26 @ 
         vmin.s16  d26, d27, d26 @ 
        
         vsub.s16   d24, d11, d20 @              /*clip min*/
         vadd.s16   d25, d11, d20 @              /*clip max*/
         vmax.s16  d24, d3, d24 @ 
         vmin.s16  d24, d24, d25 @ 

         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d26, d22 @             /* merge result */
         vand.u16  d4, d4, d23 @         
         vorr.u16   d4, d26, d4 @                 /*d4: p1 [-2]*/
        
         vand.u16  d24, d24, d22 @ 
         vand.u16  d3, d3, d23 @         
         vorr.u16  d3, d3, d25 @                /* d3: q1 [1]*/
        
         #if 0
         ldr     r3, [r2, #24] @                      /*chroma*/
         movs r3, r3 @ 
        
         beq 1f @ 
         vadd.s16 q14, q4, q5 @ 
         vadd.s16 q15, q6, q7 @   
         vadd.s16 q15, q14, q15 @ 
         vmov.s16 q12, q15 @ 
         vmov.s16 q13, #64 @ 
         vadd.s16 d30, d30, d10 @ 
         vadd.s16 d31, d31, d13 @ 
         vadd.s16 q12, q12, q13 @ 
         vshr.u16 q14, q15, #4 @               /*16x*/
         vshr.u16 q13, q15, #3 @               /*8x*/
         vadd.s16 q15, q15, q14 @            
         vadd.s16 q15, q15, q13 @             /*25x*/
         vsub.s16 d30, d30, d14 @ 
         vsub.s16 d31, d31, d9 @ 
         vadd.s16 d31, d31, d12 @ 
         vshr.u16 d2, d30, #7 @                /*d2: p2, [-3]*/
         vshr.u16 d5, d31, #7 @                /*d5: q2, [2]*/
         beq 2f @ 
        
         1:@ 
         #endif
         vmov.u16  d2, d10  @ 
         vmov.u16  d5, d13  @           

         @ 2:@ 
         vmov.u16  d0, d8  @ 
         vmov.u16  d7, d15  @   

         vmovl.u16 q10, d17 @ 

         vtrn.32  q0, q2 @      /*transpose back, q0([-4], [0]), q1([-3], [1], q2([-2], [2]), q3([-1], [3])  */
         vtrn.32  q1, q3 @ 
         vmov     r4, r5, d20 @ 
         vmov.u16 q15, #0xff00 @
         add       r3, r0, r1, LSL #1 @
         vtrn.16  q0, q1 @ 
         vtrn.16  q2, q3 @ 

         movs     r4, r4 @ 
         beq  7f @ 
         vshl.u16 q0, q0, #8 @
         vld1.64  {d8, d9}, [r0] @
         vbit.u16 q4, q0, q15 @
         vst1.64  {d8, d9}, [r0] @
         7:  @
         movs     r5, r5 @
         vmov     r4, r5, d21 @  
         beq    8f @
         vshl.u16 q1, q1, #8 @
         vld1.64  {d10, d11}, [r0, r1] @
         vbit.u16 q5, q1, q15 @
         vst1.64  {d10, d11}, [r0, r1] @
         8:  @
         movs     r4, r4 @ 
         beq    9f @
         vshl.u16 q2, q2, #8 @
         vld1.64  {d12, d13}, [r3] @
         vbit.u16 q6, q2, q15 @
         vst1.64  {d12, d13}, [r3] @
         9:  @
         movs     r5, r5 @
         beq    6f @
         vshl.u16 q3, q3, #8 @
         vld1.64  {d14, d15}, [r3, r1] @   
         vbit.u16 q7, q3, q15 @
         vst1.64  {d14, d15}, [r3, r1] @
                 
         b 6f @ 
        
         4:@              /*weak-deblock: d16-d19: diff_p1p0[i], diff_q1q0[i], diff_p1p2[i], diff_q1q2[i]*/
         ldr    r3, [r2, #12] @          /*r3: alpha*/

         vsub.s16 d27, d9, d14 @     /*d27:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r3 @ 
         vabs.s16 d25, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d25, d0[0]@    
         vshr.u32  q14, q15, #7 @    
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vmov.u16 d31, #0 @ 
         vcgt.u16 d26, d27, d31 @ 
         mov      r6, #3 @ 
         and       r7, r4, r5 @ 
         sub       r6, r6, r7 @ 
         vdup.16  d31, r6 @ 
         vcgt.u16 d28, d27, d31 @ 
         vcge.s16 d29, d31, d26 @ 
         vand.u16 d20, d28, d29 @   /* d20:t!=0 && sflag<=1 (reserved !!) */

         vmov       r7, r8, d20 @       /*strong filter check*/
         orrs          r7, r8, r7 @ 
         beq          6f @ 

         ands r6, r5, r4 @    /*r4=filter_p1, r5=filter_q1, r10: lims , r9 beta<<2*/
         mov  r9, r9, LSR #2 @      /*r9: beta*/
         ldr    r7, [r2, #4] @          /*r7: lim_q1*/
         ldr    r8, [r2, #8] @          /*r8: lim_p1*/
         ittt     eq @ 
         moveq  r10, r10, LSR #1 @ 
         moveq  r7, r7, LSR #1 @ 
         moveq  r8, r8, LSR #1 @ 

         vshl.u16   d27, d27, #2 @    /*t<<=2*/
         beq 3f @ 
         vsub.s16  d28, d12, d11 @    /*d28: src[-2*step] - src[1*step]*/
         vadd.s16  d27, d27, d28 @ 
         3:@ 
         vmov.s16 d22, #4 @ 
         vadd.s16 d27, d27, d22 @ 
         vshr.u16  d27, #3 @           /*d27: (t + 4) >> 3*/
         vdup.16   d30, r10 @          /*d30: lim_p0q0(lims)*/
         vabs.s16  d31, d30 @         /*max*/
         vneg.s16  d30, d31 @         /*min*/
         vmax.s16  d30, d30, d27 @  
         vmin.s16   d24, d30, d31 @     /*d24: diff=CLIP_SYMM((t + 4) >> 3, lim_p0q0)*/

         vmov.u16  q0, q4 @               /*copy*/
         vmov.u16  q1, q5 @ 
         vmov.u16  q2, q6 @ 
         vmov.u16  q3, q7 @ 
        
         vsub.s16   d1, d8, d24 @        /*d1: src[ 0*step] - diff;*/
         vadd.s16   d6, d14, d24 @      /*d6: src[-1*step] + diff;*/

         vdup.16     d21, r9 @             /*d21: beta*/
         vdup.16     d22, r4 @             /*d22: filter_p1*/
         vdup.16     d23, r5 @             /*d23: filter_q1*/

         vabs.s16   d28, d18 @           /*d28: FFABS(diff_p1p2)*/  
         vabs.s16   d29, d19 @           /*d28: FFABS(diff_q1q2)*/

         vmov.u16   d31, #0 @ 
         vcgt.u16    d22, d22, d31 @     /*condition mask (filter_p1)*/
         vcgt.u16    d23, d23, d31 @     /*condition mask (filter_q1)*/

         vcge.s16    d30, d21, d28 @ 
         vcge.s16    d31, d21, d29 @         
         vand.u16    d22, d22, d30 @    /*condition mask if(FFABS(diff_p1p2) <= beta && filter_p1)*/
         vand.u16    d23, d23, d31 @    /*condition mask if(FFABS(diff_q1q2) <= beta && filter_q1)*/

         vadd.s16    d28, d16, d17 @ 
         vadd.s16    d29, d18, d19 @ 
         vsub.s16    d28, d28, d24 @ 
         vadd.s16    d29, d29, d24 @ 
         vshr.u16     d28, d28, #1 @    /*d28: t = (diff_p1p0 + diff_p1p2 - diff) >> 1;*/
         vshr.u16     d29, d29, #1 @    /*d29: t = (diff_q1q0 + diff_q1q2 + diff) >> 1;*/

         vdup.16     d21, r8 @             /*d21: lim_p1*/
         vdup.16     d25, r7 @             /*d25: lim_q1*/

         vneg.s16    d30, d21 @ 
         vneg.s16    d31, d25 @      

         vmin.s16    d21, d21, d28 @ 
         vmin.s16    d25, d25, d29 @ 
         vmax.s16   d21, d21, d30 @     /*d21: CLIP_SYMM(t, lim_p1)*/
         vmax.s16   d25, d25, d31 @     /*d25: CLIP_SYMM(t, lim_q1)*/

         vsub.s16    d16, d12, d21 @ 
         vsub.s16    d17, d11, d25 @ 

         vbit.u16      d4,  d16, d22 @ 
         vbit.u16      d3,  d17, d23 @     /*insert result according to mask*/

         vmovl.u16 q10, d20 @ 

         vtrn.32  q0, q2 @      /*transpose back, q0([-4], [0]), q1([-3], [1], q2([-2], [2]), q3([-1], [3])  */
         vtrn.32  q1, q3 @ 
         vmov     r4, r5, d20 @ 
         vmov.u16 q15, #0xff00 @
         add       r3, r0, r1, LSL #1 @
         vtrn.16  q0, q1 @ 
         vtrn.16  q2, q3 @ 

         movs     r4, r4 @ 
         beq  7f @ 
         vshl.u16 q0, q0, #8 @
         vld1.64  {d8, d9}, [r0] @
         vbit.u16 q4, q0, q15 @
         vst1.64  {d8, d9}, [r0] @
         7:  @
         movs     r5, r5 @
         vmov     r4, r5, d21 @  
         beq    8f @
         vshl.u16 q1, q1, #8 @
         vld1.64  {d10, d11}, [r0, r1] @
         vbit.u16 q5, q1, q15 @
         vst1.64  {d10, d11}, [r0, r1] @
         8:  @
         movs     r4, r4 @ 
         beq    9f @
         vshl.u16 q2, q2, #8 @
         vld1.64  {d12, d13}, [r3] @
         vbit.u16 q6, q2, q15 @
         vst1.64  {d12, d13}, [r3] @
         9:  @
         movs     r5, r5 @
         beq    6f @
         vshl.u16 q3, q3, #8 @
         vld1.64  {d14, d15}, [r3, r1] @   
         vbit.u16 q7, q3, q15 @
         vst1.64  {d14, d15}, [r3, r1] @
                 
         6:@ 
          ldmfd sp!,      {r4-r10}
            
@        :
@        : r (src),  r (stride),  r (argu) /*,  r (dmode),  r (lim_q1), r (lim_p1), r (alpha), r (beta),  r (beta2),  r (chroma),  r (edge)*/
@        // r0          r1            r2              %3           %4             %5          %6          %7            %8              %9
@        : r3 ,  r4 ,  r5 ,  r6 ,  r7 ,  r8 ,  r9 ,  r10 ,  cc ,  memory 
        bx              lr
        ..endfunc  

function _rv40_h_loop_filter_neon_y , export =1
         stmfd sp!,      {r4-r10}
         sub r3, r0, r1, LSL #2   
         pld [r3] 
         pld [r0]
         pld [r3, r1] @   
         pld [r0, r1] @
         ldr r4, [r3]
         ldr r5, [r0]
         pld [r3, r1, LSL #1] @ 
         pld [r0, r1, LSL #1] @ 
         ldr r6, [r3, r1]
         add r10, r1, r1, LSL #1
         pld [r3, r10]
         pld [r0, r10]
         ldr r7, [r0, r1]
         vmov d0, r4, r5
         vmov d1, r6, r7
         ldr r4, [r3, r1, LSL #1]
         ldr r5, [r0, r1, LSL #1]
         ldr r6, [r3, r10]
         ldr r7, [r0, r10]  
         vmovl.u8 q4, d0
         vmov d2, r4, r5
         vmov d3, r6, r7
         vmovl.u8 q5, d1
         vmovl.u8 q6, d2
         vmovl.u8 q7, d3
         
         ldr        r9, [r2, #16] @     /*r9= beta*/
         
         vsub.s16 d16, d12, d14 @   /*d16: diff_p1p0[i] = ptr[-2*step] - ptr[-1*step] */
         vsub.s16 d17, d11, d9 @     /*d17: diff_q1q0[i] = ptr[ 1*step] - ptr[ 0*step] */
         vpadd.s16 d4, d16, d17 @   /*add sum*/
         ldr        r6, [r2, #20] @     /*r6= beta2*/
         mov      r9, r9, LSL #2 @     /*beta<<=2*/
         vpaddl.s16 d5, d4 @           /*sum_p1p0: d5[0], sum_q1q0:d5[1]*/ 
         vabs.s32 d6, d5 @             /*get absolute value*/
         vmov r4, r5, d6 @             /*r4=sum_p1p0, r5=sum_q1q0*/
         cmp r4, r9 @ 
         ite   lt @ 
         movlt r4, #1 @                 /*r4=filter_p1*/
         movge r4, #0 @         
         cmp r5, r9 @ 
         ite   lt @ 
         movlt r5, #1 @                 /*r5=filter_q1*/
         movge r5, #0 @         
         orrs r6, r5, r4 @ 
         beq 6f @                      /*if(!filter_p1 && !filter_q1) return*/
         vsub.s16 d18, d12, d10 @     /*d18:  diff_p1p2[i] = ptr[-2*step] - ptr[-3*step] */
         vsub.s16 d19, d11, d13 @     /*d19:  diff_q1q2[i] = ptr[ 1*step] - ptr[ 2*step]; */
         vpadd.s16 d6, d18, d19 @   /*add sum*/
         vpaddl.s16 d7, d6 @           /*sum_p1p2: d7[0], sum_q1q2:d7[1]*/ 
         vabs.s32 d4, d7 @             /*get absolute value*/        
         vmov r7, r8, d4 @             /*r7=sum_p1p2, r8=sum_q1q2*/
         cmp r7, r6 @ 
         ite  lt @ 
         movlt r7, #2 @                 /*r7=filter_p2*/
         movge r7, #0 @         
         cmp r8, r6 @ 
         ite  lt @ 
         movlt r8, #2 @                 /*r8=filter_q2*/
         movge r8, #0 @         
         orr r7, r7, r4 @ 
         ldr  r3, [r2, #4] @           /* load */
         ldr  r10, [r2, #8] @ 
         ldr  r6, [r2, #28] @ 
         add     r3, r3, r10 @          /*calculate lims*/
         add     r10, r4, r5 @ 
         add     r10, r10, r3, LSR #1 @         
         orr r8, r8, r5 @ 
         add     r10, r10, #1 @         /*r10: lims (reserved !!)*/
         orr r8, r8, r7 @               
         orrs r8, r8, r6 @               /*r8, flag_strong0 && flag_strong1*/
         beq    4f @ 
        
         ldr    r4, [r2, #12] @          /*start strong filter: r10:lims, r4: alpha*/
         vsub.s16 d17, d9, d14 @     /*d17:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r4 @ 
         vabs.s16 d21, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d21, d0[0] @    
         vshr.u32  q14, q15, #7 @   
         vmov.u16 d29, #0 @        /*d29: 0*/
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vcgt.u16 d26, d17, d29 @ 
         vmov.s16 d31, #1 @ 
         vcgt.u16 d26, d17, d29 @ 
         vcge.s16 d27, d31, d16 @ 
         vand.u16 d17, d26, d27 @   /* d17:t!=0 && sflag<=1 (reserved !!) */

         vmov       r5, r6, d17 @       /*strong filter check*/
         orrs          r7, r5, r6 @ 
         beq          6f @ 
        
         ldr        r5, [r2] @              /*dmode*/
         ldr        r6, [r2, #32] @              /*r6: rv40_dither_l*/
         ldr        r7, [r2, #36] @              /*r6: rv40_dither_r*/
         ldr        r8, [r6, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         ldr        r9, [r7, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         vmov    d31, r8, r9  @ 
         vmovl.u8 q9, d31 @              /*q9: rv40_dither_l[dmode + i] and rv40_dither_r[dmode + i] (reserved !!)*/
         vadd.s16  d24, d12, d14 @     /*src[-2]+ src[-1]*/
         vadd.s16  d23, d9, d11 @      /*src[0]+ src[1]*/
         vadd.s16  d24, d24, d23 @      /* d24:src[0]+ src[1]+src[-2]+src[-1]*/

         vdup.16    d20, r10 @                      /*d20 expand limis (!!reserved)*/
       
         vadd.s16  d0, d24, d10 @     /*cal p0: d3*/
         vshl.u16   d28, d0, #4 @      /*16x*/
         vshl.u16   d29, d0, #3 @      /*8x*/        
         vadd.u16  d29, d28, d29 @      /*24x*/
         vadd.u16  d0, d0, d29 @      /*25x*/
         vadd.u16  d0, d0, d18 @   
         vsub.s16  d0, d0, d11 @ 
         vadd.s16  d0, d24, d0 @    
         vshr.u16   d6, d0, #7 @         /*d6: p0: [-1]*/

         vadd.s16  d7, d24, d13 @      /*cal q0: d4*/
         vshl.u16   d30, d7, #4 @      /*16x*/
         vshl.u16   d31, d7, #3 @      /*8x*/        
         vadd.u16  d31, d30, d31 @      /*24x*/
         vadd.u16  d7, d7, d31 @      /*25x*/
         vadd.u16  d7, d7, d19 @   
         vsub.s16  d7, d7, d12 @ 
         vadd.s16  d7, d24, d6 @  
         vshr.u16   d1, d7, #7 @         /*d1: q0: [0]*/

         vsub.s16   d26, d14, d20 @              /*clip min*/
         vadd.s16   d27, d14, d20 @              /*clip max*/
         vmax.s16  d26, d26, d6 @ 
         vmin.s16  d26, d26, d27 @ 
        
         vsub.s16   d24, d9, d20 @              /*clip min*/
         vadd.s16   d25, d9, d20 @              /*clip max*/
         vmax.s16  d24, d1, d24 @ 
         vmin.s16  d24, d24, d25 @ 
        
         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d22, d26 @             /* merge result */
         vand.u16  d6, d23, d6 @         
         vorr.u16  d6, d6, d26 @                 /*d6: p0: [-1]*/

         vand.u16  d24, d22, d24 @ 
         vand.u16  d1, d23, d1 @         
         vorr.u16  d1, d1, d24 @                /*d1: q0: [0]*/ 

         vadd.s16 d28, d8, d10 @ 
         vadd.s16 d29, d12, d9 @ 
         vadd.s16 d29, d28, d29 @ 
         vadd.s16 d26, d29, d3 @ 
         vshr.u16 d28, d26, #4 @        /*16x*/
         vshr.u16 d29, d26, #3 @        /*8x*/
         vadd.s16 d29, d28, d29 @     /*24x*/        
         vadd.s16 d29, d26, d29 @       /*25x*/
         vadd.s16 d28, d10, d12 @ 
         vadd.s16 d29, d29, d28 @ 
         vadd.s16 d28, d6, d18 @ 
         vadd.s16 d29, d29, d28 @ 
         vshr.u16  d4, d29, #7 @            /*d4: p1 [-2]*/

         vadd.s16 d30, d14, d11 @ 
         vadd.s16 d31, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d27, d31, d4 @ 
         vshr.u16 d30, d27, #4 @        /*16x*/
         vshr.u16 d31, d27, #3 @        /*8x*/
         vadd.s16 d31, d30, d31 @     /*24x*/        
         vadd.s16 d31, d27, d31 @       /*25x*/
         vadd.s16 d30, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d30, d1, d19 @ 
         vadd.s16 d31, d30, d31 @ 
         vshr.u16  d3, d31, #7 @          /* d3: q1 [1]*/

         vsub.s16   d26, d12, d20 @              /*clip min*/
         vadd.s16   d27, d12, d20 @              /*clip max*/
         vmax.s16  d26, d4, d26 @ 
         vmin.s16  d26, d27, d26 @ 
        
         vsub.s16   d24, d11, d20 @              /*clip min*/
         vadd.s16   d25, d11, d20 @              /*clip max*/
         vmax.s16  d24, d3, d24 @ 
         vmin.s16  d24, d24, d25 @ 

         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d26, d22 @             /* merge result */
         vand.u16  d4, d4, d23 @         
         vorr.u16   d4, d26, d4 @                 /*d4: p1 [-2]*/
        
         vand.u16  d24, d24, d22 @ 
         vand.u16  d3, d3, d23 @         
         vorr.u16  d3, d3, d25 @                /* d3: q1 [1]*/
        

         @ldr     r3, [r2, #24] @                      /*chroma*/
         @movs r3, r3 @ 
        
         @beq 1f @ 
         vadd.s16 q14, q4, q5 @ 
         vadd.s16 q15, q6, q7 @   
         vadd.s16 q15, q14, q15 @ 
         vmov.s16 q12, q15 @ 
         vmov.s16 q13, #64 @ 
         vadd.s16 d30, d30, d10 @ 
         vadd.s16 d31, d31, d13 @ 
         vadd.s16 q12, q12, q13 @ 
         vshr.u16 q14, q15, #4 @               /*16x*/
         vshr.u16 q13, q15, #3 @               /*8x*/
         vadd.s16 q15, q15, q14 @            
         vadd.s16 q15, q15, q13 @             /*25x*/
         vsub.s16 d30, d30, d14 @ 
         vsub.s16 d31, d31, d9 @ 
         vadd.s16 d31, d31, d12 @ 
         vshr.u16 d2, d30, #7 @                /*d2: p2, [-3]*/
         vshr.u16 d5, d31, #7 @                /*d5: q2, [2]*/
         @beq 2f @ 
        
         @1:@ 
         @vmov.u16  d2, d10  @ 
         @vmov.u16  d5, d13  @           

         @2:@ 
         vmov.u16  d0, d8  @ 
         vmov.u16  d7, d15  @   

         sub   r3, r0, r1, LSL #2 @
         add   r10, r1, r1, LSL #1 @

         vmov     d16, d17 @
         vmovn.u16 d26, q8 @mask: d17, d26 

         ldr  r4,  [r3] @
         ldr  r5,  [r0] @
         vmovn.u16  d22, q0 @
         vmovn.u16  d23, q1 @ 
         ldr  r6,  [r3, r1] @
         ldr  r7,  [r0, r1] @

         vmov      d8, r4, r5  @
         vmov      d9, r6, r7  @
         vbit.u8    d8, d22, d26 @
         vbit.u8    d9, d23, d26 @
         vmov      r4, r5, d8  @
         vmov      r6, r7, d9  @
         str         r4, [r3] @
         str         r5, [r0] @
         str          r6, [r3, r1] @
         str          r7, [r0, r1] @

         ldr  r4,  [r3, r1, LSL #1] @
         ldr  r5,  [r0, r1, LSL #1] @
         vmovn.u16  d24, q2 @
         vmovn.u16  d25, q3 @ 
         ldr  r6,  [r3, r10] @
         ldr  r7,  [r0, r10] @

         vmov      d8, r4, r5  @
         vmov      d9, r6, r7  @
         vbit.u8    d8, d24, d26 @
         vbit.u8    d9, d25, d26 @
         vmov      r4, r5, d8  @
         vmov      r6, r7, d9  @
         str         r4, [r3, r1, LSL #1] @
         str         r5, [r0, r1, LSL #1] @
         str          r6, [r3, r10] @
         str          r7, [r0, r10] @        

         b 6f @ 
        
         4:@              /*weak-deblock: d16-d19: diff_p1p0[i], diff_q1q0[i], diff_p1p2[i], diff_q1q2[i]*/
         ldr    r3, [r2, #12] @          /*r3: alpha*/

         vsub.s16 d27, d9, d14 @     /*d27:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r3 @ 
         vabs.s16 d25, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d25, d0[0]@    
         vshr.u32  q14, q15, #7 @    
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vmov.u16 d31, #0 @ 
         vcgt.u16 d26, d27, d31 @ 
         mov      r6, #3 @ 
         and       r7, r4, r5 @ 
         sub       r6, r6, r7 @ 
         vdup.16  d31, r6 @ 
         vcgt.u16 d28, d27, d31 @ 
         vcge.s16 d29, d31, d26 @ 
         vand.u16 d20, d28, d29 @   /* d20:t!=0 && sflag<=1 (reserved !!) */

         vmov       r7, r8, d20 @       /*strong filter check*/
         orrs          r7, r8, r7 @ 
         beq          6f @ 

         ands r6, r5, r4 @    /*r4=filter_p1, r5=filter_q1, r10: lims , r9 beta<<2*/
         mov  r9, r9, LSR #2 @      /*r9: beta*/
         ldr    r7, [r2, #4] @          /*r7: lim_q1*/
         ldr    r8, [r2, #8] @          /*r8: lim_p1*/
         ittt     eq @ 
         moveq  r10, r10, LSR #1 @ 
         moveq  r7, r7, LSR #1 @ 
         moveq  r8, r8, LSR #1 @ 

         vshl.u16   d27, d27, #2 @    /*t<<=2*/
         beq 3f @ 
         vsub.s16  d28, d12, d11 @    /*d28: src[-2*step] - src[1*step]*/
         vadd.s16  d27, d27, d28 @ 
         3:@ 
         vmov.s16 d22, #4 @ 
         vadd.s16 d27, d27, d22 @ 
         vshr.u16  d27, #3 @           /*d27: (t + 4) >> 3*/
         vdup.16   d30, r10 @          /*d30: lim_p0q0(lims)*/
         vabs.s16  d31, d30 @         /*max*/
         vneg.s16  d30, d31 @         /*min*/
         vmax.s16  d30, d30, d27 @  
         vmin.s16   d24, d30, d31 @     /*d24: diff=CLIP_SYMM((t + 4) >> 3, lim_p0q0)*/

         vmov.u16  q0, q4 @               /*copy*/
         vmov.u16  q1, q5 @ 
         vmov.u16  q2, q6 @ 
         vmov.u16  q3, q7 @ 
        
         vsub.s16   d1, d8, d24 @        /*d1: src[ 0*step] - diff;*/
         vadd.s16   d6, d14, d24 @      /*d6: src[-1*step] + diff;*/

         vdup.16     d21, r9 @             /*d21: beta*/
         vdup.16     d22, r4 @             /*d22: filter_p1*/
         vdup.16     d23, r5 @             /*d23: filter_q1*/

         vabs.s16   d28, d18 @           /*d28: FFABS(diff_p1p2)*/  
         vabs.s16   d29, d19 @           /*d28: FFABS(diff_q1q2)*/

         vmov.u16   d31, #0 @ 
         vcgt.u16    d22, d22, d31 @     /*condition mask (filter_p1)*/
         vcgt.u16    d23, d23, d31 @     /*condition mask (filter_q1)*/

         vcge.s16    d30, d21, d28 @ 
         vcge.s16    d31, d21, d29 @         
         vand.u16    d22, d22, d30 @    /*condition mask if(FFABS(diff_p1p2) <= beta && filter_p1)*/
         vand.u16    d23, d23, d31 @    /*condition mask if(FFABS(diff_q1q2) <= beta && filter_q1)*/

         vadd.s16    d28, d16, d17 @ 
         vadd.s16    d29, d18, d19 @ 
         vsub.s16    d28, d28, d24 @ 
         vadd.s16    d29, d29, d24 @ 
         vshr.u16     d28, d28, #1 @    /*d28: t = (diff_p1p0 + diff_p1p2 - diff) >> 1;*/
         vshr.u16     d29, d29, #1 @    /*d29: t = (diff_q1q0 + diff_q1q2 + diff) >> 1;*/

         vdup.16     d21, r8 @             /*d21: lim_p1*/
         vdup.16     d25, r7 @             /*d25: lim_q1*/

         vneg.s16    d30, d21 @ 
         vneg.s16    d31, d25 @      

         vmin.s16    d21, d21, d28 @ 
         vmin.s16    d25, d25, d29 @ 
         vmax.s16   d21, d21, d30 @     /*d21: CLIP_SYMM(t, lim_p1)*/
         vmax.s16   d25, d25, d31 @     /*d25: CLIP_SYMM(t, lim_q1)*/

         vsub.s16    d16, d12, d21 @ 
         vsub.s16    d17, d11, d25 @ 

         vbit.u16      d4,  d16, d22 @ 
         vbit.u16      d3,  d17, d23 @     /*insert result according to mask*/

         vmov     d16, d17 @
         vmovn.u16 d26, q8 @mask: d17, d26 
         vtrn.32  q0, q2 @      /*transpose back, q0([-4], [0]), q1([-3], [1], q2([-2], [2]), q3([-1], [3])  */
         vtrn.32  q1, q3 @ 
         vmov     r4, r5, d20 @ 
         vtrn.16  q0, q1 @ 
         vtrn.16  q2, q3 @ 
         add       r10, r1, r1, LSL #1 @ 
         
         vmovn.u16  d22, q0 @ 
         vmovn.u16  d23, q1 @ 
         vmovn.u16  d24, q2 @ 
         vmovn.u16  d25, q3 @ 
        
         sub   r3, r0, r1, LSL #2 @
         add   r10, r1, r1, LSL #1 @

         vmov     d16, d17 @
         vmovn.u16 d26, q8 @mask: d17, d26 

         ldr  r4,  [r3] @
         ldr  r5,  [r0] @
         vmovn.u16  d22, q0 @
         vmovn.u16  d23, q1 @ 
         ldr  r6,  [r3, r1] @
         ldr  r7,  [r0, r1] @

         vmov      d8, r4, r5  @
         vmov      d9, r6, r7  @
         vbit.u8    d8, d22, d26 @
         vbit.u8    d9, d23, d26 @
         vmov      r4, r5, d8  @
         vmov      r6, r7, d9  @
         str         r4, [r3] @
         str         r5, [r0] @
         str          r6, [r3, r1] @
         str          r7, [r0, r1] @

         ldr  r4,  [r3, r1, LSL #1] @
         ldr  r5,  [r0, r1, LSL #1] @
         vmovn.u16  d24, q2 @
         vmovn.u16  d25, q3 @ 
         ldr  r6,  [r3, r10] @
         ldr  r7,  [r0, r10] @

         vmov      d8, r4, r5  @
         vmov      d9, r6, r7  @
         vbit.u8    d8, d24, d26 @
         vbit.u8    d9, d25, d26 @
         vmov      r4, r5, d8  @
         vmov      r6, r7, d9  @
         str         r4, [r3, r1, LSL #1] @
         str         r5, [r0, r1, LSL #1] @
         str          r6, [r3, r10] @
         str          r7, [r0, r10] @   
        
         6:@ 
          ldmfd sp!,      {r4-r10}
            
@        :
@        : r (src),  r (stride),  r (argu) /*,  r (dmode),  r (lim_q1), r (lim_p1), r (alpha), r (beta),  r (beta2),  r (chroma),  r (edge)*/
@        // r0          r1            r2              %3           %4             %5          %6          %7            %8              %9
@        : r3 ,  r4 ,  r5 ,  r6 ,  r7 ,  r8 ,  r9 ,  r10 ,  cc ,  memory 
        bx              lr
        ..endfunc   

function _rv40_h_loop_filter_neon_u , export =1
         sub r3, r0, r1, LSL #2   
         pld [r3] 
         pld [r0]
         pld [r3, r1] @   
         pld [r0, r1] @
         vld1.64 d8, [r3] @
         
         stmfd sp!,      {r4-r10}
         vld1.64 d9, [r0] @
         pld [r3, r1, LSL #1] @ 
         pld [r0, r1, LSL #1] @ 
         vld1.64 d10, [r3, r1] @
         add r10, r1, r1, LSL #1 @
         pld [r3, r10] @
         pld [r0, r10] @
         vld1.64 d11, [r0, r2] @
         vmov.u16 q15, #0x00ff @
         vorr.u16  q4, q4, q15 @
         vld1.64 d12, [r3, r1, LSL #1] @
         vorr.u16  q5, q5, q15 @
         vld1.64 d13, [r0, r1, LSL #1] @
         vld1.64 d14, [r3, r10] @
         vorr.u16  q6, q6, q15 @
         vld1.64 d15, [r0, r10] @ 
         vorr.u16  q7, q7, q15 @
        
         ldr        r9, [r2, #16] @     /*r9= beta*/
         
         vsub.s16 d16, d12, d14 @   /*d16: diff_p1p0[i] = ptr[-2*step] - ptr[-1*step] */
         vsub.s16 d17, d11, d9 @     /*d17: diff_q1q0[i] = ptr[ 1*step] - ptr[ 0*step] */
         vpadd.s16 d4, d16, d17 @   /*add sum*/
         ldr        r6, [r2, #20] @     /*r6= beta2*/
         mov      r9, r9, LSL #2 @     /*beta<<=2*/
         vpaddl.s16 d5, d4 @           /*sum_p1p0: d5[0], sum_q1q0:d5[1]*/ 
         vabs.s32 d6, d5 @             /*get absolute value*/
         vmov r4, r5, d6 @             /*r4=sum_p1p0, r5=sum_q1q0*/
         cmp r4, r9 @ 
         ite   lt @ 
         movlt r4, #1 @                 /*r4=filter_p1*/
         movge r4, #0 @         
         cmp r5, r9 @ 
         ite   lt @ 
         movlt r5, #1 @                 /*r5=filter_q1*/
         movge r5, #0 @         
         orrs r6, r5, r4 @ 
         beq 6f @                      /*if(!filter_p1 && !filter_q1) return*/
         vsub.s16 d18, d12, d10 @     /*d18:  diff_p1p2[i] = ptr[-2*step] - ptr[-3*step] */
         vsub.s16 d19, d11, d13 @     /*d19:  diff_q1q2[i] = ptr[ 1*step] - ptr[ 2*step]; */
         vpadd.s16 d6, d18, d19 @   /*add sum*/
         vpaddl.s16 d7, d6 @           /*sum_p1p2: d7[0], sum_q1q2:d7[1]*/ 
         vabs.s32 d4, d7 @             /*get absolute value*/        
         vmov r7, r8, d4 @             /*r7=sum_p1p2, r8=sum_q1q2*/
         cmp r7, r6 @ 
         ite  lt @ 
         movlt r7, #2 @                 /*r7=filter_p2*/
         movge r7, #0 @         
         cmp r8, r6 @ 
         ite  lt @ 
         movlt r8, #2 @                 /*r8=filter_q2*/
         movge r8, #0 @         
         orr r7, r7, r4 @ 
         ldr  r3, [r2, #4] @           /* load */
         ldr  r10, [r2, #8] @ 
         ldr  r6, [r2, #28] @ 
         add     r3, r3, r10 @          /*calculate lims*/
         add     r10, r4, r5 @ 
         add     r10, r10, r3, LSR #1 @         
         orr r8, r8, r5 @ 
         add     r10, r10, #1 @         /*r10: lims (reserved !!)*/
         orr r8, r8, r7 @               
         orrs r8, r8, r6 @               /*r8, flag_strong0 && flag_strong1*/
         beq    4f @ 
        
         ldr    r4, [r2, #12] @          /*start strong filter: r10:lims, r4: alpha*/
         vsub.s16 d17, d9, d14 @     /*d17:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r4 @ 
         vabs.s16 d21, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d21, d0[0] @    
         vshr.u32  q14, q15, #7 @   
         vmov.u16 d29, #0 @        /*d29: 0*/
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vcgt.u16 d26, d17, d29 @ 
         vmov.s16 d31, #1 @ 
         vcgt.u16 d26, d17, d29 @ 
         vcge.s16 d27, d31, d16 @ 
         vand.u16 d17, d26, d27 @   /* d17:t!=0 && sflag<=1 (reserved !!) */

         vmov       r5, r6, d17 @       /*strong filter check*/
         orrs          r7, r5, r6 @ 
         beq          6f @ 
        
         ldr        r5, [r2] @              /*dmode*/
         ldr        r6, [r2, #32] @              /*r6: rv40_dither_l*/
         ldr        r7, [r2, #36] @              /*r6: rv40_dither_r*/
         ldr        r8, [r6, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         ldr        r9, [r7, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         vmov    d31, r8, r9  @ 
         vmovl.u8 q9, d31 @              /*q9: rv40_dither_l[dmode + i] and rv40_dither_r[dmode + i] (reserved !!)*/
         vadd.s16  d24, d12, d14 @     /*src[-2]+ src[-1]*/
         vadd.s16  d23, d9, d11 @      /*src[0]+ src[1]*/
         vadd.s16  d24, d24, d23 @      /* d24:src[0]+ src[1]+src[-2]+src[-1]*/

         vdup.16    d20, r10 @                      /*d20 expand limis (!!reserved)*/
       
         vadd.s16  d0, d24, d10 @     /*cal p0: d3*/
         vshl.u16   d28, d0, #4 @      /*16x*/
         vshl.u16   d29, d0, #3 @      /*8x*/        
         vadd.u16  d29, d28, d29 @      /*24x*/
         vadd.u16  d0, d0, d29 @      /*25x*/
         vadd.u16  d0, d0, d18 @   
         vsub.s16  d0, d0, d11 @ 
         vadd.s16  d0, d24, d0 @    
         vshr.u16   d6, d0, #7 @         /*d6: p0: [-1]*/

         vadd.s16  d7, d24, d13 @      /*cal q0: d4*/
         vshl.u16   d30, d7, #4 @      /*16x*/
         vshl.u16   d31, d7, #3 @      /*8x*/        
         vadd.u16  d31, d30, d31 @      /*24x*/
         vadd.u16  d7, d7, d31 @      /*25x*/
         vadd.u16  d7, d7, d19 @   
         vsub.s16  d7, d7, d12 @ 
         vadd.s16  d7, d24, d6 @  
         vshr.u16   d1, d7, #7 @         /*d1: q0: [0]*/

         vsub.s16   d26, d14, d20 @              /*clip min*/
         vadd.s16   d27, d14, d20 @              /*clip max*/
         vmax.s16  d26, d26, d6 @ 
         vmin.s16  d26, d26, d27 @ 
        
         vsub.s16   d24, d9, d20 @              /*clip min*/
         vadd.s16   d25, d9, d20 @              /*clip max*/
         vmax.s16  d24, d1, d24 @ 
         vmin.s16  d24, d24, d25 @ 
        
         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d22, d26 @             /* merge result */
         vand.u16  d6, d23, d6 @         
         vorr.u16  d6, d6, d26 @                 /*d6: p0: [-1]*/

         vand.u16  d24, d22, d24 @ 
         vand.u16  d1, d23, d1 @         
         vorr.u16  d1, d1, d24 @                /*d1: q0: [0]*/ 

         vadd.s16 d28, d8, d10 @ 
         vadd.s16 d29, d12, d9 @ 
         vadd.s16 d29, d28, d29 @ 
         vadd.s16 d26, d29, d3 @ 
         vshr.u16 d28, d26, #4 @        /*16x*/
         vshr.u16 d29, d26, #3 @        /*8x*/
         vadd.s16 d29, d28, d29 @     /*24x*/        
         vadd.s16 d29, d26, d29 @       /*25x*/
         vadd.s16 d28, d10, d12 @ 
         vadd.s16 d29, d29, d28 @ 
         vadd.s16 d28, d6, d18 @ 
         vadd.s16 d29, d29, d28 @ 
         vshr.u16  d4, d29, #7 @            /*d4: p1 [-2]*/

         vadd.s16 d30, d14, d11 @ 
         vadd.s16 d31, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d27, d31, d4 @ 
         vshr.u16 d30, d27, #4 @        /*16x*/
         vshr.u16 d31, d27, #3 @        /*8x*/
         vadd.s16 d31, d30, d31 @     /*24x*/        
         vadd.s16 d31, d27, d31 @       /*25x*/
         vadd.s16 d30, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d30, d1, d19 @ 
         vadd.s16 d31, d30, d31 @ 
         vshr.u16  d3, d31, #7 @          /* d3: q1 [1]*/

         vsub.s16   d26, d12, d20 @              /*clip min*/
         vadd.s16   d27, d12, d20 @              /*clip max*/
         vmax.s16  d26, d4, d26 @ 
         vmin.s16  d26, d27, d26 @ 
        
         vsub.s16   d24, d11, d20 @              /*clip min*/
         vadd.s16   d25, d11, d20 @              /*clip max*/
         vmax.s16  d24, d3, d24 @ 
         vmin.s16  d24, d24, d25 @ 

         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d26, d22 @             /* merge result */
         vand.u16  d4, d4, d23 @         
         vorr.u16   d4, d26, d4 @                 /*d4: p1 [-2]*/
        
         vand.u16  d24, d24, d22 @ 
         vand.u16  d3, d3, d23 @         
         vorr.u16  d3, d3, d25 @                /* d3: q1 [1]*/
        
         #if 0
         @ldr     r3, [r2, #24] @                      /*chroma*/
         @movs r3, r3 @ 
        
         @beq 1f @ 
         vadd.s16 q14, q4, q5 @ 
         vadd.s16 q15, q6, q7 @   
         vadd.s16 q15, q14, q15 @ 
         vmov.s16 q12, q15 @ 
         vmov.s16 q13, #64 @ 
         vadd.s16 d30, d30, d10 @ 
         vadd.s16 d31, d31, d13 @ 
         vadd.s16 q12, q12, q13 @ 
         vshr.u16 q14, q15, #4 @               /*16x*/
         vshr.u16 q13, q15, #3 @               /*8x*/
         vadd.s16 q15, q15, q14 @            
         vadd.s16 q15, q15, q13 @             /*25x*/
         vsub.s16 d30, d30, d14 @ 
         vsub.s16 d31, d31, d9 @ 
         vadd.s16 d31, d31, d12 @ 
         vshr.u16 d2, d30, #7 @                /*d2: p2, [-3]*/
         vshr.u16 d5, d31, #7 @                /*d5: q2, [2]*/
         @beq 2f @ 
        
         @1:@ 
         #endif
         vmov.u16  d2, d10  @ 
         vmov.u16  d5, d13  @           

         @2:@ 
         vmov.u16  d0, d8  @ 
         vmov.u16  d7, d15  @   

         vmov     d16, d17 @
         sub   r3, r0, r1, LSL #2 @
         vmov.u16 q9, #0x00ff @
         add   r10, r1, r1, LSL #1 @
         vand.u16  q10, q9, q8 @

         vld1.64 d8, [r3] @
         vld1.64 d9, [r0] @         
         vld1.64 d10, [r3, r1] @
         vld1.64 d11, [r0, r1] @ 
         vbit.u8 q4, q0, q10 @
         vbit.u8 q5, q1, q10 @ 
         vld1.64 d12, [r3, r1, LSL #1] @
         vld1.64 d13, [r0, r1, LSL #1] @    
         vld1.64 d14, [r3, r10] @
         vld1.64 d15, [r0, r10] @  
         vst1.64 d8, [r3] @
         vst1.64 d9, [r0] @
         vbit.u8 q6, q2, q10 @
         vbit.u8 q7, q3, q10 @ 
         vst1.64 d10, [r3, r1] @
         vst1.64 d11, [r0, r1] @ 
         vst1.64 d12, [r3, r1, LSL #1] @
         vst1.64 d13, [r0, r1, LSL #1] @    
         vst1.64 d14, [r3, r10] @
         vst1.64 d15, [r0, r10] @ 

         b 6f @ 
        
         4:@              /*weak-deblock: d16-d19: diff_p1p0[i], diff_q1q0[i], diff_p1p2[i], diff_q1q2[i]*/
         ldr    r3, [r2, #12] @          /*r3: alpha*/

         vsub.s16 d27, d9, d14 @     /*d27:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r3 @ 
         vabs.s16 d25, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d25, d0[0]@    
         vshr.u32  q14, q15, #7 @    
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vmov.u16 d31, #0 @ 
         vcgt.u16 d26, d27, d31 @ 
         mov      r6, #3 @ 
         and       r7, r4, r5 @ 
         sub       r6, r6, r7 @ 
         vdup.16  d31, r6 @ 
         vcgt.u16 d28, d27, d31 @ 
         vcge.s16 d29, d31, d26 @ 
         vand.u16 d20, d28, d29 @   /* d20:t!=0 && sflag<=1 (reserved !!) */

         vmov       r7, r8, d20 @       /*strong filter check*/
         orrs          r7, r8, r7 @ 
         beq          6f @ 

         ands r6, r5, r4 @    /*r4=filter_p1, r5=filter_q1, r10: lims , r9 beta<<2*/
         mov  r9, r9, LSR #2 @      /*r9: beta*/
         ldr    r7, [r2, #4] @          /*r7: lim_q1*/
         ldr    r8, [r2, #8] @          /*r8: lim_p1*/
         ittt     eq @ 
         moveq  r10, r10, LSR #1 @ 
         moveq  r7, r7, LSR #1 @ 
         moveq  r8, r8, LSR #1 @ 

         vshl.u16   d27, d27, #2 @    /*t<<=2*/
         beq 3f @ 
         vsub.s16  d28, d12, d11 @    /*d28: src[-2*step] - src[1*step]*/
         vadd.s16  d27, d27, d28 @ 
         3:@ 
         vmov.s16 d22, #4 @ 
         vadd.s16 d27, d27, d22 @ 
         vshr.u16  d27, #3 @           /*d27: (t + 4) >> 3*/
         vdup.16   d30, r10 @          /*d30: lim_p0q0(lims)*/
         vabs.s16  d31, d30 @         /*max*/
         vneg.s16  d30, d31 @         /*min*/
         vmax.s16  d30, d30, d27 @  
         vmin.s16   d24, d30, d31 @     /*d24: diff=CLIP_SYMM((t + 4) >> 3, lim_p0q0)*/

         vmov.u16  q0, q4 @               /*copy*/
         vmov.u16  q1, q5 @ 
         vmov.u16  q2, q6 @ 
         vmov.u16  q3, q7 @ 
        
         vsub.s16   d1, d8, d24 @        /*d1: src[ 0*step] - diff;*/
         vadd.s16   d6, d14, d24 @      /*d6: src[-1*step] + diff;*/

         vdup.16     d21, r9 @             /*d21: beta*/
         vdup.16     d22, r4 @             /*d22: filter_p1*/
         vdup.16     d23, r5 @             /*d23: filter_q1*/

         vabs.s16   d28, d18 @           /*d28: FFABS(diff_p1p2)*/  
         vabs.s16   d29, d19 @           /*d28: FFABS(diff_q1q2)*/

         vmov.u16   d31, #0 @ 
         vcgt.u16    d22, d22, d31 @     /*condition mask (filter_p1)*/
         vcgt.u16    d23, d23, d31 @     /*condition mask (filter_q1)*/

         vcge.s16    d30, d21, d28 @ 
         vcge.s16    d31, d21, d29 @         
         vand.u16    d22, d22, d30 @    /*condition mask if(FFABS(diff_p1p2) <= beta && filter_p1)*/
         vand.u16    d23, d23, d31 @    /*condition mask if(FFABS(diff_q1q2) <= beta && filter_q1)*/

         vadd.s16    d28, d16, d17 @ 
         vadd.s16    d29, d18, d19 @ 
         vsub.s16    d28, d28, d24 @ 
         vadd.s16    d29, d29, d24 @ 
         vshr.u16     d28, d28, #1 @    /*d28: t = (diff_p1p0 + diff_p1p2 - diff) >> 1;*/
         vshr.u16     d29, d29, #1 @    /*d29: t = (diff_q1q0 + diff_q1q2 + diff) >> 1;*/

         vdup.16     d21, r8 @             /*d21: lim_p1*/
         vdup.16     d25, r7 @             /*d25: lim_q1*/

         vneg.s16    d30, d21 @ 
         vneg.s16    d31, d25 @      

         vmin.s16    d21, d21, d28 @ 
         vmin.s16    d25, d25, d29 @ 
         vmax.s16   d21, d21, d30 @     /*d21: CLIP_SYMM(t, lim_p1)*/
         vmax.s16   d25, d25, d31 @     /*d25: CLIP_SYMM(t, lim_q1)*/

         vsub.s16    d16, d12, d21 @ 
         vsub.s16    d17, d11, d25 @ 

         vbit.u16      d4,  d16, d22 @ 
         vbit.u16      d3,  d17, d23 @     /*insert result according to mask*/

         vmov     d21, d20 @
         sub   r3, r0, r1, LSL #2 @
         vmov.u16 q9, #0x00ff @
         add   r10, r1, r1, LSL #1 @
         vand.u16  q10, q9, q10 @

         vld1.64 d8, [r3] @
         vld1.64 d9, [r0] @         
         vld1.64 d10, [r3, r1] @
         vld1.64 d11, [r0, r1] @ 
         vbit.u8 q4, q0, q10 @
         vbit.u8 q5, q1, q10 @ 
         vld1.64 d12, [r3, r1, LSL #1] @
         vld1.64 d13, [r0, r1, LSL #1] @    
         vld1.64 d14, [r3, r10] @
         vld1.64 d15, [r0, r10] @  
         vst1.64 d8, [r3] @
         vst1.64 d9, [r0] @
         vbit.u8 q6, q2, q10 @
         vbit.u8 q7, q3, q10 @ 
         vst1.64 d10, [r3, r1] @
         vst1.64 d11, [r0, r1] @ 
         vst1.64 d12, [r3, r1, LSL #1] @
         vst1.64 d13, [r0, r1, LSL #1] @    
         vst1.64 d14, [r3, r10] @
         vst1.64 d15, [r0, r10] @ 
        
         6:@ 
          ldmfd sp!,      {r4-r10}
            
@        :
@        : r (src),  r (stride),  r (argu) /*,  r (dmode),  r (lim_q1), r (lim_p1), r (alpha), r (beta),  r (beta2),  r (chroma),  r (edge)*/
@        // r0          r1            r2              %3           %4             %5          %6          %7            %8              %9
@        : r3 ,  r4 ,  r5 ,  r6 ,  r7 ,  r8 ,  r9 ,  r10 ,  cc ,  memory 
        bx              lr
        ..endfunc   


function _rv40_h_loop_filter_neon_v , export =1
         sub r3, r0, r1, LSL #2   
         pld [r3] 
         pld [r0]
         pld [r3, r1] @   
         pld [r0, r1] @
         vld1.64 d8, [r3] @
         
         stmfd sp!,      {r4-r10}
         vld1.64 d9, [r0] @
         pld [r3, r1, LSL #1] @ 
         pld [r0, r1, LSL #1] @ 
         vld1.64 d10, [r3, r1] @
         add r10, r1, r1, LSL #1 @
         pld [r3, r10] @
         pld [r0, r10] @
         vld1.64 d11, [r0, r2] @
         @vmov.u16 q15, #0x00ff @
         vshr.u16  q4, q4, #8 @
         vld1.64 d12, [r3, r1, LSL #1] @
         vshr.u16  q5, q5, #8 @
         vld1.64 d13, [r0, r1, LSL #1] @
         vld1.64 d14, [r3, r10] @
         vshr.u16  q6, q6, #8 @
         vld1.64 d15, [r0, r10] @ 
         vshr.u16  q7, q7, #8 @
        
         ldr        r9, [r2, #16] @     /*r9= beta*/
         
         vsub.s16 d16, d12, d14 @   /*d16: diff_p1p0[i] = ptr[-2*step] - ptr[-1*step] */
         vsub.s16 d17, d11, d9 @     /*d17: diff_q1q0[i] = ptr[ 1*step] - ptr[ 0*step] */
         vpadd.s16 d4, d16, d17 @   /*add sum*/
         ldr        r6, [r2, #20] @     /*r6= beta2*/
         mov      r9, r9, LSL #2 @     /*beta<<=2*/
         vpaddl.s16 d5, d4 @           /*sum_p1p0: d5[0], sum_q1q0:d5[1]*/ 
         vabs.s32 d6, d5 @             /*get absolute value*/
         vmov r4, r5, d6 @             /*r4=sum_p1p0, r5=sum_q1q0*/
         cmp r4, r9 @ 
         ite   lt @ 
         movlt r4, #1 @                 /*r4=filter_p1*/
         movge r4, #0 @         
         cmp r5, r9 @ 
         ite   lt @ 
         movlt r5, #1 @                 /*r5=filter_q1*/
         movge r5, #0 @         
         orrs r6, r5, r4 @ 
         beq 6f @                      /*if(!filter_p1 && !filter_q1) return*/
         vsub.s16 d18, d12, d10 @     /*d18:  diff_p1p2[i] = ptr[-2*step] - ptr[-3*step] */
         vsub.s16 d19, d11, d13 @     /*d19:  diff_q1q2[i] = ptr[ 1*step] - ptr[ 2*step]; */
         vpadd.s16 d6, d18, d19 @   /*add sum*/
         vpaddl.s16 d7, d6 @           /*sum_p1p2: d7[0], sum_q1q2:d7[1]*/ 
         vabs.s32 d4, d7 @             /*get absolute value*/        
         vmov r7, r8, d4 @             /*r7=sum_p1p2, r8=sum_q1q2*/
         cmp r7, r6 @ 
         ite  lt @ 
         movlt r7, #2 @                 /*r7=filter_p2*/
         movge r7, #0 @         
         cmp r8, r6 @ 
         ite  lt @ 
         movlt r8, #2 @                 /*r8=filter_q2*/
         movge r8, #0 @         
         orr r7, r7, r4 @ 
         ldr  r3, [r2, #4] @           /* load */
         ldr  r10, [r2, #8] @ 
         ldr  r6, [r2, #28] @ 
         add     r3, r3, r10 @          /*calculate lims*/
         add     r10, r4, r5 @ 
         add     r10, r10, r3, LSR #1 @         
         orr r8, r8, r5 @ 
         add     r10, r10, #1 @         /*r10: lims (reserved !!)*/
         orr r8, r8, r7 @               
         orrs r8, r8, r6 @               /*r8, flag_strong0 && flag_strong1*/
         beq    4f @ 
        
         ldr    r4, [r2, #12] @          /*start strong filter: r10:lims, r4: alpha*/
         vsub.s16 d17, d9, d14 @     /*d17:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r4 @ 
         vabs.s16 d21, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d21, d0[0] @    
         vshr.u32  q14, q15, #7 @   
         vmov.u16 d29, #0 @        /*d29: 0*/
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vcgt.u16 d26, d17, d29 @ 
         vmov.s16 d31, #1 @ 
         vcgt.u16 d26, d17, d29 @ 
         vcge.s16 d27, d31, d16 @ 
         vand.u16 d17, d26, d27 @   /* d17:t!=0 && sflag<=1 (reserved !!) */

         vmov       r5, r6, d17 @       /*strong filter check*/
         orrs          r7, r5, r6 @ 
         beq          6f @ 
        
         ldr        r5, [r2] @              /*dmode*/
         ldr        r6, [r2, #32] @              /*r6: rv40_dither_l*/
         ldr        r7, [r2, #36] @              /*r6: rv40_dither_r*/
         ldr        r8, [r6, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         ldr        r9, [r7, r5, LSL #2] @                 /*read 4 bytes, need fixed, 4bytes align?*/
         vmov    d31, r8, r9  @ 
         vmovl.u8 q9, d31 @              /*q9: rv40_dither_l[dmode + i] and rv40_dither_r[dmode + i] (reserved !!)*/
         vadd.s16  d24, d12, d14 @     /*src[-2]+ src[-1]*/
         vadd.s16  d23, d9, d11 @      /*src[0]+ src[1]*/
         vadd.s16  d24, d24, d23 @      /* d24:src[0]+ src[1]+src[-2]+src[-1]*/

         vdup.16    d20, r10 @                      /*d20 expand limis (!!reserved)*/
       
         vadd.s16  d0, d24, d10 @     /*cal p0: d3*/
         vshl.u16   d28, d0, #4 @      /*16x*/
         vshl.u16   d29, d0, #3 @      /*8x*/        
         vadd.u16  d29, d28, d29 @      /*24x*/
         vadd.u16  d0, d0, d29 @      /*25x*/
         vadd.u16  d0, d0, d18 @   
         vsub.s16  d0, d0, d11 @ 
         vadd.s16  d0, d24, d0 @    
         vshr.u16   d6, d0, #7 @         /*d6: p0: [-1]*/

         vadd.s16  d7, d24, d13 @      /*cal q0: d4*/
         vshl.u16   d30, d7, #4 @      /*16x*/
         vshl.u16   d31, d7, #3 @      /*8x*/        
         vadd.u16  d31, d30, d31 @      /*24x*/
         vadd.u16  d7, d7, d31 @      /*25x*/
         vadd.u16  d7, d7, d19 @   
         vsub.s16  d7, d7, d12 @ 
         vadd.s16  d7, d24, d6 @  
         vshr.u16   d1, d7, #7 @         /*d1: q0: [0]*/

         vsub.s16   d26, d14, d20 @              /*clip min*/
         vadd.s16   d27, d14, d20 @              /*clip max*/
         vmax.s16  d26, d26, d6 @ 
         vmin.s16  d26, d26, d27 @ 
        
         vsub.s16   d24, d9, d20 @              /*clip min*/
         vadd.s16   d25, d9, d20 @              /*clip max*/
         vmax.s16  d24, d1, d24 @ 
         vmin.s16  d24, d24, d25 @ 
        
         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d22, d26 @             /* merge result */
         vand.u16  d6, d23, d6 @         
         vorr.u16  d6, d6, d26 @                 /*d6: p0: [-1]*/

         vand.u16  d24, d22, d24 @ 
         vand.u16  d1, d23, d1 @         
         vorr.u16  d1, d1, d24 @                /*d1: q0: [0]*/ 

         vadd.s16 d28, d8, d10 @ 
         vadd.s16 d29, d12, d9 @ 
         vadd.s16 d29, d28, d29 @ 
         vadd.s16 d26, d29, d3 @ 
         vshr.u16 d28, d26, #4 @        /*16x*/
         vshr.u16 d29, d26, #3 @        /*8x*/
         vadd.s16 d29, d28, d29 @     /*24x*/        
         vadd.s16 d29, d26, d29 @       /*25x*/
         vadd.s16 d28, d10, d12 @ 
         vadd.s16 d29, d29, d28 @ 
         vadd.s16 d28, d6, d18 @ 
         vadd.s16 d29, d29, d28 @ 
         vshr.u16  d4, d29, #7 @            /*d4: p1 [-2]*/

         vadd.s16 d30, d14, d11 @ 
         vadd.s16 d31, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d27, d31, d4 @ 
         vshr.u16 d30, d27, #4 @        /*16x*/
         vshr.u16 d31, d27, #3 @        /*8x*/
         vadd.s16 d31, d30, d31 @     /*24x*/        
         vadd.s16 d31, d27, d31 @       /*25x*/
         vadd.s16 d30, d13, d15 @ 
         vadd.s16 d31, d30, d31 @ 
         vadd.s16 d30, d1, d19 @ 
         vadd.s16 d31, d30, d31 @ 
         vshr.u16  d3, d31, #7 @          /* d3: q1 [1]*/

         vsub.s16   d26, d12, d20 @              /*clip min*/
         vadd.s16   d27, d12, d20 @              /*clip max*/
         vmax.s16  d26, d4, d26 @ 
         vmin.s16  d26, d27, d26 @ 
        
         vsub.s16   d24, d11, d20 @              /*clip min*/
         vadd.s16   d25, d11, d20 @              /*clip max*/
         vmax.s16  d24, d3, d24 @ 
         vmin.s16  d24, d24, d25 @ 

         vmov.u16  d31, #0 @ 
         vcgt.u16   d22, d16, d31 @               /*clip mask*/
         vcge.u16    d23, d31, d16 @ 

         vand.u16  d26, d26, d22 @             /* merge result */
         vand.u16  d4, d4, d23 @         
         vorr.u16   d4, d26, d4 @                 /*d4: p1 [-2]*/
        
         vand.u16  d24, d24, d22 @ 
         vand.u16  d3, d3, d23 @         
         vorr.u16  d3, d3, d25 @                /* d3: q1 [1]*/
        
         #if 0
         @ldr     r3, [r2, #24] @                      /*chroma*/
         @movs r3, r3 @ 
        
         @beq 1f @ 
         vadd.s16 q14, q4, q5 @ 
         vadd.s16 q15, q6, q7 @   
         vadd.s16 q15, q14, q15 @ 
         vmov.s16 q12, q15 @ 
         vmov.s16 q13, #64 @ 
         vadd.s16 d30, d30, d10 @ 
         vadd.s16 d31, d31, d13 @ 
         vadd.s16 q12, q12, q13 @ 
         vshr.u16 q14, q15, #4 @               /*16x*/
         vshr.u16 q13, q15, #3 @               /*8x*/
         vadd.s16 q15, q15, q14 @            
         vadd.s16 q15, q15, q13 @             /*25x*/
         vsub.s16 d30, d30, d14 @ 
         vsub.s16 d31, d31, d9 @ 
         vadd.s16 d31, d31, d12 @ 
         vshr.u16 d2, d30, #7 @                /*d2: p2, [-3]*/
         vshr.u16 d5, d31, #7 @                /*d5: q2, [2]*/
         @beq 2f @ 
        
         @1:@ 
         #endif
         vmov.u16  d2, d10  @ 
         vmov.u16  d5, d13  @           

         @2:@ 
         vmov.u16  d0, d8  @ 
         vmov.u16  d7, d15  @   

         vmov     d16, d17 @
         sub   r3, r0, r1, LSL #2 @
         vmov.u16 q9, #0xff00 @
         add   r10, r1, r1, LSL #1 @
         vand.u16  q10, q9, q8 @

         vld1.64 d8, [r3] @
         vld1.64 d9, [r0] @
         vshl.u16 q0, q0, #8 @
         vshl.u16 q1, q1, #8 @
         vld1.64 d10, [r3, r1] @
         vld1.64 d11, [r0, r1] @ 
         vbit.u8 q4, q0, q10 @
         vbit.u8 q5, q1, q10 @ 
         vld1.64 d12, [r3, r1, LSL #1] @
         vld1.64 d13, [r0, r1, LSL #1] @    
         vld1.64 d14, [r3, r10] @
         vld1.64 d15, [r0, r10] @
         vshl.u16 q2, q2, #8 @
         vshl.u16 q3, q3, #8 @
         vst1.64 d8, [r3] @
         vst1.64 d9, [r0] @
         vbit.u8 q6, q2, q10 @
         vbit.u8 q7, q3, q10 @ 
         vst1.64 d10, [r3, r1] @
         vst1.64 d11, [r0, r1] @ 
         vst1.64 d12, [r3, r1, LSL #1] @
         vst1.64 d13, [r0, r1, LSL #1] @    
         vst1.64 d14, [r3, r10] @
         vst1.64 d15, [r0, r10] @ 

         b 6f @ 
        
         4:@              /*weak-deblock: d16-d19: diff_p1p0[i], diff_q1q0[i], diff_p1p2[i], diff_q1q2[i]*/
         ldr    r3, [r2, #12] @          /*r3: alpha*/

         vsub.s16 d27, d9, d14 @     /*d27:t = src[0*step] - src[-1*step]  (reserved !!)*/
         vmov.16  d0[0], r3 @ 
         vabs.s16 d25, d17 @          /* d21: abs(t)*/
         vmull.s16 q15, d25, d0[0]@    
         vshr.u32  q14, q15, #7 @    
         vmovn.u32 d16, q14 @     /*d16: sflag (reserved !!)*/

         vmov.u16 d31, #0 @ 
         vcgt.u16 d26, d27, d31 @ 
         mov      r6, #3 @ 
         and       r7, r4, r5 @ 
         sub       r6, r6, r7 @ 
         vdup.16  d31, r6 @ 
         vcgt.u16 d28, d27, d31 @ 
         vcge.s16 d29, d31, d26 @ 
         vand.u16 d20, d28, d29 @   /* d20:t!=0 && sflag<=1 (reserved !!) */

         vmov       r7, r8, d20 @       /*strong filter check*/
         orrs          r7, r8, r7 @ 
         beq          6f @ 

         ands r6, r5, r4 @    /*r4=filter_p1, r5=filter_q1, r10: lims , r9 beta<<2*/
         mov  r9, r9, LSR #2 @      /*r9: beta*/
         ldr    r7, [r2, #4] @          /*r7: lim_q1*/
         ldr    r8, [r2, #8] @          /*r8: lim_p1*/
         ittt     eq @ 
         moveq  r10, r10, LSR #1 @ 
         moveq  r7, r7, LSR #1 @ 
         moveq  r8, r8, LSR #1 @ 

         vshl.u16   d27, d27, #2 @    /*t<<=2*/
         beq 3f @ 
         vsub.s16  d28, d12, d11 @    /*d28: src[-2*step] - src[1*step]*/
         vadd.s16  d27, d27, d28 @ 
         3:@ 
         vmov.s16 d22, #4 @ 
         vadd.s16 d27, d27, d22 @ 
         vshr.u16  d27, #3 @           /*d27: (t + 4) >> 3*/
         vdup.16   d30, r10 @          /*d30: lim_p0q0(lims)*/
         vabs.s16  d31, d30 @         /*max*/
         vneg.s16  d30, d31 @         /*min*/
         vmax.s16  d30, d30, d27 @  
         vmin.s16   d24, d30, d31 @     /*d24: diff=CLIP_SYMM((t + 4) >> 3, lim_p0q0)*/

         vmov.u16  q0, q4 @               /*copy*/
         vmov.u16  q1, q5 @ 
         vmov.u16  q2, q6 @ 
         vmov.u16  q3, q7 @ 
        
         vsub.s16   d1, d8, d24 @        /*d1: src[ 0*step] - diff;*/
         vadd.s16   d6, d14, d24 @      /*d6: src[-1*step] + diff;*/

         vdup.16     d21, r9 @             /*d21: beta*/
         vdup.16     d22, r4 @             /*d22: filter_p1*/
         vdup.16     d23, r5 @             /*d23: filter_q1*/

         vabs.s16   d28, d18 @           /*d28: FFABS(diff_p1p2)*/  
         vabs.s16   d29, d19 @           /*d28: FFABS(diff_q1q2)*/

         vmov.u16   d31, #0 @ 
         vcgt.u16    d22, d22, d31 @     /*condition mask (filter_p1)*/
         vcgt.u16    d23, d23, d31 @     /*condition mask (filter_q1)*/

         vcge.s16    d30, d21, d28 @ 
         vcge.s16    d31, d21, d29 @         
         vand.u16    d22, d22, d30 @    /*condition mask if(FFABS(diff_p1p2) <= beta && filter_p1)*/
         vand.u16    d23, d23, d31 @    /*condition mask if(FFABS(diff_q1q2) <= beta && filter_q1)*/

         vadd.s16    d28, d16, d17 @ 
         vadd.s16    d29, d18, d19 @ 
         vsub.s16    d28, d28, d24 @ 
         vadd.s16    d29, d29, d24 @ 
         vshr.u16     d28, d28, #1 @    /*d28: t = (diff_p1p0 + diff_p1p2 - diff) >> 1;*/
         vshr.u16     d29, d29, #1 @    /*d29: t = (diff_q1q0 + diff_q1q2 + diff) >> 1;*/

         vdup.16     d21, r8 @             /*d21: lim_p1*/
         vdup.16     d25, r7 @             /*d25: lim_q1*/

         vneg.s16    d30, d21 @ 
         vneg.s16    d31, d25 @      

         vmin.s16    d21, d21, d28 @ 
         vmin.s16    d25, d25, d29 @ 
         vmax.s16   d21, d21, d30 @     /*d21: CLIP_SYMM(t, lim_p1)*/
         vmax.s16   d25, d25, d31 @     /*d25: CLIP_SYMM(t, lim_q1)*/

         vsub.s16    d16, d12, d21 @ 
         vsub.s16    d17, d11, d25 @ 

         vbit.u16      d4,  d16, d22 @ 
         vbit.u16      d3,  d17, d23 @     /*insert result according to mask*/

         vmov     d21, d20 @
         sub   r3, r0, r1, LSL #2 @
         vmov.u16 q9, #0xff00 @
         add   r10, r1, r1, LSL #1 @
         vand.u16  q10, q9, q10 @

         vld1.64 d8, [r3] @
         vld1.64 d9, [r0] @
         vshl.u16 q0, q0, #8 @
         vshl.u16 q1, q1, #8 @
         vld1.64 d10, [r3, r1] @
         vld1.64 d11, [r0, r1] @ 
         vbit.u8 q4, q0, q10 @
         vbit.u8 q5, q1, q10 @ 
         vld1.64 d12, [r3, r1, LSL #1] @
         vld1.64 d13, [r0, r1, LSL #1] @    
         vld1.64 d14, [r3, r10] @
         vld1.64 d15, [r0, r10] @
         vshl.u16 q2, q2, #8 @
         vshl.u16 q3, q3, #8 @
         vst1.64 d8, [r3] @
         vst1.64 d9, [r0] @
         vbit.u8 q6, q2, q10 @
         vbit.u8 q7, q3, q10 @ 
         vst1.64 d10, [r3, r1] @
         vst1.64 d11, [r0, r1] @ 
         vst1.64 d12, [r3, r1, LSL #1] @
         vst1.64 d13, [r0, r1, LSL #1] @    
         vst1.64 d14, [r3, r10] @
         vst1.64 d15, [r0, r10] @ 
        
         6:@ 
          ldmfd sp!,      {r4-r10}
            
@        :
@        : r (src),  r (stride),  r (argu) /*,  r (dmode),  r (lim_q1), r (lim_p1), r (alpha), r (beta),  r (beta2),  r (chroma),  r (edge)*/
@        // r0          r1            r2              %3           %4             %5          %6          %7            %8              %9
@        : r3 ,  r4 ,  r5 ,  r6 ,  r7 ,  r8 ,  r9 ,  r10 ,  cc ,  memory 
        bx              lr
        ..endfunc   

