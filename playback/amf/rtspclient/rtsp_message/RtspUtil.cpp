#if NEW_RTSP_CLIENT

#include <string.h>
#include "RtspUtil.h"

string RtspUtil::myMethods[] =
{
    string(""),
    string("ANNOUNCE"),
    string("DESCRIBE"),
    string("PLAY"),
    string("RECORD"),
    string("SETUP"),
    string("TEARDOWN"),
    string("PAUSE"),

    string("GET_PARAMETER"),
    string("OPTIONS"),
    string("REDIRECT"),
    string("SET_PARAMETER"),
    string("")
};

string RtspUtil::myHeaders[] =
{
    string("Accept"),
    string("CSeq"),
    string("User-Agent"),
    string("Range"),
    string("Session"),
    string("Transport"),
    string("Content-Base"),
    string("Content-Length"),
    string("Content-Type"),
    string("Date"),
    string("RTP-Info"),
    string("Public"),
    string("")

};

int RtspUtil::myStatusCodes[] =
{
    0,

    100, //Continue

    200, //OK
    201, //Created
    250, //Low on Storage Space

    300, //Multiple Choices
    301, //Moved Permanently
    302, //Moved Temporarily
    303, //See Other
    305, //Use Proxy

    400, //Bad Request
    401, //Unauthorized
    402, //Payment Required
    403, //Forbidden
    404, //Not Found
    405, //Method Not Allowed
    406, //Not Acceptable
    407, //Proxy Authentication Required
    408, //Request Time-out
    410, //Gone

    411, //Length Required
    412, //Precondition Failed
    413, //Request Entity Too Large
    414, //Request-URI Too Large
    415, //Unsupported Media Type

    451, //Parameter Not Understood
    452, //Conference Not Found
    453, //Not Enough Bandwidth
    454, //Session Not Found
    455, //Method Not Valid in this State
    456, //Header Field Not Valid For Resource
    457, //Invalid Range
    458, //Parameter Is Read-Only
    459, //Aggregate Option Not Allowed
    460, //Only Aggregate Option Allowed
    461, //Unsupported Transport
    462, //Destination Unreachable

    500, //Internal Server Error
    501, //Not Implemented
    502, //Bad Gateway
    503, //Service Unavailable
    504, //Gateway Timeout
    505, //RTSP Version not supported
    551, //Option Not Supported

    0
};

string RtspUtil::myStatusCodeStrings[] =
{
    string(""),

    string("100"), //Continue

    string("200"), //OK
    string("201"), //Created
    string("250"), //Low on Storage Space

    string("300"), //Multiple Choices
    string("301"), //Moved Permanently
    string("302"), //Moved Temporarily
    string("303"), //See Other
    string("305"), //Use Proxy

    string("400"), //Bad Request
    string("401"), //Unauthorized
    string("402"), //Payment Required
    string("403"), //Forbidden
    string("404"), //Not Found
    string("405"), //Method Not Allowed
    string("406"), //Not Acceptable
    string("407"), //Proxy Authentication Required
    string("408"), //Request Time-out
    string("410"), //Gone

    string("411"), //Length Required
    string("412"), //Precondition Failed
    string("413"), //Request Entity Too Large
    string("414"), //Request-URI Too Large
    string("415"), //Unsupported Media Type

    string("451"), //Parameter Not Understood
    string("452"), //Conference Not Found
    string("453"), //Not Enough Bandwidth
    string("454"), //Session Not Found
    string("455"), //Method Not Valid in this State
    string("456"), //Header Field Not Valid For Resource
    string("457"), //Invalid Range
    string("458"), //Parameter Is Read-Only
    string("459"), //Aggregate Option Not Allowed
    string("460"), //Only Aggregate Option Allowed
    string("461"), //Unsupported Transport
    string("462"), //Destination Unreachable

    string("500"), //Internal Server Error
    string("501"), //Not Implemented
    string("502"), //Bad Gateway
    string("503"), //Service Unavailable
    string("504"), //Gateway Timeout
    string("505"), //RTSP Version not supported
    string("551"), //Option Not Supported

    string("0")
};

