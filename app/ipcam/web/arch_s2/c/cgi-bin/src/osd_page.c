
#include <stdio.h>
#include <string.h>

#include "params.h"
#include "utils.h"
#include "page.h"
#include "config_page.h"
#include "osd_page.h"


typedef enum
{
	NO_ROTATE = 0,
	BMP_ENABLE,
	TIME_ENABLE,
	TEXT_ENABLE,
	TEXT,
	TEXT_SIZE,
	TEXT_OUTLINE,
	TEXT_COLOR,
	TEXT_BOLD,
	TEXT_ITALIC,
	TEXT_STARTX,
	TEXT_STARTY,
	TEXT_BOXW,
	TEXT_BOXH,
	OSD_PARAM_TYPE_NUM
}OSD_PARAMS;


#define STREAM_NUM 4
#define OSD_PARAM_NUM (OSD_PARAM_TYPE_NUM*STREAM_NUM)

#define NAME_LEN 20
#define MAXSTRLEN 128
#define MAX_OPTION_LEN 16

static ParamData osd_params[OSD_PARAM_NUM];
static Label_Option options[MAX_OPTION_LEN];

typedef enum {
	FTSIZE_OPS_SMALLER,
	FTSIZE_OPS_SMALL,
	FTSIZE_OPS_MIDDLE,
	FTSIZE_OPS_LARGE,
	FTSIZE_OPS_LARGER,
	FTSIZE_OPS_NUM
}FTSIZE_OPTIONS;

typedef enum {
	COLOR_OPS_BLACK,
	COLOR_OPS_RED,
	COLOR_OPS_BLUE,
	COLOR_OPS_GREEN,
	COLOR_OPS_YELLOW,
	COLOR_OPS_MAGENTA,
	COLOR_OPS_CYAN,
	COLOR_OPS_WHITE,
	COLOR_OPS_NUM
}COLOR_OPTIONS;


static int _get_osd_Index (int streamID, char* text)
{
	char* osd_params_option[] = {"no_rotate","bmp_enable","time_enable","text_enable","text",\
		"text_size","text_outline","text_color","text_bold","text_italic","text_startx",\
		"text_starty","text_boxw","text_boxh"};
	int i;
	for (i = 0 ; i < OSD_PARAM_TYPE_NUM; i++) {
		if (strcmp(text, osd_params_option[i]) == 0) {
			return streamID *OSD_PARAM_TYPE_NUM + i;
		}
	}
	return -1;
}

static int _get_name (char* ret, int streamID, char* name)
{
	sprintf(ret,"s%d_%s",streamID,name);
	return 0;
}

