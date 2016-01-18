

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "params.h"
#include "utils.h"
#include "page.h"
#include "base_page.h"
#include "config_page.h"
#include "cam_basic_page.h"



typedef enum {
	VIN_MODE = 0,
	VIN_FRAMERATE,
	VIN_MIRROR,
	VIN_BAYER,
	VOUT_TYPE,
	VOUT_MODE,
	VINVOUT_PARAMS_NUM
}VINVOUT_PARAMS;


typedef enum {
	VIDEO_OPS_AUTO,
	VIDEO_OPS_1080P,
	VIDEO_OPS_720P,
	VIDEO_OPS_NUM
}VIDEO_MODE_OPTIONS;

typedef enum {
	FPS_OPS_AUTO,
	FPS_OPS_5,
	FPS_OPS_6,
	FPS_OPS_10,
	FPS_OPS_13,
	FPS_OPS_15,
	FPS_OPS_25,
	FPS_OPS_29,
	FPS_OPS_30,
	FPS_OPS_59,
	FPS_OPS_60,
	FPS_OPS_NUM
}FPS_OPTIONS;

typedef enum {
	MIR_BAY_AUTO,
	MIR_BAY_0,
	MIR_BAY_1,
	MIR_BAY_2,
	MIR_BAY_3,
	MIR_BAY_OPTIONS_NUM
}MIR_BAY_OPTIONS;


typedef enum {
	VOUT_TYPE_OFF,
	VOUT_TYPE_CVBS,
	VOUT_TYPE_HDMI,
	VOUT_TYPE_NUM
}VOUT_TYPE_OPTIONS;

typedef enum {
	VOUT_VIDEO_480P,
	VOUT_VIDEO_576P,
	VOUT_VIDEO_720P,
	VOUT_VIDEO_1080I,
	VOUT_VIDEO_1080P30,
	VOUT_VIDEO_OPS_NUM
}VOUT_VIDEO_OPTIONS;


#define MAX_OPTION_LEN 16
static ParamData vinvout_params[VINVOUT_PARAMS_NUM];
extern Page_Ops virtual_PageOps;
extern char postInfo[];
static Label_Option options[MAX_OPTION_LEN];


static int _create_params () {
	memset(vinvout_params, 0, sizeof(ParamData)*VINVOUT_PARAMS_NUM);
	strcat(vinvout_params[VIN_MODE].param_name, "vin_mode");
	vinvout_params[VIN_MODE].value = 0;

	strcat(vinvout_params[VIN_FRAMERATE].param_name, "vin_framerate");
	vinvout_params[VIN_FRAMERATE].value =0;

	strcat(vinvout_params[VIN_MIRROR].param_name, "vin_mirror");
	vinvout_params[VIN_MIRROR].value = -1;

	strcat(vinvout_params[VIN_BAYER].param_name, "vin_bayer");
	vinvout_params[VIN_BAYER].value = -1;

	strcat(vinvout_params[VOUT_TYPE].param_name, "vout_type");
	vinvout_params[VOUT_TYPE].value = VOUT_CVBS;

	strcat(vinvout_params[VOUT_MODE].param_name, "vout_mode");
	vinvout_params[VOUT_MODE].value = VIDEO_MODE_480I;
	return 0;
}

