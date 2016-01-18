/*******************************************************************************
 * am_config_base.h
 *
 * Histroy:
 *  2012-2-20 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMCONFIGBASE_H_
#define AMCONFIGBASE_H_

/*******************************************************************************
 * AmConfig class provides methods to retrieve and store configurations
 * of Vin, Vout, Image and Stream, DO NOT use this class directly,
 * use the inherited class instead
 ******************************************************************************/

class AmConfigBase
{
  public:
    AmConfigBase(const char *configFileName);
    virtual ~AmConfigBase();

  public:
    bool save_config();
    void  dump_ini_file() { /* Dump INI file to stdout: debug only*/
      iniparser_dump(mDictionary, stdout);
    }
    amba_video_mode str_to_video_mode(const char *mode);
    const char*     video_mode_to_str(amba_video_mode mode);
    uint32_t        get_video_mode_w(amba_video_mode mode);
    uint32_t        get_video_mode_h(amba_video_mode mode);

  protected:
    bool init();

    int   get_section_number() {
      return iniparser_getnsec(mDictionary);
    }
    char* get_section_name(int n) {
      return iniparser_getsecname(mDictionary, n);
    }
    char* get_string(const char *key, const char *defValue) {
      return iniparser_getstring(mDictionary, (char *)key, (char *)defValue);
    }
    int   get_int(const char *key, int defValue) {
      return iniparser_getint(mDictionary, (char *)key, defValue);
    }
#ifdef CONFIG_ARCH_S2
    double get_double(const char *key, double defValue) {
      return iniparser_getdouble(mDictionary, (char *)key, defValue);
    }
#endif
    bool  get_boolean(const char *key, bool defValue) {
      return (iniparser_getboolean(mDictionary,
                                   (char *)key,
                                   (defValue ? 1 : 0)) ? true : false);
    }

    bool  set_value(const char *entry, const char *value) {
      return (iniparser_set(mDictionary, (char *)entry, (char *)value) == 0);
    }
    bool  set_value(const char *entry, int32_t value) {
      char valuestr[64] = {0};
      sprintf(valuestr, "%d", value);
      return (iniparser_set(mDictionary, (char *)entry, valuestr) == 0);
    }
    bool  set_value(const char *entry, int16_t value) {
      char valuestr[64] = {0};
      sprintf(valuestr, "%hd", value);
      return (iniparser_set(mDictionary, (char *)entry, valuestr) == 0);
    }
    bool set_value(const char *entry, unsigned long long value) {
      char valuestr[64] = {0};
      sprintf(valuestr, "%llu", value);
      return (iniparser_set(mDictionary, (char *)entry, valuestr) == 0);
    }
    bool set_value(const char *entry, unsigned long value) {
      char valuestr[64] = {0};
      sprintf(valuestr, "%lu", value);
      return (iniparser_set(mDictionary, (char *)entry, valuestr) == 0);
    }
    bool set_value(const char *entry, uint32_t value) {
      char valuestr[64] = {0};
      sprintf(valuestr, "%u", value);
      return (iniparser_set(mDictionary, (char *)entry, valuestr) == 0);
    }
    bool set_value(const char *entry, uint16_t value) {
      char valuestr[64] = {0};
      sprintf(valuestr, "%hu", value);
      return (iniparser_set(mDictionary, (char *)entry, valuestr) == 0);
    }
    bool set_value(const char *entry, uint8_t value) {
      char valuestr[64] = {0};
      sprintf(valuestr, "%hhu", value);
      return (iniparser_set(mDictionary, (char *)entry, valuestr) == 0);
    }
#ifdef CONFIG_ARCH_S2
    bool set_value(const char *entry, double value) {
      char valuestr[64] = {0};
      sprintf(valuestr, "%f", value);
      return (iniparser_set(mDictionary, (char *)entry, valuestr) == 0);
    }
#endif
    void  delete_entry(char *entry) {
      iniparser_unset(mDictionary, entry);
    }

  protected:
    char            *mConfigFile;
    dictionary      *mDictionary;
};

#endif /* AMCONFIGBASE_H_ */
