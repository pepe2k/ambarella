#ifndef __ADTS_RTP_SINK__
#define __ADTS_RTP_SINK__

#include "rtp_sink.h"

class CamAdtsRtpSink: public CamRtpSink
{
	typedef CamRtpSink inherited;
public:
	static CamAdtsRtpSink* Create();

protected:
	CamAdtsRtpSink(): inherited(98, 48000, 2)
		{}	
	virtual ~CamAdtsRtpSink() {}
	AM_ERR Construct();
	
	virtual AM_BOOL GetOneFrame(AM_U8** ppFrameStart, AM_U32* pFrameSize, AM_BOOL* pLastFragment);
	virtual AM_U32 DoSpecialFrameHandling(AM_U8* outBuf, AM_U8* pFrameStart,
		AM_U32 frameSize, AM_BOOL lastFragment);
	
	CAutoString GetAuxSDPLine() const;
};

#endif
