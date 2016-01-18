/*******************************************************************************
 * rtsp_authenticator.cpp
 *
 * History:
 *   2013年10月16日 - [ypchang] created file
 *
 * Copyright (C) 2008-2013, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <queue>

#include "openssl/md5.h"

#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_network.h"

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_muxer_info.h"
#include "am_media_info.h"
#include "output_record_if.h"

#include "rtp_packager.h"
#include "rtsp_filter.h"
#include "rtsp_server.h"
#include "rtsp_requests.h"
#include "rtsp_client_session.h"
#include "rtsp_authenticator.h"
#include "rtp_session.h"
#include "rtp_session_audio.h"
#include "rtp_session_video.h"

#define USER_DATA_FILE ((const char*)"/etc/webpass.txt")

CRtspAuthenticator::CRtspAuthenticator() :
  mRealm(NULL),
  mNonce(NULL),
  mUserDB(NULL)
{
}

CRtspAuthenticator::~CRtspAuthenticator()
{
  delete[] mRealm;
  delete[] mNonce;
  while (mUserDB && !mUserDB->empty()) {
    delete mUserDB->front();
    mUserDB->pop();
  }
  delete mUserDB;
}

bool CRtspAuthenticator::Init()
{
  return UpdateUserDataBase();
}

void CRtspAuthenticator::SetRealm(const char* realm)
{
  delete[] mRealm;
  mRealm = NULL;
  mRealm = amstrdup(realm);
}

void CRtspAuthenticator::SetNonce(const char* nonce)
{
  delete[] mNonce;
  mNonce = NULL;
  mNonce = amstrdup(nonce);
}

bool CRtspAuthenticator::Authenticate(RtspRequest& reqHeader,
                                      RtspAuthHeader& authHeader)
{
  bool ret = false;

  if (AM_LIKELY(authHeader.is_ok())) {
    /*
     * MD5(<userHash>:<nonce>:MD5(<command>:<uri>))
     */
    char* userHash = LookupUserHashCode(authHeader.username);
    if (AM_LIKELY(userHash)) {
      unsigned char data[128] = {0};
      unsigned char dataHash[48] = {0};
      char hashString[48] = {0};

      sprintf((char*)data, "%s:%s", reqHeader.command, authHeader.uri);
      MD5(data, strlen((const char*)data), dataHash);
      for (AM_UINT i = 0; i < 16; ++ i) {
        sprintf(&hashString[strlen(hashString)], "%02x", dataHash[i]);
      }
      /*NOTICE("MD5(%s) = %s", data, hashString);*/
      sprintf((char*)data, "%s:%s:%s",
              userHash, authHeader.nonce, hashString);
      MD5(data, strlen((const char*)data), dataHash);
      memset(hashString, 0, sizeof(hashString));
      for (AM_UINT i = 0; i < 16; ++ i) {
        sprintf(&hashString[strlen(hashString)], "%02x", dataHash[i]);
      }
      /*NOTICE("MD5(%s) = %s", data, hashString);*/

      ret = is_str_equal(hashString, authHeader.response);
      NOTICE("\nServer response: %s\nClient response: %s\nAuthentication %s!",
             hashString, authHeader.response, (ret ? "Successfully": "Failed"));
    } else {
      NOTICE("%s is not registered in this server!", authHeader.username);
    }
  }

  return ret;
}

char* CRtspAuthenticator::LookupUserHashCode(const char* user)
{
  char* hash = NULL;

  if (AM_LIKELY(user && UpdateUserDataBase() && !mUserDB->empty())) {
    size_t size = mUserDB->size();
    for (size_t i = 0; i < size; ++ i) {
      UserDB* db = mUserDB->front();
      mUserDB->pop();
      mUserDB->push(db);
      if (AM_LIKELY(is_str_same(user, db->username))) {
        hash = db->hashcode;
        NOTICE("Found user: %s@%s", db->username, db->realm);
        break;
      }
    }
  }

  return hash;
}

bool CRtspAuthenticator::UpdateUserDataBase()
{
  bool ret = true;
  FILE* db = fopen(USER_DATA_FILE, "r");

  if (AM_LIKELY(!mUserDB)) {
    mUserDB = new UserDataBase();
  }
  if (AM_LIKELY(db && mUserDB)) {
    /* Purge previous data base */
    while (!mUserDB->empty()) {
      delete mUserDB->front();
      mUserDB->pop();
    }
    while (!feof(db)) {
      char buf[512] = { 0 };
      if (AM_LIKELY((1 == fscanf(db, "%[^\n]\n", buf)) && !ferror(db))) {
        char user[strlen(buf)];
        char realm[strlen(buf)];
        char hash[strlen(buf)];
        memset(user, 0, sizeof(user));
        memset(realm, 0, sizeof(realm));
        memset(hash, 0, sizeof(hash));
        buf[strlen(buf)] = ':';
        if (AM_UNLIKELY(3 != sscanf(buf, "%[^:]:%[^:]:%[^:]:",
                                    user, realm, hash))) {
          ERROR("Malformed user data:\n%s\n%s %s %s", buf, user, realm, hash);
        } else {
          UserDB* userDB = new UserDB();
          userDB->set_user_name(user);
          userDB->set_realm(realm);
          userDB->set_hash_code(hash);
          mUserDB->push(userDB);
          INFO("Get User Info: %s, %s, %s", user, realm, hash);
        }
      } else {
        if (AM_LIKELY(ferror(db))) {
          PERROR("fscanf");
        } else {
          ERROR("Broken user data!");
        }
      }
    }
  } else {
    if (AM_LIKELY(!db)) {
      PERROR("fopen");
    }
    if (AM_LIKELY(!mUserDB)) {
      ERROR("Failed to new user data base.");
    }
    ret = false;
  }

  if (AM_LIKELY(db)) {
    fclose(db);
  }

  return ret;
}
