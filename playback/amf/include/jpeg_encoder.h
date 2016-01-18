/*
 * jpeg_encoder.h
 *
 * History:
 *    2012/10/25 - [Sky Chen] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _JPEGENC_H_
#define _JPEGENC_H_

typedef struct{
     int vout;
     int dec_id;
     int file_index;
     int video_width;
     int video_height;
     int quality;
     bool enlog;
}SCaptureJpegInfo;

class JpegEncoder
{
public:
       JpegEncoder(int iav_fd);
       ~JpegEncoder();
       int SetParams(SCaptureJpegInfo* p_param);
       int PlaybackCapture(unsigned int cap_coded = 1, unsigned int cap_thumbnail = 1, unsigned int cap_screennail = 1, bool setName=false, char* namecoded=NULL, char* namethumb=NULL, char* namescreen=NULL);

private:
       int mIavFd;
       int mVoutWidth;
       int mVoutHeight;
       SCaptureJpegInfo* mpSParam;
};
#endif