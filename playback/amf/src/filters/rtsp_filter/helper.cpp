/*
 * helper.cpp
 *
 * History:
 *    2011/8/31 - [Jay Zhang] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
 
 #include "string.h"
#include "helper.h"
#include "stdio.h"

struct CAutoString::Srep {
	char *pStr;			// pointer to string
	int refCount;

	Srep(int max, const char* p)
	{
		refCount = 1;
		pStr = new char[max+1];	// add space for terminator
		strcpy(pStr, p);
//		printf("new str %p\n", pStr);
	}
	~Srep() 
	{ 
//		printf("del str %p\n", pStr);
		delete[] pStr; 
	}

private:		//prevent copying
	Srep(const Srep&);
	Srep& operator=(const Srep&);
};

CAutoString::CAutoString()
{
	rep = new Srep(0, "");
}

CAutoString::CAutoString(int strLen)
{
	rep = new Srep(strLen, "");
}

CAutoString::CAutoString(const char* s)
{
	rep = new Srep(strlen(s), s);
}
CAutoString::CAutoString(const CAutoString& x)
{
	x.rep->refCount++;
	rep = x.rep;
}

CAutoString::~CAutoString() { 
	if (--rep->refCount == 0)
		delete rep;
}

CAutoString& CAutoString::operator=(const CAutoString& x)
{
	x.rep->refCount++;		// protects against "st = st"
	if (--rep->refCount == 0)
		delete rep;
	rep = x.rep;			// re-point to the new representation
	return *this;
}

CAutoString& CAutoString::operator+=(const CAutoString& x)
{
	int currLen = strlen(rep->pStr);
	Srep* newRep = new Srep(currLen + x.StrLen(), rep->pStr);
	strcpy(newRep->pStr + currLen, x.GetStr());

	if (--rep->refCount == 0)
		delete rep;

	rep = newRep;
	return *this;
}

char* CAutoString::GetStr() const
{ 
	return rep->pStr;
}

int CAutoString::StrLen() const
{
	return strlen(rep->pStr);
}

CAutoString operator+(const CAutoString& x, const CAutoString& y)
{
	CAutoString autoStr(x.StrLen() + y.StrLen());
	strcpy(autoStr.GetStr(), x.GetStr());
	strcpy(autoStr.GetStr() + x.StrLen(), y.GetStr());
	return autoStr;
}

