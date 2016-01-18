#include "H264FramedLiveSource.hh"
#include "rtsp_service_impl.h"

static MyMap s_map;

H264FramedLiveSource* H264FramedLiveSource::createNew( UsageEnvironment& env,
	                                       char const* fileName,
	                                       unsigned preferredFrameSize /*= 0*/,
	                                       unsigned playTimePerFrame /*= 0*/ ){
	H264FramedLiveSource* newSource = new H264FramedLiveSource(env, fileName, preferredFrameSize, playTimePerFrame);
	return newSource;
}


void signalNewFrameDataH264(void *device) {
  H264FramedLiveSource *ourDevice = (H264FramedLiveSource*)device;
  TaskScheduler* ourScheduler = &ourDevice->envir().taskScheduler();
  if (ourScheduler != NULL) { // sanity check
    ourScheduler->triggerEvent(ourDevice->eventTriggerId, ourDevice);
  }
}

H264FramedLiveSource::H264FramedLiveSource( UsageEnvironment& env,
	char const* fileName,
	unsigned preferredFrameSize,
	unsigned playTimePerFrame)
	: FramedSource(env){
    fileName_ = strDup(fileName);
     {
         unsigned char *extra_data = NULL;
         int extra_data_len;
         if(RtspService::instance()->getVideoParameters(fileName_,extra_data,extra_data_len) < 0){
             sps_ = NULL;
             sps_len_ = 0;
             pps_ = NULL;
             pps_len_ = 0;
         }
         if(extra_data && extra_data_len){
            #   define AV_RB32(x)                           \
             ((((const unsigned char*)(x))[0] << 24) |         \
             (((const unsigned char*)(x))[1] << 16) |         \
             (((const unsigned char*)(x))[2] <<  8) |         \
             ((const unsigned char*)(x))[3])
            //parse extra_data to get sps/pps,TODO
            unsigned char *sps = extra_data;
            int sps_len = 0;
            unsigned char *pps;
            int pps_len = 0;
            for(int i = 4; i < extra_data_len - 4; i++){
                if((AV_RB32(&sps[i]) == 0x00000001) /*|| (AV_RB24(&sps[i]) == 0x000001)*/){
                    sps_len = i;
                    break;
                }
            }
            if(!sps_len_){
                printf("H264FramedLiveSource --- invalid extra_data\n");
                sps_ = NULL;
                sps_len_ = 0;
                pps_ = NULL;
                pps_len_ = 0;
            }else{
                pps = sps + sps_len;
                pps_len = extra_data_len - sps_len;
                sps_ = new unsigned char [(sps_len + 15)/16 * 16];
                if(sps_){
                    memcpy(sps_,sps + 4,sps_len - 4);
                    sps_len_ = sps_len - 4;
                }else{
                    sps_len_ = 0;
                }
                pps_ = new unsigned char [(pps_len + 15)/16 * 16];
                if(pps_){
                    memcpy(pps_,pps + 4,pps_len - 4);
                    pps_len_ = pps_len - 4;
                }else{
                    pps_len_ = 0;
               }
            }
            delete []extra_data;
         }else{
            sps_ = NULL;
            sps_len_ = 0;
            pps_ = NULL;
            pps_len_ = 0;
        }
        fuSecsPerFrame = (unsigned)((double)1000000/30); //TODO,calc duration according to framerate
        parser_ = new H264NalParser();
    }

    if(s_map.get_and_inc((char*)fileName_)== 0){
         printf("H264FramedLiveSource::H264FramedLiveSource\n");
         RtspService::instance()->flushStreamSource(fileName_,1);
         eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
         RtspService::instance()->registerSigFrameData(fileName_,signalNewFrameDataH264,this);
    }
}

H264FramedLiveSource::~H264FramedLiveSource(){
    if(sps_) delete []sps_;
    if(pps_) delete []pps_;
    delete parser_;
    if(s_map.dec_and_get(fileName_) != 0){
        delete []fileName_,fileName_ = NULL;
        return;
    }
    RtspService::instance()->unregisterSigFrameData(fileName_);
    envir().taskScheduler().deleteEventTrigger(eventTriggerId);
    RtspService::instance()->flushStreamSource(fileName_,1,1);
    delete []fileName_,fileName_ = NULL;
    printf("H264FramedLiveSource::~H264FramedLiveSource\n");
}

