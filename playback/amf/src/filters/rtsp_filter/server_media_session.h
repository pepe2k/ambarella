#ifndef __SERVER_MEDIA_SESSION_HH__
#define __SERVER_MEDIA_SESSION_HH__

#include <stdio.h>
#include <sys/time.h>
#include "am_types.h"
#include "osal.h"

class CAutoString;
class CamRtpSink;
class CamRtspServerInput;
class CamServerMediaSubsession;

class CamServerMediaSession {
public:
	static CamServerMediaSession *Create(const char* pStreamName, const char* pInfo,
		const char* pDescription);
	void Delete();
		
	const char* GetStreamName() const { return _streamName; }
	CAutoString GenerateSDPDescription();
	AM_ERR AddSubsession(CamServerMediaSubsession* pSubsession);
	CamServerMediaSubsession* LookupSubsessionByTrackId(const char* pTrackId);
	float duration() const;

protected:
	CamServerMediaSession(): _pSubsessionsHead(NULL), _pSubsessionsTail(NULL),
		_subsessionCounter(0)
		{}
	virtual ~CamServerMediaSession();
	AM_ERR Construct(char const* pStreamName, char const* pInfo,
		char const* pDescription);

private:
	enum { rtsp_string_max = 64 };
	
	CamServerMediaSubsession* _pSubsessionsHead;
	CamServerMediaSubsession* _pSubsessionsTail;
	AM_U32 _subsessionCounter;

	char _streamName[rtsp_string_max];
	char _infoSDPString[rtsp_string_max];
	char _descriptionSDPString[rtsp_string_max];
	struct timeval _creationTime;

public:
	CamServerMediaSubsession* GetSubsessionsHead() const { return _pSubsessionsHead; }
};

class CamDestination;

class CamServerMediaSubsession {
	friend class CamRTSPServer;
public:
	void Delete();
	const char* GetMediaTypeString() const { return _pMediaTypeStr; }
	AM_ERR AddDestination(AM_U32 clientSessionID, int sock, AM_U32 addr, AM_U16 port);
	AM_ERR DeleteDestination(AM_U32 clientSessionID);
	CamDestination* GetNextDestination() const;
	CamRtpSink* GetRtpSink() const { return _pRtpSink; }
	void LockRtpSink() const { __LOCK(_pRtpSinkMutex); }
	void UnlockRtpSink() const { __UNLOCK(_pRtpSinkMutex); }
	AM_U32 GetDestinationCount() const { return _destinationCount; }
	void RemoveRtpSink();
	CAutoString GetTrackIdStr() const;
	AM_U32 AddRefCount() { return ++_refCount; };
	AM_U32 DecRefCount() { return --_refCount; };

protected:
	CamServerMediaSubsession(): _pMediaTypeStr(NULL), 
		_serverAddressForSDP(0), _portNumForSDP(0),
		_pNext(NULL), _trackNumber(0), _destinationCount(0),
		_pRtpSink(NULL), _refCount(0)
	{}
	virtual ~CamServerMediaSubsession();
	AM_ERR Construct(const char* pMediaTypeStr);
	float duration() const;

private:
	CAutoString GetSdpLines();
	CAutoString GetRangeSdpLine() const;
	virtual CamRtpSink* CreateNewRtpSink() = 0;

private:
	enum { max_client_num = 64 };
	friend class CamServerMediaSession;
	CamServerMediaSession* _pParentSession;
	char* _pMediaTypeStr;
	AM_U32 _serverAddressForSDP;
	AM_U16 _portNumForSDP;

	CamServerMediaSubsession* _pNext;
	AM_U32 _trackNumber;
	const char* _trackId;

	CamDestination* _pDestinations[max_client_num];
	AM_U32 _destinationCount;
	CamRtpSink* _pRtpSink;
	CMutex* _pRtpSinkMutex;
	AM_U32 _refCount;

public:
	CamServerMediaSubsession* GetNext() const { return _pNext; }
};

class CamDestination {
public:
	CamDestination(AM_U32 clientSessionID, int sock, AM_U32 destAddr, AM_U16 rtpDestPort) 
		: _clientSessionID(clientSessionID), _socket(sock), _isTCP(AM_FALSE), _addr(destAddr),
		_rtpPort(rtpDestPort) {}
	CamDestination(AM_U32 clientSessionID, int sock) 
		: _clientSessionID(clientSessionID), _socket(sock), _isTCP(AM_TRUE), _addr(0),
		_rtpPort(0) {}
	AM_U32 GetSessionID() const { return _clientSessionID; }
public:
	AM_U32 _clientSessionID;
	int _socket;
	AM_BOOL _isTCP;
	AM_U32 _addr;
	AM_U16 _rtpPort;
};

#endif
