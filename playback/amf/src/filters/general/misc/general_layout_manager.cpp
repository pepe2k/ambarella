/*
 * general_layout_manager.cpp
 *
 * History:
 *    2012/12/20 - [QingXiong Z] create file
 *
 * Copyright (C) 2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "general_header.h"
#include "general_interface.h"
#include "general_parse.h"
#include "am_mdec_if.h"

#include "general_layout_manager.h"


static AM_INT SdNumFullScreenOnTable(AM_INT dsp, AM_BOOL noHd)
{
    AM_INT sdUdec, sdWin;
    sdUdec = dsp - (noHd?0:1);
    if(sdUdec <= 0)
        return ME_BAD_PARAM;
    if (sdUdec == 1) {
        sdWin = 1;
    } else if (sdUdec <= 4) {
        sdWin = 4;
    } else if (sdUdec <= 6) {
        sdWin = 6;
    } else if (sdUdec <= 9){
        sdWin = 9;
    } else {
        sdWin = 16;
    }
    return sdWin;
}
//---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------
CGeneralLayoutManager* CGeneralLayoutManager::Create(CGConfig* pConfig, MdecInfo* pInfo)
{
    CGeneralLayoutManager* result = new CGeneralLayoutManager(pConfig, pInfo);
    if (result && result->Construct() != ME_OK){
        delete result;
        result = NULL;
    }
    return result;
}

CGeneralLayoutManager::CGeneralLayoutManager(CGConfig* pConfig, MdecInfo* pInfo):
    mpGConfig(pConfig),
    mpInfo(pInfo),
    mCurLayout(LAYOUT_TYPE_TABLE),
    mWantLayout(LAYOUT_TYPE_TABLE)
{
    AM_INT i = 0;
     for(; i < MDEC_SOURCE_MAX_NUM; i++){
        mGroupOnWin[i] = i;
    }
}

AM_ERR CGeneralLayoutManager::Construct()
{
//    AM_INT hdWinIndex, sdWin, sdUdec;

    mbNoHd = mpGConfig->globalFlag & NO_HD_WIN_NVR_PB;
    mDspNum = mpGConfig->dspConfig.dspNumRequested;
    mCurHdWin = SdNumFullScreenOnTable(mDspNum, mbNoHd); //from 0
    return ME_OK;
}

void CGeneralLayoutManager::Delete()
{
    CObject::Delete();
}

CGeneralLayoutManager::~CGeneralLayoutManager()
{
}

//the sourcIndex on group sourceGroup will be assigned to which window, and use which dsp render.
AM_ERR CGeneralLayoutManager::InitWinRenMap(AM_INT sourceIndex, AM_INT sourceGroup, AM_INT flag)
{
    if(flag & SOURCE_FULL_HD){
        mpGConfig->indexTable[sourceIndex].winIndex = mCurHdWin;
        mpGConfig->indexTable[sourceIndex].dsprenIndex = 0;
    }else{
        mpGConfig->indexTable[sourceIndex].winIndex = sourceGroup;
        mpGConfig->indexTable[sourceIndex].dsprenIndex = sourceGroup;
    }
    return ME_OK;
}

LAYOUT_TYPE CGeneralLayoutManager::GetLayoutType()
{

    return mCurLayout;
}

AM_ERR CGeneralLayoutManager::SetLayoutType(LAYOUT_TYPE type, AM_INT index)
{
    AM_ERR err = ME_OK;
    if(type == mCurLayout){
        AM_INFO("Layout already on Type:%d\n", type);
        //return ME_OK;
    }
    mWantLayout = type;
    err = HandleReLayout(type, index);
    if(err != ME_OK){
        AM_ASSERT(0);
    }else{
        mCurLayout = mWantLayout;
    }
    return ME_OK;
}

AM_ERR CGeneralLayoutManager::SetPreLayoutType(LAYOUT_PRE_TYPE type, AM_INT index)
{
    AM_ERR err = ME_OK;
    if(mbNoHd){
        AM_INFO("No HD, do not use pre-layout..\n");
        return ME_OK;
    }
    if((type == LAYOUT_TYPE_PRE_TABLE) && (mCurLayout == LAYOUT_TYPE_TABLE)){
        AM_INFO("Layout already on Type:%d\n", mCurLayout);
        return ME_OK;
    }
    if((type == LAYOUT_TYPE_PRE_TELECONFERENCE) && (mCurLayout == LAYOUT_TYPE_TELECONFERENCE)){
        AM_INFO("Layout already on Type:%d\n", mCurLayout);
        return ME_OK;
    }
    err = HandlePreLayout(type, index);
    if(err != ME_OK){
        AM_ASSERT(0);
    }else{
    }
    return ME_OK;
}

AM_ERR CGeneralLayoutManager::CheckActionByLayout(ACTION_TYPE type, AM_INT index)
{
    AM_INFO("CheckActionByLayout, action:%d, winIndex:%d\n", type, index);
    AM_ERR err = ME_OK;
    switch(type)
    {
    case ACTION_SWITCH:
        if(mCurLayout == LAYOUT_TYPE_TELECONFERENCE)
            //if(index == 0)
            //    err = ME_CLOSED;
        break;

    case ACTION_BACK:
        if(mCurLayout == LAYOUT_TYPE_TELECONFERENCE)
            err = ME_CLOSED;
        break;

    default:
        AM_ERROR("ACTION FAILED!");
        break;
    }
    return err;
}


AM_ERR CGeneralLayoutManager::SyncLayoutByAction(ACTION_TYPE type, AM_INT sourceGroup)
{
    AM_INFO("SyncLayoutByAction, action:%d, sourceGroup:%d\n", type, sourceGroup);
    AM_ERR err=ME_OK;
    switch(type)
    {
    case ACTION_SWITCH:
        err = HandleSyncLayoutSwitch(sourceGroup);
        break;

    case ACTION_BACK:
        err = HandleSyncLayoutBack(sourceGroup);
        break;

    case ACTION_PRE_SWITCH:
        err = HandleSyncLayoutPreSwitch(sourceGroup);
        break;

    default:
        AM_ERROR("ACTION FAILED!");
        break;
    }
    AM_VERBOSE("err=%d\n", err);
    return ME_OK;
}

AM_INT CGeneralLayoutManager::RenderSourceNum(ACTION_TYPE type)
{
    AM_INT showNum = 0;
    switch(type)
    {
    case ACTION_SWITCH:
        //err = HandleSyncLayoutSwitch(sourceGroup);
        break;

    case ACTION_BACK:
        if(mCurLayout == LAYOUT_TYPE_TABLE){
            showNum = mDspNum - (mbNoHd?0:1);
        }
        break;

    default:
        AM_ERROR("ACTION FAILED!");
        break;
    }
    AM_INFO("RenderSourceNum showNum render:%d\n", showNum);
    return showNum;
}

AM_ERR CGeneralLayoutManager::HandleSyncLayoutBack(AM_INT sourceGroup)
{
    AM_INT sourceIndex;//, sGroup
    AM_INT i, num;
    num = RenderSourceNum(ACTION_BACK);
    if(mCurLayout == LAYOUT_TYPE_TABLE)
    {
        for(i = 0; i < num; i++){
            sourceIndex = mpInfo->FindSdBySourceGroup(mGroupOnWin[i]);
            BindSourceWindow(sourceIndex, i, i);
        }
    }else if(mCurLayout == LAYOUT_TYPE_TELECONFERENCE){




    }
    UpdateLayout();
    return ME_OK;
}

AM_ERR CGeneralLayoutManager::HandleSyncLayoutPreSwitch(AM_INT sourceGroup)
{
    AM_INFO("HandleSyncLayoutPreSwitch mCurLayout %d, sourceGroup:%d\n", mCurLayout, sourceGroup);

    if(mbNoHd){
        AM_INFO("No HD streams.\n");
        return ME_OK;
    }
    if(mCurLayout == LAYOUT_TYPE_TABLE){
        BindSourceWindow(mpInfo->FindSdBySourceGroup(sourceGroup), mCurHdWin, 0);
    }
    if(mCurLayout == LAYOUT_TYPE_TELECONFERENCE){
        AM_INT curHDIndex = mpInfo->FindHdBySourceGroup(mGroupOnWin[0]);
        AM_INT tmpIndex = sourceGroup;
        AM_INT tmpWin = mpGConfig->indexTable[tmpIndex].winIndex;
        AM_INT tmpRen = -1;
        for(AM_INT i = 1; i < mDspNum-1; i++){
            if(mpGConfig->indexTable[i].winIndex == tmpWin){
                tmpRen = mpGConfig->indexTable[i].dsprenIndex;
                break;
            }
        }
        if(tmpRen == -1){
            AM_ERROR("BindSourceWindow: Cannot find window %d.\n", tmpWin);
            tmpRen = mpGConfig->indexTable[tmpIndex].dsprenIndex;
        }

        //BindSourceWindow(tmpIndex, curHDIndex, 0);
        BindSourceWindow(tmpIndex, 0, 0);
        //mpGConfig->indexTable[tmpIndex].dsprenIndex = tmpRen;
        AM_INFO("BindSourceWindow[%d] Done, dsprenIndex: %d, dspIndex:%d, tmpWin: %d\n",
            tmpIndex,
            mpGConfig->indexTable[tmpIndex].dsprenIndex,
            mpGConfig->indexTable[tmpIndex].dspIndex,
            tmpWin);
        for(AM_INT i = 1; i < mDspNum-1; i++){
            if(i == tmpWin){
                //BindSourceWindow(curHDIndex, i, i);
                BindSourceWindow(curHDIndex, tmpWin, tmpRen);
                //mpGConfig->indexTable[curHDIndex].dsprenIndex = 0;
                AM_INFO("BindSourceWindow curHDIndex[%d] Done, dsprenIndex: %d, dspIndex:%d\n",
                    curHDIndex,
                    mpGConfig->indexTable[curHDIndex].dsprenIndex,
                    mpGConfig->indexTable[curHDIndex].dspIndex);
            }else{
                mpGConfig->dspRenConfig.renNumNeedConfig++;
            }
            AM_INFO("BindSourceWindow[%d] Done, dsprenIndex: %d, dspIndex:%d\n", i,
                mpGConfig->indexTable[i].dsprenIndex,
                mpGConfig->indexTable[i].dspIndex);
        }
    }
    UpdateLayout();
    AM_INFO("HandleSyncLayoutPreSwitch mCurLayout %d, sourceGroup:%d done!!\n", mCurLayout, sourceGroup);
    return ME_OK;
}

AM_ERR CGeneralLayoutManager::HandleSyncLayoutSwitch(AM_INT sourceGroup)
{
    AM_INT sourceIndex;
    AM_INT i, tmpWin, tmpIndex;
    AM_INT sIndex;//, hIndex
    if(mCurLayout == LAYOUT_TYPE_TABLE)
    {
        if(mbNoHd)
            sourceIndex = mpInfo->FindSdBySourceGroup(sourceGroup);
        else
            sourceIndex = mpInfo->FindHdBySourceGroup(sourceGroup);
        BindSourceWindow(sourceIndex, mCurHdWin, 0);
    }else if((mCurLayout == LAYOUT_TYPE_TELECONFERENCE) || (mCurLayout == LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN)){
        //sourceGroup to HD
        AM_INT num;
        tmpIndex = mpInfo->FindSdBySourceGroup(mGroupOnWin[0]);
        sIndex = mpInfo->FindSdBySourceGroup(sourceGroup);
        tmpWin = mpGConfig->indexTable[sIndex].winIndex;
        AM_INFO("Bind Dump: sIndex:%d, tmpIndex:%d, tmpWin:%d\n", sIndex, tmpIndex, tmpWin);
        if(mbNoHd){
            BindSourceWindow(mpInfo->FindSdBySourceGroup(sourceGroup), mCurHdWin, 0);
            num = mDspNum;
        }else{
            BindSourceWindow(mpInfo->FindHdBySourceGroup(sourceGroup), mCurHdWin, 0);
            num = mDspNum -1;
        }
        for(i = 1; i < num; i++)
        {
            if(i != tmpWin)
                BindSourceWindow(mpInfo->FindSdBySourceGroup(mGroupOnWin[i]), i, i);
            else
                BindSourceWindow(tmpIndex, i, i);
        }
    }
    //UpdateGroupWinMap();
    UpdateLayout();
    return ME_OK;
}

AM_ERR CGeneralLayoutManager::HandlePreLayout(LAYOUT_PRE_TYPE type, AM_INT index)
{
    AM_ERR err = ME_OK;
    switch (type) {
        case LAYOUT_TYPE_PRE_TABLE:
            err = HandleLayoutPreTable();
            break;
        case LAYOUT_TYPE_PRE_TELECONFERENCE:
            err = HandleReLayoutPreTeleconference(index);
            break;
        default:
            err = ME_NO_IMPL;
            AM_WARNING("HandlePreLayout: no implementation for pre layout:%d\n", type);
            break;
    }

    if (err != ME_OK) {
        AM_ERROR("HandlePreLayout: failure for pre layout:%d\n", type);
        return err;
    }

    return ME_OK;
}

AM_ERR CGeneralLayoutManager::HandleReLayout(AM_INT layout, AM_INT index)
{
    if (mCurLayout == layout) {
        AM_INFO("HandleReLayout: layout has been %d\n", layout);
        //return ME_OK;
    }
    AM_ERR err = ME_OK;

    if(mbNoHd){
        return HandleReLayoutNoHD((LAYOUT_TYPE)layout);
    }

    switch (layout) {
        case LAYOUT_TYPE_TABLE:
            err = HandleReLayoutTable();
            break;
        case LAYOUT_TYPE_TELECONFERENCE:
            err = HandleReLayoutTeleconference(index);
            break;
        case LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN:
            err = HandleReLayoutBottomleftHighlightenMode(index);
            break;
        case LAYOUT_TYPE_SINGLE_HD_FULLSCREEN:
            err = HandleReLayoutSingleHDFullScreen(index);
            break;
        default:
            err = ME_NO_IMPL;
            AM_WARNING("HandleReLayout: no implementation for layout:%d\n", layout);
            break;
    }

    if (err != ME_OK) {
        AM_ERROR("HandleReLayout: failure for layout:%d\n", layout);
        return err;
    }

    return ME_OK;
}

inline AM_BOOL CGeneralLayoutManager::IsShowOnLCD()
{
    AM_ASSERT(mpGConfig);

    return (AM_BOOL)(mpGConfig->globalFlag & NVR_ONLY_SHOW_ON_LCD);
}

AM_ERR CGeneralLayoutManager::BindSourceWindow(AM_INT sourceIndex, AM_INT winIndex, AM_INT renIndex)
{
    AM_INFO("BindSourceWindow, sourceIndex: %d, winIndex:%d, renIndex:%d\n",  sourceIndex, winIndex, renIndex);
    AM_INT dspIndex;
    if(sourceIndex < 0){
        //no source for this dsp yet.
        dspIndex = winIndex;
        if(winIndex == 0 && mWantLayout == LAYOUT_TYPE_TELECONFERENCE)
            dspIndex = mDspNum - 1;
    }else{
        dspIndex = mpGConfig->indexTable[sourceIndex].dspIndex;
    }
    CDspRenConfig* renConfig = &(mpGConfig->dspRenConfig);
    CSubDspRenConfig *pRenderConfig = &(renConfig->renConfig[renIndex]);
    pRenderConfig->renIndex = renIndex;
    pRenderConfig->winIndex = winIndex;
    pRenderConfig->dspIndex = dspIndex;
    pRenderConfig->winIndex2 = 0xff;
    renConfig->renNumNeedConfig++;
    renConfig->renChanged = AM_TRUE;

    if(sourceIndex >= 0){
        mpGConfig->indexTable[sourceIndex].dsprenIndex = renIndex;
        mpGConfig->indexTable[sourceIndex].winIndex = winIndex;
        mGroupOnWin[winIndex] = mpGConfig->indexTable[sourceIndex].sourceGroup;
    }
    AM_INFO("BindSourceWindow Done, renNum: %d, dspIndex:%d\n", renConfig->renNumNeedConfig, dspIndex);
    return ME_OK;
}

//switch to dspIndex(used for destIndex) on Render which sourceIndex used.
AM_ERR CGeneralLayoutManager::SwitchStream(AM_INT sourceIndex, AM_INT destIndex, AM_BOOL seamless)
{
    IUDECHandler* udecHandler = mpGConfig->dspConfig.udecHandler;
    AM_INT dspIndex = mpGConfig->indexTable[destIndex].dspIndex;
    udecHandler->SwitchStream(mpGConfig->indexTable[sourceIndex].dsprenIndex, dspIndex, seamless);
    return ME_OK;
}

AM_ERR CGeneralLayoutManager::UpdateLayout()
{
    IUDECHandler* udecHandler = mpGConfig->dspConfig.udecHandler;
    udecHandler->ConfigWindowRender(mpGConfig);
    return ME_OK;
}

AM_INT CGeneralLayoutManager::GetSourceGroupByWindow(AM_INT index)
{
    AM_INT sourceGroup = mGroupOnWin[index];
    AM_DBG("GetSourceGroupByWindow: win index:%d, sourceGroup:%d\n", index, sourceGroup);
    return sourceGroup;
}

AM_ERR CGeneralLayoutManager::ConfigRender(AM_INT render, AM_INT window, AM_INT dsp)
{
    //if(render < 0
    CDspRenConfig* renConfig = &(mpGConfig->dspRenConfig);
    CSubDspRenConfig *pRenderConfig = &(renConfig->renConfig[render]);
    pRenderConfig->renIndex = render;
    pRenderConfig->winIndex = window;
    pRenderConfig->dspIndex = dsp;
    pRenderConfig->winIndex2 = 0xff;
    renConfig->renNumNeedConfig++;
    renConfig->renChanged = AM_TRUE;
    //UpdateLayout();
    return ME_OK;
}

AM_ERR CGeneralLayoutManager::HideTeleSmallWindows(AM_BOOL en)
{
    if(mCurLayout != LAYOUT_TYPE_TELECONFERENCE){
        AM_ERROR("Layout is NOT Teleconference now!!\n");
        return ME_BAD_STATE;
    }

    if(en == AM_FALSE){
        mpGConfig->dspRenConfig.renNumNeedConfig = mDspNum -1;
    }else{
        mpGConfig->dspRenConfig.renNumNeedConfig = 1;
    }
    mpGConfig->dspRenConfig.renChanged = AM_TRUE;
    return ME_OK;
}

AM_ERR CGeneralLayoutManager::ConfigTargetWindow(AM_INT target, CParam* pParam)
{
    bool changed = AM_FALSE;
    CParam param = *pParam;

    AM_INT paraCnt = param.GetCount();
    AM_INT parType = param[paraCnt-1];
    if(parType >= LAYOUT_TELE_HIDE_SMALL){
        if(parType == LAYOUT_TELE_HIDE_SMALL)
            return HideTeleSmallWindows(AM_TRUE);
        if(parType == LAYOUT_TELE_SHOW_SMALL)
            return HideTeleSmallWindows(AM_FALSE);
        AM_ERROR("Get parType = %d???\n", parType);
    }

    AM_INT center_x = param[0];
    AM_INT center_y = param[1];
    AM_INT width = param[2];
    AM_INT height = param[3];

    AM_INT defDisplayWidth;
    AM_INT defDisplayHeight;
    SharedResource* oldShared = (SharedResource*)(mpGConfig->oldCompatibility);
    if (IsShowOnLCD()) {
        defDisplayWidth = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].height;
        defDisplayHeight = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].width;
    } else {
        defDisplayWidth = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].width;
        defDisplayHeight = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].height;
    }

    if ((!width) && (!height)) {
        width = defDisplayWidth;
        height = defDisplayHeight;
        AM_WARNING("choose full window, %dx%d\n", width, height);
    }


    if(param.CheckNum(4) != ME_OK)
        return ME_ERROR;

    CSubDspWinConfig *pWinConfig = &(mpGConfig->dspWinConfig.winConfig[target]);

    if((width > 0) && (height > 0)){
        if(width > defDisplayWidth)
            width = defDisplayWidth;
        if(height > defDisplayHeight)
            height = defDisplayHeight;

        if((pWinConfig->winOffsetX + width) > defDisplayWidth)
            pWinConfig->winOffsetX = (AM_U16)(defDisplayWidth - width);
        if((pWinConfig->winOffsetY + height) > defDisplayHeight)
            pWinConfig->winOffsetY = (AM_U16)(defDisplayHeight - height);
        pWinConfig->winWidth = (AM_U16)width & (~0x03);
        pWinConfig->winHeight = (AM_U16)height & (~0x03);

        changed = AM_TRUE;
    }

    if((center_x >= 0) && (center_y >= 0)){
        if(center_x < (AM_INT)(pWinConfig->winWidth >> 1)){
            center_x = (AM_INT)pWinConfig->winWidth >> 1;
        }else if((center_x + (AM_INT)(pWinConfig->winWidth >> 1)) > defDisplayWidth){
            center_x = defDisplayWidth - (AM_INT)(pWinConfig->winWidth >> 1);
        }
        if(center_y < (AM_INT)(pWinConfig->winHeight >> 1)){
            center_y = (AM_INT)pWinConfig->winHeight >> 1;
        }else if((center_y + (AM_INT)(pWinConfig->winHeight >> 1)) > defDisplayHeight){
            center_y = defDisplayHeight - (AM_INT)(pWinConfig->winHeight >> 1);
        }

        pWinConfig->winOffsetX = (AM_U16)(center_x - (AM_INT)(pWinConfig->winWidth >> 1))  & (~0x03);
        pWinConfig->winOffsetY = (AM_U16)(center_y - (AM_INT)(pWinConfig->winHeight >> 1))  & (~0x03);

        changed = AM_TRUE;
    }

    AM_INFO("target %d, window size %dx%d, offset %dx%d, changed=%d\n", target, pWinConfig->winWidth, pWinConfig->winHeight, pWinConfig->winOffsetX, pWinConfig->winOffsetY, changed);

    //change render order
    AM_INT sIndex;
    AM_INT wIndex, rIndex = 1;
    AM_INT targetorirIndex=0, targetdesrIndex=0;

    if (mCurHdWin == target) {
        if(!mbNoHd){
            sIndex = mpInfo->FindHdBySourceGroup(mGroupOnWin[target]);
        }else{
            sIndex = mpInfo->FindSdBySourceGroup(mGroupOnWin[target]);
        }
    }else sIndex = mpInfo->FindSdBySourceGroup(mGroupOnWin[target]);
    targetorirIndex = mpGConfig->indexTable[sIndex].dsprenIndex;

    for(wIndex = 1; wIndex < mDspNum -1; wIndex++){
        if(wIndex == target)
            continue;
        sIndex = mpInfo->FindSdBySourceGroup(mGroupOnWin[wIndex]);
        targetdesrIndex= mpGConfig->indexTable[sIndex].dsprenIndex;
        AM_INFO("target %d, render Index %d->%d, \n", sIndex, targetorirIndex, targetdesrIndex);
        if(targetdesrIndex>targetorirIndex) targetdesrIndex--;
        BindSourceWindow(sIndex, wIndex, targetdesrIndex);
        rIndex++;
    }
    if (mCurHdWin == target) {
        AM_INT hdIndex;
        AM_WARNING("MoveTargetWindow, can't move HD window!\n");
        if(!mbNoHd){
            hdIndex = mpInfo->FindHdBySourceGroup(mGroupOnWin[target]);
        }else{
            AM_WARNING("MoveTargetWindow, SD\n");
            hdIndex = mpInfo->FindSdBySourceGroup(mGroupOnWin[target]);
        }
        BindSourceWindow(hdIndex, target, 0);
    }else{
        sIndex = mpInfo->FindSdBySourceGroup(mGroupOnWin[target]);
        AM_INFO("target %d, render Index %d->%d, \n", sIndex, targetorirIndex, targetdesrIndex);
        BindSourceWindow(sIndex, target, rIndex);
    }
    mpGConfig->dspRenConfig.renNumNeedConfig = mDspNum -1;
    mpGConfig->dspWinConfig.winNumNeedConfig = mDspNum -1;
    mpGConfig->dspWinConfig.winChanged = AM_TRUE;

    return ME_OK;
}

AM_ERR CGeneralLayoutManager::HandleLayoutPreTable()
{
    AM_INT dspNum = mDspNum - 1;
    AM_INT winNum = dspNum;
    SharedResource* oldShared = (SharedResource*)(mpGConfig->oldCompatibility);
    AM_INT i;
    //TODO: merge with HandleLayoutTable()
    AM_INFO("HandleLayoutPreTable:\n");

    // config window
    {
        AM_INT defDisplayWidth;
        AM_INT defDisplayHeight;
        if (IsShowOnLCD()) {
            defDisplayWidth = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].height;
            defDisplayHeight = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].width;
        } else {
            defDisplayWidth = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].width;
            defDisplayHeight = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].height;
        }

        AM_INT subWinCnt = winNum;
        AM_INT groupCnt = 2;
        AM_INT winCntPerGroup = subWinCnt/groupCnt;
        AM_INT h = defDisplayHeight/groupCnt;
        AM_INT w = defDisplayWidth/winCntPerGroup;
        w = (w>>2)<<2;  // assure w is by 4 times
        if(winCntPerGroup*w < defDisplayWidth) w += 4;

        AM_INT winID = 0;
        CSubDspWinConfig *pWinConfig = &(mpGConfig->dspWinConfig.winConfig[0]);
        for (i = 0; i < groupCnt; i++) {
            for (int j = 0; j < winCntPerGroup; j++) {
                pWinConfig->winIndex = winID;
                //pWinConfig->winOffsetX = w*j;
                pWinConfig->winOffsetX = w*j - (((w*winCntPerGroup-defDisplayWidth)*j/(winCntPerGroup-1))&0xfffffffe);
                pWinConfig->winOffsetY = h*i;
                pWinConfig->winWidth = w;
                pWinConfig->winHeight = h;
                pWinConfig++;
                winID++;
            }
        }
        //I think the full hd windows should aslo assigned
        winNum++;
        pWinConfig->winIndex = winID;
        pWinConfig->winOffsetX =  0;
        pWinConfig->winOffsetY = 0;
        pWinConfig->winWidth = defDisplayWidth;
        pWinConfig->winHeight = defDisplayHeight;

        mpGConfig->dspWinConfig.winNumNeedConfig = winNum;
        mpGConfig->dspWinConfig.winChanged = AM_TRUE;
    }

    AM_INT hdStreaamIndex = mpInfo->FindHdBySourceGroup(mGroupOnWin[0]);
    AM_INT sIndex;
    BindSourceWindow(hdStreaamIndex, mGroupOnWin[0], mGroupOnWin[0]);
    for(i = 0; i < mDspNum -1; i++){
        if(i == mGroupOnWin[0]) continue;
        sIndex = mpInfo->FindSdBySourceGroup(mGroupOnWin[i]);
        BindSourceWindow(sIndex, i, i);
    }

    return ME_OK;
}

AM_ERR CGeneralLayoutManager::HandleReLayoutPreTeleconference(AM_INT index)
{
    AM_INT dspNum = mDspNum - 1;
    AM_INT winNum = dspNum;
    AM_INT i;
    //TODO: merge with HandleReLayoutTeleconference()
    AM_INFO("HandleReLayoutPreTeleconference:\n");

    // config window
    AM_INT defDisplayWidth;
    AM_INT defDisplayHeight;
    SharedResource* oldShared = (SharedResource*)(mpGConfig->oldCompatibility);
    if (IsShowOnLCD()) {
        defDisplayWidth = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].height;
        defDisplayHeight = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].width;
    } else {
        defDisplayWidth = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].width;
        defDisplayHeight = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].height;
    }

    if (winNum == 4) {
        CSubDspWinConfig *pWinConfig = &(mpGConfig->dspWinConfig.winConfig[0]);
        //const AM_INT defVerticalGap = 40;
        const AM_INT defHorizontalGap =  128;

        AM_INT winID = 0;
        pWinConfig->winIndex = winID;
        pWinConfig->winOffsetX = 0;
        pWinConfig->winOffsetY = 0;
        pWinConfig->winWidth = defDisplayWidth;
        pWinConfig->winHeight = defDisplayHeight;
        pWinConfig++;
        winID++;

        AM_INT subWinCnt = winNum - 1;
        AM_INT winCntPerGroup = subWinCnt;
        AM_INT w = (defDisplayWidth - (winCntPerGroup + 1)*defHorizontalGap)/winCntPerGroup;
        //AM_INT h = (defDisplayHeight - (winCntPerGroup + 1)*defVerticalGap)/winCntPerGroup;
        AM_INT h = w*9/16 &(~0x1);
        w = (w>>2)<<2;  // assure w is by 4 times

        for (i = 0; i < subWinCnt; i++) {
            pWinConfig->winIndex = winID;
            pWinConfig->winOffsetX = (w + defHorizontalGap)*i + defHorizontalGap;
            pWinConfig->winOffsetY = defDisplayHeight - h - defHorizontalGap/4;
            pWinConfig->winWidth = w;
            pWinConfig->winHeight = h;
            pWinConfig++;
            winID++;
        }

        mpGConfig->dspWinConfig.winNumNeedConfig = winNum;
        mpGConfig->dspWinConfig.winChanged = AM_TRUE;
    } else if (winNum == 6) {
        CSubDspWinConfig *pWinConfig = &(mpGConfig->dspWinConfig.winConfig[0]);
        const AM_INT defVerticalGap = 40;
        const AM_INT defHorizontalGap = 40;

        AM_INT subWinCnt = winNum - 1;
        AM_INT winCntPerGroup = (subWinCnt - 1)/2 + 1;
        AM_INT h = (defDisplayHeight - (winCntPerGroup + 1)*defVerticalGap)/winCntPerGroup;
        AM_INT w = (defDisplayWidth - (winCntPerGroup + 1)*defHorizontalGap)/winCntPerGroup;
        w = (w>>2)<<2;  // assure w is by 4 times

        AM_INT winID = 0;
        pWinConfig->winIndex = winID;
        pWinConfig->winOffsetX = 0;
        pWinConfig->winOffsetY = 0;
        pWinConfig->winWidth = defDisplayWidth;
        pWinConfig->winHeight = defDisplayHeight;
        pWinConfig++;
        winID++;

        // horizontal
        for (int i = 0; i < winCntPerGroup-1; i++) {
            pWinConfig->winIndex = winID;
            pWinConfig->winOffsetX = (w + defHorizontalGap)*(winCntPerGroup - 1) + defHorizontalGap;
            pWinConfig->winOffsetY = (h + defVerticalGap)*i + defVerticalGap;
            pWinConfig->winWidth = w;
            pWinConfig->winHeight = h;
            pWinConfig++;
            winID++;
        }

        // vertical
        for (int i = 0; i < winCntPerGroup; i++) {
            pWinConfig->winIndex = winID;
            pWinConfig->winOffsetX = (w + defHorizontalGap)*i + defHorizontalGap;
            pWinConfig->winOffsetY = (h + defVerticalGap)*(winCntPerGroup - 1) + defVerticalGap;
            pWinConfig->winWidth = w;
            pWinConfig->winHeight = h;
            pWinConfig++;
            winID++;
        }

        mpGConfig->dspWinConfig.winNumNeedConfig = winNum;
        mpGConfig->dspWinConfig.winChanged = AM_TRUE;
    } else {
        AM_ERROR("ReconfigWindowRenderLayoutTeleconference: doesn't handle winNum:%d\n", mpGConfig->dspWinConfig.winNumNeedConfig);
    }

    AM_INT sIndex;
    AM_INT prehdStreaamIndex = mpInfo->FindSdBySourceGroup(mGroupOnWin[index]);
    BindSourceWindow(prehdStreaamIndex, 0, 0);
    for(i = 0; i < mDspNum -1; i++){
        sIndex = mpInfo->FindSdBySourceGroup(i);
        if(i<index) BindSourceWindow(sIndex, i+1, i+1);
        else if(i==index) continue;
        else BindSourceWindow(sIndex, i, i);
    }
    return ME_OK;
}

AM_ERR CGeneralLayoutManager::HandleReLayoutSingleHDFullScreen(AM_INT index)
{
    return ME_OK;
}

AM_ERR CGeneralLayoutManager::HandleReLayoutBottomleftHighlightenMode(AM_INT index)
{
    AM_INT dspNum = mDspNum - 1;
    AM_INT winNum = dspNum;
    AM_INT i;
    // config window
    AM_INT win_width, win_height;
//    AM_INT highlighten_width;
//    AM_INT highlighten_height;

    AM_INT winID = 0;
    AM_INT top_num, right_num, top_right = 1;
    AM_INT top_height, right_width;
    AM_INT sub_width, sub_height;

    CSubDspWinConfig *pWinConfig = &(mpGConfig->dspWinConfig.winConfig[0]);

    AM_ASSERT(winNum > 0);
    if (winNum < 1) {
        AM_ERROR("BAD winNum %d\n", winNum);
        return ME_BAD_PARAM;
    } else if (winNum < 4) {
        AM_WARNING("it's better not choose bottom left highlighten mode when window number(%d) less than 4\n", winNum);
        if (winNum < 2) {
            top_right = 0;
            top_num = 0;
            right_num = 0;
        } else {
            top_right = 1;
            right_num = (winNum - 1 - top_right)/2;
            top_num = winNum - 1 - top_right - right_num;
        }
    } else {
            top_right = 1;
            right_num = (winNum - 1 - top_right)/2;
            top_num = winNum - 1 - top_right - right_num;
    }
    AM_WARNING("[layout]: bottom left, winNum %d, top_right %d, top_num %d, right_num %d\n", winNum, top_right, top_num, right_num);

    SharedResource* oldShared = (SharedResource*)(mpGConfig->oldCompatibility);
    if (IsShowOnLCD()) {
        win_width = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].height;
        win_height = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].width;
    } else {
        win_width = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].width;
        win_height = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].height;
    }

    top_height = (win_height/4) &0xfffffffe;
    right_width = (win_width/4) &0xfffffffe;

    //highlighten window
    pWinConfig->winIndex = winID;
    pWinConfig->winOffsetX = 0;
    pWinConfig->winOffsetY = top_height;
    pWinConfig->winWidth = win_width - right_width;
    pWinConfig->winHeight = win_height - top_height;
    pWinConfig++;
    winID++;

    //non highlighten window, top
    if (top_num) {
        sub_width = ((win_width - right_width)/top_num) & 0xfffffffe;
        for (i = 0; i < top_num; i++) {
            pWinConfig->winIndex = winID;
            pWinConfig->winOffsetX = sub_width * i;
            pWinConfig->winOffsetY = 0;
            if ((1 + i) != top_num) {
                pWinConfig->winWidth = sub_width;
            } else {
                pWinConfig->winWidth = (win_width - right_width) - ((top_num -1) * sub_width);
            }
            pWinConfig->winHeight = top_height;
            pWinConfig++;
            winID++;
        }
    }

    //non highlighten window, top right
    pWinConfig->winIndex = winID;
    pWinConfig->winOffsetX = win_width - right_width;
    pWinConfig->winOffsetY = 0;
    pWinConfig->winWidth = right_width;
    pWinConfig->winHeight = top_height;
    pWinConfig++;
    winID++;

    //non highlighten window, right
    if (right_num) {
        sub_height = ((win_height - top_height)/right_num) & 0xfffffffe;
        for (i = 0; i < right_num; i++) {
            pWinConfig->winIndex = winID;
            pWinConfig->winOffsetX = win_width - right_width;
            pWinConfig->winOffsetY = (sub_height * i) + top_height;
            if ((1 + i) != top_num) {
                pWinConfig->winHeight = sub_height;
            } else {
                pWinConfig->winHeight = (win_height - top_height) - ((top_num -1) * sub_height);
            }
            pWinConfig->winWidth = right_width;
            pWinConfig++;
            winID++;
        }
    }

    mpGConfig->dspWinConfig.winNumNeedConfig = winNum;
    mpGConfig->dspWinConfig.winChanged = AM_TRUE;

    AM_INT hdStreaamIndex = mpInfo->FindHdBySourceGroup(mGroupOnWin[0]);
    AM_INT sIndex;
    BindSourceWindow(hdStreaamIndex, 0, 0);
    for(i = 1; i < mDspNum -1; i++){
        sIndex = mpInfo->FindSdBySourceGroup(mGroupOnWin[i]);
        BindSourceWindow(sIndex, i, i);
    }

    //releated save
    mCurHdWin = 0;
    return ME_OK;
}

//window index:
//     0
// 1   2   3
AM_ERR CGeneralLayoutManager::HandleReLayoutTeleconference(AM_INT index)
{
    AM_INT dspNum = mDspNum - 1;
    AM_INT winNum = dspNum;
    AM_INT i;
    // config window
    AM_INT defDisplayWidth;
    AM_INT defDisplayHeight;
    SharedResource* oldShared = (SharedResource*)(mpGConfig->oldCompatibility);
    if (IsShowOnLCD()) {
        defDisplayWidth = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].height;
        defDisplayHeight = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].width;
    } else {
        defDisplayWidth = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].width;
        defDisplayHeight = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].height;
    }

    if (winNum == 4) {
        CSubDspWinConfig *pWinConfig = &(mpGConfig->dspWinConfig.winConfig[0]);
        //const AM_INT defVerticalGap = 40;
        const AM_INT defHorizontalGap = 128;

        AM_INT winID = 0;
        pWinConfig->winIndex = winID;
        pWinConfig->winOffsetX = 0;
        pWinConfig->winOffsetY = 0;
        pWinConfig->winWidth = defDisplayWidth;
        pWinConfig->winHeight = defDisplayHeight;
        pWinConfig++;
        winID++;

        AM_INT subWinCnt = winNum - 1;
        AM_INT winCntPerGroup = subWinCnt;
        AM_INT w = (defDisplayWidth - (winCntPerGroup + 1)*defHorizontalGap)/winCntPerGroup;
        //AM_INT h = (defDisplayHeight - (winCntPerGroup + 1)*defVerticalGap)/winCntPerGroup;
        AM_INT h = w*9/16 &(~0x1);
        w = (w>>2)<<2;  // assure w is by 4 times

        for (i = 0; i < subWinCnt; i++) {
            pWinConfig->winIndex = winID;
            pWinConfig->winOffsetX = (w + defHorizontalGap)*i + defHorizontalGap;
            pWinConfig->winOffsetY = defDisplayHeight - h - defHorizontalGap/4;
            pWinConfig->winWidth = w;
            pWinConfig->winHeight = h;
            pWinConfig++;
            winID++;
        }

        mpGConfig->dspWinConfig.winNumNeedConfig = winNum;
        mpGConfig->dspWinConfig.winChanged = AM_TRUE;
    } else if (winNum == 6) {
        CSubDspWinConfig *pWinConfig = &(mpGConfig->dspWinConfig.winConfig[0]);
        const AM_INT defVerticalGap = 40;
        const AM_INT defHorizontalGap = 40;

        AM_INT subWinCnt = winNum - 1;
        AM_INT winCntPerGroup = (subWinCnt - 1)/2 + 1;
        AM_INT h = (defDisplayHeight - (winCntPerGroup + 1)*defVerticalGap)/winCntPerGroup;
        AM_INT w = (defDisplayWidth - (winCntPerGroup + 1)*defHorizontalGap)/winCntPerGroup;
        w = (w>>2)<<2;  // assure w is by 4 times

        AM_INT winID = 0;
        pWinConfig->winIndex = winID;
        pWinConfig->winOffsetX = 0;
        pWinConfig->winOffsetY = 0;
        pWinConfig->winWidth = defDisplayWidth;
        pWinConfig->winHeight = defDisplayHeight;
        pWinConfig++;
        winID++;

        // horizontal
        for (int i = 0; i < winCntPerGroup-1; i++) {
            pWinConfig->winIndex = winID;
            pWinConfig->winOffsetX = (w + defHorizontalGap)*(winCntPerGroup - 1) + defHorizontalGap;
            pWinConfig->winOffsetY = (h + defVerticalGap)*i + defVerticalGap;
            pWinConfig->winWidth = w;
            pWinConfig->winHeight = h;
            pWinConfig++;
            winID++;
        }

        // vertical
        for (int i = 0; i < winCntPerGroup; i++) {
            pWinConfig->winIndex = winID;
            pWinConfig->winOffsetX = (w + defHorizontalGap)*i + defHorizontalGap;
            pWinConfig->winOffsetY = (h + defVerticalGap)*(winCntPerGroup - 1) + defVerticalGap;
            pWinConfig->winWidth = w;
            pWinConfig->winHeight = h;
            pWinConfig++;
            winID++;
        }

        mpGConfig->dspWinConfig.winNumNeedConfig = winNum;
        mpGConfig->dspWinConfig.winChanged = AM_TRUE;
    } else {
        AM_ERROR("ReconfigWindowRenderLayoutTeleconference: doesn't handle winNum:%d\n", mpGConfig->dspWinConfig.winNumNeedConfig);
    }

    AM_INT hdStreaamIndex = mpInfo->FindHdBySourceGroup(index);
    AM_INT sIndex;
    BindSourceWindow(hdStreaamIndex, 0, 0);
    for(i = 0; i < mDspNum -1; i++){
        sIndex = mpInfo->FindSdBySourceGroup(i);
        if(i<index) BindSourceWindow(sIndex, i+1, i+1);
        else if(i==index) continue;
        else BindSourceWindow(sIndex, i, i);
    }

    //releated save
    mCurHdWin = 0;
    return ME_OK;
}

//Window Index
// 0    1  (HD on 4)
// 2    3
AM_ERR CGeneralLayoutManager::HandleReLayoutTable()
{
    AM_INT dspNum = mDspNum - 1;
    AM_INT winNum = dspNum;
    SharedResource* oldShared = (SharedResource*)(mpGConfig->oldCompatibility);
    AM_INT i;
    // config window
    {
        AM_INT defDisplayWidth;
        AM_INT defDisplayHeight;
        if (IsShowOnLCD()) {
            defDisplayWidth = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].height;
            defDisplayHeight = oldShared->dspConfig.voutConfigs.voutConfig[eVoutLCD].width;
        } else {
            defDisplayWidth = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].width;
            defDisplayHeight = oldShared->dspConfig.voutConfigs.voutConfig[eVoutHDMI].height;
        }

        AM_INT subWinCnt = winNum;
        AM_INT groupCnt = 2;
        AM_INT winCntPerGroup = subWinCnt/groupCnt;
        AM_INT h = defDisplayHeight/groupCnt;
        AM_INT w = defDisplayWidth/winCntPerGroup;
        w = (w>>2)<<2;  // assure w is by 4 times
        if(winCntPerGroup*w < defDisplayWidth) w += 4;

        AM_INT winID = 0;
        CSubDspWinConfig *pWinConfig = &(mpGConfig->dspWinConfig.winConfig[0]);
        for (i = 0; i < groupCnt; i++) {
            for (int j = 0; j < winCntPerGroup; j++) {
                pWinConfig->winIndex = winID;
                //pWinConfig->winOffsetX = w*j;
                pWinConfig->winOffsetX = w*j - (((w*winCntPerGroup-defDisplayWidth)*j/(winCntPerGroup-1))&0xfffffffe);
                pWinConfig->winOffsetY = h*i;
                pWinConfig->winWidth = w;
                pWinConfig->winHeight = h;
                pWinConfig++;
                winID++;
            }
        }
        //I think the full hd windows should aslo assigned
        winNum++;
        pWinConfig->winIndex = winID;
        pWinConfig->winOffsetX =  0;
        pWinConfig->winOffsetY = 0;
        pWinConfig->winWidth = defDisplayWidth;
        pWinConfig->winHeight = defDisplayHeight;

        mpGConfig->dspWinConfig.winNumNeedConfig = winNum;
        mpGConfig->dspWinConfig.winChanged = AM_TRUE;
    }
    AM_INT sIndex;
    for(i = 0; i < mDspNum -1; i++){
        sIndex = mpInfo->FindSdBySourceGroup(i);
        BindSourceWindow(sIndex, i, i);
    }
    mCurHdWin = SdNumFullScreenOnTable(mDspNum, mbNoHd); //from 0
    return ME_OK;
}


AM_ERR CGeneralLayoutManager::HandleReLayoutNoHD(LAYOUT_TYPE layout, AM_INT index)
{
    AM_ERR err = ME_OK;
    //todo: merge this api into HandleReLayoutNoHD
    AM_INFO("HandleReLayoutNoHD: mCurLayout:%d, change to: %d\n", mCurLayout, layout);

//    AM_ERR err = ME_ERROR;
    AM_INT dspNum = mDspNum;
    AM_INT winNum = dspNum;
    AM_INT VoutIndex = (mpGConfig->globalFlag & NVR_ONLY_SHOW_ON_LCD)? eVoutLCD : eVoutHDMI;
    AM_INT i;

    SharedResource* oldShared = (SharedResource*)(mpGConfig->oldCompatibility);

    DSPVoutConfig *pVoutConfig = &(oldShared->dspConfig.voutConfigs.voutConfig[VoutIndex]);
    AM_INT defDisplayWidth = (mpGConfig->globalFlag & NVR_ONLY_SHOW_ON_LCD) ? pVoutConfig->height : pVoutConfig->width;
    AM_INT defDisplayHeight = (mpGConfig->globalFlag & NVR_ONLY_SHOW_ON_LCD) ? pVoutConfig->width : pVoutConfig->height;
//    AM_INT focusWinIndex = 0;//hardcode here

    if(layout == LAYOUT_TYPE_TELECONFERENCE){
        AM_INFO("MD_LAYOUT_TELECONFERENCE\n");
        CSubDspWinConfig *pWinConfig = &(mpGConfig->dspWinConfig.winConfig[0]);
        const AM_INT defVerticalGap = (VoutIndex==eVoutLCD)?16:40;
        const AM_INT defHorizontalGap = (VoutIndex==eVoutLCD)?16:40;

        AM_INFO("pWinConfig:\n   index\tX\tY\tWidth\tHeight\n");
        AM_INT winID = 0;
        pWinConfig->winIndex = winID;
        pWinConfig->winOffsetX = 0;
        pWinConfig->winOffsetY = 0;
        pWinConfig->winWidth = defDisplayWidth;
        pWinConfig->winHeight = defDisplayHeight;
        AM_INFO("%d,\t%u,\t%u,\t%u,\t%u\n",
            pWinConfig->winIndex,pWinConfig->winOffsetX, pWinConfig->winOffsetY, pWinConfig->winWidth, pWinConfig->winHeight);
        pWinConfig++;
        winID++;

        AM_INT subWinCnt = winNum - 1;
        AM_INT winCntPerGroup = subWinCnt;
        AM_INT h = (defDisplayHeight - (winCntPerGroup + 1)*defVerticalGap)/winCntPerGroup;
        AM_INT w = (defDisplayWidth - (winCntPerGroup + 1)*defHorizontalGap)/winCntPerGroup;
        w = (w>>2)<<2;  // assure w is by 4 times

        for (i = 0; i < subWinCnt; i++) {
            pWinConfig->winIndex = winID;
            pWinConfig->winOffsetX = (w + defHorizontalGap)*i + defHorizontalGap;
            pWinConfig->winOffsetY = (h + defVerticalGap)*(winCntPerGroup - 1) + defVerticalGap;
            pWinConfig->winWidth = w;
            pWinConfig->winHeight = h;
            AM_INFO("%d,\t%u,\t%u,\t%u,\t%u\n",
                pWinConfig->winIndex,pWinConfig->winOffsetX, pWinConfig->winOffsetY, pWinConfig->winWidth, pWinConfig->winHeight);
            pWinConfig++;
            winID++;
        }

        mpGConfig->dspWinConfig.winNumNeedConfig = winNum;
        mpGConfig->dspWinConfig.winChanged = AM_TRUE;
    }else if(layout == LAYOUT_TYPE_TABLE){
        AM_INFO("MD_LAYOUT_TABLE\n");

        AM_INT subWinCnt = winNum;
        AM_INT groupCnt = 2;
        AM_INT winCntPerGroup = subWinCnt/groupCnt;
        AM_INT h = defDisplayHeight/groupCnt;
        AM_INT w = defDisplayWidth/winCntPerGroup;
        w = (w>>2)<<2;  // assure w is by 4 times

        AM_INT winID = 0;
        CSubDspWinConfig *pWinConfig = &(mpGConfig->dspWinConfig.winConfig[0]);
        AM_INFO("pWinConfig:\nindex\tX\tY\tWidth\tHeight\n");
        for (int i = 0; i < groupCnt; i++) {
            for (int j = 0; j < winCntPerGroup; j++) {
                pWinConfig->winIndex = winID;
                pWinConfig->winOffsetX = w*j;
                pWinConfig->winOffsetY = h*i;
                pWinConfig->winWidth = w;
                pWinConfig->winHeight = h;
                AM_INFO("%d,\t%u,\t%u,\t%u,\t%u\n",
                    pWinConfig->winIndex,pWinConfig->winOffsetX, pWinConfig->winOffsetY, pWinConfig->winWidth, pWinConfig->winHeight);
                pWinConfig++;
                winID++;
            }
        }

        mpGConfig->dspWinConfig.winNumNeedConfig = winNum;
        mpGConfig->dspWinConfig.winChanged = AM_TRUE;

    }else if(layout == LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN){
        AM_INFO("LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN\n");

        AM_INT i;
        // config window
        AM_INT win_width = defDisplayWidth, win_height = defDisplayHeight;
//        AM_INT highlighten_width;
//        AM_INT highlighten_height;

        AM_INT winID = 0;
        AM_INT top_num, right_num, top_right = 1;
        AM_INT top_height, right_width;
        AM_INT sub_width, sub_height;
        CSubDspWinConfig *pWinConfig = &(mpGConfig->dspWinConfig.winConfig[0]);

        AM_ASSERT(winNum > 0);
        if (winNum < 1) {
            AM_ERROR("BAD winNum %d\n", winNum);
            return ME_BAD_PARAM;
        } else if (winNum < 4) {
            AM_WARNING("it's better not choose bottom left highlighten mode when window number(%d) less than 4\n", winNum);
            if (winNum < 2) {
                top_right = 0;
                top_num = 0;
                right_num = 0;
            } else {
                top_right = 1;
                right_num = (winNum - top_right -1)/2;
                top_num = winNum - top_right -1 - right_num;
            }
        } else {
                top_right = 1;
                right_num = (winNum - top_right -1)/2;
                top_num = winNum - top_right -1 - right_num;
        }
        AM_WARNING("[layout]: bottom left, winNum %d, top_right %d, top_num %d, right_num %d\n", winNum, top_right, top_num, right_num);

        top_height = (win_height/4) &0xfffffffe;
        right_width = (win_width/4) &0xfffffffe;

        //highlighten window
        pWinConfig->winIndex = winID;
        pWinConfig->winOffsetX = 0;
        pWinConfig->winOffsetY = top_height;
        pWinConfig->winWidth = win_width - right_width;
        pWinConfig->winHeight = win_height - top_height;
        pWinConfig++;
        winID++;

        //non highlighten window, top
        if (top_num) {
            sub_width = ((win_width - right_width)/top_num) & 0xfffffffe;
            for (i = 0; i < top_num; i++) {
                pWinConfig->winIndex = winID;
                pWinConfig->winOffsetX = sub_width * i;
                pWinConfig->winOffsetY = 0;
                if ((1 + i) != top_num) {
                    pWinConfig->winWidth = sub_width;
                } else {
                    pWinConfig->winWidth = (win_width - right_width) - ((top_num -1) * sub_width);
                }
                pWinConfig->winHeight = top_height;
                pWinConfig++;
                winID++;
            }
        }

        //non highlighten window, top right
        pWinConfig->winIndex = winID;
        pWinConfig->winOffsetX = win_width - right_width;
        pWinConfig->winOffsetY = 0;
        pWinConfig->winWidth = right_width;
        pWinConfig->winHeight = top_height;
        pWinConfig++;
        winID++;

        //non highlighten window, right
        if (right_num) {
            sub_height = ((win_height - top_height)/right_num) & 0xfffffffe;
            for (i = 0; i < right_num; i++) {
                pWinConfig->winIndex = winID;
                pWinConfig->winOffsetX = win_width - right_width;
                pWinConfig->winOffsetY = (sub_height * i) + top_height;
                if ((1 + i) != top_num) {
                    pWinConfig->winHeight = sub_height;
                } else {
                    pWinConfig->winHeight = (win_height - top_height) - ((top_num -1) * sub_height);
                }
                pWinConfig->winWidth = right_width;
                pWinConfig++;
                winID++;
            }
        }

        mpGConfig->dspWinConfig.winNumNeedConfig = winNum;
        mpGConfig->dspWinConfig.winChanged = AM_TRUE;

    }else{
        AM_ERROR("unsupported layout: %d.\n", layout);
        err = ME_NO_IMPL;
    }

    AM_INT sIndex;
    if(layout == LAYOUT_TYPE_TABLE){
        for(i = 0; i < mDspNum; i++){
            sIndex = mpInfo->FindSdBySourceGroup(i);
            BindSourceWindow(sIndex, i, i);
        }
        mCurHdWin = SdNumFullScreenOnTable(mDspNum, mbNoHd);
    }else if(layout == LAYOUT_TYPE_TELECONFERENCE){
        for(i = 0; i < mDspNum; i++){
            sIndex = mpInfo->FindSdBySourceGroup(i);
            BindSourceWindow(sIndex, i, i);
        }
        mCurHdWin = 0;
    }else if(layout == LAYOUT_TYPE_BOTTOM_LEFT_HIGHLIGHTEN){
        for(i = 0; i < mDspNum; i++){
            sIndex = mpInfo->FindSdBySourceGroup(i);
            BindSourceWindow(sIndex, i, i);
        }
        mCurHdWin = 0;
    }else{
        AM_ASSERT(0);
        err = ME_NO_IMPL;
    }

    return err;
}

AM_ERR CGeneralLayoutManager::Dump(AM_INT flag)
{
    AM_INT i;
    const char* name;
    switch(mCurLayout)
    {
    case LAYOUT_TYPE_TABLE:
        name = "Layout Table";
        break;

    case LAYOUT_TYPE_TELECONFERENCE:
        name = "Layout Teleconference";
        break;

    default:
        name = "Error";
        break;
    }
    AM_INFO("       {---Layout: %s, Cur render num:%d. Cur hd window:%d.}\n", name,
        mpGConfig->dspRenConfig.renNumConfiged, mCurHdWin);
    AM_INFO("       {---Window-SourceGroup Map: ");
    for(i = 0; i < mpGConfig->dspRenConfig.renNumConfiged; i++)
    {
        AM_INFO("(Win:%d--SG:%d)  ", i, mGroupOnWin[i]);
    }
    AM_INFO("}\n");
    return ME_OK;
}