static int _create_params ()
{
	char name[NAME_LEN] = {0};
	int i;
	memset(osd_params, 0, sizeof(ParamData)*OSD_PARAM_NUM);
	for (i = 0; i < STREAM_NUM; i++) {
		memset(name,0,NAME_LEN);
		_get_name(name, i, "no_rotate");
		strcat(osd_params[_get_osd_Index(i, "no_rotate")].param_name, name);
		osd_params[_get_osd_Index(i, "no_rotate")].value = 0;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "bmp_enable");
		strcat(osd_params[_get_osd_Index(i, "bmp_enable")].param_name, name);
		osd_params[_get_osd_Index(i, "bmp_enable")].value = 0;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "time_enable");
		strcat(osd_params[_get_osd_Index(i, "time_enable")].param_name, name);
		osd_params[_get_osd_Index(i, "time_enable")].value = 0;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "text_enable");
		strcat(osd_params[_get_osd_Index(i, "text_enable")].param_name, name);
		osd_params[_get_osd_Index(i, "text_enable")].value = 0;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "text");
		strcat(osd_params[_get_osd_Index(i, "text")].param_name, name);
		osd_params[_get_osd_Index(i, "text")].value = -100;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "text_size");
		strcat(osd_params[_get_osd_Index(i, "text_size")].param_name, name);
		osd_params[_get_osd_Index(i, "text_size")].value = FTSIZE_NORMAL;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "text_outline");
		strcat(osd_params[_get_osd_Index(i, "text_outline")].param_name, name);
		osd_params[_get_osd_Index(i, "text_outline")].value = 0;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "text_color");
		strcat(osd_params[_get_osd_Index(i, "text_color")].param_name, name);
		osd_params[_get_osd_Index(i, "text_color")].value = 0;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "text_bold");
		strcat(osd_params[_get_osd_Index(i, "text_bold")].param_name, name);
		osd_params[_get_osd_Index(i, "text_bold")].value = 0;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "text_italic");
		strcat(osd_params[_get_osd_Index(i, "text_italic")].param_name, name);
		osd_params[_get_osd_Index(i, "text_italic")].value = 0;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "text_startx");
		strcat(osd_params[_get_osd_Index(i, "text_startx")].param_name, name);
		osd_params[_get_osd_Index(i, "text_startx")].value = 0;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "text_starty");
		strcat(osd_params[_get_osd_Index(i, "text_starty")].param_name, name);
		osd_params[_get_osd_Index(i, "text_starty")].value = 0;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "text_boxw");
		strcat(osd_params[_get_osd_Index(i, "text_boxw")].param_name, name);
		osd_params[_get_osd_Index(i, "text_boxw")].value = 50;

		memset(name,0,NAME_LEN);
		_get_name(name, i, "text_boxh");
		strcat(osd_params[_get_osd_Index(i, "text_boxh")].param_name, name);
		osd_params[_get_osd_Index(i, "text_boxh")].value = 50;
	}
	return 0;
}


static int _add_osd_text_box (Page* currentPage, char* text, int streamID, char* enabled)
{
	strcat(text,"<fieldset><legend>Text Box (0~100%)</legend><br>");
	char name[NAME_LEN] = {0};
	text_Entry text_entry;

	_get_name(name,streamID,"text_startx");
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Offset X : ";
	text_entry.name = name;
	text_entry.value = osd_params[_get_osd_Index(streamID, "text_startx")].value;
	text_entry.maxlen = 2;
	text_entry.ro = enabled;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"&nbsp; &nbsp; ");

	memset(name,0,NAME_LEN);
	_get_name(name,streamID,"text_starty");
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Y : ";
	text_entry.name = name;
	text_entry.value = osd_params[_get_osd_Index(streamID, "text_starty")].value;
	text_entry.maxlen = 2;
	text_entry.ro = enabled;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br>");

	memset(name,0,NAME_LEN);
	_get_name(name,streamID,"text_boxw");
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Width : ";
	text_entry.name = name;
	text_entry.value = osd_params[_get_osd_Index(streamID, "text_boxw")].value;
	text_entry.maxlen = 2;
	text_entry.ro = enabled;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"&nbsp; &nbsp; ");


	memset(name,0,NAME_LEN);
	_get_name(name,streamID,"text_boxh");
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Height : ";
	text_entry.name = name;
	text_entry.value = osd_params[_get_osd_Index(streamID, "text_boxh")].value;
	text_entry.maxlen = 2;
	text_entry.ro = enabled;
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br>");

	strcat(text,"<br></fieldset>");
	return 0;
}

