/*
 * am_sink.cpp
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
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "am_types.h"
#include "am_if.h"
#include "am_sink.h"

#ifndef BUFSIZ
#define BUFSIZ 255
#endif

CFileSink::CFileSink()
	:_fp(NULL)
	,_custom_buf(NULL)
	,_buf_size(BUFSIZ) {
}

CFileSink::~CFileSink() {
	if (_fp) {
		Close();
	}
	if (_custom_buf) {
		delete[] _custom_buf;
		_custom_buf = NULL;
	}
}

void *CFileSink::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_ISeekableSink) {
		return (ISeekableSink *)this;
	} else if (refiid == IID_IInterface) {
		return (IInterface *)this;
	} else { 
		return NULL;
	}
}

AM_ERR CFileSink::Open(char* name,int mode = 0) {
	if(name == NULL) {
		_fp = tmpfile();
	}else {
		if (mode == 0) {
			_fp = fopen(name, "w+");
		} else if (mode == 1) {
			_fp = fopen(name, "r");
		}
	}
	if (_fp) {
		return ME_OK;
	} else {
		printf("Can't open file %s!\n",name);
		return ME_ERROR;
	}
}

int CFileSink::Write(AM_U8 * pData, AM_UINT len) {
	if (_fp == NULL) {
		return -1;
	}
	return fwrite(pData, sizeof(AM_U8), len, _fp);
}

int CFileSink::Read(AM_U8 * pData, AM_UINT len) {
	if (_fp == NULL) {
		return -1;
	}
	return fread(pData, sizeof(AM_U8), len, _fp);
}

AM_ERR CFileSink::Seek(AM_U32 offset, AM_UINT whence) {
	if (_fp == NULL) {
		return ME_ERROR;
	}
	if (fseek(_fp,offset,whence) < 0) {
		return ME_IO_ERROR;
	}
	return ME_OK;
}

AM_ERR CFileSink::Flush()
{
	if (_fp == NULL) {
		return ME_ERROR;
	}
	if (0!= fflush(_fp)) {
		return ME_IO_ERROR;
	}
	return ME_OK;
}

AM_ERR CFileSink::Close()
{
	if (_fp) {
		fclose(_fp);
		_fp = NULL;
	}
	return ME_OK;
}

AM_ERR CFileSink::Setbuf(AM_UINT size) {
	if (_fp == NULL) {
		return ME_ERROR;
	}
	if (_custom_buf) {
		delete[] _custom_buf;
		_custom_buf = NULL;
	}
	int ret = 0;
	if (size == 0) {
		ret = setvbuf(_fp, NULL, _IONBF,0);
	} else {
		if ( (_custom_buf = new char[size]) == NULL ) {
			return ME_NO_MEMORY;
		}
		ret = setvbuf(_fp, _custom_buf,_IOFBF, size);
	}
	if (ret !=0) {
		return ME_ERROR;
	} else {
		_buf_size = size;
		return ME_OK;
	}
}

AM_ERR CFileSink::Getbufsize(AM_UINT* buf_size) {
	if (_buf_size < 0) {
		return ME_ERROR;
	}
	*buf_size = _buf_size;
	return ME_OK;
}

void CFileSink::Delete() {
	delete this;
}

#define DEFAULT_HOST "10.0.0.1"
#define DEFAULT_PORT 1234

CUdpSink::CUdpSink()
	:_socket(-1)
	,_buf_size(0)
	,_buf_cursor(0)
	,_custom_buf(NULL){
}

CUdpSink::~CUdpSink() {
	if (_socket >= 0) {
		Close();
	}
	if (_custom_buf) {
		delete[] _custom_buf;
		_custom_buf = NULL;
	}
}

void *CUdpSink::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_ISeekableSink) {
		return (ISeekableSink *)this;
	} else if (refiid == IID_IInterface) {
		return (IInterface *)this;
	} else { 
		return NULL;
	}
}

AM_ERR CUdpSink::Open(char* name,int mode = 0)
{
	char server_host[256];
	int server_port = 0;
	if (NULL == name) {
		server_port = DEFAULT_PORT;
		strcpy(server_host,DEFAULT_HOST);
	}
	if ( 0 != sscanf(name,"%[0-9.]:%d",server_host,&server_port)) {
		if (server_port == 0) {
			server_port = DEFAULT_PORT;
		}
	} else {
		return ME_BAD_PARAM;
	}

	struct sockaddr_in servaddr;
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(server_port);
	if(inet_pton(AF_INET, server_host, &servaddr.sin_addr) <= 0) {
		AM_ERROR("[%s] is not a valid IPaddress\n", server_host);
		return ME_BAD_PARAM;
	}

	if ((_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		AM_PERROR("create socket");
		return ME_ERROR;
	}

	if (connect(_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
	{
		AM_PERROR("connect error\n");
		return ME_ERROR;
	}

	return ME_OK;
}

int CUdpSink::Write(AM_U8 * pData, AM_UINT len)
{
	if(_socket< 0) {
		return -1;
	}
	int sendout_len = 0;
	if ((int)len >_buf_size) {
		if (Flush() != ME_OK) {
			return -1;
		}
		sendout_len = send(_socket, pData,  len, 0);
		if (sendout_len < 0) {
			return -1;
		}
		_buf_cursor = 0;
		return len;
	}

	int buf_left_size = _buf_size - _buf_cursor;

	if ((int)len < buf_left_size) {
		memcpy(_custom_buf+_buf_cursor, pData, len);
		_buf_cursor += len;
	} else {
		if (Flush() != ME_OK ){
			return -1;
		}
		memcpy(_custom_buf, pData, len);
		_buf_cursor = len;
	}
	return len;
}

int CUdpSink::Read(AM_U8 * pData, AM_UINT len) {
	if(_socket< 0) {
		return -1;
	}
	return recv(_socket, pData, len, 0);
}

AM_ERR CUdpSink::Seek(AM_U32 offset, AM_UINT whence) {
	return ME_NOT_SUPPORTED;
}

AM_ERR CUdpSink::Flush()
{
	if (_socket < 0) {
		return ME_ERROR;
	}
	if (send(_socket, _custom_buf, _buf_cursor, 0) < 0) {
		return ME_ERROR;
	}
	_buf_cursor = 0;
	return ME_OK;
}

AM_ERR CUdpSink::Close()
{
	if (_socket < 0) {
		return ME_OK;
	}
	if (Flush() != ME_OK) {
		return ME_ERROR;
	}
	close(_socket);
	_socket = -1;
	return ME_OK;
}


AM_ERR CUdpSink::Setbuf(AM_UINT size) {
	if (_custom_buf) {
		delete[] _custom_buf;
		_custom_buf = NULL;
	}
	if (size > 0) {
		if (_custom_buf) {
			delete[] _custom_buf;
			_custom_buf = NULL;
		}
		if ((_custom_buf = new char[size])== NULL) {
			return ME_NO_MEMORY;
		}
		_buf_size = size;
		_buf_cursor = 0;
		return ME_OK;
	} else {
		return ME_ERROR;
	}
}

AM_ERR CUdpSink::Getbufsize(AM_UINT* buf_size) {
	if (_buf_size < 0) {
		return ME_ERROR;
	}
	*buf_size = _buf_size;
	return ME_OK;
}

void CUdpSink::Delete() {
	delete this;
}

