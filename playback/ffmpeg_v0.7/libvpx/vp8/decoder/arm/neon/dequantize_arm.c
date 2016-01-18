#include "vpx_ports/config.h"
#include "dequantize.h"
#include "predictdc.h"
#include "idct.h"
#include "vpx_mem/vpx_mem.h"

#if HAVE_ARMV7
extern void vp8_dequantize_b_loop_neon(short *Q, short *DQC, short *DQ);


void vp8_dequantize_b_neon(BLOCKD *d)
{
    int i;
    short *DQ  = d->dqcoeff;
    short *Q   = d->qcoeff;
    short *DQC = &d->dequant[0][0];
	
	//printf("\n\n--------call vp8_dequantize_b_neon--------\n\n");

    vp8_dequantize_b_loop_neon(Q, DQC, DQ);
}
#endif
