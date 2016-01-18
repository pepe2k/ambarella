#ifndef _H264FRAMEDLIVESOURCE_HH_
#define _H264FRAMEDLIVESOURCE_HH_

#include <FramedSource.hh>

#include "rtsp_queue.h"
class H264NalParser{
public:
    H264NalParser();
    ~H264NalParser();
    int isEmpty() {return tmp_size == 0;}
    int getNal(char *streamName,unsigned char *buf, unsigned &nalSize,int &update_pts,int &isLive, struct timeval &ts);
private:
    unsigned char *tmp_buffer_;
    unsigned tmp_size;
    unsigned tmp_pos;
    RtspData *tmp_data_;
    int is_live_;
};

class H264FramedLiveSource : public FramedSource
{
public:
	static H264FramedLiveSource* createNew(UsageEnvironment& env,
		char const* fileName,
		unsigned preferredFrameSize = 0,
		unsigned playTimePerFrame = 0);

       unsigned char *fSps() { return sps_;}
       int fSpsLen() {return sps_len_;}
       unsigned char *fPps() {return pps_;}
       int fPpsLen() {return pps_len_;}
protected:
	H264FramedLiveSource(UsageEnvironment& env,
		char const* fileName,
		unsigned preferredFrameSize,
		unsigned playTimePerFrame);
	// called only by createNew()
	~H264FramedLiveSource();

private:
	// redefined virtual functions:
    virtual void doGetNextFrame();
    void deliverFrame();
    static void deliverFrame0(void* clientData);

public:
    EventTriggerId eventTriggerId;

protected:
    char *fileName_;
    unsigned char *sps_;
    int sps_len_;
    unsigned char *pps_;
    int pps_len_;
    unsigned fuSecsPerFrame;
    H264NalParser *parser_;
    struct timeval fPTAdjustment;
};
#endif //_H264FRAMEDLIVESOURCE_HH_
