
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "params.h"
#include "utils.h"
#include "page.h"
#include "base_page.h"
#include "config_page.h"
#include "enc_page.h"

#define MAXSTRLEN 512
#define SHORTLEN 16

#define STREAM_NUM 4
#define STREAM_NODE_LEN (STREAM_PARAM_TYPE_NUM*STREAM_NUM)
#define ENC_NODE_LEN 25
#define NAME_LEN 20
#define ENC_PARAM_TYPE_NUM 4
#define ENC_PARAM_SPEC_TYPE_NUM 2
#define MAX_OPTION_LEN 16

extern Page_Ops virtual_PageOps;
static Label_Option options[MAX_OPTION_LEN];
typedef enum {
	TYPE,
	ENC_FPS,
	ENC_DPTZ,
	FLIP_ROTATE,
	ENC_MODE = 16,
	WIDTH,
	HEIGHT
}ENC_PARAMS_TYPE;

typedef enum {
	H264_ID,
	M,
	N,
	IDR_INTERVAL,
	GOP_MODEL,
	PROFILE,
	BRC,
	CBR_AVG_BPS,
	VBR_MIN_BPS,
	VBR_MAX_BPS,
	QUALITY,
	STREAM_PARAM_TYPE_NUM
}STREAM_PARAMS_TYPE;

typedef enum {
	CBR,
	VBR,
	CBR_Q,
	VBR_Q,
	BRC_LEN
}BRC_OPTIONS;

typedef enum {
	OFF,
	H_264,
	MJPEG,
	TYPE_LEN
}TYPE_OPTIONS;

typedef enum {
	ENC_FPS_60,
	ENC_FPS_30,
	ENC_FPS_25,
	ENC_FPS_20,
	ENC_FPS_15,
	ENC_FPS_10,
	ENC_FPS_6,
	ENC_FPS_5,
	ENC_FPS_4,
	ENC_FPS_3,
	ENC_FPS_2,
	ENC_FPS_1,
	ENC_FPS_LIST_LEN
}ENC_FPS_LIST;

typedef enum {
	DPTZ_DISABLE,
	DPTZ_ENABLE,
	DPTZ_LEN
}DPTZ_OPTIONS;


typedef enum {
	RES_OPS_1920x1080,
	RES_OPS_1440x1080,
	RES_OPS_1280x1024,
	RES_OPS_1280x960,
	RES_OPS_1280x720,
	RES_OPS_800x600,
	RES_OPS_720x576,
	RES_OPS_720x480,
	RES_OPS_640x480,
	RES_OPS_352x288,
	RES_OPS_352x240,
	RES_OPS_320x240,
	RES_OPS_176x144,
	RES_OPS_176x120,
	RES_OPS_160x120,
	RES_OPTIONS_LEN
}RES_OPTIONS;

typedef enum {
	HIGHRES_OPS_2592x1944,
	HIGHRES_OPS_2560x1440,
	HIGHRES_OPS_2304x1296,
	HIGHRES_OPS_2048x1536,
	HIGHRES_OPTIONS_LEN
}HIGHRES_OPTIONS;

typedef enum {
	FR_OPS_NORMAL,
	FR_OPS_HFLIP,
	FR_OPS_VFLIP,
	FR_OPS_ROTATE_90,
	FR_OPS_ROTATE_180,
	FR_OPS_ROTATE_270,
	FR_OPTIONS_LEN
}FR_OPTIONS;



static ParamData stream_params[STREAM_NODE_LEN];
static ParamData enc_params[ENC_NODE_LEN];
extern char postInfo[];

static int _get_name (char* ret, int streamID, char* name)
{
	sprintf(ret, "s%d_%s", streamID, name);
	return 0;
}


