/*
 *build_ts.h
 *
 * History:
 *    2011/4/17 - [Kaiming Xie] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __BUILD_TS_H__
#define __BUILD_TS_H__

#include "mpeg_ts_defs.h"

/******
  Defines
 ******/

#define PAT_HDR_SIZE            (8)
#define PAT_ELEMENT_SIZE        (4)
#define PMT_HDR_SIZE            (12)
#define PMT_ELEMENT_SIZE        (5)
#define PMT_STREAM_DSC_SIZE     (3)
#define PMT_HDR_SIZE            (12)

#define PES_HEADER_LEN          (9)
#define ADAPT_HEADER_LEN         (2)

#define CTSMUXPSI_STREAM_TOT     (2) //audio & video

#define PCR_FIELD_SIZE		(6)
#define PTS_FIELD_SIZE		(5)
#define DTS_FIELD_SIZE		(5)


/**********
  Typedefs
 **********/

// Stream Descriptor
typedef struct {
   AM_U32 pat  :5; /*!< version number for pat table. */
   AM_U32 pmt  :5; /*!< version number for pmt table. */
} CTSMUXPSI_VER;


/**
 *
 * Stream configuration
 */
typedef struct {
   AM_U8 firstFrame;
   AM_U8 firstSlice;
   AM_U8 withPCR;
   AM_U8* pPlayload;
   AM_INT payloadSize;
   AM_PTS pts;
   AM_PTS dts;
   AM_PTS pcr_base;
   AM_U16 pcr_ext;
} CTSMUXPES_PAYLOAD_INFO;

/**
 *
 * Stream configuration
 */
typedef struct {
   MPEG_SI_STREAM_TYPE type; /*!< Program type. */
   AM_U16              pid; /*!< Packet ID to be used in the TS. */
   AM_U8               descriptor_tag;
   AM_UINT             descriptor_len;
   AM_U8*              pDescriptor;
} CTSMUXPSI_STREAM_INFO;

/**
 *
 * Program configuration.
 */
typedef struct {
   AM_U16 pidPMT; /*!< Packet ID to be used to store the Program Map Table [PMT]. */
   AM_U16 pidPCR; /*!< Packet ID to be used to store the Program Clock Referene [PCR]. */
   AM_U16 prgNum; /*!< Program Number to be used in Program Map Table [PMT]. */
} CTSMUXPSI_PRG_INFO;

/**
 *
 * Program Map Table [PMT] configuration.
 */
typedef struct {
   CTSMUXPSI_PRG_INFO*    prgInfo;
   /*!< Total number of stream within the program. */
   AM_U16                 totStream;
   /*!< List of stream configurations. */
   CTSMUXPSI_STREAM_INFO* stream[CTSMUXPSI_STREAM_TOT];
   AM_U8                  descriptor_tag;
   AM_UINT                descriptor_len;
   AM_U8*                 pDescriptor;
} CTSMUXPSI_PMT_INFO;

/**
 *
 * Program Association table [PAT] configuration
 */
typedef struct {
   AM_U16              totPrg; /*!< Total number of valid programs. */
   CTSMUXPSI_PRG_INFO* prgInfo; /*!< List of program configurations. */
} CTSMUXPSI_PAT_INFO;


/**
 *
 * Header for a Program Association table [PAT]
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
typedef struct {
#ifdef BIGENDIAN
   /*Btype 7*/
   AM_U8 last_section_number      : 8;
   /*Btype 6*/
   AM_U8 section_number           : 8;
   /*Btype 5*/
   AM_U8 reserved1                : 2;
   AM_U8 version_number           : 5;
   AM_U8 current_next_indicator   : 1;
   /*Btype 4*/
   AM_U8 transport_stream_id_l    : 8;
   /*Btype 3*/
   AM_U8 transport_stream_id_h    : 8;
   /*Btype 2*/
   AM_U8 section_length0to7       : 8;
   /*Btype 1*/
   AM_U8 section_syntax_indicator : 1;
   AM_U8 b0                       : 1;
   AM_U8 reserved0                : 2;
   AM_U8 section_length8to11      : 4;
   /*Btype 0*/
   AM_U8 table_id                 : 8;
