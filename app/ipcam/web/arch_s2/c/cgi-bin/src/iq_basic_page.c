
#include <stdio.h>
#include <string.h>

#include "params.h"
#include "utils.h"
#include "page.h"
#include "config_page.h"
#include "iq_basic_page.h"

#define MAXSTRLEN 256
#define MAX_OPTION_LEN 16
static char* const cssStyle = ".textinput { width:40px; }";



typedef enum {
	PREFERENCE = 0,
	DN_MODE,
	EXPOSURE_MODE,
	BACKLIGHT_COMP,
	DC_IRIS,
	DC_IRIS_DUTY,
	LOCAL_EXPOSURE,
	MCTF_STRENGTH,
	SHUTTER_MIN,
	SHUTTER_MAX,
	MAX_GAIN,
	AE_TARGET_RATIO,
	VIN_FPS,
	SATURATION,
	BRIGHTNESS,
	HUE,
	CONTRAST,
	SHARPNESS,
	WBC,
	IMG_PARAMS_NUM
}IMG_PARAMS;

typedef enum {
	PRE_NORMAL,
	PRE_LOW_LIGHT,
	PRE_TRAFFIC,
	PRE_OPTIONS_NUM
}PRE_OPTIONS;

typedef enum {
	LEM_OFF,
	LEM_AUTO,
	LEM_WEAK,
	LEM_MEDIUM,
	LEM_STRONG,
	LEM_OPS_NUM
}LEM_OPTIONS;

typedef enum {
	EC_50HZ,
	EC_60HZ,
	EC_AUTO,
	EC_HOLD,
	EC_OPS_NUM
}EC_OPTIONS;

typedef enum {
	SHUTTER_OPS_8000,
	SHUTTER_OPS_1024,
	SHUTTER_OPS_960,
	SHUTTER_OPS_480,
	SHUTTER_OPS_240,
	SHUTTER_OPS_120,
	SHUTTER_OPS_100,
	SHUTTER_OPS_60,
	SHUTTER_OPS_50,
	SHUTTER_OPS_30,
	SHUTTER_OPS_25,
	SHUTTER_OPS_15,
	SHUTTER_OPS_7,
	SHUTTER_OPS_NUM
}SHUTTER_OPTIONS;

typedef enum {
	GAIN_OPS_30,
	GAIN_OPS_36,
	GAIN_OPS_42,
	GAIN_OPS_48,
	GAIN_OPS_54,
	GAIN_OPS_60,
	GAIN_OPS_NUM
}GAIN_OPTIONS;

typedef enum {
	WB_OPS_AUTO,
	WB_OPS_HOLD,
	WB_OPS_INCANDESCENT,
	WB_OPS_D4000,
	WB_OPS_D5000,
	WB_OPS_SUNNY,
	WB_OPS_CLOUDY,
	WB_OPS_FLASH,
	WB_OPS_FLUORESCENT,
	WB_OPS_FLUORESCENT_HIGH,
	WB_OPS_UNDER_WATER,
	WB_OPS_CUSTOM,
	WB_OPS_NUM
}WB_OPTIONS;


static ParamData img_params[IMG_PARAMS_NUM];
extern Page_Ops virtual_PageOps;
static Label_Option options[MAX_OPTION_LEN];
static Label_Option com_options[MAX_OPTION_LEN];


