/*
 * bmp_convert.c
 *
 * History:
 * 2011/1/13 - [jy qiu] created for A5s
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>

#include <signal.h>


static int loadbmp_flag = 0;
static char bmp_filename[256];

static int 	osd_clut_flag = 0;
static int 	osd_data_flag = 0;
static char osd_clut_filename[256];
static char osd_data_filename[256];

const static char *default_osd_clut_filename="bmp_clut.dat";
const static char *default_osd_data_filename="bmp_data.dat";
const static char *default_bmp_filename="/usr/local/bin/data/Ambarella-256x128-8bit.bmp";

//bmp file header  14bytes
typedef struct tagBITMAPFILEHEADER
{
u16	bfType;			// file type
u32	bfSize;			//file size
u16	bfReserved1;
u16	bfReserved2;
u32	bfOffBits;
}BITMAPFILEHEADER;
//bmp inforamtion header 40bytes
typedef struct tagBITMAPINFOHEADER
{
u32	biSize;
u32	biWidth;				//bmp width
u32        biHeight;		//bmp height
u16        biPlanes;
u16        biBitCount;		// 1,4,8,16,24 ,32 color attribute
u32        biCompression;
u32        biSizeImage;		//Image size
u32        biXPelsPerMerer;
u32        biYPelsPerMerer;
u32        biClrUsed;
u32        biClrImportant;
}BITMAPINFOHEADER;

//clut, if 256 bit map it needs,
typedef struct tagRGBQUAD
{
u8        rgbBlue;
u8        rgbGreen;
u8        rgbRed;
u8        rgbReserved;
}RGBQUAD;


typedef struct osd_clut_s {
	u8 v;
	u8 u;
	u8 y;
	u8 alpha;
} osd_clut_t;


static void RGB_To_YUV(const RGBQUAD rgb, osd_clut_t *yuv )
{
	yuv->y= (unsigned char)( 0.257f * rgb.rgbRed+ 0.504f * rgb.rgbGreen+ 0.098f * rgb.rgbBlue+16) ;
	yuv->u= (unsigned char)(+ 0.439f * rgb.rgbBlue -0.291f * rgb.rgbGreen -0.148f* rgb.rgbRed +128);
	yuv->v= (unsigned char)( 0.439f *  rgb.rgbRed - 0.368f * rgb.rgbGreen - 0.071f * rgb.rgbBlue+128);

}


int write_file(const char *filename, u8 *buffer, u32 buffer_len)
{
	int fd;
		printf ("write_file %s",filename);
	if ((fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
		printf("fail\n");
		return -1;
	}

	if (write(fd, buffer, buffer_len) <0) {
		printf("fail\n");
		close(fd);
		return -1;
	}
	printf (" success !\n");
	close(fd);
	return 0;
}


#define NO_ARG	  0
#define HAS_ARG	 1
static struct option long_options[] = {
	{"help", NO_ARG, 0, 'h'},
	{"clut_file", HAS_ARG, 0, 'c'},
	{"data_file", HAS_ARG, 0, 'd'},
	{"load_bmp", HAS_ARG, 0, 'L'},

	{0, 0, 0, 0}
};

static const char *short_options = "c:d:L:h";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"usage", "if there is no option, use the default bmp: /usr/locl/bin/data/Ambarella-256x128-8bit.bmp"},
	{"clut_name", "\tgenrate clut file"},
	{"data_file", "\tgenrate yuv index file"},
	{"bmp name", "\t load 8bit BMP"},

};

void usage(void)
{
	int i;

	printf("bmp_convert usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
}

static int  bmp_convert(void)
{
	FILE * fp;
	if ((fp= fopen(bmp_filename, "r") ) ==NULL)
		{
	 	printf ("%s open fail or not exists \n",bmp_filename);
		return -1;
		}

	BITMAPFILEHEADER filehead, *fileheadp = &filehead;
	BITMAPINFOHEADER infohead, *infoP = &infohead;

	 memset(fileheadp, 0, sizeof(BITMAPFILEHEADER));
	 memset(infoP, 0, sizeof(BITMAPINFOHEADER));



	 fread(&filehead.bfType, sizeof(filehead.bfType), 1, fp);
	 fread(&filehead.bfSize, sizeof(filehead.bfSize), 1, fp);
	 fread(&filehead.bfReserved1, sizeof(filehead.bfReserved1), 1, fp);
	 fread(&filehead.bfReserved2, sizeof(filehead.bfReserved2), 1, fp);
	 fread(&filehead.bfOffBits, sizeof(filehead.bfOffBits), 1, fp);

	if (filehead.bfType!=0x4d42)
		{
		printf("this is not a bmp file \n");
		fclose(fp);
		return -1;
	}

 	fread(infoP, sizeof(BITMAPINFOHEADER), 1, fp);

	 int biW = infohead.biWidth;
	 int biH =infohead.biHeight;
	 int sizeImg = ( infohead.biSizeImage);

	if (infohead.biBitCount>8)
		{
		printf("please use 8bit bmp file \n");
		fclose(fp);
		return -1;
	}
//	printf("bfType [%d]\t  bfSize[%d]\t filehead.bfOffBits[%d]\t\n",filehead.bfType,filehead.bfSize,filehead.bfOffBits);

 	int color_count=(filehead.bfOffBits-54)/4;

	printf("bmp_Width [%d]\t  bmp_height[%d]\t color_count[%d]\t\n",biW,biH,color_count);


	RGBQUAD  rgbtables[color_count];
	int i=0,j=0;

	for(i=0;i<color_count;i++)
		fread(&rgbtables[i], sizeof(RGBQUAD), 1, fp);

	u8 Index_data[sizeImg];
	for(i=0;i<sizeImg;i++)
		fread(&Index_data[i], sizeof(u8), 1, fp);
	fclose(fp);

	u8 Index_data_convert[sizeImg];
	if (sizeImg==biW*biH)  //convert the bmp index sorting
		for (i=0;i<biH;i++)
			for (j=0;j<biW;j++)
				Index_data_convert[(biH-i)*biW+j]=Index_data[i*biW+j];


	osd_clut_t   yuvtables[color_count][1];
	for(j=0;j<color_count; j++)
		{
		RGB_To_YUV(rgbtables[j], yuvtables[j]);
		yuvtables[j][0].alpha=255;
	}

	write_file(osd_clut_filename,(u8 *)yuvtables,color_count*sizeof(osd_clut_t)/sizeof(u8));

	write_file(osd_data_filename,Index_data_convert,sizeImg);

	return 0;
}


int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
			case 'h':
					usage();
					return -1;
			case 'c':
					strcpy(osd_clut_filename, optarg);
					osd_clut_flag = 1;
					break;
			case 'd':
					strcpy(osd_data_filename, optarg);
					osd_data_flag = 1;
					break;
			case 'L':
					strcpy(bmp_filename, optarg);
					loadbmp_flag=1;
					break;
			default:
					printf("unknown option found: %c\n", ch);
					return -1;
		}
	}
	if (!osd_clut_flag)
		strcpy(osd_clut_filename, default_osd_clut_filename);
	if (!osd_data_flag)
		strcpy(osd_data_filename, default_osd_data_filename);
	if (!loadbmp_flag)
		{
		printf("load default bmp\n");
		strcpy(bmp_filename, default_bmp_filename);
		}
	return 0;
}

int main(int argc, char **argv)
{

	if (init_param(argc, argv) < 0)
		return -1;

	if(bmp_convert()<0)
			return -1;

	return 0;
}