#else
   /*Btype 0*/
   AM_U8 table_id                 : 8;
   /*Btype 1*/
   AM_U8 section_length8to11      : 4;
   AM_U8 reserved0                : 2;
   AM_U8 b0                       : 1;
   AM_U8 section_syntax_indicator : 1;
   /*Btype 2*/
   AM_U8 section_length0to7       : 8;
   /*Btype 3*/
   AM_U8 transport_stream_id_h    : 8;
   /*Btype 4*/
   AM_U8 transport_stream_id_l    : 8;
   /*Btype 5*/
   AM_U8 current_next_indicator   : 1;
   AM_U8 version_number           : 5;
   AM_U8 reserved1                : 2;
   /*Btype 6*/
   AM_U8 section_number           : 8;
   /*Btype 7*/
   AM_U8 last_section_number      : 8;
#endif
} PAT_HDR;

/**
 *
 * Program Association table [PAT] Element
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
typedef struct {
#ifdef BIGENDIAN
   /* Btype 3 */
   AM_U8 program_map_PID_l : 8;
   /* Btype 2 */
   AM_U8 reserved2         : 3;
   AM_U8 program_map_PID_h : 5;
   /* Btype 1 */
   AM_U8 program_number_l  : 8;
   /* Btype 0 */
   AM_U8 program_number_h  : 8;
#else
   /* Btype 0 */
   AM_U8 program_number_h  : 8;
   /* Btype 1 */
   AM_U8 program_number_l  : 8;
   /* Btype 2 */
   AM_U8 program_map_PID_h : 5;
   AM_U8 reserved2         : 3;
   /* Btype 3 */
   AM_U8 program_map_PID_l : 8;
#endif
} PAT_ELEMENT;
/**
 *
 * Header for a Program Map Table [PMT]
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
typedef struct {
#ifdef BIGENDIAN
   /*Btype 11*/
   AM_U8 program_info_length0to7  : 8;
   /*Btype 10*/
   AM_U8 reserved3                : 4;
   AM_U8 program_info_length8to11 : 4;
   /*Btype 9*/
   AM_U8 PCR_PID0to7              : 8;
   /*Btype 8*/
   AM_U8 reserved2                : 3;
   AM_U8 PCR_PID8to12             : 5;
   /*Btype 7*/
   AM_U8 last_section_number      : 8;
   /*Btype 6*/
   AM_U8 section_number           : 8;
   /*Btype 5*/
   AM_U8 reserved1                : 2;
   AM_U8 version_number           : 5;
   AM_U8 current_next_indicator   : 1;
   /*Btype 4*/
   AM_U8 program_number_l         : 8;
   /*Btype 3*/
   AM_U8 program_number_h         : 8;
   /*Btype 2*/
   AM_U8 section_length0to7       : 8;
   /*Btype 1*/
   AM_U8 section_syntax_indicator : 1;
   AM_U8 b0                       : 1;
   AM_U8 reserved0                : 2;
   AM_U8 section_length8to11      : 4;
   /*Btype 0*/
   AM_U8 table_id                 : 8;
#else
   /*Btype 0*/
   AM_U8 table_id                 : 8;
   /*Btype 1*/
   AM_U8 section_length8to11      : 4;
   AM_U8 reserved0                : 2;
   AM_U8 b0                       : 1;
   AM_U8 section_syntax_indicator : 1;
   /*Btype 2*/
   AM_U8 section_length0to7       : 8;
   /*Btype 3*/
   AM_U8 program_number_h         : 8;
   /*Btype 4*/
   AM_U8 program_number_l         : 8;
   /*Btype 5*/
   AM_U8 current_next_indicator   : 1;
   AM_U8 version_number           : 5;
   AM_U8 reserved1                : 2;
   /*Btype 6*/
   AM_U8 section_number           : 8;
   /*Btype 7*/
   AM_U8 last_section_number      : 8;
   /*Btype 8*/
   AM_U8 PCR_PID8to12             : 5;
   AM_U8 reserved2                : 3;
   /*Btype 9*/
   AM_U8 PCR_PID0to7              : 8;
   /*Btype 10*/
   AM_U8 program_info_length8to11 : 4;
   AM_U8 reserved3                : 4;
   /*Btype 11*/
   AM_U8 program_info_length0to7  : 8;
