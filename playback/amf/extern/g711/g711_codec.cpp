/*
 * g711_codec.cpp
 *
 * History:
 *    2013/11/20 - [SkyChen] create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "string.h"
#include "am_types.h"

#include "g711_codec.h"
/////////////////CreateG711Codec/////////////////
CG711Codec* CG711Codec::CreateG711Codec()
{
    CG711Codec* ret = new CG711Codec();
    return ret;
}

CG711Codec::CG711Codec()
{
}

void CG711Codec::ConfigCodec()
{
}

AM_INT CG711Codec::Encoder(AM_U8* input, AM_U8* output, AM_UINT length)
{
    if (input == NULL || output == NULL)
        return -1;

    AM_S16* pcm = (AM_S16*)input;
    AM_INT i;
    for (i = 0; i < (length >> 1); i++) {
        output[i] = alawEncode((AM_INT)pcm[i]);
    }
    return 0;
}

AM_U8 CG711Codec::alawEncode(AM_INT pcm)
{
    AM_INT value = pcm;
    AM_UINT ret_value = 0;  // A-law value we are forming
    if (value < 0) {
        // -ve value
        // Note, ones compliment is used here as this keeps encoding symetrical
        // and equal spaced around zero cross-over, (it also matches the standard).
        value = ~value;
        ret_value = 0x00; // sign = 0
    } else {
        // +ve value
        ret_value = 0x80; // sign = 1
    }
    // Calculate segment and interval numbers
    value >>= 4;
    if (value >= 0x20) {
        if (value>=0x100) {
            value >>= 4;
            ret_value += 0x40;
        }
        if (value>=0x40) {
            value >>= 2;
            ret_value += 0x20;
        }
        if(value>=0x20) {
            value >>= 1;
            ret_value += 0x10;
        }
    }
    // a&0x70 now holds segment value and 'p' the interval number
    ret_value += value;  // a now equal to encoded A-law value

    return ret_value^0x55;	// A-law has alternate bits inverted for transmission
}

AM_INT CG711Codec::alawDecode(AM_U8 alaw)
{
    AM_U8 sign;
    AM_INT linear;
    AM_UINT shift;

    alaw ^= 0x55;  // A-law has alternate bits inverted for transmission
    sign = alaw&0x80;
    linear = alaw&0x1f;
    linear <<= 4;
    linear += 8;  // Add a 'half' bit (0x08) to place PCM value in middle of range

    alaw &= 0x7f;
    if (alaw>=0x20) {
        linear |= 0x100;  // Put in MSB
        shift = (alaw>>4)-1;
        linear <<= shift;
    }
    if (!sign)
        return -linear;
    else
        return linear;
}

/*
* u-law, A-law and linear PCM conversions.
*/
#define SIGN_BIT (0x80) /* Sign bit for a A-law byte. */
#define QUANT_MASK (0xf) /* Quantization field mask. */
#define NSEGS (8) /* Number of A-law segments. */
#define SEG_SHIFT (4) /* Left shift for segment number. */
#define SEG_MASK (0x70) /* Segment field mask. */

static AM_S16 seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,
0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};


/* copy from CCITT G.711 specifications */
static AM_U8 _u2a[128] = { /* u- to A-law conversions */
    1, 1, 2, 2, 3, 3, 4, 4,
    5, 5, 6, 6, 7, 7, 8, 8,
    9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24,
    25, 27, 29, 31, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 42, 43, 44,
    46, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 58, 59, 60, 61, 62,
    64, 65, 66, 67, 68, 69, 70, 71,
    72, 73, 74, 75, 76, 77, 78, 79,
    81, 82, 83, 84, 85, 86, 87, 88,
    89, 90, 91, 92, 93, 94, 95, 96,
    97, 98, 99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112,
    113, 114, 115, 116, 117, 118, 119, 120,
    121, 122, 123, 124, 125, 126, 127, 128
};

static AM_U8 _a2u[128] = { /* A- to u-law conversions */
    1, 3, 5, 7, 9, 11, 13, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31,
    32, 32, 33, 33, 34, 34, 35, 35,
    36, 37, 38, 39, 40, 41, 42, 43,
    44, 45, 46, 47, 48, 48, 49, 49,
    50, 51, 52, 53, 54, 55, 56, 57,
    58, 59, 60, 61, 62, 63, 64, 64,
    65, 66, 67, 68, 69, 70, 71, 72,
    73, 74, 75, 76, 77, 78, 79, 79,
    80, 81, 82, 83, 84, 85, 86, 87,
    88, 89, 90, 91, 92, 93, 94, 95,
    96, 97, 98, 99, 100, 101, 102, 103,
    104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127
};

static AM_INT search( AM_INT val, AM_S16 *table, AM_INT size)
{
    AM_INT i;
    for (i = 0; i < size; i++) {
        if (val <= *table++)
            return (i);
    }
    return (size);
}

