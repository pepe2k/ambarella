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

#include "G711LiveSource.hh"
#include <GroupsockHelper.hh>
#include "rtsp_service_impl.h"

static MyMap s_map;
G711LiveSource* G711LiveSource::createNew(UsageEnvironment& env,char *const streamName) {
    return new G711LiveSource(env,streamName);
}

void signalNewFrameDataG711(void *device) {
  G711LiveSource *ourDevice = (G711LiveSource*)device;
  TaskScheduler* ourScheduler = &ourDevice->envir().taskScheduler();
  if (ourScheduler != NULL) { // sanity check
    ourScheduler->triggerEvent(ourDevice->eventTriggerId, ourDevice);
  }
}

G711LiveSource
::G711LiveSource(UsageEnvironment& env, char *const streamName)
  : FramedSource(env) {

  streamName_ = strDup(streamName);
  fuSecsPerFrame = 1000000/100; //TODO
  tmp_data_ = NULL;
  if(s_map.get_and_inc(streamName_) == 0){
    RtspService::instance()->flushStreamSource(streamName_,0);
    eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
    RtspService::instance()->registerSigFrameDataAudio(streamName_,signalNewFrameDataG711,this);
    printf("G711LiveSource::G711LiveSource , streamName [%s]\n",streamName_);
 }
}

G711LiveSource::~G711LiveSource() {
  if(tmp_data_) RtspData::rtsp_data_free(tmp_data_);
  if(s_map.dec_and_get(streamName_) != 0){
     if(streamName_)  delete [] streamName_,streamName_ = NULL;
     return;
  }
  RtspService::instance()->unregisterSigFrameDataAudio(streamName_);
  envir().taskScheduler().deleteEventTrigger(eventTriggerId);
  eventTriggerId = 0;
  RtspService::instance()->flushStreamSource(streamName_,0,1);
  printf("G711LiveSource::~G711LiveSource ,streamName [%s]\n",streamName_);
  if(streamName_) delete []streamName_,streamName_ = NULL;
}

void G711LiveSource::deliverFrame0(void* clientData){
     ((G711LiveSource*)clientData)->deliverFrame();
}

void G711LiveSource::deliverFrame(){\
     if (!isCurrentlyAwaitingData()) return; // we're not ready for the data yet
     doGetNextFrame();
}

void G711LiveSource::doGetNextFrame() {
  int isLive;
  if(RtspService::instance()->getAudioFrame(streamName_,tmp_data_,isLive) < 0){
    printf("G711LiveSource::doGetNextFrame -- handleClosure\n");
    handleClosure(this);
    return;
  }
  if(!tmp_data_){
    return;
  }
  if(!tmp_data_->len){
    printf("G711LiveSource::doGetNextFrame -- data_len == 0, unexpected\n");
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