static int _add_vin (Page* currentPage, char* text) {
	char* fieldset = "<fieldset><legend>VIN Settings</legend><br>\n";
	strncat(text, fieldset, strlen(fieldset));



	//video mode
	memset(options, 0, sizeof(Label_Option)*MAX_OPTION_LEN);
	strcat(options[VIDEO_OPS_AUTO].option, "Auto");
	options[VIDEO_OPS_AUTO].value = VIDEO_MODE_AUTO;

	strcat(options[VIDEO_OPS_1080P].option, "1080P");
	options[VIDEO_OPS_1080P].value = VIDEO_MODE_1080P;

	strcat(options[VIDEO_OPS_720P].option, "720P");
	options[VIDEO_OPS_720P].value = VIDEO_MODE_720P;

	select_Label select_label;
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Resolution :";
	select_label.name = "vin_mode";
	select_label.options = options;
	select_label.option_len = VIDEO_OPS_NUM;
	select_label.value = vinvout_params[VIN_MODE].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);

	//fps
	memset(options, 0, sizeof(Label_Option)*MAX_OPTION_LEN);
	strcat(options[FPS_OPS_AUTO].option, "Auto");
	options[FPS_OPS_AUTO].value = FPS_AUTO;

	strcat(options[FPS_OPS_5].option, "5");
	options[FPS_OPS_5].value = FPS_5;

	strcat(options[FPS_OPS_6].option, "6");
	options[FPS_OPS_6].value = FPS_6;

	strcat(options[FPS_OPS_10].option, "10");
	options[FPS_OPS_10].value = FPS_10;

	strcat(options[FPS_OPS_13].option, "13");
	options[FPS_OPS_13].value = FPS_13;

	strcat(options[FPS_OPS_15].option, "15");
	options[FPS_OPS_15].value = FPS_15;

	strcat(options[FPS_OPS_25].option, "25");
	options[FPS_OPS_25].value = FPS_25;

	strcat(options[FPS_OPS_29].option, "29.97");
	options[FPS_OPS_29].value = FPS_29;

	strcat(options[FPS_OPS_30].option, "30");
	options[FPS_OPS_30].value = FPS_30;

	strcat(options[FPS_OPS_59].option, "59.94");
	options[FPS_OPS_59].value = FPS_59;

	strcat(options[FPS_OPS_60].option, "60");
	options[FPS_OPS_60].value = FPS_60;

	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Frame Rate (fps) :";
	select_label.name = "vin_framerate";
	select_label.options = options;
	select_label.option_len = FPS_OPS_NUM;
	select_label.value = vinvout_params[VIN_FRAMERATE].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);

	/*
	*Mirror and Bayer button disable
	*/
#if 0
	//strcat(text,"<br><br>");
	//mirror & Bayer

	/*memset(options, 0, sizeof(Label_Option)*MAX_OPTION_LEN);
	strcat(options[MIR_BAY_AUTO].option, "Auto");
	options[MIR_BAY_AUTO].value = 4;

	strcat(options[MIR_BAY_0].option, "0");
	options[MIR_BAY_0].value = 0;

	strcat(options[MIR_BAY_1].option, "1");
	options[MIR_BAY_1].value = 1;

	strcat(options[MIR_BAY_2].option, "2");
	options[MIR_BAY_2].value = 2;

	strcat(options[MIR_BAY_3].option, "3");
	options[MIR_BAY_3].value = 3;

	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Mirror Pattern :";
	select_label.name = "vin_mirror";
	select_label.options = options;
	select_label.option_len = MIR_BAY_OPTIONS_NUM;
	select_label.value = vinvout_params[VIN_MIRROR].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);


	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Bayer Pattern :";
	select_label.name = "vin_bayer";
	select_label.options = options;
	select_label.option_len = MIR_BAY_OPTIONS_NUM;
	select_label.value = vinvout_params[VIN_BAYER].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);*/
#endif
	char* fieldset_end = "<br><br></fieldset><br>\n";
	strncat(text, fieldset_end, strlen(fieldset_end));
	return 0;
}


