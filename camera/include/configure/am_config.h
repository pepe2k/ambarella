/*******************************************************************************
 * am_config.h
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

#ifndef AMCONFIG_H_
#define AMCONFIG_H_

/*******************************************************************************
 * AmConfig class provides methods to retrieve and store configurations
 * of Vin, Vout, Image and Stream.
 ******************************************************************************/
#define DEFAULT_VIN_CONFIG_PATH         BUILD_AMBARELLA_CAMERA_CONF_DIR"/video_vin.conf"
#define DEFAULT_VOUT_CONFIG_PATH        BUILD_AMBARELLA_CAMERA_CONF_DIR"/vout.conf"
#define DEFAULT_VDEV_CONFIG_PATH        BUILD_AMBARELLA_CAMERA_CONF_DIR"/vdevice.conf"
#define DEFAULT_RECORD_CONFIG_PATH      BUILD_AMBARELLA_CAMERA_CONF_DIR"/record.conf"
#define DEFAULT_PHOTO_CONFIG_PATH       BUILD_AMBARELLA_CAMERA_CONF_DIR"/photo.conf"
#define DEFAULT_AUDIO_CONFIG_PATH       BUILD_AMBARELLA_CAMERA_CONF_DIR"/audio.conf"
#define DEFAULT_WIFI_CONFIG_PATH        BUILD_AMBARELLA_CAMERA_CONF_DIR"/wifi.conf"
#define DEFAULT_AUDIODETECT_CONFIG_PATH BUILD_AMBARELLA_CAMERA_CONF_DIR"/audiodetect.conf"
#define DEFAULT_MOTIONDETECT_CONFIG_PATH BUILD_AMBARELLA_CAMERA_CONF_DIR"/motiondetect.conf.mse"
#define DEFAULT_LBR_CONFIG_PATH         BUILD_AMBARELLA_CAMERA_CONF_DIR"/lbr.conf"

#ifdef CONFIG_ARCH_S2
#define DEFAULT_FISHEYE_CONFIG_PATH     BUILD_AMBARELLA_CAMERA_CONF_DIR"/fisheye.conf"
class AmConfigFisheye;
#endif

class AmConfigVin;
class AmConfigVout;
class AmConfigVDevice;
class AmConfigEncoder;
class AmConfigStream;
class AmConfigRecord;
class AmConfigPhoto;
class AmConfigAudio;
class AmConfigWifi;
class AmConfigAudioDetect;
class AmConfigMotionDetect;
class AmConfigLBR;

class AmConfig
{
  public:
    AmConfig();
    virtual ~AmConfig();

  public:
    bool load_vdev_config();
    bool load_record_config();
    bool load_photo_config();
    bool load_audio_config();
    bool load_wifi_config ();
    bool load_audiodetect_config ();
    bool load_motiondetect_config();
    bool load_lbr_config();
#ifdef CONFIG_ARCH_S2
    bool load_fisheye_config();
#endif

  public:
    VDeviceParameters       *vdevice_config();
    VinParameters           *vin_config(uint32_t vinId);
    VoutParameters          *vout_config(uint32_t voutId);
    EncoderParameters       *encoder_config();
    StreamParameters        *stream_config(uint32_t streamId);
    RecordParameters        *record_config();
    PhotoParameters         *photo_config();
    AudioParameters         *audio_config();
    AacEncoderParameters    *aac_config();
    WifiParameters          *wifi_config ();
    AudioDetectParameters   *audiodetect_config ();
    MotionDetectParameters  *motiondetect_config();
    LBRParameters           *lbr_config();
#ifdef CONFIG_ARCH_S2
    FisheyeParameters       *fisheye_config();
#endif

  public:
    void set_vin_config_path(const char *path);
    void set_vout_config_path(const char *path);
    void set_vdevice_config_path(const char *path);
    void set_record_config_path(const char *path);
    void set_photo_config_path(const char *path);
    void set_audio_config_path(const char *path);
    void set_wifi_config_path (const char *path);
    void set_audiodetect_config_path (const char *path);
    void set_motiondetect_config_path(const char *path);
    void set_lbr_config_path(const char *path);
#ifdef CONFIG_ARCH_S2
    void set_fisheye_config_path(const char *path);
#endif

    void print_vin_config();
    void print_vout_config();
    void print_vdev_config();
    void print_encoder_config();
    void print_record_config();
    void print_photo_config();
    void print_audio_config();
    void print_wifi_config ();
    void print_audiodetect_config ();
    void print_motiondetect_config();
    void print_lbr_config();
#ifdef CONFIG_ARCH_S2
    void print_fisheye_config();
#endif

