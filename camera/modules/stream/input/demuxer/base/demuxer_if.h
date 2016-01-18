/*******************************************************************************
 * demuxer_if.h
 *
 * History:
 *   2013-3-5 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef DEMUXER_IF_H_
#define DEMUXER_IF_H_

extern const AM_IID IID_IDemuxer;

class IDemuxer: public IInterface {
  public:
    DECLARE_INTERFACE(IDemuxer, IID_IDemuxer);
    virtual AM_ERR AddMedia(const char* uri) = 0;
    virtual AM_ERR Play(const char* uri) = 0;
    virtual AM_ERR Start() = 0;
    virtual AM_ERR Stop() = 0;
};

#endif /* DEMUXER_IF_H_ */
