/** 
 * fut_if.h 
 * FUT stand for Filter Unit Test
 * History:
 *    2010/6/1 - [QXiong Zheng] created file
 *    
 *
 * Copyright (C) 2010-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */


#ifndef __FUT_IF_H__
#define __FUT_IF_H__


extern const AM_IID IID_IFUTEngine;



//-----------------------------------------------------------------------
//
// IFUTEngine
//
//-----------------------------------------------------------------------
class IFUTEngine: public IEngine
{
	typedef IEngine inherited;

public:
	enum {
		//MSG_PARSE_MEDIA_DONE = inherited::MSG_LAST,		
	};

public:
	DECLARE_INTERFACE(IFUTEngine, IID_IFUTEngine);
};



#endif