static int _get_enc_params_index (int streamID, char* type)
{
	int Index = 0;
	if (strcmp(type,"type") == 0) {
		Index = streamID*ENC_PARAM_TYPE_NUM + TYPE;
	}
	if (strcmp(type,"enc_fps") == 0) {
		Index = streamID*ENC_PARAM_TYPE_NUM + ENC_FPS;
	}
	if (strcmp(type,"dptz") == 0) {
		Index = streamID*ENC_PARAM_TYPE_NUM + ENC_DPTZ;
	}
	if (strcmp(type,"flip_rotate") == 0) {
		Index = streamID*ENC_PARAM_TYPE_NUM + FLIP_ROTATE;
	}
	if (strcmp(type,"enc_mode") == 0) {
		Index = ENC_MODE;
	}
	if (strcmp(type,"width") == 0) {
		Index = streamID*ENC_PARAM_SPEC_TYPE_NUM + WIDTH;
	}
	if (strcmp(type,"height") == 0) {
		Index = streamID*ENC_PARAM_SPEC_TYPE_NUM + HEIGHT;
	}
	return Index;
}

static int _get_stream_params_index (int streamID, char* type)
{
	int Index = 0;
	if (strcmp(type,"h264_id") == 0) {
		Index = streamID * STREAM_PARAM_TYPE_NUM + H264_ID;
	}
	if (strcmp(type,"M") == 0) {
		Index = streamID * STREAM_PARAM_TYPE_NUM + M;
	}
	if (strcmp(type,"N") == 0) {
		Index = streamID * STREAM_PARAM_TYPE_NUM + N;
	}
	if (strcmp(type,"idr_interval") == 0) {
		Index = streamID * STREAM_PARAM_TYPE_NUM + IDR_INTERVAL;
	}
	if (strcmp(type,"gop_model") == 0) {
		Index = streamID * STREAM_PARAM_TYPE_NUM + GOP_MODEL;
	}
	if (strcmp(type,"profile") == 0) {
		Index = streamID * STREAM_PARAM_TYPE_NUM + PROFILE;
	}
	if (strcmp(type,"brc") == 0) {
		Index = streamID * STREAM_PARAM_TYPE_NUM + BRC;
	}
	if (strcmp(type,"cbr_avg_bps") == 0) {
		Index = streamID * STREAM_PARAM_TYPE_NUM + CBR_AVG_BPS;
	}
	if (strcmp(type,"vbr_min_bps") == 0) {
		Index = streamID * STREAM_PARAM_TYPE_NUM + VBR_MIN_BPS;
	}
	if (strcmp(type,"vbr_max_bps") == 0) {
		Index = streamID * STREAM_PARAM_TYPE_NUM + VBR_MAX_BPS;
	}
	if (strcmp(type,"quality") == 0) {
		Index = streamID * STREAM_PARAM_TYPE_NUM + QUALITY;
	}
	return Index;
}

static int _create_params ()
{
	int i = 0;
	int j = 0;
	int streamID;
	char ret[NAME_LEN] = {0};
	for(streamID = 0; streamID < 4; streamID++) {
		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "h264_id");
		strcat(stream_params[i].param_name,ret);
		stream_params[i].value = 0;
		i++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "M");
		strcat(stream_params[i].param_name,ret);
		stream_params[i].value = 0;
		i++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "N");
		strcat(stream_params[i].param_name,ret);
		stream_params[i].value = 0;
		i++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "idr_interval");
		strcat(stream_params[i].param_name,ret);
		stream_params[i].value = 0;
		i++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "gop_model");
		strcat(stream_params[i].param_name,ret);
		stream_params[i].value = 0;
		i++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "profile");
		strcat(stream_params[i].param_name,ret);
		stream_params[i].value = 0;
		i++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "brc");
		strcat(stream_params[i].param_name,ret);
		stream_params[i].value = 0;
		i++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "cbr_avg_bps");
		strcat(stream_params[i].param_name,ret);
		stream_params[i].value = 0;
		i++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "vbr_min_bps");
		strcat(stream_params[i].param_name,ret);
		stream_params[i].value = 0;
		i++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "vbr_max_bps");
		strcat(stream_params[i].param_name,ret);
		stream_params[i].value = 0;
		i++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "quality");
		strcat(stream_params[i].param_name,ret);
		stream_params[i].value = 0;
		i++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "type");
		strcat(enc_params[j].param_name,ret);
		enc_params[j].value = 0;
		j++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "enc_fps");
		strcat(enc_params[j].param_name,ret);
		enc_params[j].value = 0;
		j++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "dptz");
		strcat(enc_params[j].param_name,ret);
		enc_params[j].value = 0;
		j++;

		memset(ret,0,NAME_LEN);
		_get_name(ret, streamID, "flip_rotate");
		strcat(enc_params[j].param_name,ret);
		enc_params[j].value = 0;
		j++;

	}

	strcat(enc_params[j].param_name,"enc_mode");
	enc_params[j].value = 0;
	j++;

	strcat(enc_params[j].param_name,"s0_width");
	enc_params[j].value = 1280;
	j++;

	strcat(enc_params[j].param_name,"s0_height");
	enc_params[j].value = 720;
	j++;

	strcat(enc_params[j].param_name,"s1_width");
	enc_params[j].value = 720;
	j++;

	strcat(enc_params[j].param_name,"s1_height");
	enc_params[j].value = 480;
	j++;

	strcat(enc_params[j].param_name,"s2_width");
	enc_params[j].value = 352;
	j++;

	strcat(enc_params[j].param_name,"s2_height");
	enc_params[j].value = 240;
	j++;

	strcat(enc_params[j].param_name,"s3_width");
	enc_params[j].value = 352;
	j++;

	strcat(enc_params[j].param_name,"s3_height");
	enc_params[j].value = 240;

	return 0;

}

