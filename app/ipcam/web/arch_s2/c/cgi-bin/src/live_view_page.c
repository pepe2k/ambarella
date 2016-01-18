
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"
#include "page.h"
#include "video_page.h"
#include "live_view_page.h"
#include "params.h"


#define MAXSTRLEN 256
#define SHORTLEN 32
#define STREAM_NUM 4
#define MAX_OPTION_LEN 12

typedef enum
{
	ENCODE_TYPE = 0,
	ENCODE_FPS,
	ENCODE_WIDTH,
	ENCODE_HEIGHT,
	BRC_MODE,
	CBR_AVG_BPS,
	VBR_MIN_BPS,
	VBR_MAX_BPS,
	LIVE_PARAMS_NUM
}LIVE_PARAMS;


static ParamData live_params[LIVE_PARAMS_NUM];
static char* const streamName[] = {"Main","Second","Third","Fourth"};
extern Page_Ops virtual_PageOps;
static Label_Option options[MAX_OPTION_LEN];
int videoWidth = 0;
int videoHeight = 0;

static int _create_params ()
{
	live_params[ENCODE_TYPE].value = 0;
	memset(live_params[ENCODE_TYPE].param_name, 0, PARAM_NAME_LEN);
	strcat(live_params[ENCODE_TYPE].param_name, "encode_type");

	live_params[ENCODE_FPS].value = 30;
	memset(live_params[ENCODE_FPS].param_name, 0, PARAM_NAME_LEN);
	strcat(live_params[ENCODE_FPS].param_name, "encode_fps");

	live_params[ENCODE_WIDTH].value = 1280;
	memset(live_params[ENCODE_WIDTH].param_name, 0, PARAM_NAME_LEN);
	strcat(live_params[ENCODE_WIDTH].param_name, "encode_width");

	live_params[ENCODE_HEIGHT].value =720;
	memset(live_params[ENCODE_HEIGHT].param_name, 0, PARAM_NAME_LEN);
	strcat(live_params[ENCODE_HEIGHT].param_name, "encode_height");

	live_params[BRC_MODE].value = 0;
	memset(live_params[BRC_MODE].param_name, 0, PARAM_NAME_LEN);
	strcat(live_params[BRC_MODE].param_name, "brc_mode");

	live_params[CBR_AVG_BPS].value = 4000000;
	memset(live_params[CBR_AVG_BPS].param_name, 0, PARAM_NAME_LEN);
	strcat(live_params[CBR_AVG_BPS].param_name, "cbr_avg_bps");

	live_params[VBR_MIN_BPS].value = 1000000;
	memset(live_params[VBR_MIN_BPS].param_name, 0, PARAM_NAME_LEN);
	strcat(live_params[VBR_MIN_BPS].param_name, "vbr_min_bps");

	live_params[VBR_MAX_BPS].value = 6000000;
	memset(live_params[VBR_MAX_BPS].param_name, 0, PARAM_NAME_LEN);
	strcat(live_params[VBR_MAX_BPS].param_name, "vbr_max_bps");

	return 0;
}


int AmbaLiveView_init (Page* currentPage, int streamID, int browser)
{
	extern Page VideoPage;
	memset(currentPage, 0, sizeof(Page));

	AmbaVideo_init(&VideoPage, browser);
	*currentPage = VideoPage;

	currentPage->superPage = &VideoPage;
	strncat(currentPage->name, "liveview", strlen("liveview"));
	currentPage->streamId = streamID;
	currentPage->statSize = 0;

	memset(&LiveViewPage_Ops, 0, sizeof(Page_Ops));
	currentPage->page_ops = &LiveViewPage_Ops;

	(&LiveViewPage_Ops)->get_params = AmbaLiveView_get_params;
	(&LiveViewPage_Ops)->add_settings = AmbaLiveView_add_settings;
	(&LiveViewPage_Ops)->add_controls = AmbaLiveView_add_controls;
	(&LiveViewPage_Ops)->add_body_JS = AmbaLiveView_add_body_JS;

	_create_params();
	return 0;
}

