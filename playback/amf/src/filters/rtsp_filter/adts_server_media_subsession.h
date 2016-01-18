#ifndef __ADTS_SERVER_MEDIA_SUBSESSION_H__
#define __ADTS_SERVER_MEDIA_SUBSESSION_H__

#include "server_media_session.h"

class CamAdtsServerMediaSubsession: public CamServerMediaSubsession
{
	typedef CamServerMediaSubsession inherited;
public:
	static CamAdtsServerMediaSubsession* Create(const char* pMediaTypeStr);
	virtual ~CamAdtsServerMediaSubsession(){}
private:
	CamAdtsServerMediaSubsession(): inherited() {}
	AM_ERR Construct(const char* pMediaTypeStr) { return inherited::Construct(pMediaTypeStr); }
	virtual CamRtpSink* CreateNewRtpSink();
};


#endif
