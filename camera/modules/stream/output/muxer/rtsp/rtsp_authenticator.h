/*******************************************************************************
 * rtsp_authenticator.h
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

#ifndef RTSP_AUTHENTICATOR_H_
#define RTSP_AUTHENTICATOR_H_

struct UserDB
{
    char* username;
    char* realm;
    char* hashcode;
    UserDB() :
      username(NULL),
      realm(NULL),
      hashcode(NULL)
    {}

    ~UserDB()
    {
      delete[] username;
      delete[] realm;
      delete[] hashcode;
    }

    void set_user_name(char* name)
    {
      delete[] username;
      username = amstrdup(name);
    }

    void set_realm(char* rlm)
    {
      delete[] realm;
      realm = amstrdup(rlm);
    }

    void set_hash_code(char* hash)
    {
      delete[] hashcode;
      hashcode = amstrdup(hash);
    }
};

typedef std::queue<UserDB*> UserDataBase;

struct RtspAuthHeader;
class CRtspAuthenticator
{
  public:
    CRtspAuthenticator();
    virtual ~CRtspAuthenticator();

  public:
    bool Init();

  public:
    const char* GetRealm()
    {
      if (AM_UNLIKELY(!mUserDB || mUserDB->empty())) {
        SetRealm("Ambarella-xMan");
      } else {
        SetRealm(mUserDB->front()->realm);
      }
      return mRealm;
    }
    const char* GetNonce()
    {
      return mNonce;
    }
    void SetRealm(const char* realm);
    void SetNonce(const char* nonce);
    bool Authenticate(RtspRequest& reqHeader, RtspAuthHeader& authHeader);

  private:
    char* LookupUserHashCode(const char* user);
    bool UpdateUserDataBase();

  private:
    char*         mRealm;
    char*         mNonce;
    UserDataBase* mUserDB;
};

#endif /* RTSP_AUTHENTICATOR_H_ */