static int _create_params ()
{
	memset(img_params,0,sizeof(ParamData)*IMG_PARAMS_NUM);
	strcat(img_params[PREFERENCE].param_name, "preference");
	img_params[PREFERENCE].value = 0;

	strcat(img_params[DN_MODE].param_name, "dn_mode");
	img_params[DN_MODE].value = 0;

	strcat(img_params[EXPOSURE_MODE].param_name, "exposure_mode");
	img_params[EXPOSURE_MODE].value = 0;

	strcat(img_params[BACKLIGHT_COMP].param_name, "backlight_comp");
	img_params[BACKLIGHT_COMP].value = 0;

	strcat(img_params[DC_IRIS].param_name, "dc_iris");
	img_params[DC_IRIS].value = 0;

	strcat(img_params[DC_IRIS_DUTY].param_name, "dc_iris_duty");
	img_params[DC_IRIS_DUTY].value = 0;

	strcat(img_params[LOCAL_EXPOSURE].param_name, "local_exposure");
	img_params[LOCAL_EXPOSURE].value = 0;

	strcat(img_params[MCTF_STRENGTH].param_name, "mctf_strength");
	img_params[MCTF_STRENGTH].value = 0;

	strcat(img_params[SHUTTER_MIN].param_name, "shutter_min");
	img_params[SHUTTER_MIN].value = SHUTTER_8000;

	strcat(img_params[SHUTTER_MAX].param_name, "shutter_max");
	img_params[SHUTTER_MAX].value = SHUTTER_30;

	strcat(img_params[MAX_GAIN].param_name, "max_gain");
	img_params[MAX_GAIN].value = GAIN_36db;

	strcat(img_params[AE_TARGET_RATIO].param_name, "ae_target_ratio");
	img_params[AE_TARGET_RATIO].value = 100;

	strcat(img_params[VIN_FPS].param_name, "vin_fps");
	img_params[VIN_FPS].value = FPS_29;

	strcat(img_params[SATURATION].param_name, "saturation");
	img_params[SATURATION].value = 20;

	strcat(img_params[BRIGHTNESS].param_name, "brightness");
	img_params[BRIGHTNESS].value = 40;

	strcat(img_params[HUE].param_name, "hue");
	img_params[HUE].value = 60;

	strcat(img_params[CONTRAST].param_name, "contrast");
	img_params[CONTRAST].value = 80;

	strcat(img_params[SHARPNESS].param_name, "sharpness");
	img_params[SHARPNESS].value = 100;

	strcat(img_params[WBC].param_name, "wbc");
	img_params[WBC].value = 0;

	return 0;
}


static int _add_preference (Page* currentPage, char* text)
{
	char* fieldset = "<fieldset><legend>Preference</legend><br>\n";
	strncat(text, fieldset, strlen(fieldset));


	memset(options, 0, sizeof(Label_Option)*MAX_OPTION_LEN);
	strcat(options[PRE_NORMAL].option, "Normal");
	options[PRE_NORMAL].value = 0;

	strcat(options[PRE_LOW_LIGHT].option, "Low Light");
	options[PRE_LOW_LIGHT].value = 1;

	strcat(options[PRE_TRAFFIC].option, "Traffic");
	options[PRE_TRAFFIC].value = 2;


	select_Label select_label;
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Preference :";
	select_label.name = "preference";
	select_label.options = options;
	select_label.option_len = PRE_OPTIONS_NUM;
	select_label.value = img_params[PREFERENCE].value;
	select_label.action = "";


	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	char* fieldset_end = "<br><br></fieldset><br>\n";
	strncat(text, fieldset_end, strlen(fieldset_end));
	return 0;
}

static int _add_basic (Page* currentPage, char* text)
{
	char* fieldset = "<fieldset><legend>Basic Settings</legend><br>\n";
	strncat(text, fieldset, strlen(fieldset));

	memset(com_options,0,sizeof(Label_Option)*MAX_OPTION_LEN);
	memset(options,0,sizeof(Label_Option)*MAX_OPTION_LEN);

	strcat(com_options[0].option, "Off");
	com_options[0].value = 0;
	strcat(com_options[1].option, "On");
	com_options[1].value = 1;

	strcat(options[LEM_OFF].option, "Off");
	options[LEM_OFF].value = 0;

	strcat(options[LEM_AUTO].option, "Auto");
	options[LEM_AUTO].value = 1;

	strcat(options[LEM_WEAK].option, "Manual Weak");
	options[LEM_WEAK].value = 128;

	strcat(options[LEM_MEDIUM].option, "Manual Medium");
	options[LEM_MEDIUM].value = 192;

	strcat(options[LEM_STRONG].option, "Manual Strong");
	options[LEM_STRONG].value = 256;

	select_Label select_label;
	text_Entry text_entry;


	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Day Night Mode :";
	select_label.name = "dn_mode";
	select_label.options = com_options;
	select_label.option_len = 2;
	select_label.value = img_params[DN_MODE].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"&nbsp; &nbsp; ");

	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "3D De-noise Filter (0 ~ 255) :";
	text_entry.name = "mctf_strength";
	text_entry.value = img_params[MCTF_STRENGTH].value;
	text_entry.maxlen = 3;
	text_entry.ro = "";
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br>");

	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Local Exposure Mode :";
	select_label.name = "local_exposure";
	select_label.options = options;
	select_label.option_len = LEM_OPS_NUM;
	select_label.value = img_params[LOCAL_EXPOSURE].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"&nbsp; &nbsp; ");

	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Back-light Compensation :";
	select_label.name = "backlight_comp";
	select_label.options = com_options;
	select_label.option_len = 2;
	select_label.value = img_params[BACKLIGHT_COMP].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"<br><br>");
	strcat(text,"</fieldset><br>\n");
	return 0;
}


