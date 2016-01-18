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
// "liveMedia"
// Copyright (c) 1996-2013 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from an AAC audio file in ADTS format
// Implementation

#include "AACLiveServerMediaSubsession.hh"
#include "AACLiveSource.hh"
#include "MPEG4GenericRTPSink.hh"

AACLiveServerMediaSubsession*
AACLiveServerMediaSubsession::createNew(UsageEnvironment& env,
					     char const* fileName,
					     Boolean reuseFirstSource) {
  return new AACLiveServerMediaSubsession(env, fileName, reuseFirstSource);
}

AACLiveServerMediaSubsession
::AACLiveServerMediaSubsession(UsageEnvironment& env,
				    char const* fileName, Boolean reuseFirstSource)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource) {
  printf("AACLiveServerMediaSubsession::AACLiveServerMediaSubsession()\n");
}

AACLiveServerMediaSubsession
::~AACLiveServerMediaSubsession() {
  printf("AACLiveServerMediaSubsession::~AACLiveServerMediaSubsession()\n");
}

FramedSource* AACLiveServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 96; // kbps, estimate

  return AACLiveSource::createNew(envir(), (char*)fFileName);
}

RTPSink* AACLiveServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* inputSource) {
  AACLiveSource* aacSource = (AACLiveSource*)inputSource;
  return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
					rtpPayloadTypeIfDynamic,
					aacSource->samplingFrequency(),
					"audio", "AAC-hbr", aacSource->configStr(),
					aacSource->numChannels());
}