static int _add_h264 (Page* currentPage, char* text, int streamId, char * enabled)
{
	char* fieldset = "<fieldset><legend>H.264</legend><br>";
	strncat(text, fieldset, strlen(fieldset));
	char string[MAXSTRLEN] = {0};
	char action[MAXSTRLEN] = {0};
	char text_buffer[MAXSTRLEN] = {0};
	char name[NAME_LEN] = {0};
	if (streamId == 0) {
		strcat(string, "(1-3)");
		strcat(action, "");
	}
	else {
		strcat(string, "");
		strcat(action, "disabled");
	}
	strncat(action, enabled, strlen(enabled));

	text_Entry text_entry;
	// type: M
	sprintf(text_buffer, "M %s :", string);
	_get_name(name, streamId, "M");
	int index = _get_stream_params_index(streamId, "M");
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = text_buffer;
	text_entry.name = name;
	text_entry.value = stream_params[index].value;
	text_entry.maxlen = 1;
	text_entry.ro = action;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"&nbsp; &nbsp;");

	//type: N
	memset(name,0,NAME_LEN);
	_get_name(name,streamId,"N");
	index = _get_stream_params_index(streamId,"N");
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "N (1-255) :";
	text_entry.name = name;
	text_entry.value = stream_params[index].value;
	text_entry.maxlen = 3;
	text_entry.ro = enabled;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br>");

	//type:idr
	memset(name,0,NAME_LEN);
	_get_name(name,streamId,"idr_interval");
	index = _get_stream_params_index(streamId,"idr_interval");
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "IDR interval (1-100) :";
	text_entry.name = name;
	text_entry.value = stream_params[index].value;
	text_entry.maxlen = 4;
	text_entry.ro = enabled;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br>");

	memset(options, 0, MAX_OPTION_LEN*sizeof(Label_Option));
	select_Label select_label;
	//profile
	memset(name,0,NAME_LEN);
	_get_name(name,streamId,"profile");
	index = _get_stream_params_index(streamId,"profile");
	strcat(options[0].option,"Main");
	options[0].value = PROFILE_MAIN;
	strcat(options[1].option,"Baseline");
	options[1].value = PROFILE_BASELINE;
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Profile :";
	select_label.name = name;
	select_label.options = options;
	select_label.option_len = 2;
	select_label.value = stream_params[index].value;
	select_label.action = enabled;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"<br><br>");

	//brc
	memset(options, 0, MAX_OPTION_LEN*sizeof(Label_Option));
	strcat(options[CBR].option, "CBR");
	options[CBR].value = BRC_CBR;

	strcat(options[VBR].option, "VBR");
	options[VBR].value = BRC_VBR;

	strcat(options[CBR_Q].option, "CBR (keep quality)");
	options[CBR_Q].value = BRC_CBRQ;

	strcat(options[VBR_Q].option, "VBR (keep quality)");
	options[VBR_Q].value = BRC_VBRQ;

	memset(action, 0, MAXSTRLEN);
	sprintf(action, "onchange=\"setBRCMode(this.options[this.selectedIndex].value, %d)\" %s",\
		streamId, enabled);
	memset(name, 0, NAME_LEN);
	_get_name(name, streamId, "brc");
	index = _get_stream_params_index(streamId, "brc");
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Bitrate control :";
	select_label.name = name;
	select_label.options = options;
	select_label.option_len = BRC_LEN;
	select_label.value = stream_params[index].value;
	select_label.action = action;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text, "<br><br>");

	memset(action,0,MAXSTRLEN);
	if (stream_params[index].value & 0x1) {
		strcat(action, "disabled");
	}
	else {
		strcat(action, "");
	}
	if ((strcmp(action, "disabled") == 0) || (strcmp(enabled, "disabled") == 0)) {
		memset(action, 0, MAXSTRLEN);
		strcat(action, "disabled");
	}

	//cbr_avg_bps
	memset(name, 0, NAME_LEN);
	_get_name(name, streamId, "cbr_avg_bps");
	index = _get_stream_params_index(streamId, "cbr_avg_bps");
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Average Bitrate  :";
	text_entry.name = name;
	text_entry.value = stream_params[index].value;
	text_entry.maxlen = 10;
	text_entry.ro = action;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br>");

	//vbr_min_bps
	memset(name, 0, NAME_LEN);
	_get_name(name, streamId, "vbr_min_bps");
	index = _get_stream_params_index(streamId, "vbr_min_bps");
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Min bitrate (bps) :";
	text_entry.name = name;
	text_entry.value = stream_params[index].value;
	text_entry.maxlen = 10;
	text_entry.ro = action;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text, "&nbsp; &nbsp; ");

	//vbr_max_bps
	memset(name, 0, NAME_LEN);
	_get_name(name,streamId, "vbr_max_bps");
	index = _get_stream_params_index(streamId, "vbr_max_bps");
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Max bitrate (bps) :";
	text_entry.name = name;
	text_entry.value = stream_params[index].value;
	text_entry.maxlen = 10;
	text_entry.ro = action;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);

	char* fieldset_end = "<br></fieldset><br>";
	strncat(text, fieldset_end, strlen(fieldset_end));

	return 0;
}

