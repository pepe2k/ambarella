
/**
 * record_if.cpp
 *
 * History:
 *    2010/1/6 - [Oliver Li] created file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <stdio.h>
#include "am_new.h"
#include "am_types.h"
#include "osal.h"
#include "am_queue.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_base.h"
#include "record_if.h"

//-----------------------------------------------------------------------
//
// GUIDs
//
//-----------------------------------------------------------------------

// {4EF0E38B-EA12-4d9e-93C2-B51AE9C256A0}
AM_DEFINE_IID(IID_IRecordEngine,
0x4ef0e38b, 0xea12, 0x4d9e, 0x93, 0xc2, 0xb5, 0x1a, 0xe9, 0xc2, 0x56, 0xa0);

// {E61F0445-29E3-477a-B335-97EBFD59E467}
AM_DEFINE_IID(IID_IRecordControl,
0xe61f0445, 0x29e3, 0x477a, 0xb3, 0x35, 0x97, 0xeb, 0xfd, 0x59, 0xe4, 0x67);

// {42CD49DE-4BEF-47a0-A3D9-C61C73B10C17}
AM_DEFINE_IID(IID_IRecordControl2,
0x42cd49de, 0x4bef, 0x47a0, 0xa3, 0xd9, 0xc6, 0x1c, 0x73, 0xb1, 0xc, 0x17);

// {EAB0B161-4CD9-43e1-9414-3F43DA0512A1}
AM_DEFINE_IID(IID_IMuxer,
0xeab0b161, 0x4cd9, 0x43e1, 0x94, 0x14, 0x3f, 0x43, 0xda, 0x5, 0x12, 0xa1);

// {C228B8EB-6BE4-45E8-A114-EBA9BBB7F1ED}
AM_DEFINE_IID(IID_IMuxerControl,
0xc228b8eb, 0x6be4, 0x45e8, 0xa1, 0x14, 0xeb, 0xa9, 0xbb, 0xb7, 0xf1, 0xed);

// {DB2067D3-8747-4e38-84E8-877806BA02F2}
AM_DEFINE_IID(IID_IVideoControl,
0xdb2067d3, 0x8747, 0x4e38, 0x84, 0xe8, 0x87, 0x78, 0x6, 0xba, 0x2, 0xf2);

// {37EE28BB-82C1-46AE-B981-288BCD67F8AD}
AM_DEFINE_IID(IID_IEncoder,
0x37ee28bb, 0x82c1, 0x46ae, 0xb9, 0x81, 0x28, 0x8b, 0xcd, 0x67, 0xf8, 0xad);

// {91EACF84-91C6-44f5-BC59-5B1E9D404DCA}
AM_DEFINE_IID(IID_IVideoEncoder,
0x91eacf84, 0x91c6, 0x44f5, 0xbc, 0x59, 0x5b, 0x1e, 0x9d, 0x40, 0x4d, 0xca);

// {09D3F689-4545-448f-98E9-A81D0CFD6B71}
AM_DEFINE_IID(IID_IAudioInput,
0x9d3f689, 0x4545, 0x448f, 0x98, 0xe9, 0xa8, 0x1d, 0xc, 0xfd, 0x6b, 0x71);

// {58F05ACE-8117-4830-BB9B-9FBC308B8E7C}
AM_DEFINE_IID(IID_IAudioEncoder,
0x58f05ace, 0x8117, 0x4830, 0xbb, 0x9b, 0x9f, 0xbc, 0x30, 0x8b, 0x8e, 0x7c);

// {4da31be1-1bcb-441a-b90d-861ab369a867}
AM_DEFINE_IID(IID_IPridataComposer,
0x4da31be1, 0x1bcb, 0x441a, 0xb9, 0x0d, 0x86, 0x1a, 0xb3, 0x69, 0xa8, 0x67);

// {D8B1CC5D-6B09-416b-97E3-32F93C05EB0D}
AM_DEFINE_IID(IID_IFileWriter,
0xd8b1cc5d, 0x6b09, 0x416b, 0x97, 0xe3, 0x32, 0xf9, 0x3c, 0x5, 0xeb, 0xd);


//-----------------------------------------------------------------------
//
// CFileWriter
//
//-----------------------------------------------------------------------
CFileWriter* CFileWriter::Create()
{
	CFileWriter *result = new CFileWriter;
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CFileWriter::Construct()
{
	return ME_OK;
}

CFileWriter::~CFileWriter()
{
	__CloseFile();
}

AM_ERR CFileWriter::CreateFile(const char *pFileName, bool isApend)
{
	__CloseFile();

	if(isApend)
		mpFile = ::fopen(pFileName, "a+");
	else
		mpFile = ::fopen(pFileName, "w");
	if (mpFile == NULL) {
		AM_PERROR(pFileName);
		return ME_IO_ERROR;
	}

	return ME_OK;
}

void CFileWriter::CloseFile()
{
	__CloseFile();
}


AM_ERR CFileWriter::SeekFile(am_file_off_t offset)
{
	if (::fseeko((FILE*)mpFile, offset, SEEK_SET) < 0) {
		AM_PERROR("fseeko");
		return ME_IO_ERROR;
	}
	return ME_OK;
}

AM_ERR CFileWriter::WriteFile(const void *pBuffer, AM_UINT size)
{
	if (::fwrite(pBuffer, 1, size, (FILE*)mpFile) != size) {
		AM_PERROR("fwrite");
		return ME_IO_ERROR;
	}
	return ME_OK;
}

void CFileWriter::__CloseFile()
{
	if (mpFile) {
		::fclose((FILE*)mpFile);
		mpFile = NULL;
	}
}

