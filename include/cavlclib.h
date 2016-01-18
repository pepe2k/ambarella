/*
 * cavlclib.h
 *
 * History:
 *	2009/5/11 - [Louis Sun] created file
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_CAVLC_LIB_H__
#define __IAV_CAVLC_LIB_H__


#ifdef __cplusplus
extern "C" {
#endif




/* this function allocates memory buffer
 * to store CAVLC encoded result, calling more than once
 * will reuse the first allocated buffer
 */
int cavlc_encode_init(int fd_iav);



int cavlc_encode_setup(void);


/* this function encodes one CAVLC frame from intermediate pjpeg
 *   fd_iav: 		input ,   IAV handle
 *  pjpeg_info:		input,    bitstream info of pjpeg (intermediate CAVLC)
 *  p_frame_start:		output,  frame start of CAVLC
 *  p_frame_size:		output,  frame size  of CAVLC
 */
int cavlc_encode_frame(bits_info_ex_t * bsinfo, u8 ** p_frame_start, u32 * p_frame_size);



/* this function shutdown cavlc encoding and deinitialize related memory 
 * Only need to claim back the cavlc memory. 
 * once it's shutdown, you need to call cavlc_encode_init again to allocate memory
 */
int cavlc_encode_shutdown(void);




#ifdef __cplusplus
}
#endif
#endif





