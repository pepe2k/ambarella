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
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from an AAC Live Streaming
// C++ header

#ifndef _AAC_LIVE_SERVER_MEDIA_SUBSESSION_HH
#define _AAC_LIVE_SERVER_MEDIA_SUBSESSION_HH

#include <FileServerMediaSubsession.hh>

class AACLiveServerMediaSubsession: public FileServerMediaSubsession{
public:
  static AACLiveServerMediaSubsession*
  createNew(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource);

protected:
  AACLiveServerMediaSubsession(UsageEnvironment& env,
				      char const* fileName, Boolean reuseFirstSource);
      // called only by createNew();
  virtual ~AACLiveServerMediaSubsession();

protected: // redefined virtual functions
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);
};

#endif //_AAC_LIVE_SERVER_MEDIA_SUBSESSION_HH