int AmbaLiveView_get_params (Page* currentPage)
{
	SectionPort_Index sectionPort = LIVE;
	section_Param section_param;
	char* sectionName = "LIVE";

	char extroInfo[4];
	sprintf(extroInfo, "%d", currentPage->streamId);

	int ret = (&virtual_PageOps)->process_PostData(currentPage);
	if (ret < 0) {
		fprintf(stdout, "1:set params failed");
	} else {
		if (ret == 0) {
			fprintf(stdout, "0:set params succeed");
		} else {
			if (ret == 1) {
				memset(&section_param, 0, sizeof(section_param));
				section_param.sectionName = sectionName;
				section_param.sectionPort = sectionPort;
				section_param.paramData = live_params;
				section_param.extroInfo =extroInfo;
				section_param.paramDataNum = LIVE_PARAMS_NUM;
				if ((&virtual_PageOps)->get_section_param(currentPage,&section_param) == -1) {
					return -1;
				}
			} else {
				fprintf(stdout, "1:unexpected error %d", ret);
			}
		}
	}
	return ret;
}
int AmbaLiveView_add_stream_resolution ()
{
	videoWidth = live_params[ENCODE_WIDTH].value;
	videoHeight = live_params[ENCODE_HEIGHT].value;
	return 0;
}
int AmbaLiveView_add_settings (Page* currentPage, char* text)
{
	AmbaLiveView_add_stream_resolution();

	char* string_format = "&nbsp; &nbsp; <a class=\"nav\" href=\"/cgi-bin/webdemo.cgi?page\
						=enc\">Setting</a>\n\
						&nbsp;<a class=\"nav\" href=\"/cgi-bin/webdemo.cgi?page=help\" \
						target=\"_blank\" >Help</a>\n\
						&nbsp; &nbsp; &nbsp; &nbsp; <label>Live Stream &nbsp;";
	strncat(text, string_format, strlen(string_format));
	int i;
	char select[SHORTLEN];
	char* text_stream = "<a class=\"nav%s\" href=\"/cgi-bin/webdemo.cgi?streamId=\
						%d\">%s</a>\n";
	char text_buffer[MAXSTRLEN];
	for (i = 0; i < STREAM_NUM; i++) {
		memset(select, 0, SHORTLEN);
		memset(text_buffer, 0, MAXSTRLEN);
		if (i == currentPage->streamId) {
			strncat(select, " active", strlen(" active"));
		}
		sprintf(text_buffer, text_stream, select, i, streamName[i]);
		strncat(text, text_buffer, strlen(text_buffer));
	}
	char* label_format_end = "</label>\n&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; ";
	strncat(text, label_format_end, strlen(label_format_end));

	// Stat
	button_Input button;
	/*
	* State and DPTZ disabled
	*/
	if (currentPage->agent == MSIE) {
		memset(&button, 0, sizeof(button_Input));
		button.name = "Stat";
		button.value = "Hide Statistics";
		button.exprop = "none";
		button.action = "ShowStat()";
		memset(text_buffer, 0, MAXSTRLEN);
		(&virtual_PageOps)->create_button(currentPage, text_buffer, &button);
		strncat(text, text_buffer, strlen(text_buffer));
		label_format_end = "&nbsp; &nbsp; &nbsp; &nbsp;";
		strncat(text, label_format_end, strlen(label_format_end));

		//DPTZ
		memset(&button, 0, sizeof(button_Input));
		button.name = "DPTZ";
		button.value = "Hide Digital PTZ";
		button.exprop = "hidden";
		button.action = "ShowDPTZ()";
		memset(text_buffer,0,MAXSTRLEN);
		(&virtual_PageOps)->create_button(currentPage, text_buffer, &button);
		strncat(text, text_buffer, strlen(text_buffer));
		label_format_end = "&nbsp; &nbsp; &nbsp; &nbsp;";
		strncat(text, label_format_end, strlen(label_format_end));
	} else {
		memset(&button, 0, sizeof(button_Input));
		button.name = "Z";
		button.value = "Z";
		button.exprop = "hidden";
		button.action = "";
		memset(text_buffer, 0, MAXSTRLEN);
		(&virtual_PageOps)->create_button(currentPage, text_buffer, &button);
		strncat(text, text_buffer, strlen(text_buffer));
		label_format_end = "&nbsp; &nbsp; &nbsp; &nbsp;";
		strncat(text, label_format_end, strlen(label_format_end));
	}
	return 0;
}


