
#include "active_mdec_engine.h"
//Note: We all use queue_component demuxer or decoder.
//time info on gmf(engine-filter-dsp) are all based on 90000

///Note 7/18, videodecoder's pause must first be run before the net_demuxer's onHide.(onHide will release the bufferpool)
//time info on gmf(engine-filter-dsp) are all based on 90000, if has ffmpeg, then will be a change.


//8/7 Update the Audio system, audio will only fill data to AudioQ, and the efecte video only exist on bufferQ

//state cnt on demuxer:
//sentcnt: ++ after send a frame(The outside frame num), on current stage code, this is only note the frame send to general-filter.
//mcount: ++ when fill frame to bufferpool, increse from 0, the frame on bp is mcount+1
//buffer count: same vaule as mcount
//consume cnt: ++ when release a frame. should same with mcount.


//eos handle
//on demuxer->decoder: driverd by data_buffer_pool's eos buffer
//on decoder->renderer: send to general filter and call by general render

//switch
//on case local switch, the sd's audio should also be maintained. so no removeAudio during sd->hd.(if cur sd has audio, then remove it)
//while hd->sd will always need removeAudio

AM_ERR CActiveMDecEngine::HandleRBNoAudioLocal()
{
    if(mbOnNVR == AM_TRUE){
        AM_ERROR("Already on Nvr!\n");
        return ME_OK;
    }

    CDemuxerConfig* curConfig = NULL;
    AM_ERR err;
    CParam64 parVec(3);

    AM_ASSERT(mNvrAudioIndex < 0);
    //switch hd to cur sd
    err = mpVideoDecoder->QueryInfo(DEC_MAP(mHdInfoIndex), INFO_TIME_PTS, parVec);
    if(err != ME_OK){
        AM_ASSERT(0);
        parVec[0] = parVec[1] = 0;
    }
    AM_INFO("Info Dump for Stream %d:Feed Pts:%lld, Render Pts:%lld\n", DEC_MAP(mHdInfoIndex), parVec[0], parVec[1]);
    mpDemuxer->SeekTo(mSdInfoIndex, parVec[1]);
    curConfig = &(mpConfig->demuxerConfig[mSdInfoIndex]);
    curConfig->hided = AM_FALSE;
    mpDemuxer->Config(mSdInfoIndex); //msmall is flushed on sw, need not resume
    //switch
    AM_INFO("Switch to Dsp, Wait!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    mpConfig->generalCmd |= RENDEER_SPECIFY_CONFIG;
    mpConfig->specifyIndex = REN_MAP(mSdInfoIndex); //switch this udec instance
    mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = RENDER_SWITCH_BACK;
    mpRenderer->Config(REN_MAP(mSdInfoIndex));
    mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = 0;
    mpConfig->generalCmd &= ~RENDEER_SPECIFY_CONFIG;
    AM_INFO("Well!\n");
    //handle 4sd and hd
    DoResumeSmallDspMw();//1handle paused
    InfoHideAllSmall(AM_FALSE);
    curConfig = &(mpConfig->demuxerConfig[mHdInfoIndex]);
    curConfig->hided = AM_TRUE;
    mpDemuxer->Config(CMD_EACH_COMPONENT);

    if(mpMdecInfo->unitInfo[mHdInfoIndex].isPaused == AM_TRUE){
        AM_INFO("Paused will be resumed by switch back for Hd %d.\n", mHdInfoIndex);
        DoFlushSpecifyDspMw(mHdInfoIndex);
    }else{
        mpVideoDecoder->Pause(DEC_MAP(mHdInfoIndex));
        DoFlushSpecifyDspMw(mHdInfoIndex);
        mpVideoDecoder->Resume(DEC_MAP(mHdInfoIndex));
    }

    mpMdecInfo->isNvr = AM_TRUE;
    mpMdecInfo[mFullWinIndex]->isPaused = 0;
    mpMdecInfo[mFullWinIndex]->playStream = TYPE_VIDEO;
    //mpMdecInfo[mFullWinIndex]->fullWindow = AM_FALSE;
    mbOnNVR = AM_TRUE;

    UpdateNvrOsdInfo();
    return ME_OK;
}

AM_ERR CActiveMDecEngine::HandleSWNoAudioLocal(AM_INT winIndex)
{
    if(mbOnNVR == AM_FALSE){
        AM_ERROR("Todo Me, Support switch to other HD when palyback a HD.\n");
        return ME_ERROR;
    }

    CDemuxerConfig* curConfig = NULL;
    AM_INT i, sIndex, hdIndex;
    AM_INT flag = 0;
    AM_ERR err;
    CParam64 parVec(3);
    //AM_U64 targetMs;

    sIndex = InfoChangeWinIndex(winIndex);
    if(sIndex < 0)
        return ME_BAD_PARAM;
    //find the connect HD source
    hdIndex = InfoFindHDIndex(sIndex);
    if(hdIndex < 0){
        AM_ERROR("HD Source for source %d donot seted!\n", winIndex);
        return ME_CLOSED;
    }
    mNvrAudioIndex = mpConfig->GetAudioSource();
    AM_ASSERT(mNvrAudioIndex < 0);
    AM_INFO("Auto Switch Info: sIndex:%d, hdIndex:%d, Audio:%d\n", sIndex, hdIndex, mNvrAudioIndex);
    if(mpMdecInfo->unitInfo[sIndex].isPaused == AM_TRUE){
        AM_INFO("Paused will be resumed by switch for Nvr Windows %d.\n", sIndex);
        //mpMdecInfo->unitInfo[sIndex].isPaused = AM_FALSE;
    }

    //Switch then pause.

    //one: 4sd+1hd
    //hd change, cur hd bufferQ is empty, so it will wait until change cmd arrived.
    //seek hd
    err = mpVideoDecoder->QueryInfo(DEC_MAP(sIndex), INFO_TIME_PTS, parVec);
    if(err != ME_OK){
        AM_ASSERT(0);
        parVec[0] = parVec[1] = 0;
    }
    AM_INFO("Info Dump for Stream %d:Feed Pts:%lld, Render Pts:%lld\n", sIndex, parVec[0], parVec[1]);
    mpDemuxer->SeekTo(hdIndex, parVec[1]);
    curConfig = &(mpConfig->demuxerConfig[hdIndex]);
    curConfig->hided = AM_FALSE;
    mpDemuxer->Config(hdIndex); //think about hd change, no need mpvideodecoder->pause(hd);
    //switch
    //if(mpMdecInfo[hdIndex] && pause);
    AM_INFO("Switch to Dsp, Wait!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    mpConfig->generalCmd |= RENDEER_SPECIFY_CONFIG;
    mpConfig->specifyIndex = REN_MAP(sIndex); //switch this udec instance
    mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = RENDER_SWITCH_HD;
    mpRenderer->Config(REN_MAP(sIndex));
    mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = 0;
    mpConfig->generalCmd &= ~RENDEER_SPECIFY_CONFIG;
    AM_INFO("Well.\n");
    //process 4sd, cur sd +3sd
    DoPauseSmallDspMw();
    //mpVideoDecoder->Flush(sIndex);
    //mpRenderer->Flush(sIndex);
    //mpVideoDecoder->Flush(sIndex);
    //mpDemuxer->Flush(sIndex);
    InfoHideAllSmall(AM_TRUE);
    mpDemuxer->Config(CMD_EACH_COMPONENT);
    DoFlushSpecifyDspMw(sIndex);//handle pause/resume


    mpMdecInfo->isNvr = AM_FALSE;
    mpMdecInfo->unitInfo[hdIndex].playStream = TYPE_VIDEO;
    //update for mdecinfo
    mpMdecInfo[hdIndex]->isPaused = 0;
    mpMdecInfo[hdIndex]->playStream = TYPE_VIDEO;
    mpMdecInfo[hdIndex]->fullWindow = AM_TRUE;
    mHdInfoIndex = hdIndex;
    mSdInfoIndex = sIndex;
    mbOnNVR = AM_FALSE;

    UpdateNvrOsdInfo();
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoFlushSpecifyDspMw(AM_INT index)
{
    //no a winIndex
    MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[index]);
    mpRenderer->Flush(REN_MAP(index));
    mpVideoDecoder->Flush(DEC_MAP(index));
    mpDemuxer->Flush(index);
    if(curInfo->isPaused == AM_TRUE){
        curInfo->isPaused = AM_FALSE;//not need resume dsp after flush
        mpVideoDecoder->Resume(DEC_MAP(index));
    }

    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoPauseSmallDspMw()
{
    AM_INT i;
    AM_INT indexRen, indexDec;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE && curInfo->isPaused == AM_FALSE){
            curInfo->isPaused = AM_TRUE;
            //indexDec = mpConfig->decoderConfig[i].component;
            mpRenderer->Pause(DEC_MAP(i));
            //indexRen = mpConfig->rendererConfig[i].component;
            mpRenderer->Pause(REN_MAP(i));
        }
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::DoResumeSmallDspMw()
{
    AM_INT i;
    AM_INT indexRen, indexDec;
    MdecInfo::MdecUnitInfo* curInfo = NULL;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        curInfo = &(mpMdecInfo->unitInfo[i]);
        if(curInfo->isUsed == AM_TRUE && curInfo->is1080P == AM_FALSE && curInfo->isPaused == AM_TRUE){
            curInfo->isPaused = AM_FALSE;
            mpRenderer->Resume(DEC_MAP(i));
            mpRenderer->Resume(REN_MAP(i));
        }
    }
    return ME_OK;
}

AM_ERR CActiveMDecEngine::InitSourceConfigNet(AM_INT source, AM_INT flag)
{
    AM_INT index = mpConfig->curIndex;
    MdecInfo::MdecUnitInfo* curInfo = &(mpMdecInfo->unitInfo[index]);
    if(curInfo->isUsed == AM_TRUE)
        return ME_BUSY;
    curInfo->isUsed = AM_TRUE;
    curInfo->sourceGroup = source;

    //flag and mdecinfo handle
    AM_INT i = 0;
    if(flag & SOURCE_ENABLE_AUDIO){
        AM_INFO("Audio No support on Net playback!\n");
        flag &= ~SOURCE_ENABLE_AUDIO;
    }

    if((flag & SOURCE_ENABLE_AUDIO) == 0 && (flag & SOURCE_ENABLE_VIDEO) == 0)
        AM_ASSERT(flag & SOURCE_FULL_HD);

    if(flag & SOURCE_FULL_HD)
    {
        //set a hd to config windows.
        AM_INFO("SOURCE_FULL_HD\n");
        SharedResource* pS = (SharedResource* )mpConfig->oldCompatibility;
        DSPConfig* dc = &(pS->dspConfig);
        dc->hdWin = 1;
        //end set
        curInfo->playStream = TYPE_VIDEO;
        curInfo->is1080P = AM_TRUE;
        curInfo->isPaused = AM_FALSE;
    }else{
        curInfo->playStream = TYPE_VIDEO;
        curInfo->isPaused = AM_FALSE;
    }

    CDemuxerConfig* pDemConfig = &(mpConfig->demuxerConfig[index]);
    //CMapTable* pMap = &(mpConfig->indexTable[index]);
    pDemConfig->disableAudio = true;
    pDemConfig->disableVideo = false;
    if(mpMdecInfo[index]->is1080P == AM_TRUE){
        pDemConfig->hided = AM_TRUE;
        pDemConfig->hd = AM_TRUE;
    }
    pDemConfig->configIndex = index;
    mpConfig->InitMapTable(index, mpCurSource);

    return ME_OK;
}


AM_ERR CActiveMDecEngine::HandleStartNet()
{
    AM_ERR err;
    err = RunAllFilters();
    if (err != ME_OK) {
        AM_ERROR("Run all filter Failed!\n");
        ClearGraph();
        return err;
    }

    AM_INT i;
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        if(mpMdecInfo[i]->isUsed == AM_TRUE){
            mpConfig->demuxerConfig[i].started = AM_TRUE;
        }
    }
    mpDemuxer->Config(CMD_EACH_COMPONENT);
    for(i = 0; i < MDEC_SOURCE_MAX_NUM; i++){
        if(mpMdecInfo[i]->isUsed == AM_TRUE){
            mpConfig->demuxerConfig[i].started = AM_FALSE;
        }
    }

    SetState(STATE_ONRUNNING);
    return ME_OK;
}


//FOR NET, Seem
AM_ERR CActiveMDecEngine::HandlePausePlay(AM_INT winIndex)
{



    return ME_OK;
}
























AM_ERR CActiveMDecEngine::HandleSWNoAudioStreaming(AM_INT winIndex)
{
    if(mbOnNVR == AM_FALSE){
        AM_ERROR("Todo Me, Support switch to other HD when palyback a HD.\n");
        return ME_ERROR;
    }

    CDemuxerConfig* curConfig = NULL;
    AM_INT i, sIndex, hdIndex;
    AM_INT flag = 0;
    sIndex = InfoChangeWinIndex(winIndex);
    if(sIndex < 0)
        return ME_BAD_PARAM;
    //find the connect HD source
    hdIndex = InfoFindHDIndex(sIndex);
    if(hdIndex < 0){
        AM_ERROR("HD Source for source %d donot seted!\n", winIndex);
        return ME_CLOSED;
    }
    mNvrAudioIndex = mpConfig->GetAudioSource();
    AM_ASSERT(mNvrAudioIndex < 0);
    AM_INFO("Auto Switch Info: sIndex:%d, hdIndex:%d, Audio:%d\n", sIndex, hdIndex, mNvrAudioIndex);

    //Switch then pause.

    //one: 4sd+1hd
    //hd change, cur hd bufferQ is empty, so it will wait until change cmd arrived.
    curConfig = &(mpConfig->demuxerConfig[hdIndex]);
    curConfig->hided = AM_FALSE;
    mpDemuxer->Config(hdIndex); //think about hd change, no need mpvideodecoder->pause(hd);
    //switch
    AM_INFO("Switch to Dsp, Wait!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    mpConfig->generalCmd |= RENDEER_SPECIFY_CONFIG;
    mpConfig->specifyIndex = sIndex; //switch this udec instance
    mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = RENDER_SWITCH_HD;
    mpRenderer->Config(sIndex);
    mpConfig->rendererConfig[mpConfig->specifyIndex].configCmd = 0;
    mpConfig->generalCmd &= ~RENDEER_SPECIFY_CONFIG;
    AM_INFO("Well.\n");
    //process 4sd
    InfoHideAllSmall(AM_TRUE);
    mpDemuxer->Config(CMD_EACH_COMPONENT);//no pause decoder strem playback
    DoFlushSmallStream();

    return ME_OK;
}

