#ifndef RECON_ARM_H
#define RECON_ARM_H



#if HAVE_ARMV7
extern prototype_recon_block(vp8_recon_b_neon);
extern prototype_recon_block(vp8_recon2b_neon);
extern prototype_recon_block(vp8_recon4b_neon);

extern prototype_copy_block(vp8_copy_mem8x8_neon);
extern prototype_copy_block(vp8_copy_mem8x4_neon);
extern prototype_copy_block(vp8_copy_mem16x16_neon);

#undef  vp8_recon_recon
#define vp8_recon_recon vp8_recon_b_neon

#undef  vp8_recon_recon2
#define vp8_recon_recon2 vp8_recon2b_neon

#undef  vp8_recon_recon4
#define vp8_recon_recon4 vp8_recon4b_neon

#undef  vp8_recon_copy8x8
#define vp8_recon_copy8x8 vp8_copy_mem8x8_neon

#undef  vp8_recon_copy8x4
#define vp8_recon_copy8x4 vp8_copy_mem8x4_neon

#undef  vp8_recon_copy16x16
#define vp8_recon_copy16x16 vp8_copy_mem16x16_neon
#endif

#endif
