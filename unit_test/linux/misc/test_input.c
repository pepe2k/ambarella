/*
 * test_ir_controller.c
 *
 * History:
 *	2009/10/19 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <linux/input.h>
#include <pthread.h>
#include <assert.h>

#include "basetypes.h"

struct key_info {
	int	input_key;
	char	name[32];
};

struct key_info INPUT_KEY_INFO[] = {
	{ KEY_BATTERY,	"KEY_BATTERY_COVER_DECTION"},
	{ KEY_UP,	"KEY_UP"},
	{ KEY_DOWN,	"KEY_DOWN"},
	{ KEY_LEFT,	"KEY_LEFT"},
	{ KEY_RIGHT,	"KEY_RIGHT"},
	{ KEY_OK,	"KEY_OK"},
	{ KEY_MENU,	"KEY_MENU"},
	{ KEY_5,	"KEY_LCD"},
	{ KEY_DELETE,	"KEY_DELETE"},
	{ KEY_A,	"KEY_RESERVED1"},
	{ KEY_0,	"KEY_RESERVED2"},
	{ KEY_9,	"BTN_VIDEO_CAPTURE"},
	{ KEY_8,	"KEY_RESERVED3"},
	{ KEY_PLAY,	"KEY_PLAYBACK"},
	{ KEY_RECORD,	"KEY_RECORD"},
	{ KEY_7,	"KEY_RESERVED4"},
	{ KEY_PHONE,	"ADC_KEY_TELE"},
	{ KEY_6,	"ADC_KEY_WIDE"},
	{ KEY_RESERVED,	"ADC_KEY_BASE"},

	{ KEY_1,	"Start/Stop"},
	{ KEY_2,	"Photo"},
	{ KEY_3,	"Display"},
	{ KEY_4,	"Self timer"},
	{ KEY_ZOOMIN,	"Zoom wide"},
	{ KEY_ZOOMOUT,	"Zoom tele"},
	{ KEY_PREVIOUS,	"Prev section"},
	{ KEY_NEXT,	"Next section"},
	{ KEY_BACK,	"Fast backward"},
	{ KEY_FORWARD,	"Fast forward"},
	{ KEY_STOP,	"Stop"},
	{ KEY_PLAYPAUSE,"Play/Pause"},
	{ KEY_SLOW,	"Slow forward"},
	{ KEY_MENU,	"Menu"},
	{ KEY_NEWS,	"Q-Menu"},

	{ KEY_ESC,	"GPIOKEY_ESC"},
	{ KEY_POWER,	"GPIOKEY_POWER"},
};

#define INPUT_KEY_INFO_TABLE_SIZE (sizeof(INPUT_KEY_INFO) / sizeof((INPUT_KEY_INFO)[0]))

struct keypadctrl {
	pthread_t thread;
	int running;
	int fd[4];
};

struct keypadctrl G_keypadctrl;

 static int key_cmd_process(struct input_event *cmd)
{
	int i;

	if (cmd->type != EV_KEY)
		return 0;

	for (i = 0; i < INPUT_KEY_INFO_TABLE_SIZE; i++) {
		if (cmd->code == INPUT_KEY_INFO[i].input_key) {
			break;
		}
	}

	if (i == INPUT_KEY_INFO_TABLE_SIZE) {
		printf(" can't find the key map for event.code %d\n", cmd->code);
		return 0;
	}

	if (cmd->value)
		printf(" == Key [%s] is pressed ==\n", INPUT_KEY_INFO[i].name);
	else
		printf(" == Key [%s] is release ==\n", INPUT_KEY_INFO[i].name);

	return 0;
}

static int event_channel = 1;

 static void *keypadctrl_thread(void *args)
{
	struct keypadctrl *keypad = (struct keypadctrl *) args;
	struct input_event event;
	fd_set rfds;
	int rval;
	int i = 0;
	char str[64];

	keypad->running = 1;
	printf("keypadctrl_thread started!\n");

	for (i = 0; i < event_channel; i++) {
		sprintf(str, "/dev/input/event%d", i);
		keypad->fd[i] = open(str, O_NONBLOCK);
		if (keypad->fd[i] < 0) {
			printf("open %s error", str);
			goto done;
		}
	}

	FD_ZERO(&rfds);
	for (i = 0; i < event_channel; i++)
		FD_SET(keypad->fd[i], &rfds);


	while (keypad->running) {
		rval = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
		if (rval < 0) {
			continue;
		}

	for (i = 0; i < event_channel; i++) {
		if (FD_ISSET(keypad->fd[i], &rfds)) {
			rval = read(keypad->fd[i], &event, sizeof(event));
			if (rval < 0) {
				printf("read keypad->fd[%d] error",i);
				continue;
			}

			key_cmd_process(&event);
			}
		}
	}

done:

	keypad->running = 0;
	printf("keypadctrl_thread stopped!\n");

	return NULL;
}

int keypadctrl_init(void)
{
	struct keypadctrl *keypad = (struct keypadctrl *) &G_keypadctrl;

	memset(keypad, 0x0, sizeof(*keypad));

	return 0;
}

void keypadctrl_cleanup(void)
{
	struct keypadctrl *keypad = (struct keypadctrl *) &G_keypadctrl;
	int i;

	if (!keypad->running)
		return;
	for (i = 0; i < event_channel; i++)
		close(keypad->fd[i]);

	keypad->running = 0;
	pthread_join(keypad->thread, NULL);

	memset(keypad, 0x0, sizeof(*keypad));
}

int keypadctrl_start(void)
{
	struct keypadctrl *keypad = (struct keypadctrl *) &G_keypadctrl;
	int rval = 0;

	if (keypad->running)
		return 1;

	rval = pthread_create(&keypad->thread, NULL,
			      keypadctrl_thread, keypad);
	if (rval < 0)
		perror("pthread_create keypadctrl_thread failed\n");

	return 0;
}

int main(int argc, char ** argv)
{
	char c;

	keypadctrl_init();
	keypadctrl_start();

	while (1) {
		scanf("%c", &c);
		if (c == 'q') {
			break;
		}
	}

	keypadctrl_cleanup();

	return 0;
}

