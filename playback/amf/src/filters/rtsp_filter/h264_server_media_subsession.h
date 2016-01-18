#ifndef __H264_SERVER_MEDIA_SUBSESSION_H__
#define __H264_SERVER_MEDIA_SUBSESSION_H__

#include "server_media_session.h"

class CamH264ServerMediaSubsession: public CamServerMediaSubsession
{
	typedef CamServerMediaSubsession inherited;
public:
	static CamH264ServerMediaSubsession* Create(const char* pMediaTypeStr);
	virtual ~CamH264ServerMediaSubsession(){}
private:
	CamH264ServerMediaSubsession(): inherited() {}
	AM_ERR Construct(const char* pMediaTypeStr) { return inherited::Construct(pMediaTypeStr); }
	virtual CamRtpSink* CreateNewRtpSink();
};


#endif

