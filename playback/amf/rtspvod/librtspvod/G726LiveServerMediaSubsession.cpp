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

#include "G726LiveServerMediaSubsession.hh"
#include "G726LiveSource.hh"
#include "SimpleRTPSink.hh"
#include "rtsp_service_impl.h"

G726LiveServerMediaSubsession*
G726LiveServerMediaSubsession::createNew(UsageEnvironment& env,
					     char const* fileName,
					     Boolean reuseFirstSource) {
  return new G726LiveServerMediaSubsession(env, fileName, reuseFirstSource);
}

G726LiveServerMediaSubsession
::G726LiveServerMediaSubsession(UsageEnvironment& env,
				    char const* fileName, Boolean reuseFirstSource)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource) {

  //get profile, samplingFrequencyIndex, channelConfiguration
  int samplingFrequency,profile,channels,bitrate;
  if(RtspService::instance()->getAudioParameters((char *)fileName,profile,samplingFrequency,channels,bitrate) < 0){
    printf("G726LiveServerMediaSubsession::getAudioParameters -- failed\n");
  }else{
    profile = profile;//disable warnings
    samplingFrequency = samplingFrequency;//disable warnings
    channels = channels;//disable warnings
    bitrate_ = bitrate;
  }
  printf("G726LiveServerMediaSubsession::G726LiveServerMediaSubsession()\n");
}

G726LiveServerMediaSubsession
::~G726LiveServerMediaSubsession() {
  printf("G726LiveServerMediaSubsession::~G726LiveServerMediaSubsession()\n");
}

FramedSource* G726LiveServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = bitrate_/1000;
  return G726LiveSource::createNew(envir(), (char*)fFileName);
}
           
RTPSink* G726LiveServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* inputSource) {
  switch(bitrate_){
  case 16000:return SimpleRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, 16000, "audio", "G726-16");
  case 24000:return SimpleRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, 24000, "audio", "G726-24");
  case 32000:return SimpleRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, 32000, "audio", "G726-32");
  case 40000:return SimpleRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, 40000, "audio", "G726-40");
  default: return NULL;
  }
}