string RtspUtil::myStatusStrings[] =
{
    string(""),

    string("Continue"),                                          //100

    string("OK"),                                                //200
    string("Created"),                                           //201
    string("Low on Storage Space"),                              //250

    string("Multiple Choices"),                                  //300
    string("Moved Permanently"),                                 //301
    string("Moved Temporarily"),                                 //302
    string("See Other"),                                         //303
    string("Use Proxy"),                                         //305

    string("Bad Request"),                                       //400
    string("Unauthorized"),                                      //401
    string("Payment Required"),                                  //402
    string("Forbidden"),                                         //403
    string("Not Found"),                                         //404
    string("Method Not Allowed"),                                //405
    string("Not Acceptable"),                                    //406
    string("Proxy Authentication Required"),                     //407
    string("Request Time-out"),                                  //408
    string("Gone"),                                              //410

    string("Length Required"),                                   //411
    string("Precondition Failed"),                               //412
    string("Request Entity Too Large"),                          //413
    string("Request-URI Too Large"),                             //414
    string("Unsupported Media Type"),                            //415

    string("Parameter Not Understood"),                          //451
    string("Conference Not Found"),                              //452
    string("Not Enough Bandwidth"),                              //453
    string("Session Not Found"),                                 //454
    string("Method Not Valid in this State"),                    //455
    string("Header Field Not Valid For Resource"),               //456
    string("Invalid Range"),                                     //457
    string("Parameter Is Read-Only"),                            //458
    string("Aggregate Option Not Allowed"),                      //459
    string("Only Aggregate Option Allowed"),                     //460
    string("Unsupported Transport"),                             //461
    string("Destination Unreachable"),                           //462

    string("Internal Server Error"),                             //500
    string("Not Implemented"),                                   //501
    string("Bad Gateway"),                                       //502
    string("Service Unavailable"),                               //503
    string("Gateway Timeout"),                                   //504
    string("RTSP Version not supported"),                        //505
    string("Option Not Supported"),                              //551

    string("")
};

string RtspUtil::myVersion[] =
{
    string("RTSP/1.0")
};

RtspHeadersType
RtspUtil::getHeaderInNumber(const char * hdrData)
{
    if (*hdrData=='\0')
        return RTSP_UNKNOWN_HDR;

    for (int i = 0; i < RTSP_UNKNOWN_HDR; i++)
    {
        if (strcasecmp(hdrData,myHeaders[i].c_str())==0)
            return (RtspHeadersType)i;
    }
    return RTSP_UNKNOWN_HDR;

}
RtspStatusCodesType RtspUtil::getStatusCodeInNumber(const char * codeStr)
{
    if (*codeStr == '\0')
        return RTSP_UNKNOWN_STATUS;

    for (int i = 0; i < RTSP_UNKNOWN_STATUS; i++)
    {
        if (strcasecmp(codeStr,myStatusCodeStrings[i].c_str())==0)
            return (RtspStatusCodesType)i;
    }
    return RTSP_UNKNOWN_STATUS;
}

RtspMethodsType
RtspUtil::getMethodInNumber(const char * methodData)
{
    if (*methodData== '\0')
        return RTSP_UNKNOWN_MTHD;

    for (int i = 0; i < RTSP_UNKNOWN_MTHD; i++)
    {
        if (strcasecmp(methodData,myMethods[i].c_str())==0)
            return (RtspMethodsType)i;
    }
    return RTSP_UNKNOWN_MTHD;

}
const string & RtspUtil::getMethodInStr(RtspMethodsType methodInNumber)
{
	return myMethods[methodInNumber];
}
const string & RtspUtil::getHeaderInStr(RtspHeadersType headerInNumer)
{
	return myHeaders[headerInNumer];
}
/* Local Variables: */
/* c-file-style: "stroustrup" */
/* indent-tabs-mode: nil */
/* c-file-offsets: ((access-label . -) (inclass . ++)) */
/* c-basic-offset: 4 */
/* End: */

#endif //NEW_RTSP_CLIENT