static int _add_mjpeg(Page* currentPage, char* text, int streamId, char * enabled)
{
	char* fieldset = "<fieldset><legend>MJPEG</legend><br>";
	strncat(text, fieldset, strlen(fieldset));
	char name[NAME_LEN] = {0};
	int index = 0;
	_get_name(name,streamId,"quality");
	index = _get_stream_params_index(streamId, "quality");

	text_Entry text_entry;
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Quality(0 - 100)";
	text_entry.name = name;
	text_entry.value = stream_params[index].value;
	text_entry.maxlen = 4;
	text_entry.ro = enabled;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	char* fieldset_end = "<br></fieldset><br>";
	strncat(text, fieldset_end, strlen(fieldset_end));

	return 0;
}

static int _add_expand_format(Page* currentPage, char* text, int streamId, char * enabled)
{
	char text_buffer[MAXSTRLEN] = {0};
	char headerId[NAME_LEN] = {0};
	char expandId[NAME_LEN] = {0};

	sprintf(headerId, "stream%d", streamId);
	sprintf(expandId, "setting%d", streamId);
	sprintf(text_buffer, "<br><br><div class=\"expandstyle\"><span id=\"%s\" onClick=\"expandEncodeFormat('%s', '%s', 360, %d)\">",\
		headerId, headerId, expandId, streamId);
	strncat(text, text_buffer, strlen(text_buffer));
	strcat(text, "<img src=\"../img/expand.gif\" /></span>Stream settings :<br><br>\n");

	memset(text_buffer, 0, MAXSTRLEN);
	sprintf(text_buffer, "<div class=\"expandcontent\" id=\"%s\">", expandId);
	strncat(text, text_buffer, strlen(text_buffer));
	_add_h264(currentPage, text, streamId, enabled);
	_add_mjpeg(currentPage, text, streamId, enabled);
	strcat(text,"</div>");
	strcat(text,"</div>");

	return 0;
}
static int _add_encode_format (Page* currentPage, char* text, int streamId, char* enabled, int encMode)
{
	sprintf(text, "<fieldset><legend>Stream %d</legend><br>\n", streamId);
	char name[NAME_LEN] = {0};
	int index =0;
	int value = 0;
	char action[MAXSTRLEN] = {0};
	//type
	memset(options,0,MAX_OPTION_LEN*sizeof(Label_Option));

	options[OFF].value = OFF;
	strcat(options[OFF].option, "OFF");

	options[H_264].value = H_264;
	strcat(options[H_264].option, "H.264");

	options[MJPEG].value = MJPEG;
	strcat(options[MJPEG].option, "MJPEG");

	_get_name(name,streamId, "type");
	index = _get_enc_params_index(streamId, "type");
	select_Label select_label;
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Type :";
	select_label.name = name;
	select_label.options = options;
	select_label.option_len = TYPE_LEN;
	select_label.value = enc_params[index].value;
	select_label.action = enabled;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text, "&nbsp; &nbsp; ");

	//enc_fps
	memset(options, 0, MAX_OPTION_LEN*sizeof(Label_Option));
	strcat(options[ENC_FPS_60].option, "60");
	options[ENC_FPS_60].value = FPS_60;

	strcat(options[ENC_FPS_30].option, "30");
	options[ENC_FPS_30].value = FPS_30;

	strcat(options[ENC_FPS_25].option, "25");
	options[ENC_FPS_25].value = FPS_25;

	strcat(options[ENC_FPS_20].option, "20");
	options[ENC_FPS_20].value = FPS_20;

	strcat(options[ENC_FPS_15].option, "15");
	options[ENC_FPS_15].value = FPS_15;

	strcat(options[ENC_FPS_10].option, "10");
	options[ENC_FPS_10].value = FPS_10;

	strcat(options[ENC_FPS_6].option, "6");
	options[ENC_FPS_6].value = FPS_6;

	strcat(options[ENC_FPS_5].option, "5");
	options[ENC_FPS_5].value = FPS_5;

	strcat(options[ENC_FPS_4].option, "4");
	options[ENC_FPS_4].value = FPS_4;

	strcat(options[ENC_FPS_3].option, "3");
	options[ENC_FPS_3].value = FPS_3;

	strcat(options[ENC_FPS_2].option, "2");
	options[ENC_FPS_2].value = FPS_2;

	strcat(options[ENC_FPS_1].option, "1");
	options[ENC_FPS_1].value = FPS_1;

	memset(name, 0, NAME_LEN);
	_get_name(name,streamId,"enc_fps");
	index = _get_enc_params_index(streamId, "enc_fps");
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Encode FPS :";
	select_label.name = name;
	select_label.options = options;
	select_label.option_len = ENC_FPS_LIST_LEN;
	select_label.value = enc_params[index].value;
	select_label.action = enabled;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text, "&nbsp; &nbsp; ");


	//dptz
	memset(options,0,MAX_OPTION_LEN*sizeof(Label_Option));
	strcat(options[DPTZ_DISABLE].option, "Disable");
	options[DPTZ_DISABLE].value = DPTZ_DISABLE;
	strcat(options[DPTZ_ENABLE].option, "Enable");
	options[DPTZ_ENABLE].value = DPTZ_ENABLE;

	if (((enc_params[_get_enc_params_index(streamId, "width")].value) == 0 )||\
		((enc_params[_get_enc_params_index(streamId, "height")].value) == 0)) {
		value = create_res(1280,720);
	}
	else {
		value = create_res(enc_params[_get_enc_params_index(streamId,"width")].value,\
			enc_params[_get_enc_params_index(streamId,"height")].value);
	}


	if (encMode == ENC_LOW_DELAY) {
		strcat(action, "disabled");
	}
	else {
		if (strcmp(enabled,"disabled") == 0) {
			strcat(action,"disabled");
		}
		else {
			strcat(action,"");
		}
	}

	memset(name, 0, NAME_LEN);
	_get_name(name, streamId, "dptz");
	index = _get_enc_params_index(streamId, "dptz");
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "DPTZ Type :";
	select_label.name = name;
	select_label.options = options;
	select_label.option_len = DPTZ_LEN;
	select_label.value = enc_params[index].value;
	select_label.action = action;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text, "<br><br>\n");

	//resolution
	memset(options, 0, MAX_OPTION_LEN*sizeof(Label_Option));
	memset(action, 0, MAXSTRLEN);
	sprintf(action, "onchange=\"setDPTZMode(this.options[this.selectedIndex].value, %d)\" %s",\
		streamId, enabled);
	memset(name, 0, NAME_LEN);
	_get_name(name, streamId, "resolution");
	if ((encMode != ENC_HIGH_MP) || (streamId != 0)) {
		strcat(options[RES_OPS_1920x1080].option, "1920 x 1080");
		options[RES_OPS_1920x1080].value = RES_1920x1080;

		strcat(options[RES_OPS_1440x1080].option, "1440 x 1080");
		options[RES_OPS_1440x1080].value = RES_1440x1080;

		strcat(options[RES_OPS_1280x1024].option, "1280 x 1024");
		options[RES_OPS_1280x1024].value = RES_1280x1024;

		strcat(options[RES_OPS_1280x960].option, "1280 x 960");
		options[RES_OPS_1280x960].value = RES_1280x960;

		strcat(options[RES_OPS_1280x720].option, "1280 x 720");
		options[RES_OPS_1280x720].value = RES_1280x720;

		strcat(options[RES_OPS_800x600].option, "800 x 600");
		options[RES_OPS_800x600].value = RES_800x600;

		strcat(options[RES_OPS_720x576].option, "720 x 576");
		options[RES_OPS_720x576].value = RES_720x576;

		strcat(options[RES_OPS_720x480].option, "720 x 480");
		options[RES_OPS_720x480].value = RES_720x480;

		strcat(options[RES_OPS_640x480].option, "640 x 480");
		options[RES_OPS_640x480].value = RES_640x480;

		strcat(options[RES_OPS_352x288].option, "352 x 288");
		options[RES_OPS_352x288].value = RES_352x288;

		strcat(options[RES_OPS_352x240].option, "352 x 240");
		options[RES_OPS_352x240].value = RES_352x240;

		strcat(options[RES_OPS_320x240].option, "320 x 240");
		options[RES_OPS_320x240].value = RES_320x240;

		strcat(options[RES_OPS_176x144].option, "176 x 144");
		options[RES_OPS_176x144].value = RES_176x144;

		strcat(options[RES_OPS_176x120].option, "176 x 120");
		options[RES_OPS_176x120].value = RES_176x120;

		strcat(options[RES_OPS_160x120].option, "160 x 120");
		options[RES_OPS_160x120].value = RES_160x120;

		memset(&select_label, 0, sizeof(select_Label));
		select_label.label = "Resolution :";
		select_label.name = name;
		select_label.options = options;
		select_label.option_len = RES_OPTIONS_LEN;
		select_label.value = value;
		select_label.action = action;
		(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	}else {
		strcat(options[HIGHRES_OPS_2592x1944].option, "2592x1944 (5.0M)");
		options[HIGHRES_OPS_2592x1944].value = HIGHRES_2592x1944;

		strcat(options[HIGHRES_OPS_2560x1440].option, "2560x1440 (3.7M)");
		options[HIGHRES_OPS_2560x1440].value = HIGHRES_2560x1440;

		strcat(options[HIGHRES_OPS_2304x1296].option, "2304x1296 (3.0M)");
		options[HIGHRES_OPS_2304x1296].value = HIGHRES_2304x1296;

		strcat(options[HIGHRES_OPS_2048x1536].option, "2048x1536 (3.0M)");
		options[HIGHRES_OPS_2048x1536].value = HIGHRES_2048x1536;

		memset(&select_label, 0, sizeof(select_Label));
		select_label.label = "Resolution :";
		select_label.name = name;
		select_label.options = options;
		select_label.option_len = HIGHRES_OPTIONS_LEN;
		select_label.value = value;
		select_label.action = action;
		(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	}
	strcat(text,"&nbsp; &nbsp; ");

	/*
		Flip & Rotate
	*/
	memset(options, 0, MAX_OPTION_LEN*sizeof(Label_Option));
	strcat(options[FR_OPS_NORMAL].option, "Normal");
	options[FR_OPS_NORMAL].value = FR_NORMAL;

	strcat(options[FR_OPS_HFLIP].option, "Horizontal Flip");
	options[FR_OPS_HFLIP].value = FR_HFLIP;

	strcat(options[FR_OPS_VFLIP].option, "Vertical Flip");
	options[FR_OPS_VFLIP].value = FR_VFLIP;

	strcat(options[FR_OPS_ROTATE_90].option, "Rotate Clockwise 90");
	options[FR_OPS_ROTATE_90].value = FR_ROTATE_90;

	strcat(options[FR_OPS_ROTATE_180].option, "Rotate 180");
	options[FR_OPS_ROTATE_180].value = FR_ROTATE_180;

	strcat(options[FR_OPS_ROTATE_270].option, "Rotate Clockwise 270");
	options[FR_OPS_ROTATE_270].value = FR_ROTATE_270;

	memset(name, 0, NAME_LEN);
	_get_name(name,streamId,"flip_rotate");
	index = _get_enc_params_index(streamId, "flip_rotate");
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Flip & Rotate :";
	select_label.name = name;
	select_label.options = options;
	select_label.option_len = FR_OPTIONS_LEN;
	select_label.value = enc_params[index].value;
	select_label.action = enabled;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);

	_add_expand_format(currentPage, text, streamId, enabled);

	char* fieldset_end = "</fieldset><br>";
	strncat(text, fieldset_end, strlen(fieldset_end));

	return 0;
}