static int _add_vout (Page* currentPage,char* text) {
	char* fieldset = "<fieldset><legend>VOUT Settings</legend><br>\n";
	strncat(text, fieldset, strlen(fieldset));
	char* action = "onchange=\"setVoutMode(this.options[this.selectedIndex].value)\"";

	//vout type
	memset(options,0,sizeof(Label_Option)*5);
	strcat(options[VOUT_TYPE_OFF].option, "OFF");
	options[VOUT_TYPE_OFF].value = VOUT_OFF;

	strcat(options[VOUT_TYPE_CVBS].option, "CVBS");
	options[VOUT_TYPE_CVBS].value = VOUT_CVBS;

	strcat(options[VOUT_TYPE_HDMI].option, "HDMI");
	options[VOUT_TYPE_HDMI].value = VOUT_HDMI;

	select_Label select_label;
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Type :";
	select_label.name = "vout_type";
	select_label.options = options;
	select_label.option_len = VOUT_TYPE_NUM;
	select_label.value = vinvout_params[VOUT_TYPE].value;
	select_label.action = action;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"&nbsp; &nbsp; ");

	//vout video mode
	if ((vinvout_params[VOUT_TYPE].value) == VOUT_OFF) {
		memset(options, 0, sizeof(Label_Option)*MAX_OPTION_LEN);
		strcat(options[0].option,"480I");
		options[0].value = VIDEO_MODE_480I;
		strcat(options[1].option,"576I");
		options[1].value = VIDEO_MODE_576I;

		memset(&select_label, 0, sizeof(select_Label));
		select_label.label = "Resolution :";
		select_label.name = "vout_mode";
		select_label.options = options;
		select_label.option_len = 2;
		select_label.value = vinvout_params[VOUT_MODE].value;
		select_label.action = "disabled";
		(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	} else if ((vinvout_params[VOUT_TYPE].value) == VOUT_HDMI) {
		memset(options, 0, sizeof(Label_Option)*MAX_OPTION_LEN);
		strcat(options[VOUT_VIDEO_480P].option, "480P");
		options[VOUT_VIDEO_480P].value = VIDEO_MODE_480P;

		strcat(options[VOUT_VIDEO_576P].option, "576P");
		options[VOUT_VIDEO_576P].value = VIDEO_MODE_576P;

		strcat(options[VOUT_VIDEO_720P].option, "720P");
		options[VOUT_VIDEO_720P].value = VIDEO_MODE_720P;

		strcat(options[VOUT_VIDEO_1080I].option, "1080I");
		options[VOUT_VIDEO_1080I].value = VIDEO_MODE_1080I;

		strcat(options[VOUT_VIDEO_1080P30].option, "1080P30");
		options[VOUT_VIDEO_1080P30].value = VIDEO_MODE_1080P30;

		memset(&select_label, 0, sizeof(select_Label));
		select_label.label = "Resolution :";
		select_label.name = "vout_mode";
		select_label.options = options;
		select_label.option_len = VOUT_VIDEO_OPS_NUM;
		select_label.value = vinvout_params[VOUT_MODE].value;
		select_label.action = "";
		(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	} else {
		memset(options, 0, sizeof(Label_Option)*MAX_OPTION_LEN);
		strcat(options[0].option,"480I");
		options[0].value = VIDEO_MODE_480I;
		strcat(options[1].option,"576I");
		options[1].value = VIDEO_MODE_576I;

		memset(&select_label, 0, sizeof(select_Label));
		select_label.label = "Resolution :";
		select_label.name = "vout_mode";
		select_label.options = options;
		select_label.option_len = 2;
		select_label.value = vinvout_params[VOUT_MODE].value;
		select_label.action = "";
		(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	}
	char* fieldset_end = "<br><br></fieldset><br>\n";
	strncat(text, fieldset_end, strlen(fieldset_end));
	return 0;
}

int AmbaCamBasicPage_init (Page* currentPage) {
	extern Page ConfigPage;
	memset(currentPage, 0, sizeof(Page));

	AmbaConfig_init(&ConfigPage);
	*currentPage = ConfigPage;
	currentPage->superPage = &ConfigPage;
	currentPage->activeMenu = Camera_Setting;
	currentPage->activeSubMenu = VinVout;
	strcat(currentPage->name, "csb");

	memset(&CamBasicPage_Ops,0,sizeof(Page_Ops));
	currentPage->page_ops = &CamBasicPage_Ops;

	(&CamBasicPage_Ops)->get_params = AmbaCamBasicPage_get_params;
	(&CamBasicPage_Ops)->add_params = AmbaCamBasicPage_add_params;
	(&CamBasicPage_Ops)->process_PostData = AmbaCamBasicPage_process_PostData;

	_create_params();
	return 0;
}


int AmbaCamBasicPage_process_PostData (Page * currentPage) {
	int vin_fps_changed = 0;
	int vv_changed = 0;
	AmbaTransfer transfer;
	Message msg;
	transfer_init(&transfer);

	Message image_data;
	Message vv_data;

	int i = 0;
	int req_cnt = get_value(postInfo, "req_cnt");
	if ((strstr(postInfo, "req_cnt") != NULL) \
		&& (strstr(postInfo, "sec") != NULL)\
		&& (strstr(postInfo, "data") != NULL)) {
		char* string_buffer = NULL;
		string_buffer = (char*)malloc(MSG_INFO_LEN);
		if (string_buffer == NULL) {
			LOG_MESSG("CamBasic post data error");
			return -1;
		}
		memset(string_buffer, 0, MSG_INFO_LEN);
		url_decode(string_buffer, postInfo, strlen(postInfo));
		for(i = 0; i < req_cnt; i++) {

			memset(&msg, 0, sizeof(Message));

			parse_postSec(string_buffer,&msg ,i);
			parse_postData(string_buffer,&msg ,i);

			if (strcmp(msg.section_Name, "IMAGE") == 0) {
				memset(&image_data, 0, sizeof(Message));
				vin_fps_changed = 1;
				strncat(image_data.section_Name, msg.section_Name, strlen(msg.section_Name));
				strncat(image_data.msg, msg.msg, strlen(msg.msg));

			}else if (strcmp(msg.section_Name, "VINVOUT") == 0){
				memset(&vv_data, 0, sizeof(Message));
				vv_changed = 1;
				strncat(vv_data.section_Name, msg.section_Name, strlen(msg.section_Name));
				strncat(vv_data.msg, msg.msg, strlen(msg.msg));

			}
		}
		free(string_buffer);
		string_buffer = NULL;

		if (vin_fps_changed) {
			memset(&msg, 0, sizeof(Message));
			strcat(msg.section_Name, "IMAGE");
			strcat(msg.msg, "slow_shutter = 0");

			if ((transfer.send_set_request(REQ_SET_PARAM, IMAGE, msg) < 0)) {
				return -1;
			}
		}
		if (vv_changed) {
			if ((transfer.send_set_request(REQ_SET_PARAM, VINVOUT, vv_data)) < 0) {
				return -1;
			}
		}
		else {
			return -1;
		}
		if (vin_fps_changed) {
			strcat(image_data.msg, "slow_shutter = 1");
			if ((transfer.send_set_request(REQ_SET_PARAM, IMAGE, image_data)) < 0) {
				return -1;
			}
		}
	return 0;
	}
	else {
		return 1;
	}
}


int AmbaCamBasicPage_get_params (Page * currentPage) {
	int ret = (&virtual_PageOps)->process_PostData(currentPage);
	section_Param section_param;
	if (ret < 0) {
		fprintf(stdout,"1:set params failed");
	} else {
		if (ret == 0) {
			fprintf(stdout,"0:set params succeeded");
		}
		else {
			if (ret == 1) {
				memset(&section_param, 0, sizeof(section_Param));
				section_param.sectionName = "VINVOUT";
				section_param.sectionPort = VINVOUT;
				section_param.paramData = vinvout_params;
				section_param.extroInfo = "";
				section_param.paramDataNum = VINVOUT_PARAMS_NUM;
				if ((&virtual_PageOps)->get_section_param(currentPage, &section_param) == -1) {
					return -1;
				}
			} else {
				fprintf(stdout,"1:unexpected error");
			}
		}
	}
	return ret;
}


int AmbaCamBasicPage_add_params (Page * currentPage, char * text) {
	_add_vin(currentPage, text);
	_add_vout(currentPage, text);
	strcat(text, "<p align=\"center\">\n");
	strcat(text, "<input type=\"button\" value=\"Apply\" onclick = \"javascript:setCamBasic()\"/>&nbsp; &nbsp; \n");
	strcat(text, "<input type=\"button\" value=\"Cancel\" onclick = \"javascript:showPage('csb')\"/>\n");
	strcat(text, "</p>\n");
	return 0;
}

