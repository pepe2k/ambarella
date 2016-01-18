/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// A source object for AAC Live Streaming
// Implementation

#include "AACLiveSource.hh"
#include <GroupsockHelper.hh>
#include "rtsp_service_impl.h"

static MyMap s_map;

static unsigned const samplingFrequencyTable[16] = {
  96000, 88200, 64000, 48000,
  44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000,
  7350, 0, 0, 0
};

AACLiveSource* AACLiveSource::createNew(UsageEnvironment& env,char *const streamName) {
    return new AACLiveSource(env,streamName);
}

void signalNewFrameDataAAC(void *device) {
  AACLiveSource *ourDevice = (AACLiveSource*)device;
  TaskScheduler* ourScheduler = &ourDevice->envir().taskScheduler();
  if (ourScheduler != NULL) { // sanity check
    ourScheduler->triggerEvent(ourDevice->eventTriggerId, ourDevice);
  }
}

AACLiveSource
::AACLiveSource(UsageEnvironment& env, char *const streamName)
  : FramedSource(env) {
    streamName_ = strDup(streamName);
    //get profile, samplingFrequencyIndex, channelConfiguration
    u_int8_t profile = 1;
    u_int8_t channelConfiguration = 1;
    int samplingFrequency = 48000;
    int profile_,channels,bitrate;
    if(RtspService::instance()->getAudioParameters(streamName_,profile_,samplingFrequency,channels,bitrate) < 0){
      printf("AACLiveSource::getAudioParameters -- failed\n");
    }else{
      profile = (u_int8_t)profile_;
      channelConfiguration = channels;
      bitrate = bitrate;//disable warnings
    }
    u_int8_t samplingFrequencyIndex = 3;
    int i;
    for(i = 0; i < 16; i++){
      if(samplingFrequencyTable[i] == (unsigned int)samplingFrequency){
        samplingFrequencyIndex = (u_int8_t)i;
        break;
      }
    }
    //printf("AACLiveSource -- sample_rate = %d,index = %d,channels = %d\n",samplingFrequency,samplingFrequencyIndex,channels);

    fSamplingFrequency = samplingFrequencyTable[samplingFrequencyIndex];
    fNumChannels = channelConfiguration == 0 ? 2 : channelConfiguration;
    fuSecsPerFrame
      = (1024/*samples-per-frame*/*1000000) / fSamplingFrequency/*samples-per-second*/;

    // Construct the 'AudioSpecificConfig', and from it, the corresponding ASCII string:
    unsigned char audioSpecificConfig[2];
    u_int8_t const audioObjectType = profile + 1;
    audioSpecificConfig[0] = (audioObjectType<<3) | (samplingFrequencyIndex>>1);
    audioSpecificConfig[1] = (samplingFrequencyIndex<<7) | (channelConfiguration<<3);
    sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

    if(s_map.get_and_inc(streamName_) == 0){
        RtspService::instance()->flushStreamSource(streamName_,0);
        eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
        RtspService::instance()->registerSigFrameDataAudio(streamName_,signalNewFrameDataAAC,this);
        printf("AACLiveSource::AACLiveSource\n");
    }
    tmp_data_ = NULL;
}

AACLiveSource::~AACLiveSource() {
  if(tmp_data_)   RtspData::rtsp_data_free(tmp_data_);
  if(s_map.dec_and_get(streamName_) != 0){
    if(streamName_) delete []streamName_,streamName_ = NULL;
    return;
  }
  RtspService::instance()->unregisterSigFrameDataAudio(streamName_);
  envir().taskScheduler().deleteEventTrigger(eventTriggerId);
  RtspService::instance()->flushStreamSource(streamName_,0,1);
  if(streamName_) delete []streamName_,streamName_ = NULL;
  printf("AACLiveSource::~AACLiveSource\n");
}

void AACLiveSource::deliverFrame0(void* clientData){
     ((AACLiveSource*)clientData)->deliverFrame();
}

void AACLiveSource::deliverFrame(){\
     if (!isCurrentlyAwaitingData()) return; // we're not ready for the data yet
     doGetNextFrame();
}

void AACLiveSource::doGetNextFrame() {
  int isLive;
  if(RtspService::instance()->getAudioFrame(streamName_,tmp_data_,isLive) < 0){
    printf("AACLiveSource::doGetNextFrame -- handleClosure\n");
    handleClosure(this);
    return;
  }
  if(!tmp_data_){
    return;
  }
  if(!tmp_data_->len){
    RtspData::rtsp_data_free(tmp_data_);
    return;
  }
  memcpy(fTo,tmp_data_->data,tmp_data_->len);
  fFrameSize = tmp_data_->len;
  if(isLive){
    fPresentationTime = tmp_data_->ts;
  }
  RtspData::rtsp_data_free(tmp_data_);

  if(fFrameSize){
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
      //fPresentationTime = tmp_data_->ts;
      fDurationInMicroseconds = 0;//TODO
    }
    FramedSource::afterGetting(this);
  }
}
