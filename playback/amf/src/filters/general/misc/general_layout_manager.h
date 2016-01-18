/*
 * general_layout_manager.h
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
#ifndef __GENERAL_LAYOUT_MANAGER_H__
#define __GENERAL_LAYOUT_MANAGER_H__


class CGeneralLayoutManager: public CObject
{
public:
    static CGeneralLayoutManager* Create(CGConfig* pConfig, MdecInfo* pInfo);

public:
    virtual void* GetInterface(AM_REFIID refiid)
    {
        return CObject::GetInterface(refiid);
    }
    virtual void Delete();
    AM_ERR InitWinRenMap(AM_INT sourceIndex, AM_INT sourceGroup, AM_INT flag);
    LAYOUT_TYPE GetLayoutType();
    AM_ERR SetLayoutType(LAYOUT_TYPE type, AM_INT index=0);
    AM_ERR SetPreLayoutType(LAYOUT_PRE_TYPE type, AM_INT index=0);
    AM_ERR CheckActionByLayout(ACTION_TYPE type, AM_INT index);
    AM_ERR SyncLayoutByAction(ACTION_TYPE type, AM_INT sourceGroup);

    AM_ERR BindSourceWindow(AM_INT sourceIndex, AM_INT winIndex, AM_INT renIndex);
    AM_ERR UpdateLayout();
    AM_ERR SwitchStream(AM_INT sourceIndex, AM_INT destIndex, AM_BOOL seamless);
    AM_INT GetSourceGroupByWindow(AM_INT winIndex);

    AM_ERR ConfigTargetWindow(AM_INT target, CParam* pParam);
    AM_ERR ConfigRender(AM_INT render, AM_INT window, AM_INT dsp);

    AM_ERR Dump(AM_INT flag);

private:
    CGeneralLayoutManager(CGConfig* pConfig, MdecInfo* pInfo);
    AM_ERR Construct();
    ~CGeneralLayoutManager();

private:

    AM_ERR HandleReLayout(AM_INT layout, AM_INT index=0);
    AM_ERR HandlePreLayout(LAYOUT_PRE_TYPE type, AM_INT index=0);
    AM_ERR HandleLayoutPreTable();
    AM_ERR HandleReLayoutPreTeleconference(AM_INT index=0);
    AM_ERR HandleReLayoutTeleconference(AM_INT index=0);
    AM_ERR HandleReLayoutTable();
    AM_ERR HandleReLayoutSingleHDFullScreen(AM_INT index=0);
    AM_ERR HandleReLayoutBottomleftHighlightenMode(AM_INT index=0);
    AM_ERR HandleReLayoutNoHD(LAYOUT_TYPE layout, AM_INT index=0);

    AM_ERR HandleSyncLayoutPreSwitch(AM_INT sourceGroup);
    AM_ERR HandleSyncLayoutSwitch(AM_INT sourceGroup);
    AM_ERR HandleSyncLayoutBack(AM_INT sourceGroup);

    AM_BOOL IsShowOnLCD();
    AM_INT RenderSourceNum(ACTION_TYPE type);

    AM_ERR HideTeleSmallWindows(AM_BOOL en);

private:
    CGConfig* mpGConfig;
    MdecInfo* mpInfo;
    LAYOUT_TYPE mCurLayout;
    LAYOUT_TYPE mWantLayout;

    AM_BOOL mbNoHd;
    AM_INT mDspNum;
    AM_INT mCurHdWin; //the hd win on each layout
    AM_INT mGroupOnWin[MDEC_SOURCE_MAX_NUM];
};
#endif

