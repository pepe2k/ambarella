#ifndef _H264_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH
#define _H264_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH
#include "H264VideoFileServerMediaSubsession.hh"

class H264LiveVideoServerMediaSubsession: public H264VideoFileServerMediaSubsession {
public:
  static H264LiveVideoServerMediaSubsession*
  createNew( UsageEnvironment& env,
	           char const* fileName,
				Boolean reuseFirstSource );

protected: // we're a virtual base class
  H264LiveVideoServerMediaSubsession( UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource);
   ~H264LiveVideoServerMediaSubsession();

protected: // redefined virtual functions
   FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
};

#endif //_H264_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH

