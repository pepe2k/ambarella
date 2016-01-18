
#include "h264pred.h"
#include "amba_dec_dsputil.h"

int rv40_replace_pred(rv40_neon_t* p_neon,void* p_h)
{
    H264PredContext* h=(H264PredContext*) p_h;

    h->pred4x4_nv12[VERT_PRED           ]= p_neon->pred4[intrapred_type_4x4_vertical_nv12];
    h->pred4x4_nv12[HOR_PRED            ]= p_neon->pred4[intrapred_type_4x4_horizontal_nv12];
    //h->pred4x4_nv12[DC_PRED             ]= pred4x4_dc_c_nv12;
    //h->pred4x4_nv12[DIAG_DOWN_LEFT_PRED ]= pred4x4_down_left_rv40_c_nv12;
    //h->pred4x4_nv12[DIAG_DOWN_RIGHT_PRED]= pred4x4_down_right_c_nv12;
    //h->pred4x4_nv12[VERT_RIGHT_PRED     ]= pred4x4_vertical_right_c_nv12;
    //h->pred4x4_nv12[HOR_DOWN_PRED       ]= pred4x4_horizontal_down_c_nv12;
    //h->pred4x4_nv12[VERT_LEFT_PRED      ]= pred4x4_vertical_left_rv40_c_nv12;
    //h->pred4x4_nv12[HOR_UP_PRED         ]= pred4x4_horizontal_up_rv40_c_nv12;
    //h->pred4x4_nv12[LEFT_DC_PRED        ]= pred4x4_left_dc_c_nv12;
    h->pred4x4_nv12[TOP_DC_PRED         ]= p_neon->pred4[intrapred_type_4x4_topdc_nv12];
    h->pred4x4_nv12[DC_128_PRED         ]= p_neon->pred4[intrapred_type_4x4_128dc_nv12];
    //h->pred4x4_nv12[DIAG_DOWN_LEFT_PRED_RV40_NODOWN]= pred4x4_down_left_rv40_nodown_c_nv12;
    //h->pred4x4_nv12[HOR_UP_PRED_RV40_NODOWN]= pred4x4_horizontal_up_rv40_nodown_c_nv12;
    //h->pred4x4_nv12[VERT_LEFT_PRED_RV40_NODOWN]= pred4x4_vertical_left_rv40_nodown_c_nv12;
    
    h->pred8x8[VERT_PRED8x8   ]= p_neon->pred816[intrapred_type_8x8_vertical];
    h->pred8x8[HOR_PRED8x8    ]= p_neon->pred816[intrapred_type_8x8_horizontal];
    //h->pred8x8[PLANE_PRED8x8  ]= pred8x8_plane_c;
    //h->pred8x8[DC_PRED8x8     ]= pred8x8_dc_rv40_c;
    //h->pred8x8[LEFT_DC_PRED8x8]= pred8x8_left_dc_rv40_c;
    h->pred8x8[TOP_DC_PRED8x8 ]= p_neon->pred816[intrapred_type_8x8_topdc];
    h->pred8x8[DC_128_PRED8x8 ]= p_neon->pred816[intrapred_type_8x8_128dc];

    h->pred8x8_nv12[VERT_PRED8x8   ]= p_neon->pred816[intrapred_type_8x8_vertical_nv12];
    h->pred8x8_nv12[HOR_PRED8x8    ]= p_neon->pred816[intrapred_type_8x8_horizontal_nv12];
    //h->pred8x8_nv12[PLANE_PRED8x8  ]= pred8x8_plane_c_nv12;
    //h->pred8x8_nv12[DC_PRED8x8     ]= pred8x8_dc_rv40_c_nv12;
    //h->pred8x8_nv12[LEFT_DC_PRED8x8]= pred8x8_left_dc_rv40_c_nv12;
    h->pred8x8_nv12[TOP_DC_PRED8x8 ]= p_neon->pred816[intrapred_type_8x8_topdc_nv12];
    h->pred8x8_nv12[DC_128_PRED8x8 ]= p_neon->pred816[intrapred_type_8x8_128dc_nv12];
    
    //h->pred16x16[DC_PRED8x8     ]= pred16x16_dc_c;
    h->pred16x16[VERT_PRED8x8   ]= p_neon->pred816[intrapred_type_16x16_vertical];
    h->pred16x16[HOR_PRED8x8    ]= p_neon->pred816[intrapred_type_16x16_horizontal];
    //h->pred16x16[PLANE_PRED8x8  ]= pred16x16_plane_c;
    //h->pred16x16[PLANE_PRED8x8  ]= pred16x16_plane_rv40_c;
    //h->pred16x16[LEFT_DC_PRED8x8]= pred16x16_left_dc_c;
    h->pred16x16[TOP_DC_PRED8x8 ]= p_neon->pred816[intrapred_type_16x16_topdc];
    h->pred16x16[DC_128_PRED8x8 ]= p_neon->pred816[intrapred_type_16x16_128dc];
   
    return 0;
}