int AmbaEncPage_init (Page* currentPage)
{
	extern Page ConfigPage;
	memset(currentPage, 0, sizeof(Page));

	AmbaConfig_init(&ConfigPage);
	*currentPage = ConfigPage;


	currentPage->superPage = &ConfigPage;
	currentPage->activeMenu = Camera_Setting;
	currentPage->activeSubMenu = Encode_Setting;
	strcat(currentPage->name, "enc");

	memset(&EncodePage_Ops, 0, sizeof(Page_Ops));
	currentPage->page_ops = &EncodePage_Ops;

	(&EncodePage_Ops)->process_PostData = AmbaEncPage_process_PostData;
	(&EncodePage_Ops)->get_params = AmbaEncPage_get_params;
	(&EncodePage_Ops)->add_params = AmbaEncPage_add_params;
	_create_params();
	return 0;
}


int AmbaEncPage_process_PostData (Page * currentPage)
{
	int ret = 0;
	AmbaTransfer transfer;
	Message msg;
	transfer_init(&transfer);
	int i = 0;
	int req_cnt = get_value(postInfo, "req_cnt");
	int req = 0;
	int info = 0;
	int data = 0;
	char buffer[SHORTLEN] = {0};
	char* postInfo_buf = NULL;

	postInfo_buf = strstr(postInfo, "req_cnt");
	if (postInfo_buf != NULL) {
		for(i = 0; i < req_cnt; i++) {
			if ((strstr(postInfo, "sec") != NULL) && (strstr(postInfo, "data") != NULL)) {
				char* string_buffer = NULL;
				string_buffer = (char *)malloc(MSG_INFO_LEN);
				if (string_buffer == NULL) {
					LOG_MESSG("EncPage post data error");
					return -1;
				}
				memset(string_buffer, 0, MSG_INFO_LEN);
				url_decode(string_buffer, postInfo_buf, strlen(postInfo_buf));

				memset(&msg, 0, sizeof(Message));
				parse_postSec(string_buffer, &msg, i);
				parse_postData(string_buffer, &msg, i);
				ret = transfer.send_set_request(REQ_SET_PARAM, ENCODE_PORT, msg);
				if (ret < 0) {
					return -1;
				}

				free(string_buffer);
				string_buffer = NULL;

			} else if ((strstr(postInfo,"req") != NULL)&&(strstr(postInfo,"info") != NULL)){

				memset(buffer, 0, SHORTLEN);
				sprintf(buffer, "req%d", i);
				req = get_value(postInfo, buffer);

				memset(buffer, 0, SHORTLEN);
				sprintf(buffer, "info%d", i);
				info = get_value(postInfo, buffer);

				memset(buffer, 0, SHORTLEN);
				sprintf(buffer, "data%d", i);
				data = get_value(postInfo, buffer);

				ret = transfer.send_fly_request(req, info, data);
				if (ret != 0) {
					return -1;
				}
			}
			return 0;
		}
	}
	return 1;
}

