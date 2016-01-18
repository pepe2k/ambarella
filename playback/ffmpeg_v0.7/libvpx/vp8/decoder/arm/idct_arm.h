#ifndef IDCT_ARM_H
#define IDCT_ARM_H


#if HAVE_ARMV7 
extern prototype_idct(vp8_short_idct4x4llm_1_neon);
extern prototype_idct(vp8_short_idct4x4llm_neon);
extern prototype_idct_scalar(vp8_dc_only_idct_neon);
extern prototype_second_order(vp8_short_inv_walsh4x4_1_neon);
extern prototype_second_order(vp8_short_inv_walsh4x4_neon);

#undef  vp8_idct_idct1
#define vp8_idct_idct1 vp8_short_idct4x4llm_1_neon

#undef  vp8_idct_idct16
#define vp8_idct_idct16 vp8_short_idct4x4llm_neon

#undef  vp8_idct_idct1_scalar
#define vp8_idct_idct1_scalar vp8_dc_only_idct_neon

#undef  vp8_idct_iwalsh1
#define vp8_idct_iwalsh1 vp8_short_inv_walsh4x4_1_neon

#undef  vp8_idct_iwalsh16
#define vp8_idct_iwalsh16 vp8_short_inv_walsh4x4_neon
#endif

#endif
