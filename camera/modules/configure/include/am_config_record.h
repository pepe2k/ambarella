/*******************************************************************************
 * am_config_record.h
 *
 * Histroy:
 *  2012-3-26 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_CONFIG_RECORD_H_
#define AM_CONFIG_RECORD_H_

class AmConfigRecord: public AmConfigBase
{
  public:
    AmConfigRecord(const char *configFileName) :
      AmConfigBase(configFileName),
      mRecordParameters(NULL) {
      memset(mFileTypeList, 0, sizeof(char*)*MAX_ENCODE_STREAM_NUM);
    }

    virtual ~AmConfigRecord()
    {
      delete mRecordParameters;
    }

  public:
    RecordParameters* get_record_config();
    void set_record_config(RecordParameters *config);

  private:
    inline uint32_t get_filetype_list(char *typeString);

  private:
    RecordParameters *mRecordParameters;
    char             *mFileTypeList[MAX_ENCODE_STREAM_NUM];
};


#endif /* AM_CONFIG_RECORD_H_ */
