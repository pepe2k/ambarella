/*
 * am_sink.h
 *
 * History:
 *    2011/2/26 - [Yi Zhu] created file
 *    2011/5/12 - [Hanbo Xiao] modified file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_SINK_H__
#define __AM_SINK_H__

extern const AM_IID IID_ISeekableSink;

class ISeekableSink : public IInterface
{
public:
	DECLARE_INTERFACE(ISeekableSink, IID_ISeekableSink);
	
	virtual AM_ERR Open(char* name,int mode = 0) = 0;
	virtual AM_ERR Close() = 0;
	virtual int Read(AM_U8 *data, AM_UINT len) = 0;
	virtual int Write(AM_U8 *data, AM_UINT len) = 0;
	virtual AM_ERR Seek(AM_U32 offset, AM_UINT whence) = 0;
	virtual AM_ERR Flush() = 0;
	virtual AM_ERR Setbuf(AM_UINT buf_size) = 0;
	virtual AM_ERR Getbufsize(AM_UINT* buf_size) = 0;
	virtual void Delete() = 0;
};

//local file io
class CFileSink: public ISeekableSink
{
	typedef ISeekableSink inherited;
public:
	CFileSink();
	virtual ~CFileSink();

	virtual void *GetInterface(AM_REFIID refiid);
	virtual AM_ERR Open(char* name,int mode);//0 --rw	1-read only
	virtual AM_ERR Close();
	virtual int Read(AM_U8 *data, AM_UINT len);
	virtual int Write(AM_U8 *data, AM_UINT len);
	virtual AM_ERR Seek(AM_U32 offset, AM_UINT whence);
	virtual AM_ERR Flush();
	virtual AM_ERR Setbuf(AM_UINT buf_size);
	virtual AM_ERR Getbufsize(AM_UINT* buf_size);
	virtual void Delete();
protected:
	FILE *_fp;
	char *_custom_buf;
	int _buf_size;
};


//local file io
class CUdpSink: public ISeekableSink
{
	typedef ISeekableSink inherited;
public:
	CUdpSink();
	virtual ~CUdpSink();

	virtual void *GetInterface(AM_REFIID refiid);
	virtual AM_ERR Open(char* name,int mode);
	virtual AM_ERR Close();
	virtual int Read(AM_U8 *data, AM_UINT len);
	virtual int Write(AM_U8 *data, AM_UINT len);
	virtual AM_ERR Seek(AM_U32 offset, AM_UINT whence);
	virtual AM_ERR Flush();
	virtual AM_ERR Setbuf(AM_UINT buf_size);
	virtual AM_ERR Getbufsize(AM_UINT* buf_size);
	virtual void Delete();
protected:
	int _socket;
	int _buf_size;
	int _buf_cursor;
	char *_custom_buf;
};

#endif