static int _add_ae (Page* currentPage, char* text)
{
	strcat(text,"<fieldset><legend>Exposure Settings</legend><br>\n");
	char action[MAXSTRLEN] = {0};

	memset(options,0,sizeof(Label_Option)*MAX_OPTION_LEN);
	strcat(options[EC_50HZ].option, "Anti-flicker 50Hz");
	options[EC_50HZ].value = 0;

	strcat(options[EC_60HZ].option, "Anti-flicker 60Hz");
	options[EC_60HZ].value = 1;

	strcat(options[EC_AUTO].option, "Auto");
	options[EC_AUTO].value = 2;

	strcat(options[EC_HOLD].option, "Hold");
	options[EC_HOLD].value = 3;

	select_Label select_label;
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Exposure Control :";
	select_label.name = "exposure_mode";
	select_label.options = options;
	select_label.option_len = EC_OPS_NUM;
	select_label.value = img_params[EXPOSURE_MODE].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"<br><br>");

	text_Entry text_entry;
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Exposure Target Factor (25% ~ 400%) :";
	text_entry.name = "ae_target_ratio";
	text_entry.value = img_params[AE_TARGET_RATIO].value;
	text_entry.maxlen = 3;
	text_entry.ro = "";
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);

	memset(options,0,sizeof(Label_Option)*MAX_OPTION_LEN);
	strcat(options[0].option, "Off");
	options[0].value = 0;

	strcat(options[1].option, "On");
	options[1].value = 1;
	memset(action,0,MAXSTRLEN);
	strcat(action,"onchange=\"setIRISDuty(this.options[this.selectedIndex].value)\"");

	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "DC Iris Mode :";
	select_label.name = "dc_iris";
	select_label.options = options;
	select_label.option_len = 2;
	select_label.value = img_params[DC_IRIS].value;
	select_label.action = action;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"&nbsp; &nbsp; ");

	memset(action,0,MAXSTRLEN);
	if ((img_params[DC_IRIS].value) == 0) {
		strcat(action,"");
	} else {
		strcat(action,"");
	}

	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Balance Duty (1 ~ 999) :";
	text_entry.name = "dc_iris_duty";
	text_entry.value = img_params[DC_IRIS_DUTY].value;
	text_entry.maxlen = 3;
	text_entry.ro = action;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br>");
	strcat(text,"&nbsp; Shutter Time Limit (seconds): ");

	memset(options,0,sizeof(Label_Option)*MAX_OPTION_LEN);
	strcat(options[SHUTTER_OPS_8000].option, "1/8000");
	options[SHUTTER_OPS_8000].value = SHUTTER_8000;

	strcat(options[SHUTTER_OPS_1024].option, "1/1024");
	options[SHUTTER_OPS_1024].value = SHUTTER_1024;

	strcat(options[SHUTTER_OPS_960].option, "1/960");
	options[SHUTTER_OPS_960].value = SHUTTER_960;

	strcat(options[SHUTTER_OPS_480].option, "1/480");
	options[SHUTTER_OPS_480].value = SHUTTER_480;

	strcat(options[SHUTTER_OPS_240].option, "1/240");
	options[SHUTTER_OPS_240].value = SHUTTER_240;

	strcat(options[SHUTTER_OPS_120].option, "1/120");
	options[SHUTTER_OPS_120].value = SHUTTER_120;

	strcat(options[SHUTTER_OPS_100].option, "1/100");
	options[SHUTTER_OPS_100].value = SHUTTER_100;

	strcat(options[SHUTTER_OPS_60].option, "1/60");
	options[SHUTTER_OPS_60].value = SHUTTER_60;

	strcat(options[SHUTTER_OPS_50].option, "1/50");
	options[SHUTTER_OPS_50].value = SHUTTER_50;

	strcat(options[SHUTTER_OPS_30].option, "1/30");
	options[SHUTTER_OPS_30].value = SHUTTER_30;

	strcat(options[SHUTTER_OPS_25].option, "1/25");
	options[SHUTTER_OPS_25].value = SHUTTER_25;

	strcat(options[SHUTTER_OPS_15].option, "1/15");
	options[SHUTTER_OPS_15].value = SHUTTER_15;

	strcat(options[SHUTTER_OPS_7].option, "1/7.5");
	options[SHUTTER_OPS_7].value = SHUTTER_7;


	memset(action,0,MAXSTRLEN);
	strcat(action,"onchange=\"setShutterLimit(this.options[this.selectedIndex].value)\"");

	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Min";
	select_label.name = "shutter_min";
	select_label.options = options;
	select_label.option_len = 11;
	select_label.value = img_params[SHUTTER_MIN].value;
	select_label.action = action;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);


	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Max";
	select_label.name = "shutter_max";
	select_label.options = options;
	select_label.option_len = SHUTTER_OPS_NUM;
	select_label.value = img_params[SHUTTER_MAX].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"<br><br>");


	//gain
	memset(options,0,sizeof(Label_Option)*MAX_OPTION_LEN);
	strcat(options[GAIN_OPS_30].option, "30db");
	options[GAIN_OPS_30].value = GAIN_30db;

	strcat(options[GAIN_OPS_36].option, "36db (default)");
	options[GAIN_OPS_36].value = GAIN_36db;

	strcat(options[GAIN_OPS_42].option, "42db");
	options[GAIN_OPS_42].value = GAIN_42db;

	strcat(options[GAIN_OPS_48].option, "48db");
	options[GAIN_OPS_48].value = GAIN_48db;

	strcat(options[GAIN_OPS_54].option, "54db");
	options[GAIN_OPS_54].value = GAIN_54db;

	strcat(options[GAIN_OPS_60].option, "60db");
	options[GAIN_OPS_60].value = GAIN_60db;

	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Sensor Max Gain";
	select_label.name = "max_gain";
	select_label.options = options;
	select_label.option_len = GAIN_OPS_NUM;
	select_label.value = img_params[MAX_GAIN].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"<br><br>");
	strcat(text,"</fieldset><br>\n");
	return 0;
}