#endif
} PMT_HDR;

/**
 *
 * Program Map Table [PMT] element
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
typedef struct {
#ifdef BIGENDIAN
   /*Btype 4*/
   AM_U8 ES_info_length_l    : 8;
   /*Btype 3*/
   AM_U8 reserved1           : 4;
   AM_U8 ES_info_length_h    : 4;
   /*Btype 2*/
   AM_U8 elementary_PID0to7  : 8;
   /*Btype 1*/
   AM_U8 reserved0           : 3;
   AM_U8 elementary_PID8to12 : 5;
   /*Btype 0*/
   AM_U8 stream_type         : 8;
#else
   /*Btype 0*/
   AM_U8 stream_type         : 8;
   /*Btype 1*/
   AM_U8 elementary_PID8to12 : 5;
   AM_U8 reserved0           : 3;
   /*Btype 2*/
   AM_U8 elementary_PID0to7  : 8;
   /*Btype 3*/
   AM_U8 ES_info_length_h    : 4;
   AM_U8 reserved1           : 4;
   /*Btype 4*/
   AM_U8 ES_info_length_l    : 8;
#endif
} PMT_ELEMENT;
/**
 *
 * Stream Descriptor for Program Map Table [PMT] element
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
typedef struct {
   AM_U32 descriptor_tag:8;
   AM_U32 descriptor_length:8;
   AM_U32 component_tag:8;
} PMT_STREAM_DSC;

class CTsBuilder
{
  public:
    CTsBuilder();
    ~CTsBuilder(){};
    void Delete();

    /**
     * This method creates TS packet with Program Association table [PAT].
     *
     * @param pPatInfo informaion required for PAT table.
     * @param pBufPAT  pointer to the destination of newly created PAT.
     *
     */
    AM_ERR CreatePAT(CTSMUXPSI_PAT_INFO *pPatInfo, AM_U8 * pBufPAT);

    /**
     * This method creates TS packet with Program Map Table [PMT].
     *
     * @param pPatInfo informaion required for PAT table.
     * @param pBufPMT  pointer to the destination of newly created PMT.
     *
     */
    AM_ERR CreatePMT(CTSMUXPSI_PMT_INFO *pPmtInfo, AM_U8 * pBufPMT);

    /**
     * This method creates TS packet with Program Clock Reference packet [PCR].
     *
     * @param pidPCR pid to be used for PCR packet.
     * @param pBufPCR  pointer to the destination of newly created PCR packet.
     *
     */
    AM_ERR CreatePCR(AM_U16 pidPCR, AM_U8 * pBufPCR);

    /**
     * This method creates TS NULL packet.
     *
     * @param pBuf  pointer to the destination of newly created NULL packet.
     *
     */
    AM_ERR CreateNULL(AM_U8 * pBuf);

    int  CreateTransportPacket(CTSMUXPSI_STREAM_INFO *pStr,
                               CTSMUXPES_PAYLOAD_INFO* pFd,
                               AM_U8 *pBufPES);

    AM_ERR UpdatePSIcc(AM_U8 * pBufTS); //continuity_counter increment

    int Cal_crc32(AM_U8 *buf, int size);
    void Crc32_Byte(int *preg, int x);
    void SetAudioInfo(AM_AUDIO_INFO* aInfo);
    void SetVideoInfo(AM_VIDEO_INFO* vInfo);

  private:
    // Private data
    CTSMUXPSI_VER ver;  /*!< version info for psi tables. */
    AM_AUDIO_INFO mAudioInfo;
    AM_VIDEO_INFO mVideoInfo;
};

#endif //__BUILD_TS_H__
