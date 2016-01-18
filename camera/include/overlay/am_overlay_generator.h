/*******************************************************************************
 * am_overlay_generator.h
 *
 * History:
 *  Dec 28, 2012 2012 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_OVERLAY_GENERATOR_H_
#define AM_OVERLAY_GENERATOR_H_

#define OVERLAY_PITCH_ALIGN         (0x20)
#define OVERLAY_WIDTH_ALIGN         (0x10)
#define OVERLAY_OFFSET_X_ALIGN      (0x2)
#define OVERLAY_HEIGHT_ALIGN        (0x4)
#define OVERLAY_FULL_TRANSPARENT    (0x0)
#define OVERLAY_NONE_TRANSPARENT    (0xff)

#define BITMAP_MAGIC_NUMBER         (0x4D42)
#define BITMAP_BIT_NUMBER           (8)

#define TEXT_CLUT_ENTRY_BACKGOURND  (0)
#define TEXT_CLUT_ENTRY_OUTLINE     (1)
#define TEXT_LINE_SPACING(size)     ((size) * 3 / 2)

#define TIME_STRING_MAX_SIZE                 (128)
#define DIGIT_NUM                            (10)
#define LETTER_NUM                           (26)     // Alphabet

#define DEFAULT_OVERLAY_FONT_SIZE             (32)
#define DEFAULT_OVERLAY_FONT_OUTLINE_WIDTH    (2)
#define DEFAULT_OVERLAY_FONT_TTF_PATH         "/usr/share/fonts/Vera.ttf"

#define DEFAULT_OVERLAY_FONT_Y         (235)
#define DEFAULT_OVERLAY_FONT_U         (128)
#define DEFAULT_OVERLAY_FONT_V         (128)
#define DEFAULT_OVERLAY_FONT_ALPHA     (OVERLAY_NONE_TRANSPARENT)

#define DEFAULT_OVERLAY_OUTLINE_Y      (12)
#define DEFAULT_OVERLAY_OUTLINE_U      (128)
#define DEFAULT_OVERLAY_OUTLINE_V      (128)
#define DEFAULT_OVERLAY_OUTLINE_ALPHA  (OVERLAY_NONE_TRANSPARENT)


#define DEFAULT_OVERLAY_BACKGROUND_Y     (DEFAULT_OVERLAY_FONT_Y)
#define DEFAULT_OVERLAY_BACKGROUND_U     (DEFAULT_OVERLAY_FONT_U)
#define DEFAULT_OVERLAY_BACKGROUND_V     (DEFAULT_OVERLAY_FONT_V)
#define DEFAULT_OVERLAY_BACKGROUND_ALPHA (OVERLAY_FULL_TRANSPARENT)

class AmOverlayGenerator {
  public:
    AmOverlayGenerator(uint32_t max_size = 0);
    ~AmOverlayGenerator();

  public:
    void    set_max_size(uint32_t max_size);
    Overlay *create_bitmap(EncodeSize size, Point offset, const char *bmpfile);
    Overlay *create_text(EncodeSize size, Point offset, TextBox *textbox,
                        const char *str);
    Overlay *create_time(EncodeSize size, Point offset, TextBox *textbox);
    Overlay *update_time(void);

  private:
    // All
    bool set_overlay_size(EncodeSize size, Point offset);
    bool check_overlay();
    bool fill_overlay_data();

    // Bitmap
    bool init_bitmap_info(FILE *fp);
    bool fill_bitmap_clut(FILE *fp);
    bool create_bitmap_data(FILE *fp);

    // Text / Time
    bool init_text_info(TextBox *textbox);
    bool fill_text_clut();
    bool open_textinsert_lib();
    bool close_textinsert_lib();
    bool create_text_data(const char *str);
    bool char_to_wchar(wchar_t *wide_str, const char *str, uint32_t max_len);

    // Time
    bool init_time_info(TextBox *textbox);
    bool prepare_time_data();
    void get_time_string(char *time_str, uint32_t max_size);
    bool is_lower_letter(char c);
    bool is_upper_letter(char c);
    bool is_digit(char c);

    bool rotate_fill(uint8_t *dst, uint32_t dst_pitch, uint32_t dst_width,
                    uint32_t dst_height, uint32_t dst_x, uint32_t dst_y,
                    uint8_t *src, uint32_t src_pitch, uint32_t src_height,
                    uint32_t src_x, uint32_t src_y,
                    uint32_t data_width, uint32_t data_height);


    bool alloc_clut_mem();
    bool alloc_overlay_mem();
    bool alloc_display_mem();
    void release_mem(uint8_t *addr);

  private:
    bool       mIsTimeOverlay;
    uint32_t   mMaxSize;
    uint32_t   mRotate;
    uint32_t   mColorNum;

    uint32_t   mDisplayWidth;
    uint32_t   mDisplayHeight;
    uint8_t    *mDisplayData;

    uint32_t   mFontWidth;
    uint32_t   mFontHeight;
    uint32_t   mFontPitch;

    Font        mFont;
    OverlayClut mFontColor;
    OverlayClut mOutlineColor;
    OverlayClut mBackgroundColor;

    uint32_t   mOffsetX[TIME_STRING_MAX_SIZE];
    uint32_t   mOffsetY[TIME_STRING_MAX_SIZE];

    uint32_t   mTimeLength;
    char       mTime[TIME_STRING_MAX_SIZE];

    uint8_t    *mDigitData[DIGIT_NUM];
    uint8_t    *mUpperLetterData[LETTER_NUM];
    uint8_t    *mLowerLetterData[LETTER_NUM];

    Overlay mOverlay;
};


#endif /* AM_OVERLAY_GENERATOR_H_ */
