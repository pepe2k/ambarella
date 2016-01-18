/*
 * udecutil.h
 *
 * History:
 *	2013/03/31 - [Zhi He] created file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

extern int enable_debug_log;

#if 0
#define u_printf_flow u_printf
#else
#define u_printf_flow(format, args...)	(void)0
#endif

#define u_assert(expr) do { \
	if (!(expr)) { \
		u_printf("assertion failed: %s\n\tAt %s : %d\n", #expr, __FILE__, __LINE__); \
	} \
} while (0)

int u_printf(const char *fmt, ...);

#define u_printf_error(format,args...)  do { \
	u_printf("[Error] at %s:%d: ", __FILE__, __LINE__); \
	u_printf(format,##args); \
} while (0)

#define u_printf_debug(format,args...)  do { \
	if (enable_debug_log) { \
		u_printf(format,##args); \
	} \
} while (0)

/* BMP file header structure */
typedef struct bmp_file_header_s {
	unsigned short id;			/** the magic number used to identify
				 *  'BM' - Windows 3.1x, 95, NT, ...
				 *  'BA' - OS/2 Bitmap Array
				 *  'CI' - OS/2 Color Icon
				 *  'CP' - OS/2 Color Pointer
				 *  'IC' - OS/2 Icon
				 *  'PT' - OS/2 Pointer
				 *  the BMP file: 0x42 0x4D
				 *  (ASCII code points for B and M)
				 *  Need 0x424d
				 */
	unsigned int file_size;		/* the size of the BMP file in bytes */
	unsigned int reserved;		/* reserved */
	unsigned int offset;		/** the offset, i.e. starting address,
				 *  of the byte where the bitmap data
				 *  can be found.
				 */
} bmp_file_header_t;

/* BMP info header structure */
typedef struct bmp_info_header_s {
	unsigned int bmp_header_size;	/** 0x28 : Windows V3, all Windows versions
				 *	   since Windows 3.0
				 *  0x0c : OS/2 V1
				 *  0xf0 : OS/2 V2
				 *  0x6c : Windows V4, all Windows versions
				 *	   since Windows 95/NT4
				 *  0x7c : Windows V5, Windows 98/2000 and newer
				 *  Supports 0x28 now.
				 */
	unsigned int width;		/** the bitmap width in pixels (signed integer)
				 *  Need less or equal to 360.
				 */
	unsigned int height;		/** the bitmap height in pixels (signed integer)
				 *  Need less or equal to 240.
				 */
	unsigned short planes;		/** the number of color planes being used.
				 *  Must be set to 1.
				 */
	unsigned short bits_per_pixel;	/** the number of bits per pixel,
				 *  which is the color depth of the image.
				 *  Typical values are 1, 4, 8, 16, 24 and 32.
				 *  Need to 8 now.
				 */
	unsigned int compression;	/** the compression method being used.
				 *  Support 0, none compression now.
				 */
	unsigned int bmp_data_size;	/** the image size.
				 *  This is the size of the raw bitmap data.
				 */
	unsigned int h_resolution;	/** the horizontal resolution of the image.
				 *  (pixel per meter, signed integer)
				 */
	unsigned int v_resolution;	/** the vertical resolution of the image.
				 *  (pixel per meter, signed integer)
				 */
	unsigned int used_colors;	/** the number of colors in the color palette,
				 *  or 0 to default to 2n.
				 */
	unsigned int important_colors;	/** the number of important colors used,
				 *  or 0 when every color is important;
				 *  generally ignored.
				 */
} bmp_info_header_t;

/* BMP header structure */
typedef struct bmp_header_s {
	bmp_file_header_t fheader;
	bmp_info_header_t iheader;
} bmp_header_t;

/* BMP data structure */
typedef struct bmp_s {
	bmp_header_t header;
	unsigned char *bmp_data;		/* Point to the addr that stored BMP raw data */
	unsigned char *bmp_clut;		/* Point to the color palatte */
	unsigned int width;		/* The bmp width */
	unsigned int height;		/* The bmp height */
	unsigned int pitch;		/* The bmp pitch */
	unsigned int size;		/* The bmp size */
	unsigned char  color_depth;	/* Color depth of CLUT */

	unsigned char* buf;
	FILE* fd;
	unsigned int filesize;

	//for display
	unsigned char* display_buf;
	unsigned int display_buf_size;
	unsigned int target_width;
	unsigned int target_height;
	unsigned int target_format;
} bmp_t;

enum {
	e_buffer_format_clut8 = 0,
	e_buffer_format_rgb565,
	e_buffer_format_rgba32,
};

void release_bmp_file(bmp_t *bmp);
int open_bmp_file(char* filename, bmp_t *bmp);
int bmf_file_to_display_buffer(bmp_t *bmp, unsigned int target_width, unsigned int target_height, unsigned int target_format);

void set_end_of_string(char* string, char end, unsigned int max_length);
