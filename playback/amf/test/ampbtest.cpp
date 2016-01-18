
/**
 * ampbtest.cpp
 *
 * History:
 *    2010/07/20 - [He Zhi] created file
 *
 * desc: testplayer for AMPlayer
 *   usage: ampbtest mediafile
 *          Quit: press 'q' and 'enter' 
 *   options: [-sharedfd : use setDataSource(sharedfd) ]
 *    
 *
 * Copyright (C) 2007-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
   
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_pbif.h"
#include "am_mw.h"

#include <binder/MemoryHeapBase.h>
#include <media/AudioTrack.h>
#include "MediaPlayerService.h"
#include "AMPlayer.h"
static int exit_singnal=0;

static void* getchar_thread(void*)
{
	char c;
	while(1)
	{
		c=getchar();
		if(c=='q' || c=='Q')
		{
			exit_singnal=1;
			break;
		}
	}
	return NULL;
}

int main(int argc, char* argv[])
{
	int config_use_shared_fd=0;
	int i=0;
	status_t status=0;
	int position;
       int duration;
	pthread_t tid_getchar;
	
	if (argc<2)
	{
		printf("plz specify media-file.\n");
		return 1;
	}

	//parse options
	for(i=2;i<argc;i++)
	{
		if(!strcmp("-sharedfd",argv[i]))
		{
			config_use_shared_fd=1;
		}
	}
	
	AMPlayer* p=new AMPlayer();

	if (p==NULL)
	{
		printf("new AMPlayer() fail.\n");
		return 2;		
	}
	if (!p->hardwareOutput()) 
       {
            MediaPlayerService::AudioOutput* mAudioOutput = new MediaPlayerService::AudioOutput(1);
            p->setAudioSink(mAudioOutput);
        }
	
	if (!config_use_shared_fd)
	{
		status=p->setDataSource(argv[1], NULL);		
	}
	else
	{
		int fd = open(argv[1], O_RDONLY, 0666);
		if (fd == -1)
		{
			printf("error: open_file fail.\n");
			return -1;
		}
		status=p->setDataSource(fd,0,0);
	}
	
	status=p->prepareAsync();

	status=p->prepare();
	
	status=p->start();

	pthread_create(&tid_getchar,NULL,getchar_thread,NULL);
	
	while(true)
	{
		status=p->getCurrentPosition(&position);
		if(exit_singnal)
		{
			printf("exit \n");
			break;//return
		}
		sleep(1);
	}
	
	//sleep(10);
	
	status=p->stop();

	delete p;

	void* result;
	pthread_join(tid_getchar, &result);
	
	return 0;
}