static int _add_osd_text (Page* currentPage, char* text, int streamID, char* enabled)
{
	strcat(text,"<fieldset><legend>OSD Text</legend><br>");
	char name[NAME_LEN] = {0};
	_get_name(name,streamID,"text");
	char content[MAXSTRLEN]= {0};
	strcat(content,osd_params[_get_osd_Index(streamID,"text")].param_value);
	int len = strlen(content);
	int i = 0;
	if ((content[0] == '"') && (content[len - 1] == '"')) {
		content[len-1] = '\0';
		while ((content[i + 1] != '\0')&&(content[i] != '\0')) {
			content[i] = content[i + 1];
			i++;
		}
	}
	wide_text_Entry wide_text_entry;
	memset(&wide_text_entry, 0, sizeof(wide_text_Entry));
	wide_text_entry.label = "Content (32 character) : ";
	wide_text_entry.name = name;
	wide_text_entry.value = content;
	wide_text_entry.maxlen = 32;
	wide_text_entry.ro = enabled;
	(&virtual_PageOps)->create_wide_text_entry(currentPage, text, &wide_text_entry);
	strcat(text,"<br><br>");

	// text size
	memset(name,0,NAME_LEN);
	_get_name(name,streamID,"text_size");

	memset(options,0,sizeof(Label_Option)*MAX_OPTION_LEN);
	strcat(options[FTSIZE_OPS_SMALLER].option, "Smaller");
	options[FTSIZE_OPS_SMALLER].value = FTSIZE_SMALLER;

	strcat(options[FTSIZE_OPS_SMALL].option, "Small");
	options[FTSIZE_OPS_SMALL].value = FTSIZE_SMALL;

	strcat(options[FTSIZE_OPS_MIDDLE].option, "Middle");
	options[FTSIZE_OPS_MIDDLE].value = FTSIZE_NORMAL;

	strcat(options[FTSIZE_OPS_LARGE].option, "Large");
	options[FTSIZE_OPS_LARGE].value = FTSIZE_LARGE;

	strcat(options[FTSIZE_OPS_LARGER].option, "Larger");
	options[FTSIZE_OPS_LARGER].value = FTSIZE_LARGER;

	if (osd_params[_get_osd_Index(streamID,"text_size")].value == 0) {
		osd_params[_get_osd_Index(streamID,"text_size")].value = FTSIZE_NORMAL;
	}
	select_Label select_label;
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Font size :";
	select_label.name = name;
	select_label.options = options;
	select_label.option_len = FTSIZE_OPS_NUM;
	select_label.value = osd_params[_get_osd_Index(streamID,"text_size")].value;
	select_label.action = enabled;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"&nbsp; &nbsp; ");


	//text color
	memset(name,0,NAME_LEN);
	_get_name(name,streamID,"text_color");
	memset(options,0,sizeof(Label_Option)*MAX_OPTION_LEN);
	strcat(options[COLOR_OPS_BLACK].option, "Black");
	options[COLOR_OPS_BLACK].value = COLOR_BLACK;

	strcat(options[COLOR_OPS_RED].option, "Red");
	options[COLOR_OPS_RED].value = COLOR_RED;

	strcat(options[COLOR_OPS_BLUE].option, "Blue");
	options[COLOR_OPS_BLUE].value = COLOR_BLUE;

	strcat(options[COLOR_OPS_GREEN].option, "Green");
	options[COLOR_OPS_GREEN].value = COLOR_GREEN;

	strcat(options[COLOR_OPS_YELLOW].option, "Yellow");
	options[COLOR_OPS_YELLOW].value = COLOR_YELLOW;

	strcat(options[COLOR_OPS_MAGENTA].option, "Magenta");
	options[COLOR_OPS_MAGENTA].value = COLOR_MEGENTA;

	strcat(options[COLOR_OPS_CYAN].option, "Cyan");
	options[COLOR_OPS_CYAN].value = COLOR_CYAN;

	strcat(options[COLOR_OPS_WHITE].option, "White");
	options[COLOR_OPS_WHITE].value = COLOR_WHITE;

	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Color :";
	select_label.name = name;
	select_label.options = options;
	select_label.option_len = COLOR_OPS_NUM;
	select_label.value = osd_params[_get_osd_Index(streamID,"text_color")].value;
	select_label.action = enabled;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"<br><br>");

	//text outline
	memset(name,0,NAME_LEN);
	memset(options,0,8*sizeof(Label_Option));
	_get_name(name,streamID,"text_outline");
	strcat(options[0].option, "0");
	options[0].value = 0;

	strcat(options[1].option, "1");
	options[1].value = 1;

	strcat(options[2].option, "2");
	options[2].value = 2;

	strcat(options[3].option, "3");
	options[3].value = 3;

	strcat(text,"&nbsp; &nbsp; ");
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Outline";
	select_label.name = name;
	select_label.options = options;
	select_label.option_len = 4;
	select_label.value = osd_params[_get_osd_Index(streamID,"text_outline")].value;
	select_label.action = enabled;
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"&nbsp; &nbsp;");


	// text bold
	memset(name,0,NAME_LEN);
	_get_name(name, streamID, "text_bold");
	checkBox_Input checkBox;
	memset(&checkBox, 0, sizeof(checkBox_Input));
	checkBox.label = "Bold";
	checkBox.name = name;
	checkBox.value = osd_params[_get_osd_Index(streamID, "text_bold")].value;
	checkBox.action = enabled;
	(&virtual_PageOps)->create_checkbox(currentPage, text, &checkBox);
	strcat(text,"&nbsp; &nbsp;");


	//text italic
	memset(name,0,NAME_LEN);
	_get_name(name, streamID, "text_italic");
	memset(&checkBox, 0, sizeof(checkBox_Input));
	checkBox.label = "Italic";
	checkBox.name = name;
	checkBox.value = osd_params[_get_osd_Index(streamID, "text_italic")].value;
	checkBox.action = enabled;
	(&virtual_PageOps)->create_checkbox(currentPage, text, &checkBox);
	strcat(text,"<br><br>");
	_add_osd_text_box(currentPage, text, streamID, enabled);
	strcat(text,"<br></fieldset><br>");
	return 0;
}