void H264FramedLiveSource::deliverFrame0(void* clientData){
    //printf(" H264FramedLiveSource::deliverFrame0\n");
     ((H264FramedLiveSource*)clientData)->deliverFrame();
}

void H264FramedLiveSource::deliverFrame(){\
     if (!isCurrentlyAwaitingData()) return; // we're not ready for the data yet
     if(!parser_->isEmpty() ) return;
     doGetNextFrame();
}

void H264FramedLiveSource::doGetNextFrame()
{
    //printf("H264FramedLiveSource::doGetNextFrame, fMaxSize = %d\n",fMaxSize);
    int update_pts = 0;
    int isLive;
    if(parser_->getNal(fileName_,fTo,fFrameSize,update_pts,isLive,fPresentationTime) < 0){
        printf("H264FramedLiveSource::doGetNextFrame -- handleClosure\n");
        handleClosure(this);
        return;
    }

    if(fFrameSize){
        if(update_pts){
            if(!isLive){
                // Set the 'presentation time':
                if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
                    // This is the first frame, so use the current time:
                    gettimeofday(&fPresentationTime, NULL);
                } else {
                    // Increment by the play time of the previous frame:
                    unsigned uSeconds = fPresentationTime.tv_usec + fuSecsPerFrame;
                    fPresentationTime.tv_sec += uSeconds/1000000;
                    fPresentationTime.tv_usec = uSeconds%1000000;
                }
                fDurationInMicroseconds = fuSecsPerFrame;
            }else{
                fDurationInMicroseconds = 0;//TODO
            }
        }else{
            fDurationInMicroseconds = 0;
        }
        //inform the reader that he has data:
        FramedSource::afterGetting(this);
    }
}

//
H264NalParser::H264NalParser(){
    tmp_buffer_ = NULL;
    tmp_size = 0;
    tmp_pos = 0;
    tmp_data_ = NULL;
    is_live_ = 0;
}
H264NalParser::~H264NalParser(){
    if(tmp_data_)  RtspData::rtsp_data_free(tmp_data_);
}

int H264NalParser::getNal(char *streamName,unsigned char *buf, unsigned &nalSize, int &update_pts,int &isLive,struct timeval &ts){
    update_pts = 0;
    if(!tmp_size){
        if(RtspService::instance()->getVideoFrame(streamName,tmp_data_,is_live_) < 0){
            return -1;
        }
        if(!tmp_data_){
           tmp_size = 0;
        }else if(tmp_data_->len <=4){
           RtspData::rtsp_data_free(tmp_data_);
           tmp_size = 0;
        }else{
           tmp_buffer_ = tmp_data_->data;
           tmp_size = tmp_data_->len;
           tmp_pos = 0;
           update_pts = 1;
        }
    }
    if(!tmp_size){
        nalSize = 0;
        return 0;
    }

    isLive = is_live_;
    unsigned pos = tmp_pos + 4;
    while(pos < tmp_size - 4){
        if(tmp_buffer_[pos] == 0x00)
            if(tmp_buffer_[pos + 1] == 0x00)
                if(tmp_buffer_[pos + 2] == 0x00)
                    if(tmp_buffer_[pos + 3] == 0x01)
                        break;
        ++pos;
    }
    if(pos >= tmp_size -4){
        nalSize = tmp_size - tmp_pos - 4;
        //printf("H264FramedLiveSource::H264NalParser::getNal() -1- nalSize [%d]\n",nalSize);
        memcpy(buf,&tmp_buffer_[tmp_pos + 4],nalSize);
        tmp_size = 0;
        tmp_pos = 0;
        if(isLive) ts = tmp_data_->ts;
        RtspData::rtsp_data_free(tmp_data_);
        tmp_buffer_ = NULL;
        return 0;
    }
    nalSize = pos - tmp_pos - 4;
    //printf("H264FramedLiveSource::H264NalParser::getNal() -2- nalSize [%d]\n",nalSize);
    memcpy(buf,&tmp_buffer_[tmp_pos + 4],nalSize);
    tmp_pos  = pos;
    if(isLive) ts = tmp_data_->ts;
    return 0;
};

