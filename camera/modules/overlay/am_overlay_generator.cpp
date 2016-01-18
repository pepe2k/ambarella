/*******************************************************************************
 * am_webcam_overlay.cpp
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

#ifdef __cplusplus
extern "C" {
#endif
#include "text_insert.h"
#include <wchar.h>
#include <locale.h>
#ifdef __cplusplus
}
#endif

#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_overlay.h"

// bitmap file header 14 bytes. Do not get header size by sizeof().
#define BITMAP_FILE_HEADER_BTYES           (14)
struct BitmapFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

#define BITMAP_INFO_HEADER_BTYES           (40)
struct BitmapInfoHeader {
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMerer;
    uint32_t biYPelsPerMerer;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};

struct RGB {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t reserved;
};

static const char weekStr[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

AmOverlayGenerator::AmOverlayGenerator(uint32_t max_size) :
    mIsTimeOverlay(false),
    mMaxSize(max_size),
    mRotate(0),
    mColorNum(0),
    mDisplayWidth(0),
    mDisplayHeight(0),
    mDisplayData(NULL),
    mFontWidth(0),
    mFontHeight(0),
    mFontPitch(0),
    mTimeLength(0)
{
  memset(&mFont, 0, sizeof(Font));
  memset(&mFontColor,0 , sizeof(OverlayClut));
  memset(&mOutlineColor,0 , sizeof(OverlayClut));
  memset(&mBackgroundColor,0 , sizeof(OverlayClut));
  memset(mOffsetX, 0, sizeof(mOffsetX));
  memset(mOffsetY, 0, sizeof(mOffsetY));
  memset(mTime, 0, sizeof(mTime));
  memset(&mOverlay, 0, sizeof(Overlay));

  for (uint32_t i = 0; i < DIGIT_NUM; ++i) {
    mDigitData[i] = NULL;
  }

  for (uint32_t i = 0; i < LETTER_NUM; ++i) {
    mUpperLetterData[i] = NULL;
    mLowerLetterData[i] = NULL;
  }
}

AmOverlayGenerator::~AmOverlayGenerator()
{
  release_mem(mOverlay.data_addr);
  release_mem(mOverlay.clut_addr);
  release_mem(mDisplayData);

  for (uint32_t i = 0; i < DIGIT_NUM; ++i) {
    release_mem(mDigitData[i]);
  }

  for (uint32_t i = 0; i < LETTER_NUM; ++i) {
    release_mem(mUpperLetterData[i]);
    release_mem(mLowerLetterData[i]);
  }

  DEBUG("~AmOverlayGenerator");
}

void AmOverlayGenerator::set_max_size(uint32_t max_size)
{
  mMaxSize = max_size;
}

Overlay *AmOverlayGenerator::create_bitmap(EncodeSize size, Point offset,
                                       const char *bmpfile)
{
  FILE *fp = NULL;
  Overlay *overlay = NULL;
  if (!bmpfile) {
    ERROR("bitmap file path is NULL.");
    return overlay;
  }
  if (NULL != (fp = fopen(bmpfile, "r"))) {
    if (init_bitmap_info(fp) && set_overlay_size(size, offset) && check_overlay()
        && fill_bitmap_clut(fp) && create_bitmap_data(fp) && fill_overlay_data()) {
      mOverlay.enable = 1;
      overlay = &mOverlay;
    }
    fclose(fp);
  } else {
    ERROR("failed to open bitmap file [%s].", bmpfile);
  }

  return overlay;
}

Overlay *AmOverlayGenerator::create_text(EncodeSize size, Point offset, TextBox *textbox, const char *str)
{
  Overlay *overlay = NULL;

  if (init_text_info(textbox) && set_overlay_size(size, offset) && check_overlay()) {
    if (open_textinsert_lib()) {
      if (fill_text_clut() && create_text_data(str) && fill_overlay_data()) {
        mOverlay.enable = 1;
        overlay = &mOverlay;
      }
      close_textinsert_lib();
    }
  }
  return overlay;
}

Overlay *AmOverlayGenerator::create_time(EncodeSize size, Point offset,
                                     TextBox *textbox)
{
  Overlay *overlay = NULL;
  char time_str[TIME_STRING_MAX_SIZE];
  if (init_time_info(textbox) && set_overlay_size(size, offset) && check_overlay()) {
    if (open_textinsert_lib()) {
      get_time_string(time_str, sizeof(time_str));
      if (fill_text_clut()  && prepare_time_data() && create_text_data(time_str)
          && fill_overlay_data()) {
        mOverlay.enable = 1;
        overlay = &mOverlay;
      }
      close_textinsert_lib();
    }
  }
  return overlay;
}

Overlay* AmOverlayGenerator:: update_time(void)
{
  if (AM_UNLIKELY(!mIsTimeOverlay)) {
    ERROR("Time overlay has not created yet. Create time overlay firstly.");
    return NULL;
  }
  char time_str[TIME_STRING_MAX_SIZE];
  uint32_t n = 0;
  uint8_t *src = NULL;
  get_time_string(time_str, sizeof(time_str));
  for (uint32_t i = 0; i < mTimeLength; i++) {
    if (time_str[i] != mTime[i]) {
      if (is_digit(time_str[i])) {
        n = time_str[i] - '0';
        src = (mDigitData[n] ? mDigitData[n] : NULL);
      } else if (is_lower_letter(time_str[i])) {
        n = time_str[i] - 'a';
        src = (mLowerLetterData[n] ? mLowerLetterData[n] : NULL);
      } else if (is_upper_letter(time_str[i])) {
        n = time_str[i] - 'A';
        src = (mUpperLetterData[n] ? mUpperLetterData[n] : NULL);
      } else {
        src = NULL;
      }
      if (src) {
        rotate_fill(mOverlay.data_addr, mOverlay.pitch, mOverlay.width, mOverlay.height,
            mOffsetX[i], mOffsetY[i], src, mFontPitch, mFontHeight, 0, 0,
            mFontWidth, mFontHeight);
      } else {
        ERROR("Unknown character [%c] in time string [%s].", time_str[i],
            time_str);
      }
    }
  }
  snprintf(mTime, mTimeLength + 1, "%s", time_str);
  return &mOverlay;
}

bool AmOverlayGenerator::set_overlay_size(EncodeSize size, Point offset)
{
  uint32_t stream_rotate_width, stream_rotate_height, tmp;
  if (size.rotate & AM_ROTATE_90) {
    stream_rotate_width = size.height;
    stream_rotate_height = size.width;
  } else {
    stream_rotate_width = size.width;
    stream_rotate_height = size.height;
  }

  if (mDisplayWidth > stream_rotate_width) {
    WARN("Overlay width [%u] > stream display width [%u]. Reset width to [%u],"
        "offset x to 0.",
        mDisplayWidth, stream_rotate_width, stream_rotate_width);
    mDisplayWidth = stream_rotate_width;
    offset.x = 0;
  }

  if (mDisplayHeight > stream_rotate_height) {
    WARN("Overlay height [%u] > stream display height [%u]. Reset height to [%u],"
        "offset y to 0.",
        mDisplayHeight, stream_rotate_height, stream_rotate_height);
    mDisplayHeight = stream_rotate_height;
    offset.y = 0;
  }

  if (offset.x + mDisplayWidth > stream_rotate_width) {
    tmp = stream_rotate_width - mDisplayWidth;
    WARN("Overlay width [%u] + offset x [%u] > Stream display width [%u]. "
        "Reset offset x to [%u]", mDisplayWidth, offset.x, stream_rotate_width,
        tmp);
    offset.x = tmp;
  }

  if (offset.y + mDisplayHeight > stream_rotate_height) {
    tmp = stream_rotate_height - mDisplayHeight;
    WARN("Overlay height [%u] + offset y [%u] > Stream display height [%u]. "
        "Reset offset x to [%u]", mDisplayHeight, offset.y, stream_rotate_height,
        tmp);
    offset.y = tmp;
  }

  if (size.rotate & AM_ROTATE_90) {
    mOverlay.offset_x = (uint16_t)offset.y;
    mOverlay.offset_y = (uint16_t)(size.height - mDisplayWidth - offset.x);
    mOverlay.width = mDisplayHeight;
    mOverlay.height = mDisplayWidth;
  } else {
    mOverlay.offset_x = (uint16_t)offset.x;
    mOverlay.offset_y = (uint16_t)offset.y;
    mOverlay.width = mDisplayWidth;
    mOverlay.height = mDisplayHeight;
  }

  if (size.rotate & AM_VERTICAL_FLIP) {
    mOverlay.offset_x = size.width - mOverlay.width - mOverlay.offset_x;
  }

  if (size.rotate & AM_HORIZONTAL_FLIP) {
    mOverlay.offset_y = size.height - mOverlay.height - mOverlay.offset_y;
  }

  mRotate = size.rotate;
  DEBUG("Stream [%ux%u], rotate [%u]: display [%ux%u] + offset [%ux%u] -> "
      "rotated overlay [%ux%u] + offset [%ux%u].", size.width, size.height,
      size.rotate, mDisplayWidth, mDisplayHeight, offset.x, offset.y,
      mOverlay.width, mOverlay.height, mOverlay.offset_x, mOverlay.offset_y);

  return true;
}

bool AmOverlayGenerator::check_overlay()
{
  bool ret = true;
  uint32_t area_size = 0;
  uint16_t width = 0, height = 0, offset_x = 0, offset_y = 0;
  if (mOverlay.width < OVERLAY_WIDTH_ALIGN ||  mOverlay.height < OVERLAY_HEIGHT_ALIGN) {
    ERROR("Overlay width[%u] x height[%u] cannot be smaller than [%ux%u]",
        mOverlay.width, mOverlay.height, OVERLAY_WIDTH_ALIGN, OVERLAY_HEIGHT_ALIGN);
    ret = false;
  }

  if (mOverlay.width & (OVERLAY_WIDTH_ALIGN - 1)) {
    width = round_down(mOverlay.width, OVERLAY_WIDTH_ALIGN);
    INFO("Overlay width [%u] is not aligned to %u. Round down to [%u].",
         mOverlay.width, OVERLAY_WIDTH_ALIGN, width);
    mOverlay.width = width;
  }

  if (mOverlay.offset_x & (OVERLAY_OFFSET_X_ALIGN - 1)) {
     offset_x = round_down(mOverlay.offset_x, OVERLAY_OFFSET_X_ALIGN);
     INFO("Overlay offset x [%u] is not aligned to %u. Round down to [%u].",
          mOverlay.offset_x, OVERLAY_OFFSET_X_ALIGN, offset_x);
     mOverlay.offset_x = offset_x;
   }

  if (mOverlay.height & (OVERLAY_HEIGHT_ALIGN - 1)) {
    height = round_down(mOverlay.height, OVERLAY_HEIGHT_ALIGN);
    INFO("Overlay height [%u] is not aligned to %u. Round down to [%u].",
        mOverlay.height, OVERLAY_HEIGHT_ALIGN, height);
    mOverlay.height = height;
  }

  if (mOverlay.offset_y & (OVERLAY_HEIGHT_ALIGN - 1)) {
    offset_y = round_down(mOverlay.offset_y, OVERLAY_HEIGHT_ALIGN);
    INFO("Overlay offset_y [%u] is not aligned to %u. Round down to [%u].",
        mOverlay.offset_y, OVERLAY_HEIGHT_ALIGN, offset_y);
    mOverlay.offset_y = offset_y;
  }

  mOverlay.pitch = round_up(mOverlay.width, OVERLAY_PITCH_ALIGN);

  area_size = (uint32_t)(mOverlay.pitch * mOverlay.height);
  if (area_size > mMaxSize) {
    ERROR("Overlay size %u = [%u x %u] exceeds the max size [%u].",
        area_size, mOverlay.pitch, mOverlay.height, mMaxSize);
    ret = false;
  }

  if (mRotate & AM_ROTATE_90) {
    mDisplayWidth = mOverlay.height;
    mDisplayHeight = mOverlay.width;
  } else {
    mDisplayHeight = mOverlay.height;
    mDisplayWidth = mOverlay.width;
  }

  DEBUG("Aligned overlay: display [%ux%u], Overlay [%ux%u] (pitch %u)"
      " + offset [%ux%u].", mDisplayWidth, mDisplayHeight, mOverlay.width,
      mOverlay.height, mOverlay.pitch, mOverlay.offset_x, mOverlay.offset_y);
  return ret;
}


bool AmOverlayGenerator::init_bitmap_info(FILE *fp)
{
  uint32_t max_colors = 0;
  BitmapFileHeader fileHeader = { 0 };
  BitmapInfoHeader infoHeader = { 0 };
  fread(&fileHeader.bfType, sizeof(fileHeader.bfType), 1, fp);
  if (fileHeader.bfType != BITMAP_MAGIC_NUMBER) {
    ERROR("Invalid type [%d]. Not a bitmap.", fileHeader.bfType);
    return false;
  }
  fread(&fileHeader.bfSize, sizeof(fileHeader.bfSize), 1, fp);
  fread(&fileHeader.bfReserved1, sizeof(fileHeader.bfReserved1), 1, fp);
  fread(&fileHeader.bfReserved2, sizeof(fileHeader.bfReserved2), 1, fp);
  fread(&fileHeader.bfOffBits, sizeof(fileHeader.bfOffBits), 1, fp);

  fread(&infoHeader, BITMAP_INFO_HEADER_BTYES, 1, fp);
  if (infoHeader.biBitCount != BITMAP_BIT_NUMBER) {
    ERROR("Invalid [%u]bit. Only support %u bit bitmap.", infoHeader.biBitCount, BITMAP_BIT_NUMBER);
    return false;
  }
  if (infoHeader.biSizeImage != (infoHeader.biWidth * infoHeader.biHeight)) {
    ERROR("Invalid image size [%u]. Not equal to width[%u] x height[%u].",
        infoHeader.biSizeImage, infoHeader.biWidth, infoHeader.biHeight);
    return false;
  }

  mColorNum = (fileHeader.bfOffBits - BITMAP_FILE_HEADER_BTYES
      - BITMAP_INFO_HEADER_BTYES) / sizeof(RGB);
  max_colors = MAX_OVERLAY_CLUT_SIZE / sizeof(OverlayClut);
  if (mColorNum > max_colors) {
    WARN("OffsetBits [%u], color number = %u (>[%u]). Reset to max.",
        fileHeader.bfOffBits, mColorNum, max_colors);
    mColorNum = max_colors;
  }

  mDisplayWidth = (uint16_t) infoHeader.biWidth;
  mDisplayHeight = (uint16_t) infoHeader.biHeight;
  mIsTimeOverlay = false;
  return true;
}

bool AmOverlayGenerator::fill_bitmap_clut(FILE *fp)
{
  bool ret = false;
  if (alloc_clut_mem()) {
    OverlayClut *clut = (OverlayClut *) mOverlay.clut_addr;
    RGB rgb = { 0 };
    for (uint32_t i = 0; i < mColorNum; ++i) {
      fread(&rgb, sizeof(RGB), 1, fp);
      clut[i].y = (uint8_t) (0.257f * rgb.r + 0.504f * rgb.g + 0.098f * rgb.b
          + 16);
      clut[i].u = (uint8_t) (0.439f * rgb.b - 0.291f * rgb.g - 0.148f * rgb.r
          + 128);
      clut[i].v = (uint8_t) (0.439f * rgb.r - 0.368f * rgb.g - 0.071f * rgb.b
          + 128);
      clut[i].alpha = 255;
    }
    ret = true;
  }
  return ret;
}

bool AmOverlayGenerator::create_bitmap_data(FILE *fp)
{
  bool ret = false;
  if (alloc_display_mem()) {
    for (uint32_t i = 0; i < mDisplayHeight; ++i) {
      fread(mDisplayData + (mDisplayHeight - 1 - i) * mDisplayWidth, 1,
          mDisplayWidth, fp);
    }
    ret = true;
  }
  return ret;
}

bool AmOverlayGenerator::init_text_info(TextBox *textbox)
{
  bool ret = false;
  if (textbox) {
    if (textbox->font) {
      memcpy(&mFont, textbox->font, sizeof(Font));
    } else {
      mFont.size = DEFAULT_OVERLAY_FONT_SIZE;
      mFont.outline_width = DEFAULT_OVERLAY_FONT_OUTLINE_WIDTH;
      mFont.ttf = DEFAULT_OVERLAY_FONT_TTF_PATH;
    }
    mFontPitch = mFontWidth = mFont.size;
    mFontHeight = TEXT_LINE_SPACING(mFont.size);

    if (textbox->font_color) {
      memcpy(&mFontColor, textbox->font_color, sizeof(OverlayClut));
    } else {
      mFontColor.y = DEFAULT_OVERLAY_FONT_Y;
      mFontColor.u = DEFAULT_OVERLAY_FONT_U;
      mFontColor.v = DEFAULT_OVERLAY_FONT_V;
      mFontColor.alpha = DEFAULT_OVERLAY_FONT_ALPHA;
    }

    if (textbox->background_color) {
      memcpy(&mBackgroundColor, textbox->background_color, sizeof(OverlayClut));
    } else {
      mBackgroundColor.y = DEFAULT_OVERLAY_BACKGROUND_Y;
      mBackgroundColor.u = DEFAULT_OVERLAY_BACKGROUND_U;
      mBackgroundColor.v = DEFAULT_OVERLAY_BACKGROUND_V;
      mBackgroundColor.alpha = DEFAULT_OVERLAY_BACKGROUND_ALPHA;
    }

    if (mFont.outline_width) {
      if (textbox->outline_color) {
        memcpy(&mOutlineColor, textbox->outline_color, sizeof(OverlayClut));
      } else {
        mOutlineColor.y = DEFAULT_OVERLAY_OUTLINE_Y;
        mOutlineColor.u = DEFAULT_OVERLAY_OUTLINE_U;
        mOutlineColor.v = DEFAULT_OVERLAY_OUTLINE_V;
        mOutlineColor.alpha = DEFAULT_OVERLAY_OUTLINE_ALPHA;
      }
    }

    mDisplayWidth = textbox->width;
    mDisplayHeight = textbox->height;
    mColorNum = MAX_OVERLAY_CLUT_SIZE / sizeof(OverlayClut);
    mIsTimeOverlay = false;
    ret = true;
  } else {
    ERROR("Text Box is NULL.");
  }
  return ret;
}

bool AmOverlayGenerator::open_textinsert_lib()
{
  bool ret = false;
  pixel_type_t pixel_type = {0};

  pixel_type.pixel_background = TEXT_CLUT_ENTRY_BACKGOURND;
  pixel_type.pixel_outline = TEXT_CLUT_ENTRY_OUTLINE;
  pixel_type.pixel_font = mOverlay.clut_size / sizeof(OverlayClut);

  if(text2bitmap_lib_init(&pixel_type) >= 0) {
    font_attribute_t font_attr;
    memset(&font_attr, 0, sizeof(font_attribute_t));
    font_attr.size = mFont.size;
    font_attr.outline_width = mFont.outline_width;
    font_attr.hori_bold = mFont.hor_bold;
    font_attr.vert_bold = mFont.ver_bold;
    font_attr.italic = mFont.italic * 50;
    snprintf(font_attr.type, sizeof(font_attr.type), "%s", mFont.ttf);

    if (text2bitmap_set_font_attribute(&font_attr) < 0) {
      ERROR("Failed to set up font attribute in text insert library.");
      close_textinsert_lib();
    } else {
      ret = true;
    }
  } else {
    ERROR("Failed to init text insert library.");
  }
  return ret;
}

bool AmOverlayGenerator::close_textinsert_lib()
{
  return (text2bitmap_lib_deinit() >= 0);
}


bool AmOverlayGenerator::fill_text_clut()
{
  bool ret = false;

  if (alloc_clut_mem()) {
    OverlayClut *clut = (OverlayClut *) mOverlay.clut_addr;
    // Fill Font color
    for (uint32_t i = 0; i < mColorNum; ++i) {
      clut[i].y = mFontColor.y;
      clut[i].u = mFontColor.u;
      clut[i].v = mFontColor.v;
      clut[i].alpha = ((i < mFontColor.alpha) ? i : mFontColor.alpha);
    }
    // Fill Background color
    memcpy(&clut[TEXT_CLUT_ENTRY_BACKGOURND], &mBackgroundColor,
           sizeof(OverlayClut));

    // Fill Outline color
    if (mFont.outline_width) {
      memcpy(&clut[TEXT_CLUT_ENTRY_OUTLINE], &mOutlineColor,
          sizeof(OverlayClut));
    }
    ret = true;
  }
  DEBUG("Fill text clut is done.");
  return ret;
}

bool AmOverlayGenerator::char_to_wchar(wchar_t *wide_str, const char *str,
                              uint32_t max_len)
{
  setlocale(LC_ALL, "");
  if(mbstowcs(wide_str, str, max_len) < 0) {
    ERROR("Failed to convert char to wchar");
    return false;
  }
  return true;
}

bool AmOverlayGenerator::init_time_info(TextBox *textbox)
{
  bool ret = false;

  if (init_text_info(textbox)) {
    mIsTimeOverlay = true;
    ret = true;
  }
  return ret;
}

bool AmOverlayGenerator::prepare_time_data()
{
  if (!mFontPitch || !mFontHeight) {
    ERROR("Font pitch [%u] or height [%u] is zero.", mFontPitch, mFontHeight);
    return false;
  }
  mFontWidth = 0;
  bitmap_info_t bmp_info = { 0 };
  uint32_t digit_size = mFontPitch * mFontHeight;
  for (uint32_t i = 0; i < DIGIT_NUM; ++i) {
    release_mem(mDigitData[i]);
    mDigitData[i] = (uint8_t *) malloc(digit_size);
    if (!mDigitData[i]) {
      ERROR("Cannot malloc memory for digit %u (size %ux%u).", i,
          mFontPitch, mFontHeight);
      return false;
    }
    memset(mDigitData[i], 0, digit_size);
    if (text2bitmap_convert_character(i + L'0', mDigitData[i], mFontHeight,
            mFontPitch, 0, &bmp_info) < 0) {
      ERROR("Failed to convert digit %u in text insert library.", i);
      return false;
    }
    if (bmp_info.width > (int32_t) mFontWidth) {
      mFontWidth = (uint32_t) bmp_info.width;
    }
  }

  for (uint32_t i = 0 ; i < LETTER_NUM; ++ i) {
    release_mem(mUpperLetterData[i]);
    release_mem(mLowerLetterData[i]);
  }

  uint32_t week_size = sizeof(weekStr[0]);
  uint32_t week_num = sizeof(weekStr) / week_size;
  uint32_t n = 0, letter_size = mFontPitch * mFontHeight;
  uint32_t is_lower = false;
  wchar_t wc = 0;
  for (uint32_t i = 0; i < week_num; ++i) {
    for (uint32_t j = 0; j < week_size - 1; ++j) {
      if (is_lower_letter(weekStr[i][j])) {
        n = weekStr[i][j] - 'a';
        if (mLowerLetterData[n])
          continue;
        wc = n + L'a';
        is_lower = true;
      } else if (is_upper_letter(weekStr[i][j])) {
        n = weekStr[i][j] - 'A';
        if (mUpperLetterData[n])
          continue;
        wc = n + L'A';
        is_lower = false;
      } else {
        ERROR("[%c] in weekStr [%s] is not a letter", weekStr[i][j], weekStr[i]);
        return false;
      }
      uint8_t *letter_addr = NULL;
      letter_addr = (uint8_t *) malloc(letter_size);
      if (!letter_addr) {
        ERROR("Cannot malloc memory for letter %c (size %ux%u).",
            weekStr[i][j], mFontPitch, mFontHeight);
        return false;
      }
      memset(letter_addr, 0, letter_size);
      if (is_lower) {
        mLowerLetterData[n] = letter_addr;
      } else {
        mUpperLetterData[n] = letter_addr;
      }
      if (text2bitmap_convert_character(wc, letter_addr, mFontHeight,
          mFontPitch, 0, &bmp_info) < 0) {
        ERROR("Failed to convert letter %c in text insert library.",
            weekStr[i][j]);
        return false;
      }
      if (bmp_info.width > (int32_t) mFontWidth) {
        mFontWidth = (uint32_t) bmp_info.width;
      }
    }
  }
  DEBUG("Font pitch %u, height %u, width %u.", mFontPitch, mFontHeight, mFontWidth);
  return true;
}

bool AmOverlayGenerator::create_text_data(const char *str)
{
  if (alloc_display_mem()) {
    uint32_t len = strlen(str);

    DEBUG("string is [%s]", str);
    if (mIsTimeOverlay) {
      uint32_t max_len = sizeof(mTime) - 1;
      if (AM_UNLIKELY(len > max_len)) {
        INFO("The length [%d] of time string exceeds the max length [%d]."
        " Display %d at most.", len, max_len, max_len);
        len = max_len;
      }
    }
    wchar_t *wide_str = new wchar_t[len];
    if (char_to_wchar(wide_str, str, len)) {
      uint32_t offset_x = 0, offset_y = 0, display_len = 0;
      bitmap_info_t bmp_info = { 0 };
      uint8_t *line_head = mDisplayData;
      for (uint32_t i = 0; i < len; ++i) {
        if ((offset_x + mFontPitch) > (uint32_t) mDisplayWidth) {
          // Add a new line
          DEBUG("Add a new line.");
          offset_y += mFontHeight;
          offset_x = 0;
          line_head += mDisplayWidth * mFontHeight;
        }
        if ((mFontHeight + offset_y) > (uint32_t) mDisplayHeight) {
          // No more new line
          DEBUG("No more space for a new line. %u + %u > %u", mFontHeight,
              offset_y, mDisplayHeight);
          break;
        }

        if (mIsTimeOverlay) {
          // Remember the charactor's offset in the overlay data
          mOffsetX[i] = offset_x;
          mOffsetY[i] = offset_y;
        }

        if (mIsTimeOverlay &&
            (is_digit(str[i]) || is_lower_letter(str[i]) || is_upper_letter(str[i]))) {
          // Digit and letter in time string
          uint8_t *dst = line_head + offset_x;
          uint8_t *src = NULL;
          if (is_digit(str[i])) {
            src = mDigitData[str[i] - '0'];
          } else if (is_lower_letter(str[i])) {
            src = mLowerLetterData[str[i] - 'a'];
          } else if (is_upper_letter(str[i])) {
            src = mUpperLetterData[str[i] - 'A'];
          }
          if (src) {
            for (uint32_t row = 0; row < mFontHeight; ++row) {
              memcpy(dst, src, mFontWidth);
              dst += mDisplayWidth;
              src += mFontPitch;
            }
            offset_x += mFontWidth;
          }
        } else {
          // Neither a digit nor a converted letter in time string
          if (text2bitmap_convert_character(wide_str[i], line_head,
              mFontHeight, mDisplayWidth, offset_x, &bmp_info) < 0) {
            ERROR("text2bitmap library： Failed to convert the charactor "
            "[%c].", str[i]);
            return false;
          }
          offset_x += bmp_info.width;
        }
        display_len++;
      }
      if (mIsTimeOverlay) {
        mTimeLength = display_len;
        snprintf(mTime, mTimeLength + 1, "%s", str);
        DEBUG("Time display length %u", mTimeLength);
      }
    }
    delete[] wide_str;
  } else {
    return false;
  }

  return true;
}

bool AmOverlayGenerator::fill_overlay_data()
{
  bool ret = false;
  if (alloc_overlay_mem()) {
    if (rotate_fill(
        mOverlay.data_addr, mOverlay.pitch, mOverlay.width, mOverlay.height, 0, 0,
        mDisplayData, mDisplayWidth, mDisplayHeight, 0, 0,
        mDisplayWidth, mDisplayHeight)) {
      ret = true;
    }
  }
  return ret;
}

bool AmOverlayGenerator::rotate_fill(uint8_t *dst, uint32_t dst_pitch, uint32_t dst_width,
                                 uint32_t dst_height, uint32_t dst_x, uint32_t dst_y,
                                 uint8_t *src, uint32_t src_pitch, uint32_t src_height,
                                 uint32_t src_x, uint32_t src_y,
                                 uint32_t data_width, uint32_t data_height)
{
  if (AM_UNLIKELY(!src || !dst)) {
    ERROR("src or dst is NULL.");
    return false;
  }

  if (AM_UNLIKELY(
      (data_width + src_x > src_pitch) ||
      (data_height + src_y > src_height))) {
    ERROR("Failed to fill overlay data. Rotate flag [%u]. "
        "Src: [%ux%u] >= [%ux%u] + offset [%ux%u],", mRotate,
        src_pitch, src_height, data_width, data_height,
        src_x, src_y);
    return false;
  }
  uint8_t *sp = src + src_y * src_pitch + src_x;
  uint8_t *dp = NULL;
  uint32_t row = 0, col = 0;

  switch (mRotate) {
    case AM_NO_ROTATE_FLIP:
      dp = dst + dst_y * dst_pitch + dst_x;
      for (row = 0; row < data_height; ++ row) {
        memcpy(dp, sp, data_width);
        sp += src_pitch;
        dp += dst_pitch;
      }
      break;
    case AM_CW_ROTATE_90:
      dp = dst + (dst_height - dst_x - 1) * dst_pitch + dst_y;
      for (row = 0; row < data_height; ++ row) {
        for (col = 0; col < data_width; ++ col) {
          *(dp - col * dst_pitch) = *(sp + col);
        }
        sp += src_pitch;
        dp ++;
      }
      break;
    case AM_HORIZONTAL_FLIP:
      dp = dst + dst_y * dst_pitch + dst_width - dst_x - 1;
      for (row = 0; row < data_height; ++ row) {
        for (col = 0; col < data_width; ++ col) {
          *(dp - col) = *(sp + col);
        }
        sp += src_pitch;
        dp += dst_pitch;
      }
      break;
    case AM_VERTICAL_FLIP:
      dp = dst + (dst_height - dst_y - 1) * dst_pitch + dst_x;
      for (row = 0; row < data_height; ++ row) {
        memcpy(dp, sp, data_width);
        sp += src_pitch;
        dp -= dst_pitch;
      }
      break;
    case AM_CW_ROTATE_180:
      dp = dst + (dst_height - dst_y - 1) * dst_pitch + dst_width - dst_x - 1;
      for (row = 0; row < data_height; ++ row) {
        for (col = 0; col < data_width; ++ col) {
          *(dp - col) = *(sp + col);
        }
        sp += src_pitch;
        dp -= dst_pitch;
      }
      break;
    case AM_CW_ROTATE_270:
      dp = dst + dst_x * dst_pitch + dst_width - dst_y - 1;
      for (row = 0; row < data_height; ++row) {
        for (col = 0; col < data_width; ++col) {
          *(dp + col * dst_pitch) = *(sp + col);
        }
        sp += src_pitch;
        dp--;
      }
      break;
    case (AM_HORIZONTAL_FLIP | AM_ROTATE_90):
      dp = dst + dst_x * dst_pitch + dst_y;
      for (row = 0; row < data_height; ++row) {
        for (col = 0; col < data_width; ++col) {
          *(dp + col * dst_pitch) = *(sp + col);
        }
        sp += src_pitch;
        dp++;
      }
      break;
    case (AM_VERTICAL_FLIP | AM_ROTATE_90):
      dp = dst + (dst_height - dst_x - 1) * dst_pitch + dst_width - dst_y - 1;
      for (row = 0; row < data_height; ++row) {
        for (col = 0; col < data_width; ++col) {
          *(dp - col * dst_pitch) = *(sp + col);
        }
        sp += src_pitch;
        dp--;
      }
      break;
    default:
      break;
  }

  return true;
}

bool AmOverlayGenerator::alloc_clut_mem()
{
  bool ret = false;
  release_mem(mOverlay.clut_addr);
  if (mColorNum) {
    mOverlay.clut_size = mColorNum * sizeof(OverlayClut);
    mOverlay.clut_addr = (uint8_t *)malloc(mOverlay.clut_size);
    if (mOverlay.clut_addr) {
      memset(mOverlay.clut_addr , 0, mOverlay.clut_size);
      ret = true;
    } else {
      ERROR("cannot malloc memory [%u] for overlay clut.", mOverlay.clut_size);
    }
  }  else {
    ERROR("Overlay color number is zero.");
  }
  return ret;
}

bool AmOverlayGenerator::alloc_overlay_mem()
{
  bool ret = false;
  release_mem(mOverlay.data_addr);
  if (mOverlay.pitch && mOverlay.height) {
    uint32_t data_size = (uint32_t)(mOverlay.pitch * mOverlay.height);
    mOverlay.data_addr = (uint8_t *)malloc(data_size);
    if (mOverlay.data_addr) {
      memset(mOverlay.data_addr, 0, data_size);
      ret = true;
    } else {
      ERROR("cannot malloc memory [%u] for overlay data.", data_size);
    }
  }  else {
    ERROR("Overlay size [%u]x[%u] (pitch %u) cannot be zero.", mOverlay.width,
        mOverlay.height, mOverlay.pitch);
  }
  return ret;
}

bool AmOverlayGenerator::alloc_display_mem()
{
  bool ret = false;
    release_mem(mDisplayData);
    if (mDisplayWidth && mDisplayHeight) {
      uint32_t display_size = (uint32_t)(mDisplayWidth * mDisplayHeight);
      mDisplayData = (uint8_t *)malloc(display_size);
      if (mDisplayData) {
        memset(mDisplayData, 0, display_size);
        ret = true;
      } else {
        ERROR("cannot malloc memory [%u] for display data.", display_size);
      }
    }  else {
      ERROR("Display size [%ux%u] cannot be zero.", mDisplayWidth,
          mDisplayHeight);
    }
    return ret;
}

void AmOverlayGenerator::release_mem(uint8_t *addr)
{
  if (addr) {
    free(addr);
    addr = NULL;
  }
}

void AmOverlayGenerator::get_time_string(char *time_str, uint32_t max_size)
{
  time_t t = time(NULL);
  struct tm *p = gmtime(&t);
  snprintf(time_str, max_size, "%04d-%02d-%02d %s %02d:%02d:%02d",
           (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday,
           weekStr[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
}

bool AmOverlayGenerator::is_lower_letter(char c)
{
  return c >= 'a' && c <= 'z';
}

bool AmOverlayGenerator::is_upper_letter(char c)
{
  return c >= 'A' && c <= 'Z';
}

bool AmOverlayGenerator::is_digit(char c)
{
  return c >= '0' && c <= '9';
}