int AmbaEncPage_get_params (Page* currentPage)
{
	int ret = 0;
	char name[NAME_LEN] = {0};
	int i;
	unsigned int stream_port[] = {STREAM0,STREAM1,STREAM2,STREAM3};

	ret = (&virtual_PageOps)->process_PostData(currentPage);
	section_Param section_param;
	if (ret < 0) {
		fprintf(stdout,"1:set params failed");
	} else {
		if (ret == 0) {
			fprintf(stdout,"0:set params succeeded");
		} else {
			if (ret == 1) {
				memset(&section_param, 0, sizeof(section_Param));
				section_param.sectionName = "ENCODE";
				section_param.sectionPort = ENCODE;
				section_param.paramData = enc_params;
				section_param.extroInfo = "";
				section_param.paramDataNum = ENC_NODE_LEN;
				if ((&virtual_PageOps)->get_section_param(currentPage, &section_param) == -1) {
					return -1;
				}
				for ( i = 0; i < STREAM_NUM; i++ ) {
					memset(name, 0, NAME_LEN);
					sprintf(name, "STREAM%d", i);
					memset(&section_param, 0, sizeof(section_Param));
					section_param.sectionName = name;
					section_param.sectionPort = stream_port[i];
					section_param.paramData = stream_params;
					section_param.extroInfo = "";
					section_param.paramDataNum = STREAM_NODE_LEN;
					if ((&virtual_PageOps)->get_section_param(currentPage, &section_param) == -1) {
						return -1;
					}
				}
			} else {
				fprintf(stdout,"1:unexpected error %d",ret);
			}
		}
	}
	return ret;
}