  protected:
    /* These virtual functions can be re-implemented
     * to do customised parameter check
     */
    virtual bool check_audio_parameters();
    virtual bool check_record_parameters();
    virtual bool check_photo_parameters();
    virtual bool check_wifi_parameters();
    virtual bool check_audiodetect_parameters();
    virtual bool check_motiondetect_parameters();
    virtual bool check_vout_parameters(VoutParameters *voutConfig,
                                       VoutType type);

    virtual void cross_check_vin_encoder(VinParameters *vin);
    virtual void cross_check_vout_encoder(VoutParameters *voutConfig,
                                          VoutType type);
    virtual void cross_check_encoder_resource(uint32_t streamNum);

  public:
    /* VIN */
    VinParameters *get_vin_config(VinType type);
    void set_vin_config(VinParameters *config, VinType type);
    /* VOUT */
    VoutParameters *get_vout_config(VoutType type);
    void set_vout_config(VoutParameters *config, VoutType);
    /* Video Device */
    VDeviceParameters *get_vdev_config();
    void set_vdev_config(VDeviceParameters *vdevConfig);
    EncoderParameters *get_encoder_config();
    void set_encoder_config(EncoderParameters *encoderConfig);
    StreamParameters *get_stream_config(uint32_t streamID);
    void set_stream_config(StreamParameters *streamConfig, uint32_t streamID);

    /* Record */
    RecordParameters *get_record_config();
    void set_record_config(RecordParameters *config);
    /* Photo */
    PhotoParameters *get_photo_config();
    void set_photo_config(PhotoParameters *config);
    /* audio */
    AudioParameters *get_audio_config();
    void set_audio_config(AudioParameters *config);

    /* Wifi */
    WifiParameters *get_wifi_config ();
    void set_wifi_config (WifiParameters *wifiConfig);
    /* Audio detect*/
    AudioDetectParameters *get_audiodetect_config ();
    void set_audiodetect_config (AudioDetectParameters *audiodetectConfig);
    /* Motion Detect */
    MotionDetectParameters *get_motiondetect_config();
    void set_motiondetect_config(MotionDetectParameters *config);
    /* LBR Control */
    LBRParameters *get_lbr_config();
    void set_lbr_config(LBRParameters *config);
#ifdef CONFIG_ARCH_S2
    /* Fisheye */
    FisheyeParameters *get_fisheye_config();
    void set_fisheye_config (FisheyeParameters *fisheyeConfig);
#endif

    const char*     video_mode_string(amba_video_mode mode);
    const char*     video_fps_string(int32_t fps);
    int32_t         video_mode_width(amba_video_mode mode);
    int32_t         video_mode_height(amba_video_mode mode);
    amba_video_mode hdmi_native_mode();

  private:
    AmConfigVin         *mVinConfig;
    AmConfigVout        *mVoutConfig;
    AmConfigVDevice     *mVdevConfig;
    AmConfigEncoder     *mEncoderConfig;
    AmConfigStream      *mStreamConfig;
    AmConfigRecord      *mRecordConfig;
    AmConfigPhoto       *mPhotoConfig;
    AmConfigAudio       *mAudioConfig;
    AmConfigWifi        *mWifiConfig;
    AmConfigAudioDetect *mAudioDetectConfig;
    AmConfigMotionDetect *mMotionDetectConfig;
    AmConfigLBR         *mLBRConfig;

  private:
    VinParameters        **mVinParamList;
    VoutParameters       **mVoutParamList;
    StreamParameters     **mStreamParamList;
    EncoderParameters     *mEncoderParams;
    VDeviceParameters     *mVdevParams;
    RecordParameters      *mRecordParams;
    PhotoParameters       *mPhotoParams;
    ADeviceParameters     *mAdevParams;
    AudioParameters       *mAudioParams;
    WifiParameters        *mWifiParams;
    AudioDetectParameters *mAudioDetectParams;
    MotionDetectParameters *mMotionDetectParams;
    LBRParameters         *mLBRParams;

  private:
    int   mIavFd;
    char *mVdevConfigPath;
    char *mVinConfigPath;
    char *mVoutConfigPath;
    char *mRecordConfigPath;
    char *mPhotoConfigPath;
    char *mAudioConfigPath;
    char *mWifiConfigPath;
    char *mAudioDetectConfigPath;
    char *mMotionDetectConfigPath;
    char *mLBRConfigPath;

#ifdef CONFIG_ARCH_S2
    AmConfigFisheye   *mFisheyeConfig;
    FisheyeParameters *mFisheyeParams;
    char              *mFisheyeConfigPath;
#endif
};

#endif /* AMCONFIG_H_ */
