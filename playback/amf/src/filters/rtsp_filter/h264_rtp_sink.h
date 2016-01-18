#ifndef __H264_RTP_SINK__
#define __H264_RTP_SINK__

#include "rtp_sink.h"

struct NALU
{
	AM_U32 size;
	AM_U8 * addr;
	AM_U8 nalu_type;
};

class CamH264RtpSink: public CamRtpSink
{
	typedef CamRtpSink inherited;
	enum { max_nalu = 8 };
public:
	static CamH264RtpSink* Create();

protected:
	CamH264RtpSink(): inherited(96, 90000), _naluCnt(0), _currNaluIndex(0)
		{}	
	virtual ~CamH264RtpSink() {}
	AM_ERR Construct();

	void FeedStreamData(AM_U8 *inputBuf, AM_U32 inputBufSize);
	virtual AM_BOOL GetOneFrame(AM_U8 **ppFrameStart, AM_U32 *pFrameSize, AM_BOOL *pLastFragment);
	virtual AM_U32 DoSpecialFrameHandling(AM_U8 *outBuf, AM_U8 *pFrameStart,
		AM_U32 frameSize, AM_BOOL lastFragment);
	virtual CAutoString GetAuxSDPLine() const;
private:
	NALU _nalus[max_nalu];
	AM_U32 _naluCnt;
	AM_U32 _currNaluIndex;
};




#define MAX_NALU_NUM_PER_FRAME	8


//-----------------------
#define AUDHEAD 0x00000109
#define SEIHEAD 0x00000106
#define SPSHEAD 0x00000107
#define PPSHEAD 0x00000108
#define IDRHEAD 0x00000105
#define NALUHEAD 0x00000101


#define NALU_TYPE_NONIDR	1
#define NALU_TYPE_IDR		5
#define NALU_TYPE_SEI		6
#define NALU_TYPE_SPS		7
#define NALU_TYPE_PPS		8
#define NALU_TYPE_AUD		9


#endif
