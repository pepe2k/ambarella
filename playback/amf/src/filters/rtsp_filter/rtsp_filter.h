/*
 * RTSPServer.h
 *
 * History:
 *    2008/9/11 - [Kaiming Xie] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__


class CamRTSPClientSession;
class CQueueInputPin;
class CamRtspServerInput;
class CamServerMediaSession;
class CamServerMediaSubsession;
class CAutoString;

class CamRTSPServer: public CActiveFilter
{
	typedef CActiveFilter inherited;
public:
	static CamRTSPServer *Create(IEngine * pEngine);

private:
	CamRTSPServer(IEngine * pEngine);
	AM_ERR Construct();
	virtual ~CamRTSPServer();

public:
    // IInterface
    virtual void *GetInterface(AM_REFIID refiid)
    {
//    if (refiid == IID_IRtspServer)
//    	return (IRtspServer*)this;
        return inherited::GetInterface(refiid);
    }
    virtual void Delete() { return inherited::Delete(); }

    // IFilter
    virtual void GetInfo(INFO& info);
    virtual IPin* GetInputPin(AM_UINT index);
    virtual AM_ERR Stop();
    
	// user defined func
	AM_U16 GetRtpServerPort() const { return _rtpServerPort; }
	CamServerMediaSession *LookupServerMediaSession(char const* pStreamName) const;
	CamRTSPClientSession *LookupClientSession(AM_U32 clientSessionID) const;
	CAutoString rtspURL(const CamServerMediaSession* pMediaSession, int clientSocket = -1) const;
		// returns a "rtsp://" URL that could be used to access the
		// specified session (which must already have been added to
		// us using "addServerMediaSession()".
		// This string is dynamically allocated; caller should delete[]
		// (If "clientSocket" is non-negative, then it is used (by calling "getsockname()") to determine
		//	the IP address to be used in the URL.)
	CAutoString rtspURLPrefix(int clientSocket = -1) const;
		// like "rtspURL()", except that it returns just the common prefix used by
		// each session's "rtsp://" URL.
		// This string is dynamically allocated; caller should delete[]

private:
    // IActiveObject
    virtual void OnRun();

private:	// user defined func
	static AM_ERR ServerThread(void *p);
	void ServerThreadLoop();
	AM_ERR AddServerMediaSession(CamServerMediaSession *pMediaSession);

private:
	enum { 
		mux_input_pin_0 = 0,
		mux_input_pin_1,
		mux_input_pin_2,
		mux_input_pin_3,
		mux_input_pin_audio,
		mux_input_pin_range = 5,
	};
	static const int MAX_NUM_CLIENTS = 64;
	static const int MAX_NUM_MEDIA_SESSION = 32;

	int _rtspServerSocket;
	int _rtpUDPSocket;
	int _pipeFd[2];
	AM_U16 _rtspServerPort;
	AM_U16 _rtpServerPort;
    	CThread *_pServerThread;
		
	AM_U8 _outBuf[1500];		//real outgoing buffer sent to network
	int _clientSock[MAX_NUM_CLIENTS];
	CamRTSPClientSession *_pClientSession[MAX_NUM_CLIENTS];
	CamServerMediaSession *_pMediaSession[MAX_NUM_MEDIA_SESSION];
	CamRtspServerInput *_pInput[mux_input_pin_range];
};

//-----------------------------------------------------------------------
//
// CamTsMuxerInput
//
//-----------------------------------------------------------------------
class CamRtspServerInput: public CQueueInputPin
{
	typedef CQueueInputPin inherited;
	friend class CamRTSPServer;

public:
	static CamRtspServerInput* Create(CFilter *pFilter);

private:
	CamRtspServerInput(CFilter *pFilter):
	inherited(pFilter)
	{}
	AM_ERR Construct();
	virtual ~CamRtspServerInput() {}

public:
    virtual AM_ERR CheckMediaFormat(CMediaFormat *pFormat);
    void SetMediaSubsession(CamServerMediaSubsession* pMediaSubsession)
    	{ _pMediaSubsession = pMediaSubsession; }
    CamServerMediaSubsession* GetMediaSubsession() const
	{ return _pMediaSubsession; }

private:
	CamServerMediaSubsession* _pMediaSubsession;
};

#endif //__TS_MUXER_H__
