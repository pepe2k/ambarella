/*******************************************************************************
 * input_record_if.h
 *
 * Histroy:
 *   2012-10-9 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef INPUT_RECORD_IF_H_
#define INPUT_RECORD_IF_H_

extern const AM_IID IID_IInput_Record;

class IInputRecord: public IInterface
{
  public:
    DECLARE_INTERFACE(IInputRecord, IID_IInput_Record);
    virtual AM_ERR SetStreamNumber(AM_UINT streamNum) = 0;
    virtual AM_ERR SetInputNumber(AM_UINT inputNum) = 0;
    virtual AM_UINT GetMaxEncodeNumber() = 0;
    virtual AM_UINT GetEnabledVideoId() = 0;
    virtual AM_ERR SetAudioInfo(AM_UINT samplerate, AM_UINT channels,
                                AM_UINT codecType,  void* codecInfo) = 0;
    virtual AM_ERR SetEnabledVideoId(AM_UINT streams) = 0;
    virtual AM_ERR CreateSubGraph() = 0;
    virtual AM_ERR PrepareToRun()   = 0;
    virtual IPacketPin *GetModuleInputPin()  = 0;
    virtual IPacketPin *GetModuleOutputPin() = 0;
    virtual AM_ERR SetEnabledAudioId(AM_UINT streams) = 0;
    virtual AM_ERR SetAvSyncMap(AM_UINT avSyncMap) = 0;
    virtual AM_ERR StartAll () = 0;
    virtual AM_ERR StopAll () = 0;
    virtual AM_ERR StartAudio() = 0;
    virtual AM_ERR StopAudio()  = 0;
    virtual AM_ERR StartVideo(AM_UINT streamid) = 0;
    virtual AM_ERR StopVideo(AM_UINT streamid)  = 0;
    virtual AM_ERR SendSEI(void *pUserData, AM_U32 len) = 0;
    virtual AM_ERR SendEvent(CPacket::AmPayloadAttr eventType) = 0;
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
    virtual void SetFrameStatisticsCallback(void (*callback)(void *data)) = 0;
#endif
};

#endif /* INPUT_RECORD_IF_H_ */