int AmbaLiveView_add_controls (Page* currentPage, char* text)
{
	button_Input button;

	//Play
	memset(&button, 0, sizeof(button_Input));
	button.name = "Play";
	button.value = "Play";
	button.exprop = "";
	button.action = "PlayVideo()";
	char* controlText = NULL;
	controlText = (char *)malloc(PAGE_CONTENT_LEN);
	if (controlText == NULL) {
		LOG_MESSG("LiveView add controls error");
		return -1;
	}
	memset(controlText, 0, PAGE_CONTENT_LEN);
	(&virtual_PageOps)->create_button(currentPage, controlText, &button);
	strncat(text, controlText, strlen(controlText));

	//Stop
	memset(&button, 0, sizeof(button_Input));
	button.name = "Stop";
	button.value = "Stop";
	button.exprop = "";
	button.action = "StopVideo()";
	memset(controlText, 0, PAGE_CONTENT_LEN);
	(&virtual_PageOps)->create_button(currentPage, controlText, &button);
	strncat(text, controlText, strlen(controlText));

	//Record
	if (currentPage->agent == MSIE) {
		memset(&button, 0, sizeof(button_Input));
		button.name = "Record";
		button.value = "Start Record";
		button.exprop = "";
		button.action = "Record()";
		memset(controlText, 0, PAGE_CONTENT_LEN);
		(&virtual_PageOps)->create_button(currentPage, controlText, &button);
		strncat(text, controlText, strlen(controlText));
	}

	//FlySet
	char* flySet_format = "FlySet(%d, 'ForceIdr')";
	char flySet[SHORTLEN] = {0};
	sprintf(flySet, flySet_format, currentPage->streamId);
	memset(&button, 0, sizeof(button_Input));
	button.name = "ForceIdr";
	button.value = "Force Idr";
	button.exprop = "";
	button.action = flySet;
	memset(controlText, 0, PAGE_CONTENT_LEN);
	(&virtual_PageOps)->create_button(currentPage, controlText, &button);
	strncat(text, controlText, strlen(controlText));


	char* options_buf[] = {"60","30","25","20","15","10","6","5","4","3","2","1"};
	unsigned int options_value[] = {FPS_60,FPS_30,FPS_25,FPS_20,FPS_15,FPS_10,FPS_6,\
								FPS_5,FPS_4,FPS_3,FPS_2,FPS_1};
	int i;
	for (i = 0; i < ARR_SIZE(options_buf);i++) {
		options[i].value = options_value[i];
		memset(options[i].option, 0, LABEL_OPTION_LEN);
		strncat(options[i].option, options_buf[i], strlen(options_buf[i]));
	}
	memset(flySet, 0, SHORTLEN);
	sprintf(flySet,"FlySet(%d, 'ChangeFr')", currentPage->streamId);
	select_Label select_label;
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Encode frame rate(1~30) :";
	select_label.name = "ChangeFr";
	select_label.options = options;
	select_label.option_len = ARR_SIZE(options);
	select_label.value = live_params[ENCODE_FPS].value;
	select_label.action = flySet;
	memset(controlText, 0, PAGE_CONTENT_LEN);
	(&virtual_PageOps)->create_select_label(currentPage, controlText, &select_label);
	strncat(text, controlText, strlen(controlText));


	text_Entry text_entry;
	if (live_params[BRC_MODE].value & 0x1) {
		memset(flySet, 0, SHORTLEN);
		sprintf(flySet,"FlySet(%d, 'ChangeVBRMinBps')", currentPage->streamId);
		memset(&text_entry, 0, sizeof(text_Entry));
		text_entry.label = "VBR min bitrate(kbps) :";
		text_entry.name = "ChangeVBRMinBps";
		text_entry.value = live_params[VBR_MIN_BPS].value/1000;
		text_entry.maxlen = 5;
		text_entry.ro = flySet;
		memset(controlText, 0, PAGE_CONTENT_LEN);
		(&virtual_PageOps)->create_text_entry(currentPage, controlText, &text_entry);
		strncat(text, controlText, strlen(controlText));

		memset(flySet, 0, SHORTLEN);
		sprintf(flySet, "FlySet(%d, 'ChangeVBRMaxBps')", currentPage->streamId);
		memset(&text_entry, 0, sizeof(text_Entry));
		text_entry.label = "VBR max bitrate(kbps) :";
		text_entry.name = "ChangeVBRMaxBps";
		text_entry.value = live_params[VBR_MAX_BPS].value/1000;
		text_entry.maxlen = 5;
		text_entry.ro = flySet;
		memset(controlText, 0, PAGE_CONTENT_LEN);
		(&virtual_PageOps)->create_text_entry(currentPage, controlText, &text_entry);
		strncat(text, controlText, strlen(controlText));
	}	else {

		memset(flySet, 0, SHORTLEN);
		sprintf(flySet, "FlySet(%d, 'ChangeCBRAvgBps')", currentPage->streamId);
		memset(&text_entry, 0, sizeof(text_Entry));
		text_entry.label = "CBR average bitrate(kbps) :";
		text_entry.name = "ChangeCBRAvgBps";
		text_entry.value = live_params[CBR_AVG_BPS].value/1000;
		text_entry.maxlen = 5;
		text_entry.ro = flySet;
		memset(controlText, 0, PAGE_CONTENT_LEN);
		(&virtual_PageOps)->create_text_entry(currentPage, controlText, &text_entry);
		strncat(text, controlText, strlen(controlText));
	}
	free(controlText);
	controlText =  NULL;
	return 0;
}



int AmbaLiveView_add_body_JS (Page * currentPage, char* text)
{
	char string[MAXSTRLEN] = {0};
	sprintf(string, " onload=\"javascript: OnLoadActiveX('%s', %d, %d, %d, %d);\" ", \
		currentPage->host, currentPage->streamId,\
			/*(*currentPage).recvType*/1, currentPage->statSize, 1);
	strncat(text, string, strlen(string));
	return 0;
}