static int _add_expand_osd_text (Page* currentPage, char* text, int streamID)
{
	char headerID[NAME_LEN] = {0};
	char expandID[NAME_LEN] = {0};
	char name[NAME_LEN] = {0};
	char enabled[NAME_LEN] = {0};
	char buffer[MAXSTRLEN] = {0};
	sprintf(headerID,"text%d",streamID);
	sprintf(expandID,"osd_text%d",streamID);
	_get_name(name,streamID,"text_enable");
	if (osd_params[_get_osd_Index(streamID,"text_enable")].value == 0) {
		strcat(enabled,"disabled");
	} else {
		strcat(enabled,"");
	}
	sprintf(buffer,"<div class=\"expandstyle\"><span id=\"%s\" \
		onClick=\"expandOSDText('%s', '%s', 280, %d)\">",headerID,headerID,expandID,streamID);
	strcat(text,buffer);
	strcat(text,"<img src=\"../img/expand.gif\" /></span>");
	memset(buffer,0,MAXSTRLEN);
	sprintf(buffer,"onclick=\"addOSD('text', %d)\"",streamID);
	checkBox_Input checkBox;
	memset(&checkBox, 0, sizeof(checkBox_Input));
	checkBox.label = "Add text string";
	checkBox.name = name;
	checkBox.value = osd_params[_get_osd_Index(streamID, "text_enable")].value;
	checkBox.action = buffer;
	(&virtual_PageOps)->create_checkbox(currentPage, text, &checkBox);


	strcat(text,"<br><br>\n");
	memset(buffer,0,MAXSTRLEN);
	sprintf(buffer,"<div class=\"expandcontent\" id=\"%s\">",expandID);
	strcat(text,buffer);
	_add_osd_text(currentPage,text,streamID,enabled);
	strcat(text,"</div></div>");
	return 0;
}