/*
* linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
*
* linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
*
* Linear Input Code Compressed Code
* ------------------------ ---------------
* 0000000wxyza 000wxyz
* 0000001wxyza 001wxyz
* 000001wxyzab 010wxyz
* 00001wxyzabc 011wxyz
* 0001wxyzabcd 100wxyz
* 001wxyzabcde 101wxyz
* 01wxyzabcdef 110wxyz
* 1wxyzabcdefg 111wxyz
*
* For further information see John C. Bellamy's Digital Telephony, 1982,
* John Wiley & Sons, pps 98-111 and 472-476.
*/
AM_U8 CG711Codec::linear2alaw( AM_INT pcm_val) /* 2's complement (16-bit range) */
{
    AM_INT mask;
    AM_INT seg;
    AM_U8 aval;

    if (pcm_val >= 0) {
        mask = 0xD5; /* sign (7th) bit = 1 */
    } else {
        mask = 0x55; /* sign bit = 0 */
        pcm_val = -pcm_val - 8;
    }
    /* Convert the scaled magnitude to segment number. */
    seg = search(pcm_val, seg_end, 8);
    /* Combine the sign, segment, and quantization bits. */

    if (seg >= 8) /* out of range, return maximum value. */
        return (0x7F ^ mask);
    else {
        aval = seg << SEG_SHIFT;
        if (seg < 2)
            aval |= (pcm_val >> 4) & QUANT_MASK;
        else
            aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
        return (aval ^ mask);
    }
}

/*
* alaw2linear() - Convert an A-law value to 16-bit linear PCM
*
*/
AM_INT CG711Codec::alaw2linear( AM_U8 a_val)
{
    AM_INT t;
    AM_INT seg;

    a_val ^= 0x55;

    t = (a_val & QUANT_MASK) << 4;
    seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
    switch (seg) {
        case 0:
            t += 8;
            break;
        case 1:
            t += 0x108;
            break;
        default:
            t += 0x108;
            t <<= seg - 1;
            break;
    }
    return ((a_val & SIGN_BIT) ? t : -t);
}

#define BIAS (0x84) /* Bias for linear code. */

/*
* linear2ulaw() - Convert a linear PCM value to u-law
*
* In order to simplify the encoding process, the original linear magnitude
* is biased by adding 33 which shifts the encoding range from (0 - 8158) to
* (33 - 8191). The result can be seen in the following encoding table:
*
* Biased Linear Input Code Compressed Code
* ------------------------ ---------------
* 00000001wxyza 000wxyz
* 0000001wxyzab 001wxyz
* 000001wxyzabc 010wxyz
* 00001wxyzabcd 011wxyz
* 0001wxyzabcde 100wxyz
* 001wxyzabcdef 101wxyz
* 01wxyzabcdefg 110wxyz
* 1wxyzabcdefgh 111wxyz
*
* Each biased linear code has a leading 1 which identifies the segment
* number. The value of the segment number is equal to 7 minus the number
* of leading 0's. The quantization interval is directly available as the
* four bits wxyz. * The trailing bits (a - h) are ignored.
*
* Ordinarily the complement of the resulting code word is used for
* transmission, and so the code word is complemented before it is returned.
*
* For further information see John C. Bellamy's Digital Telephony, 1982,
* John Wiley & Sons, pps 98-111 and 472-476.
*/
AM_U8 CG711Codec::linear2ulaw(AM_INT pcm_val) /* 2's complement (16-bit range) */
{
    AM_INT mask;
    AM_INT seg;
    AM_U8 uval;

    /* Get the sign and the magnitude of the value. */
    if (pcm_val < 0) {
        pcm_val = BIAS - pcm_val;
        mask = 0x7F;
    } else {
        pcm_val += BIAS;
        mask = 0xFF;
    }

    /* Convert the scaled magnitude to segment number. */
    seg = search(pcm_val, seg_end, 8);

    /*
    * Combine the sign, segment, quantization bits;
    * and complement the code word.
    */
    if (seg >= 8) /* out of range, return maximum value. */
        return (0x7F ^ mask);
    else {
        uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
        return (uval ^ mask);
    }
}

/*
* ulaw2linear() - Convert a u-law value to 16-bit linear PCM
*
* First, a biased linear code is derived from the code word. An unbiased
* output can then be obtained by subtracting 33 from the biased code.
*
* Note that this function expects to be passed the complement of the
* original code word. This is in keeping with ISDN conventions.
*/
AM_INT CG711Codec::ulaw2linear(AM_U8 u_val)
{
    AM_INT t;

    /* Complement to obtain normal u-law value. */
    u_val = ~u_val;

    /*
    * Extract and bias the quantization bits. Then
    * shift up by the segment number and subtract out the bias.
    */
    t = ((u_val & QUANT_MASK) << 3) + BIAS;
    t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;
    return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

/* A-law to u-law conversion */
AM_U8 CG711Codec::alaw2ulaw( AM_U8 aval)
{
    aval &= 0xff;
    return ((aval & 0x80) ? (0xFF ^ _a2u[aval ^ 0xD5]) : (0x7F ^ _a2u[aval ^ 0x55]));
}

/* u-law to A-law conversion */
AM_U8 CG711Codec::ulaw2alaw( AM_U8 uval)
{
    uval &= 0xff;
    return ((uval & 0x80) ? (0xD5 ^ (_u2a[0xFF ^ uval] - 1)) : (0x55 ^ (_u2a[0x7F ^ uval] - 1)));
}

