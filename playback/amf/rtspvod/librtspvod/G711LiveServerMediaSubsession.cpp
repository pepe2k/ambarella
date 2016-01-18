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

#include "G711LiveServerMediaSubsession.hh"
#include "G711LiveSource.hh"
#include "SimpleRTPSink.hh"

G711LiveServerMediaSubsession*
G711LiveServerMediaSubsession::createNew(UsageEnvironment& env,
					     char const* fileName,
					     Boolean reuseFirstSource,
					     int isALaw) {
  return new G711LiveServerMediaSubsession(env, fileName, reuseFirstSource,isALaw);
}

G711LiveServerMediaSubsession
::G711LiveServerMediaSubsession(UsageEnvironment& env,
				    char const* fileName, Boolean reuseFirstSource,int isALaw)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource) {
  isALaw_ = isALaw;
  printf("G711LiveServerMediaSubsession::G711LiveServerMediaSubsession()\n");
}

G711LiveServerMediaSubsession
::~G711LiveServerMediaSubsession() {
  printf("G711LiveServerMediaSubsession::~G711LiveServerMediaSubsession()\n");
}

FramedSource* G711LiveServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 64; // kbps, estimate
  return G711LiveSource::createNew(envir(), (char*)fFileName);
}


RTPSink* G711LiveServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* inputSource) {
  if(isALaw_){
      return SimpleRTPSink::createNew(envir(), rtpGroupsock, 8, 8000, "audio", "PCMA");
  }else{
      return SimpleRTPSink::createNew(envir(), rtpGroupsock, 0, 8000, "audio", "PCMU");
  }
}


