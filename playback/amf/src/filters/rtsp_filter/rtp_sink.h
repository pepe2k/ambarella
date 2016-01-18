#ifndef __RTP_SINK_H__
#define __RTP_SINK_H__

class CAutoString;

class CamRtpSink
{
public:
	void Delete();
	virtual AM_BOOL GetOneFrame(AM_U8 **ppFrameStart, AM_U32* pFrameSize,
		AM_BOOL* pLastFragment) = 0;
	virtual AM_U32 DoSpecialFrameHandling(AM_U8 *outBuf, AM_U8* pFrameStart,
		AM_U32 frameSize, AM_BOOL lastFragment) = 0;
	virtual CAutoString GetAuxSDPLine() const = 0;
	virtual void FeedStreamData(AM_U8* inputBuf, AM_U32 inputBufSize);
	CAutoString RtpmapLine() const;
	AM_U8 GetRtpPayloadType() const { return _rtpPayloadType; }
	AM_U16 GetCurrentSeqNo() const { return _seqNo; }
	AM_U32 GetCurrentTimestamp() const { return _timeStamp; }

protected:
	CamRtpSink(AM_U8 rtpPayloadType, AM_U32 rtpTimestampFrequency,
		AM_U8 numChannels = 1):
		_seqNo(0), _timeStamp(0), _rtpPayloadType(rtpPayloadType), 
		_rtpTimestampFrequency(rtpTimestampFrequency), _numChannels(numChannels)
		{}	
	virtual ~CamRtpSink() {}
	AM_ERR Construct(char const *pRtpPayloadFmtName);

protected:
	AM_U16 _seqNo;
	AM_U32 _timeStamp;
	AM_U32 _ssrc;
	AM_U8 *_pCurrBuf;
	AM_U32 _currBufSize;
	AM_U8 _rtpPayloadType;
	
private:
	char _rtpPayloadFormatName[32];
	AM_U32 _rtpTimestampFrequency;
	AM_U32 _numChannels;
};

#endif