static int _add_image (Page* currentPage, char* text)
{
	strcat(text,"<fieldset><legend>Image Property</legend><br>\n");

	text_Entry text_entry;
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Saturation (0 ~ 255) :";
	text_entry.name = "saturation";
	text_entry.value = img_params[SATURATION].value;
	text_entry.maxlen = 3;
	text_entry.ro = "";
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br>");

	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Brightness (-255 ~ 255) :";
	text_entry.name = "brightness";
	text_entry.value = img_params[BRIGHTNESS].value;
	text_entry.maxlen = 4;
	text_entry.ro = "";
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br>");

	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Contrast (0 ~ 128) :";
	text_entry.name = "contrast";
	text_entry.value = img_params[CONTRAST].value;
	text_entry.maxlen = 3;
	text_entry.ro = "";
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br>");

	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "De-noise / Sharpness (0 ~ 255) :";
	text_entry.name = "sharpness";
	text_entry.value = img_params[SHARPNESS].value;
	text_entry.maxlen = 4;
	text_entry.ro = "";
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br></fieldset><br>\n");
	return 0;
}

static int _add_awb (Page* currentPage, char* text)
{
	strcat(text,"<fieldset><legend>White Balance Setting</legend><br>\n");

	memset(options,0,MAX_OPTION_LEN*sizeof(Label_Option));
	strcat(options[WB_OPS_AUTO].option, "Auto");
	options[WB_OPS_AUTO].value = WB_AUTO;

	strcat(options[WB_OPS_HOLD].option, "Hold");
	options[WB_OPS_HOLD].value = WB_HOLD;

	strcat(options[WB_OPS_INCANDESCENT].option, "Incandescent");
	options[WB_OPS_INCANDESCENT].value = WB_INCANDESCENT;

	strcat(options[WB_OPS_D4000].option, "D4000");
	options[WB_OPS_D4000].value = WB_D4000;

	strcat(options[WB_OPS_D5000].option, "D5000");
	options[WB_OPS_D5000].value = WB_D5000;

	strcat(options[WB_OPS_SUNNY].option, "Sunny");
	options[WB_OPS_SUNNY].value = WB_SUNNY;

	strcat(options[WB_OPS_CLOUDY].option, "Cloudy");
	options[WB_OPS_CLOUDY].value = WB_CLOUDY;

	strcat(options[WB_OPS_FLASH].option, "Flash");
	options[WB_OPS_FLASH].value = WB_FLASH;

	strcat(options[WB_OPS_FLUORESCENT].option, "Fluorescent");
	options[WB_OPS_FLUORESCENT].value = WB_FLUORESCENT;

	strcat(options[WB_OPS_FLUORESCENT_HIGH].option, "Fluorescent High");
	options[WB_OPS_FLUORESCENT_HIGH].value = WB_FLUORESCENT_HIGH;

	strcat(options[WB_OPS_UNDER_WATER].option, "Under Water");
	options[WB_OPS_UNDER_WATER].value = WB_UNDER_WATER;

	strcat(options[WB_OPS_CUSTOM].option, "Custom");
	options[WB_OPS_CUSTOM].value = WB_CUSTOM;

	select_Label select_label;
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "White Balance Control :";
	select_label.name = "wbc";
	select_label.options = options;
	select_label.option_len = WB_OPS_NUM;
	select_label.value = img_params[WBC].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"<br><br></fieldset><br>\n");
	return 0;
}




