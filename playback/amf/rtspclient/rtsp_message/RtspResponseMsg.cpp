#if NEW_RTSP_CLIENT

#include <stdio.h>
#include "RtspResponseMsg.h"

int RtspResponseMsg::decode()
{
    if(decode_self()==-1)
        return -1;

    char code_buf[16]={0};
    if(sscanf(get_raw_buf().c_str(),"%*1[Rr]%*1[Tt]%*1[Ss]%*1[Pp]%*[^ ] %15s",code_buf)<1){
        //can't find response code!
        return -1;
    }
    code_=RtspUtil::getStatusCodeInNumber(code_buf);
    return 0;
}
const string & RtspResponseMsg::encode()
{
    if(status_=="")
    {
        set_raw_buf(RtspUtil::getVersion()+" "+RtspUtil::getStatusCodeInString(code_) +" "+RtspUtil::getStatusInString(code_)+"\r\n"+encode_self());
    }else{
        set_raw_buf(RtspUtil::getVersion()+" "+RtspUtil::getStatusCodeInString(code_) +" "+status_+"\r\n"+encode_self());
    }
    return get_raw_buf();
}
#endif //NEW_RTSP_CLIENT

