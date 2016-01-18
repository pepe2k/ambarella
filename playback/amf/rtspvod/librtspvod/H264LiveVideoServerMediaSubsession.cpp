#include "H264LiveVideoServerMediaSubsession.hh"
#include "H264FramedLiveSource.hh"
#include "H264VideoStreamFramer.hh"
#include "H264VideoStreamDiscreteFramer.hh"

H264LiveVideoServerMediaSubsession*
H264LiveVideoServerMediaSubsession::createNew( UsageEnvironment& env,char const* fileName,Boolean reuseFirstSource ){
    return new H264LiveVideoServerMediaSubsession( env, fileName, reuseFirstSource );
}

H264LiveVideoServerMediaSubsession::H264LiveVideoServerMediaSubsession( UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource)
    : H264VideoFileServerMediaSubsession( env, fileName, reuseFirstSource ){
    printf("H264LiveVideoServerMediaSubsession::H264LiveVideoServerMediaSubsession\n");
}

H264LiveVideoServerMediaSubsession::~H264LiveVideoServerMediaSubsession(){
    printf("H264LiveVideoServerMediaSubsession::~H264LiveVideoServerMediaSubsession\n");
}

FramedSource* H264LiveVideoServerMediaSubsession::createNewStreamSource( unsigned clientSessionId, unsigned& estBitrate ){
    /* Remain to do : assign estBitrate */
    estBitrate = 1000; // kbps, estimate
    // Create the video source:
    H264FramedLiveSource* liveSource = H264FramedLiveSource::createNew(envir(), fFileName);
    if (liveSource == NULL){
        return NULL;
    }
    // Create a framer for the Video Elementary Stream:
    //return H264VideoStreamFramer::createNew(envir(), liveSource);
    H264VideoStreamDiscreteFramer *framer = H264VideoStreamDiscreteFramer::createNew(envir(), liveSource);
    framer->setSPSandPPS(liveSource->fSps(),liveSource->fSpsLen(),liveSource->fPps(),liveSource->fPpsLen());
    printf("H264LiveVideoServerMediaSubsession::createNewStreamSource OK\n");
    return framer;
}