int AmbaIQBasicPage_init (Page * currentPage)
{

	extern Page ConfigPage;
	memset(currentPage, 0, sizeof(Page));

	AmbaConfig_init(&ConfigPage);
	*currentPage = ConfigPage;

	currentPage->superPage = &ConfigPage;
	currentPage->activeMenu = Image_Quality;
	currentPage->activeSubMenu = Basic;
	strcat(currentPage->name, "iqb");

	memset(&IQBasicPage_Ops, 0, sizeof(Page_Ops));
	currentPage->page_ops = &IQBasicPage_Ops;

	(&IQBasicPage_Ops)->get_params = AmbaIQBasicPage_get_params;
	(&IQBasicPage_Ops)->add_params = AmbaIQBasicPage_add_params;

	_create_params();
	return 0;
}



int AmbaIQBasicPage_get_params (Page * currentPage)
{
	section_Param section_param;
	int ret = (&virtual_PageOps)->process_PostData(currentPage);
	if (ret < 0) {
		fprintf(stdout,"1:set params failed");
	} else {
		if (ret == 0) {
			fprintf(stdout,"0:set params succeeded");
		} else {
			if (ret == 1) {
				memset(&section_param, 0, sizeof(section_Param));
				section_param.sectionName = "IMAGE";
				section_param.sectionPort = IMAGE;
				section_param.paramData = img_params;
				section_param.extroInfo = "";
				section_param.paramDataNum = IMG_PARAMS_NUM;
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


int AmbaIQBasicPage_add_params (Page* currentPage, char* text)
{

	_add_preference(currentPage, text);
	_add_basic(currentPage, text);
	_add_ae(currentPage, text);
	_add_image(currentPage, text);
	_add_awb(currentPage, text);
	strcat(text, "<p align=\"center\">\n");
	strcat(text, "<input type=\"button\" value=\"Apply\" onclick = \"javascript:setIQBasic()\"/>&nbsp; &nbsp; \n");
	strcat(text, "<input type=\"button\" value=\"Cancel\" onclick = \"javascript:showPage('iqb')\"/>\n");
	strcat(text, "</p>\n");
	return 0;
}

