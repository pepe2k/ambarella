/*******************************************************************************
 * am_config_record.cpp
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
#ifdef __cplusplus
extern "C" {
#endif
#include <iniparser.h>
#ifdef __cplusplus
}
#endif

#include "am_include.h"
#include "am_utility.h"
#include "am_data.h"

#include "am_config_base.h"
#include "am_config_record.h"

static const char *file_type_to_str[] =
{
  "NULL",
  "RAW",
  "TS",
  "TS_HTTP",
  "TS_HLS",
  "IPTS",
  "RTSP",
  "MP4",
  "JPEG",
  "MOV",
  "MKV"
  "AVI",
};

RecordParameters* AmConfigRecord::get_record_config()
{
  RecordParameters* ret = NULL;
  if (init()) {
    if (!mRecordParameters) {
      mRecordParameters = new RecordParameters();
    }
    if (mRecordParameters) {
      int32_t file_duration   = get_int("RECORD:FileDuration", 0);
      int32_t record_duration = get_int("RECORD:RecordDuration", 0);
      int32_t max_file_amount = get_int("RECORD:MaxFileAmount", 0);
      int32_t event_history_duration = get_int("RECORD:EventHistoryDuration", 0);
      int32_t event_future_duration = get_int("RECORD:EventFutureDuration", 0);
      int32_t event_stream_id = get_int("RECORD:EventStreamId", 0);

      mRecordParameters->stream_number =
          (uint32_t)get_int("RECORD:StreamNumber", 1);

      for (uint32_t i = 0; i < mRecordParameters->stream_number; ++ i) {
        char entry[32]  = {0};
        char typeList[256] = {0};
        char *file_type = NULL;
        uint32_t fileTypeNum = 0;

        sprintf(entry, "RECORD:FileType%d", i);
        file_type = get_string(entry, "TS");
        strncpy(typeList, file_type, strlen(file_type) + 1);
        fileTypeNum = get_filetype_list(typeList);
        for (uint32_t j = 0; j < fileTypeNum; ++ j) {
          file_type = mFileTypeList[j];
          DEBUG("Stream%u type%u is %s", i, j, mFileTypeList[j]);
          if (file_type) {
            if (is_str_equal(file_type, "NULL") ||
                is_str_equal(file_type, "NONE")) {
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_NULL;
            } else if (is_str_equal(file_type, "RAW")) {
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_RAW;
            } else if (is_str_equal(file_type, "MP4")) {
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_MP4;
            } else if (is_str_equal(file_type, "TS")) {
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_TS;
            } else if (is_str_equal(file_type, "AVI")) {
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_AVI;
            } else if (is_str_equal(file_type, "MOV")) {
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_MOV;
            } else if (is_str_equal(file_type, "MKV")) {
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_MKV;
            } else if (is_str_equal(file_type, "RTSP")) {
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_RTSP;
            } else if (is_str_equal(file_type, "IPTS")) {
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_IPTS;
            } else if (is_str_equal(file_type, "TS_HLS") ||
                       is_str_equal(file_type, "HLS")) {
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_TS_HLS;
            } else if (is_str_equal (file_type, "JPEG") ||
                       is_str_equal (file_type, "MJPEG")) {
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_JPEG;
            } else if (is_str_equal(file_type, "TSUPLOAD") ||
                       is_str_equal(file_type, "TS_HTTP")) {
              char *url = get_string("RECORD:TsUploadUrl", NULL);
              if (url && !is_str_equal(url, "url")) {
                mRecordParameters->set_ts_upload_url(url);
                mRecordParameters->file_type[i][j] =
                    AM_RECORD_FILE_TYPE_TS_HTTP;
              } else {
                WARN("TS Uploading URL is not set, disable stream%d!", i);
                mRecordParameters->set_ts_upload_url(NULL);
                mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_NULL;
                mRecordParameters->config_changed = 1;
              }
            } else {
              WARN("Unknown file type %s, reset to TS!", file_type);
              mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_TS;
              mRecordParameters->config_changed = 1;
            }
          } else {
            WARN("File type is not set!");
            mRecordParameters->file_type[i][j] = AM_RECORD_FILE_TYPE_NULL;
            mRecordParameters->config_changed = 1;
          }
        }
      }

      if (file_duration < 0) {
        WARN("Invalid file duration: %d seconds, reset to default duration!",
             file_duration);
        mRecordParameters->file_duration = 300;
        mRecordParameters->config_changed = 1;
      } else if (file_duration == 0) {
        INFO("Use default file recording duration 300 seconds!");
        mRecordParameters->file_duration = 300;
      } else {
        mRecordParameters->file_duration = file_duration;
      }

      if (record_duration < 0) {
        WARN("Invalid record duration: %d seconds, reset to infinite!",
             record_duration);
        mRecordParameters->record_duration = 0;
        mRecordParameters->config_changed = 1;
      } else {
        mRecordParameters->record_duration = record_duration;
      }

      if (max_file_amount < 0) {
        WARN("Invalid max file amount: %d, reset to default duration!",
             max_file_amount);
        mRecordParameters->max_file_amount = 5;
        mRecordParameters->config_changed = 1;
      } else if (max_file_amount == 0) {
        INFO("Use default max file amount: 5 !");
        mRecordParameters->max_file_amount = 5;
      } else {
        mRecordParameters->max_file_amount = max_file_amount;
      }

      if ((event_stream_id > 0) ||
          (event_stream_id < (int32_t)mRecordParameters->stream_number)) {
        mRecordParameters->event_stream_id = event_stream_id;
      } else {
        WARN("Invalid event stream id: %d, reset to default stream0!",
             event_stream_id);
        mRecordParameters->event_stream_id = 0;
      }

      if (event_history_duration < 0) {
        WARN("Invalid max event history duration: %d, reset to default duration!",
             event_history_duration);
        mRecordParameters->event_history_duration = 5;
      } else if (event_history_duration > 10) {
        INFO("Use default max event history duration: 10!");
        mRecordParameters->event_history_duration = 10;
      } else {
        mRecordParameters->event_history_duration = event_history_duration;
      }

      if (event_future_duration < 1) {
        WARN("Invalid max event future duration: %d, reset to default duration!",
             event_future_duration);
        mRecordParameters->event_future_duration = 20;
      } else if (event_future_duration > 50) {
        INFO("Use default max event future duration: 50!");
        mRecordParameters->event_future_duration = 50;
      } else {
        mRecordParameters->event_future_duration = event_future_duration;
      }

      mRecordParameters->rtsp_send_wait =
          get_boolean("RECORD:RtspSendInBlockMode", false) ? 1 : 0;
      mRecordParameters->rtsp_need_auth =
          get_boolean("RECORD:RtspNeedAuthentication", true) ? 1 : 0;
      mRecordParameters->\
      set_file_name_prefix(get_string("RECORD:FileNamePrefix", "A5s"));

      mRecordParameters->
      enable_file_name_timestamp(get_boolean("RECORD:FileNameTimestamp", true));

      mRecordParameters->\
      set_file_store_location(get_string("RECORD:FileStoreLocation",
                                         "/media/mmcblk0p1/video/"));
      if (mRecordParameters->config_changed) {
        set_record_config(mRecordParameters);
      }
    }
    ret = mRecordParameters;
  }

  return ret;
}

void AmConfigRecord::set_record_config(RecordParameters *config)
{
  if (AM_LIKELY(config)) {
    if (AM_LIKELY(init())) {
      set_value("RECORD:FileDuration", config->file_duration);
      set_value("RECORD:RecordDuration", config->record_duration);
      set_value("RECORD:FileNameTimestamp", config->file_name_timestamp);
      set_value("RECORD:MaxFileAmount", config->max_file_amount);
      set_value("RECORD:RtspSendInBlockMode",
                config->rtsp_send_wait ? "Yes" : "No");
      set_value("RECORD:RtspNeedAuthentication",
                config->rtsp_need_auth ? "Yes" : "No");

      for (uint32_t i = 0; i < config->stream_number; ++ i) {
        char entry[32] = {0};
        char fileList[64] = {0};
        sprintf(entry, "RECORD:FileType%u", i);
        for (uint32_t j = 0; j < MAX_ENCODE_STREAM_NUM; ++ j) {
          if (AM_LIKELY(config->file_type[i][j]) != AM_RECORD_FILE_TYPE_NULL) {
            uint32_t count = 1;
            uint32_t type  = (uint32_t)config->file_type[i][j];
            while ( type != 1) {
              ++ count;
              type = type >> 1;
            }
            strncat(fileList, file_type_to_str[count],
                    strlen(file_type_to_str[count]));
            strncat(fileList, ",", 1);
          }
        }
        if (strlen(fileList) == 0) {
          set_value(entry, "");
        } else {
          /* Eat the last ',' */
          fileList[strlen(fileList) - 1] = '\0';
        }
        set_value(entry, fileList);
      }

      set_value("RECORD:TsUploadUrl",
                config->ts_upload_url ? config->ts_upload_url : "");
      set_value("RECORD:FileNamePrefix",
                config->file_name_prefix ? config->file_name_prefix : "");
      set_value("RECORD:FileStoreLocation",
                config->file_store_location ? config->file_store_location : "");
      config->config_changed = 0;
      save_config();
    } else {
      WARN("Failed opening %s, record configuration NOT saved!", mConfigFile);
    }
  }
}

inline uint32_t AmConfigRecord::get_filetype_list(char *typeString)
{
  uint32_t ret = 0;
  if (AM_LIKELY(typeString)) {
    uint32_t count = 1;
    char *remain = typeString;
    mFileTypeList[0] = typeString;
    if (typeString[strlen(typeString) - 1] == ',') {
      /* Eat the last ',' */
      typeString[strlen(typeString) - 1] = '\0';
    }
    while (NULL != (remain = strchr(remain, (int)','))) {
      if (count < MAX_ENCODE_STREAM_NUM) {
        *remain = '\0'; /* Change ',' to \0 */
         ++ remain;
        while((*remain == ' ') || (*remain == '\t')) {
          /* Find next non-white space char */
          ++ remain;
        }
        mFileTypeList[count ++] = remain;
      } else {
        break;
      }
    }

    ret = count;
  }

  return ret;
}