static int _add_osd (Page* currentPage, char* text, int streamID)
{
	char buffer[MAXSTRLEN] = {0};
	char name[NAME_LEN] = {0};
	sprintf(buffer,"<fieldset><legend>Stream %d</legend><br>\n",streamID);
	strcat(text,buffer);
	/*memset(buffer,0,MAXSTRLEN);
	sprintf(buffer,"onchange=\"addOSD('no_rotate', %d)\"",streamID);
	_get_name(name,streamID,"no_rotate");*/
	strcat(text,"&nbsp; &nbsp;");

	checkBox_Input checkBox;

	/*
	* No Rotate not required
	*/
#if 0
	/*memset(&checkBox, 0, sizeof(checkBox_Input));
	checkBox.label = "No Rotate";
	checkBox.name = name;
	checkBox.value = osd_params[_get_osd_Index(streamID, "no_rotate")].value;
	checkBox.action = buffer;
	(&virtual_PageOps)->create_checkbox(currentPage, text, &checkBox);
	strcat(text,"&nbsp; &nbsp;");*/
#endif

	memset(buffer,0,MAXSTRLEN);
	sprintf(buffer,"onchange=\"addOSD('bmp', %d)\"",streamID);
	memset(name,0,NAME_LEN);
	_get_name(name,streamID,"bmp_enable");

	memset(&checkBox, 0, sizeof(checkBox_Input));
	checkBox.label = "Add BMP (8-bits, < 80K)";
	checkBox.name = name;
	checkBox.value = osd_params[_get_osd_Index(streamID, "bmp_enable")].value;
	checkBox.action = buffer;
	(&virtual_PageOps)->create_checkbox(currentPage, text, &checkBox);
	strcat(text,"&nbsp; &nbsp;");

	memset(buffer,0,MAXSTRLEN);
	sprintf(buffer,"onchange=\"addOSD('time', %d)\"",streamID);
	memset(name,0,NAME_LEN);
	_get_name(name,streamID,"time_enable");
	memset(&checkBox, 0, sizeof(checkBox_Input));
	checkBox.label = "Add current time";
	checkBox.name = name;
	checkBox.value = osd_params[_get_osd_Index(streamID, "time_enable")].value;
	checkBox.action = buffer;
	(&virtual_PageOps)->create_checkbox(currentPage, text, &checkBox);
	strcat(text,"<br><br>");

	_add_expand_osd_text(currentPage,text,streamID);
	strcat(text,"<br></fieldset><br>\n");
	return 0;
}

int AmbaOSDPage_init (Page* currentPage)
{
	extern Page ConfigPage;
	memset(currentPage,0,sizeof(Page));

	AmbaConfig_init(&ConfigPage);
	*currentPage = ConfigPage;

	currentPage->superPage = &ConfigPage;
	currentPage->activeMenu = Camera_Setting;
	currentPage->activeSubMenu = OSD_Setting;
	strcat(currentPage->name,"osd");

	memset(&OSDPage_Ops,0,sizeof(Page_Ops));
	currentPage->page_ops = &OSDPage_Ops;

	(&OSDPage_Ops)->get_params = AmbaOSDPage_get_params;
	(&OSDPage_Ops)->add_params = AmbaOSDPage_add_params;

	_create_params();
	return 0;
}

int AmbaOSDPage_get_params (Page* currentPage)
{
	section_Param section_param;
	int ret= (&virtual_PageOps)->process_PostData(currentPage);
	if (ret < 0) {
		fprintf(stdout,"1:set params failed");
	} else {
		if (ret == 0) {
			fprintf(stdout,"0:set params succeeded");
		} else {
			if (ret == 1) {
				memset(&section_param, 0, sizeof(section_Param));
				section_param.sectionName = "OSD";
				section_param.sectionPort = OSD;
				section_param.paramData = osd_params;
				section_param.extroInfo = "";
				section_param.paramDataNum =  OSD_PARAM_NUM;
				if ((&virtual_PageOps)->get_section_param(currentPage, &section_param) == -1) {
					return -1;
				}
				return 1;
			} else {
				fprintf(stdout,"1:unexpected error");
			}
		}
	}

	return 0;
}


int AmbaOSDPage_add_params (Page* currentPage, char* text)
{
	int i;
	for(i = 0; i < STREAM_NUM; i++) {
		_add_osd(currentPage,text,i);
	}
	strcat(text,"<p align=\"center\" >\n");
	strcat(text,"<input type=\"button\" value=\"Apply\" onclick = \"javascript:setOSD()\"/>&nbsp; &nbsp; \n");
	strcat(text,"<input type=\"button\" value=\"Cancel\" onclick = \"javascript:showPage('osd')\"/>\n");
	strcat(text,"'</p>\n");
	return 0;
}

