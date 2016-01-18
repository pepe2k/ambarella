#ifndef _RTSP_UTIL_H_
#define _RTSP_UTIL_H_

#if NEW_RTSP_CLIENT

#include <string>
using std::string;

enum RtspMethodsType
{
    RTSP_NULL_MTHD = 0, // first several are most common ones
    RTSP_ANNOUNCE_MTHD,
    RTSP_DESCRIBE_MTHD,
    RTSP_PLAY_MTHD,
    RTSP_RECORD_MTHD,
    RTSP_SETUP_MTHD,
    RTSP_TEARDOWN_MTHD,
    RTSP_PAUSE_MTHD,

    RTSP_GET_PARAMETER_MTHD,
    RTSP_OPTIONS_MTHD,
    RTSP_REDIRECT_MTHD,
    RTSP_SET_PARAMETER_MTHD,
    RTSP_UNKNOWN_MTHD        // 12

};
    
enum RtspHeadersType
{
    RTSP_ACCEPT_HDR = 0, // first several are most common ones 
    RTSP_CSEQ_HDR,
    RTSP_USERAGENT_HDR,
    RTSP_RANGE_HDR,
    RTSP_SESSION_HDR,
    RTSP_TRANSPORT_HDR,
    RTSP_CONTENT_BASE_HDR,
    RTSP_CONTENT_LENGTH_HDR,
    RTSP_CONTENT_TYPE_HDR,
    RTSP_DATE_HDR,
    RTSP_RTPINFO_HDR,
    RTSP_PUBLIC_HDR,
    RTSP_UNKNOWN_HDR
};

enum RtspStatusCodesType
{
    RTSP_NULL_STATUS = 0,

    RTSP_100_STATUS,

    RTSP_200_STATUS,
    RTSP_201_STATUS,
    RTSP_250_STATUS,

    RTSP_300_STATUS,
    RTSP_301_STATUS,
    RTSP_302_STATUS,
    RTSP_303_STATUS,
    RTSP_305_STATUS,

    RTSP_400_STATUS,
    RTSP_401_STATUS,
    RTSP_402_STATUS,
    RTSP_403_STATUS,
    RTSP_404_STATUS,
    RTSP_405_STATUS,
    RTSP_406_STATUS,
    RTSP_407_STATUS,
    RTSP_408_STATUS,
    RTSP_410_STATUS,

    RTSP_411_STATUS,
    RTSP_412_STATUS,
    RTSP_413_STATUS,
    RTSP_414_STATUS,
    RTSP_415_STATUS,

    RTSP_451_STATUS,
    RTSP_452_STATUS,
    RTSP_453_STATUS,
    RTSP_454_STATUS,
    RTSP_455_STATUS,
    RTSP_456_STATUS,
    RTSP_457_STATUS,
    RTSP_458_STATUS,
    RTSP_459_STATUS,
    RTSP_460_STATUS,
    RTSP_461_STATUS,
    RTSP_462_STATUS,

    RTSP_500_STATUS,
    RTSP_501_STATUS,
    RTSP_502_STATUS,
    RTSP_503_STATUS,
    RTSP_504_STATUS,
    RTSP_505_STATUS,
    RTSP_551_STATUS,

    RTSP_UNKNOWN_STATUS //45

};


/** */
class RtspUtil{
public:
    /** */
    static RtspMethodsType getMethodInNumber(const char *methodInStr);
	static const string & getMethodInStr(RtspMethodsType methodInNumber);
    /** */
    static RtspHeadersType getHeaderInNumber(const char * headerInStr);
    static const string & getHeaderInStr(RtspHeadersType headerInNumer);
    /** */
    static int getStatusRealCodeInNumber(RtspStatusCodesType code){ return myStatusCodes[code]; }
	static RtspStatusCodesType getStatusCodeInNumber(const char * codeStr);
    /** */
    static string& getStatusCodeInString(RtspStatusCodesType code){ return myStatusCodeStrings[code]; }
    /** */
    static string& getStatusInString(RtspStatusCodesType code){ return myStatusStrings[code]; }
    /** */
	static string& getVersion(){ return myVersion[0];}

private:
    /** */
    static string myMethods[];
    /** */
    static string myHeaders[];
    /** */
    static int myStatusCodes[];
    /** */
    static string myStatusCodeStrings[];
    /** */
    static string myStatusStrings[];
    /** */
    static string myVersion[];
};

#endif//NEW_RTSP_CLIENT

#endif
