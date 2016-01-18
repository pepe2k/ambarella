
/**
 * filter_registry.cpp
 *
 * History:
 *    2009/12/7 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

//#define LOG_NDEBUG 0
//#define LOG_TAG "filter_registry"
//#define AMDROID_DEBUG

#include <string.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_queue.h"
#include "am_if.h"
#include "am_pbif.h"
#include "am_mw.h"
#include "am_base.h"
#include "pbif.h"

#include "filter_list.h"
#include "filter_registry.h"



//-----------------------------------------------------------------------
//
// CMediaRecognizer
//
//-----------------------------------------------------------------------
CMediaRecognizer* CMediaRecognizer::Create(IEngine *pEngine)
{
	CMediaRecognizer* result = new CMediaRecognizer(pEngine);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CMediaRecognizer::Construct()
{
	if ((mpWorkQ = CWorkQueue::Create((IActiveObject*)this)) == NULL)
		return ME_NO_MEMORY;
	return ME_OK;
}

CMediaRecognizer::~CMediaRecognizer()
{
	DeleteParser();
	AM_DELETE(mpWorkQ);
}

void *CMediaRecognizer::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IActiveObject)
		return (IActiveObject*)this;
	return inherited::GetInterface(refiid);
}

//blocked is a flag whether the process state is blocked or not
AM_ERR CMediaRecognizer::RecognizeFile(const char *pFileName, bool bblocked )
{
	mbRecognizeBlock = bblocked;
	if(!mbRecognizeBlock)
		return mpWorkQ->SendCmd(CMD_RECOGNIZE, (void*)pFileName);
	mpFileName = pFileName;
	DoRecognize();
	if(mpFilterEntry == NULL)
		return ME_ERROR;
	return ME_OK;
}

AM_ERR CMediaRecognizer::StopRecognize()
{
	return mpWorkQ->SendCmd(CMD_STOP);
}

filter_entry *CMediaRecognizer::FindMatch(CMediaFormat& format)
{
	filter_entry **pEntry = g_filter_table;
	filter_entry *pf;

	while ((pf = *pEntry) != NULL) {
		if (pf->acceptMedia != NULL) {
			if (pf->acceptMedia(format))
				return pf;
		}
		pEntry++;
	}

	return NULL;
}

filter_entry *CMediaRecognizer::FindNextMatch(CMediaFormat& format, filter_entry *except_entry)
{
    filter_entry **pEntry = g_filter_table;
    filter_entry *pf;

    while ((pf = *pEntry) != NULL) {
        if (except_entry != pf) {
            if (pf->acceptMedia != NULL) {
                if (pf->acceptMedia(format))
                    return pf;
            }
        }
        pEntry++;
    }

    return NULL;
}

void CMediaRecognizer::OnCmd(CMD& cmd)
{
	AM_ASSERT(cmd.code == CMD_RECOGNIZE);
	mpFileName = (const char *)cmd.pExtra;
	mpWorkQ->CmdAck(ME_OK);
	DoRecognize();
}

void CMediaRecognizer::DoRecognize()
{
	DeleteParser();
	IFileReader *pFileReader = CFileReader::Create();
	if (pFileReader == NULL) {
		RecognizeDone(NULL);
		return;
	}

	parse_file_s parseFile;
	parseFile.pFileName = mpFileName;
	parseFile.pFileReader = pFileReader;
	parseFile.flags = 0;

	filter_entry **pEntry = g_filter_table;

	while (true) {
		AO::CMD cmd;
		if (mpWorkQ->PeekCmd(cmd)) {
			AM_ASSERT(cmd.code == CMD_STOP);
			AM_DELETE(pFileReader);
			mpWorkQ->CmdAck(ME_OK);
			return;
		}

		filter_entry *pf = *pEntry;
		if (pf == NULL) {
			AM_PRINTF("search done, no filter matches\n");
			//RecognizeDone(pf);
			break;
		}

		if (pf->parse) {
			int weight = pf->parse(&parseFile, &mParser);
			if (weight > 0) {
				RecognizeDone(pf);
				break;
			}
			AM_PRINTF("recognizer[%s] not match, weight=%d.\n",pf->pName,weight);
		}

		pEntry++;
	}

	AM_DELETE(pFileReader);
}

void CMediaRecognizer::RecognizeDone(filter_entry *pf)
{
	mpFilterEntry = pf;
	if(!mbRecognizeBlock)
		mpEngine->PostEngineMsg(IPBEngine::MSG_PARSE_MEDIA_DONE);
}
