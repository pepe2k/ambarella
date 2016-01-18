
/**
 * filter_registry.h
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

#ifndef __FILTER_REGISTRY_H__
#define __FILTER_REGISTRY_H__


//-----------------------------------------------------------------------
//
// CMediaRecognizer
//
//-----------------------------------------------------------------------
class CMediaRecognizer: public CObject, public IActiveObject
{
	typedef CObject inherited;
	typedef IActiveObject AO;

	enum {
		CMD_RECOGNIZE = AO::CMD_LAST,
	};

public:
	static CMediaRecognizer* Create(IEngine *pEngine);

	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IActiveObject
	virtual const char *GetName() { return "MediaRecognizer"; }
	virtual void OnRun() {}

	// called by the thread when a cmd is received
	// should return false for CMD_TERMINATE
	virtual void OnCmd(CMD& cmd);

private:
	CMediaRecognizer(IEngine *pEngine):
		mpEngine(pEngine),
		mpFileName(NULL),
		mpWorkQ(NULL),
		mpFilterEntry(NULL),
		mbRecognizeBlock(false)
	{
		mParser.context = NULL;
	}
	AM_ERR Construct();
	virtual ~CMediaRecognizer();

public:
	AM_ERR RecognizeFile(const char *pFileName, bool bblocked = false);
	AM_ERR StopRecognize();
	filter_entry *GetFilterEntry() { return mpFilterEntry; }
	const parser_obj_s &GetParser() { return mParser; }
	const char* GetFileName(){return mpFileName;}
	void ClearParser() { mParser.context = NULL; }
	static filter_entry *FindMatch(CMediaFormat& format);
	static filter_entry *FindNextMatch(CMediaFormat& format, filter_entry *start_entry);

private:
	IEngine *mpEngine;
	const char *mpFileName;
	CWorkQueue *mpWorkQ;
	filter_entry *mpFilterEntry;
	parser_obj_s mParser;

	//recognizefile block flag
	bool mbRecognizeBlock;

private:
	void DoRecognize();
	void RecognizeDone(filter_entry *pf);

private:
	void DeleteParser()
	{
		if (mParser.context && mParser.free) {
			mParser.free(mParser.context);
			mParser.context = NULL;
		}
	}
};


#endif

