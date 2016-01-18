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

#ifndef _G711_LIVE_SOURCE_HH
#define _G711_LIVE_SOURCE_HH

#include <FramedSource.hh>
#include "rtsp_queue.h"

class G711LiveSource: public FramedSource {
public:
  static G711LiveSource* createNew(UsageEnvironment& env,char *const streamName);

private:
  G711LiveSource(UsageEnvironment& env,char *const streamName);
	// called only by createNew()

  virtual ~G711LiveSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  void deliverFrame();
  static void deliverFrame0(void* clientData);

public:
  EventTriggerId eventTriggerId;

private:
  unsigned fuSecsPerFrame;
  char *streamName_;
  RtspData *tmp_data_;
};

#endif //_G711_LIVE_SOURCE_HH
