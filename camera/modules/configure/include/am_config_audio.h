/*******************************************************************************
 * am_config_audio.h
 *
 * Histroy:
 *  2012-8-2 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_CONFIG_AUDIO_H_
#define AM_CONFIG_AUDIO_H_

class AmConfigAudio: public AmConfigBase
{
  public:
    AmConfigAudio(const char *configFileName) :
      AmConfigBase(configFileName),
      mAudioParameters(NULL) {}
    virtual ~AmConfigAudio()
    {
      delete mAudioParameters;
    }

  public:
    AudioParameters* get_audio_config();
    void set_audio_config(AudioParameters *config);

    bool get_adev_config();
    void set_adev_config(ADeviceParameters* adevConfig);

    bool get_aac_config();
    void set_aac_config(AacEncoderParameters *config);

    bool get_opus_config();
    void set_opus_config(OpusEncoderParameters *config);

    bool get_pcm_config();
    void set_pcm_config(PcmEncoderParameters *config);

    bool get_bpcm_config();
    void set_bpcm_config(BpcmEncoderParameters *config);

    bool get_g726_config();
    void set_g726_config(G726EncoderParameters *config);

    bool get_audio_alert_config ();
    void set_audio_alert_config (AudioAlertParameters *config);

  private:
    AudioParameters       *mAudioParameters;
};


#endif /* AM_CONFIG_AUDIO_H_ */
