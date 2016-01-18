#ifndef __RTSP_CLIENT_SESSION_H__
#define __RTSP_CLIENT_SESSION_H__

class CamRTSPServer;
class CamRtpSink;
class CamServerMediaSession;
class CamServerMediaSubsession;

class CamRTSPClientSession {	
public:
	static CamRTSPClientSession* Create(CamRTSPServer *ourServer, int clientSocket,
		AM_U32 clientAddr, AM_U16 rtpServerPort);
	
	virtual ~CamRTSPClientSession();
	AM_ERR incomingRequestHandler();
	CamRtpSink* GetRtpSink() const { return _pRtpSink; }
	
protected:
	CamRTSPClientSession(CamRTSPServer *ourServer, int clientSocket,
		AM_U32 clientAddr, AM_U16 rtpServerPort):
		_clientSocket(clientSocket), _clientAddr(clientAddr), _serverPort(rtpServerPort), _ourServer(ourServer),
		_pRtpSink(NULL), _pMediaSession(NULL), _pMediaSubsession(NULL), _numStreamStates(0),
		_pStreamStates(NULL)
		{}
	 AM_ERR Construct();

private:
	typedef enum StreamingMode {
		RTP_UDP,
		RTP_TCP,
		RAW_UDP
	 } StreamingMode;
	// Make the handler functions for each command virtual, to allow subclasses to redefine them:
	virtual void handleCmd_bad(char const* cseq);
	virtual void handleCmd_notSupported(char const* cseq);
	virtual void handleCmd_notFound(char const* cseq);
	virtual void handleCmd_unsupportedTransport(char const* cseq);
	virtual void handleCmd_OPTIONS(char const* cseq);
	virtual void handleCmd_DESCRIBE(char const* cseq, char const* urlSuffix,
				    char const* fullRequestStr);
	virtual void handleCmd_SETUP(char const* cseq,
				 char const* urlPreSuffix, char const* urlSuffix,
				 char const* fullRequestStr);
	virtual void handleCmd_PLAY(CamServerMediaSubsession* pSubsession,
		const char* cseq, const char* fullRequestStr);
	virtual void handleCmd_GET_PARAMETER(const char* cseq, const char* fullRequestStr);
	virtual void handleCmd_SET_PARAMETER(const char* cseq, const char* fullRequestStr);
	virtual void handleCmd_withinSession(char const* cmdName,
					 char const* urlPreSuffix, char const* urlSuffix,
					 char const* cseq, char const* fullRequestStr);
	AM_BOOL parseRTSPRequestString(char const* reqStr,
		unsigned reqStrSize,
		char* resultCmdName,
		unsigned resultCmdNameMaxSize,
		char* resultURLPreSuffix,
		unsigned resultURLPreSuffixMaxSize,
		char* resultURLSuffix,
		unsigned resultURLSuffixMaxSize,
		char* resultCSeq,
		unsigned resultCSeqMaxSize);
	AM_BOOL parseTransportHeader(char const* buf,
		StreamingMode *streamingMode,
		char* streamingModeString,
		char* destinationAddressStr,
		AM_U8 *destinationTTL,
		AM_U16 *clientRTPPortNum,
		AM_U16 *clientRTCPPortNum,
		AM_U8 *rtpChannelId,
		AM_U8 *rtcpChannelId);
	void ReclaimStreamStates();
	const char * getAllowedCommandNames() {
		static const char * allowedCommandNames =
			"OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER";
		return allowedCommandNames;
	}

private:
	static const int RTSP_BUFFER_SIZE = 10000; // for incoming requests, and outgoing responses
	
	int _clientSocket;
	AM_U32 _clientAddr;
	AM_U16 _serverPort;
	AM_U32 _sessionId;
	
	CamRTSPServer* _ourServer;
	CamRtpSink* _pRtpSink;
	CamServerMediaSession *_pMediaSession;
	CamServerMediaSubsession *_pMediaSubsession;
	char _requestBuffer[RTSP_BUFFER_SIZE];
	char _responseBuffer[RTSP_BUFFER_SIZE];

	AM_U32 _numStreamStates;
	struct streamState {
		CamServerMediaSubsession* subsession;
		void* streamToken;
	} * _pStreamStates;

public:
	AM_U32 GetSessionID() const { return _sessionId; }
};


#endif
