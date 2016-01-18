#ifndef __AM_FFMPEG_H__
#define __AM_FFMPEG_H__

extern "C"{
    /* ffmpeg avcodec_open()/avcodec_close() are not thread safe,
    *   implement helper fuction mw_ffmpeg_init() to register lockmgr here
    */
    void mw_ffmpeg_init();
};

#endif /*__AM_FFMPEG_H__*/