int AmbaEncPage_add_params (Page * currentPage, char * text)
{
	memset(options, 0, MAX_OPTION_LEN*sizeof(Label_Option));
	options[0].value = 0;
	strcat(options[0].option, "Normal mode");
	options[1].value = 1;
	strcat(options[1].option, "High Mega-pixel mode");
	options[2].value = 2;
	strcat(options[2].option, "Low delay mode");

	int i;
	int encMode = 0;

	for (i = 0; i < ENC_NODE_LEN; i++) {
		if (strcmp(enc_params[i].param_name,"enc_mode") ==  0) {
			encMode = enc_params[i].value;
		}
	}
	char* action = "onchange=\"setEncodeMode(this.options[this.selectedIndex].value)\"";
	select_Label select_label;
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Encode Mode :";
	select_label.name = "enc_mode";
	select_label.options = options;
	select_label.option_len = 3;
	select_label.value = encMode;
	select_label.action = action;

	char* params = NULL;
	params = (char *)malloc(PAGE_CONTENT_LEN);
	if (params == NULL) {
		LOG_MESSG("EncPage add params error");
		return -1;
	}
	memset(params, 0, PAGE_CONTENT_LEN);
	(&virtual_PageOps)->create_select_label(currentPage, params, &select_label);
	strcat(text, params);
	strcat(text, "<br><br>\n");
	char enabled[SHORTLEN] = {0};
	for (i = 0; i < STREAM_NUM; i ++) {
		memset(enabled, 0, sizeof(enabled));
		if ((i > 0) && (encMode != ENC_HIGH_FPS)){
			strcat(enabled," disabled");
		} else {
			strcat(enabled, "");
		}

			memset(params, 0, PAGE_CONTENT_LEN);
			_add_encode_format(currentPage, params, i, enabled, encMode);
			strncat(text, params, strlen(params));
	}
	char* string = "<p align=\"center\" >\n\
		<input type=\"button\" value=\"Apply\" onclick = \"javascript:setEnc()\"/>&nbsp; &nbsp; \n\
		<input type=\"button\" value=\"Cancel\" onclick = \"javascript:showPage('enc')\"/>\n\
		</p>\n";
	strncat(text, string, strlen(string));
	free(params);
	params = NULL;
	return 0;
}
