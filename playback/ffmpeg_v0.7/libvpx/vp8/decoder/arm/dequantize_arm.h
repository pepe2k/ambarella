#ifndef DEQUANTIZE_ARM_H
#define DEQUANTIZE_ARM_H


#if HAVE_ARMV7
extern prototype_dequant_block(vp8_dequantize_b_neon);
extern prototype_dequant_idct(vp8_dequant_idct_neon);
extern prototype_dequant_idct_dc(vp8_dequant_dc_idct_neon);

#undef  vp8_dequant_block
#define vp8_dequant_block vp8_dequantize_b_neon

#undef  vp8_dequant_idct
#define vp8_dequant_idct vp8_dequant_idct_neon

#undef  vp8_dequant_idct_dc
#define vp8_dequant_idct_dc vp8_dequant_dc_idct_neon
#endif

#endif
