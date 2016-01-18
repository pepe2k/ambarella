/*******************************************************************************
 * ampm_dptz.h
 *
 * History:
 *  Apr 28, 2013 - [zkyang] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMPM_DPTZ_H_
#define AMPM_DPTZ_H_

#define MAX_PM_BUF_NUM (8)
#define COLOR_NUM (8)
#define MAX_ZOOM_IN_MAIN_BUFFER (1100)		// 11X
#define MAX_ZOOM_IN_PREVIEW_A_B (500)		// 5X
#define MAX_ZOOM_STEPS (10)

#include <assert.h>
class AmEncodeDevice;

class AmPrivacyMaskDPTZ
{
  public:

    AmPrivacyMaskDPTZ(int iav_hd, VinParameters **vin);
    virtual ~AmPrivacyMaskDPTZ();

    /**************** Privacy Mask related ****************/
    #ifdef CONFIG_ARCH_S2
    int set_pm_param(PRIVACY_MASK * pm_in, RECT* buffer);
    int get_pm_param(uint32_t * pm_in);
    #else
    int set_pm_param(PRIVACY_MASK * pm_in);
    #endif
    /**************** Privacy Mask related ****************/

    //DPTZ
    int set_dptz_param(DPTZParam	 *dptz_set);
#ifdef CONFIG_ARCH_S2
    int set_dptz_param_mainbuffer(DPTZParam	 *dptz_set);
#endif
    int get_dptz_param(uint32_t stream_id, DPTZParam *dptz_get);
    //DPTZ
#ifdef CONFIG_ARCH_S2
    int disable_pm();
    int pm_reset_all(RECT* buffer);
#else
    int pm_reset_all(int enable_pm, DPTZParam *dptz);
#endif

  protected:

    /**************** Privacy Mask related ****************/
    int map_pm(void);
    bool unmap_pm(void);
    int get_pm_buffer_info(void);
    int get_pm_rect(PRIVACY_MASK *pPrivacyMask, PRIVACY_MASK_RECT *rect);
    int calc_pm_size(void);
    int check_for_pm(PRIVACY_MASK_RECT *input_mask_pixel_rect);
    int is_masked(int block_x, int block_y, PRIVACY_MASK_RECT * block_rect);
    int find_nearest_block_rect(PRIVACY_MASK_RECT *block_rect,
      PRIVACY_MASK_RECT *input_rect);
    int fill_pm_mem(PRIVACY_MASK_RECT * rect);
    int prepare_pm(int enable, iav_privacy_mask_setup_ex_t *privacy_mask_setup);
    int copy_pm_to_next_buffer(void);

    int add_privacy_mask(PRIVACY_MASK *pPrivacyMask);
    int enable_privacy_mask(int enable);

    int get_dptz_org_param(DPTZOrgParam * pDPTZ, DPTZParam * dptz);
    int calc_dptz_I_param(DPTZParam * pDPTZ, iav_digital_zoom_ex_t *dptz);
    int parse_vin_mode_resolution(u32 vin_mode, u32 *vin_width, u32 *vin_height);
    int get_transfer_info(DPTZParam *dptz);
    int adjust_pm_in_main(PRIVACY_MASK *main_pm);
    int trans_cord_vin_to_main(PRIVACY_MASK* main_pm, const PRIVACY_MASK* vin_pm);
    int trans_cord_main_to_vin(PRIVACY_MASK* vin_pm, const PRIVACY_MASK* main_pm);
    int pm_add_one_mask(PRIVACY_MASK* pm);
    int pm_remove_all_mask();
    int del_one_node(int id);

    /**************** Privacy Mask related ****************/

    //DPTZ
    int mw_set_dptz_param(DPTZParam * pDPTZ);
    int mw_get_dptz_param(DPTZParam * pDPTZ);
    int set_dptz_control_param(DPTZParam *pDPTZ);
#ifdef CONFIG_ARCH_S2
    int calc_dptz_II_param(DPTZParam * pDPTZ, int source, iav_digital_zoom_ex_t * dptz);
#else
    int calc_dptz_II_param(DPTZParam * pDPTZ, int source, iav_digital_zoom2_ex_t * dptz);
#endif
    int set_dptz_privacy_mask(DPTZPrivacyMaskParam * pDZPM);
    int convert_dptz_zoom_factor(u32 *new_factor, u32 old_factor, int source);


  private:

    /**************** Privacy Mask related ****************/
    uint8_t pm_buffer_max_num;
    uint8_t pm_buffer_id;
    uint8_t pm_color_index;
    uint8_t g_pm_enable;
#ifdef CONFIG_ARCH_S2
    uint16_t pm_buffer_pitch;
#endif
    uint32_t pm_buffer_size;
    uint32_t g_vin_width;
    uint32_t g_vin_height;
    uint32_t g_pm_color;
    int num_block_x;
    int num_block_y;
    uint8_t * pm_start_addr[MAX_PM_BUF_NUM];
    bool mIsPrivacyMaskMapped;
    MMapInfo pm_info;
    CLUT pm_color[COLOR_NUM];

    PrivacyMaskNode* g_pm_head;
    PrivacyMaskNode* g_pm_tail;

    DPTZOrgParam g_dptzI_param;
    BufferFormat all_buffer_format[BUFFER_TOTAL_NUM];
#ifdef CONFIG_ARCH_S2
    iav_digital_zoom_ex_t G_dptz[BUFFER_TOTAL_NUM];
#else
    iav_digital_zoom2_ex_t G_dptz[BUFFER_TOTAL_NUM];
#endif
    iav_digital_zoom_ex_t G_dptz_I;
    /**************** Privacy Mask related ****************/

    DPTZPrivacyMaskParam g_dptz_pm;
    DPTZParam g_dptz_info;
    int mIav;
    VinParameters **mVinParamList;

};


#endif /* AMPM_DPTZ_H_ */
