

#include <stdio.h>
#include <string.h>

#include "params.h"
#include "utils.h"
#include "page.h"
#include "config_page.h"
#include "pm_page.h"

#define MAXSTRLEN 1024
#define SHORTLEN 128

static char* const videoHtml = "<div id=\"video\">\n\
	<OBJECT CLASSID=\"clsid:9BE31822-FDAD-461B-AD51-BE1D1C159921\" align=\"absmiddle\" \
	ID=\"vlc\" events=\"True\" style=\"width:510px;height:300px;position:absolute;display:block\">\n\
			<param name=\"MRL\" value=\"\" >\n\
			<param name=\"ShowDisplay\" value=\"True\" >\n\
			<param name=\"AutoLoop\" value=\"False\" >\n\
			<param name=\"AutoPlay\" value=\"False\" >\n\
			<param name=\"Volume\" value=\"50\" >\n\
			<param name=\"toolbar\" value=\"true\" >\n\
			<param name=\"StartTime\" value=\"0\" >\n\
			<EMBED pluginspage=\"http://www.videolan.org\"\n\
				type=\"application/x-vlc-plugin\"\n\
				version=\"VideoLAN.VLCPlugin.2\"\n\
				width=\"510\"\n\
				height=\"300\"\n\
				toolbar=\"true\"\n\
				text=\"Waiting for video\"\n\
				name=\"vlc\">\n\
			</EMBED>\n\
		</OBJECT>\n\
	</div>\n";

static char* const videoHtmlforIE = "<div id=\"videoforIE\">\n\
		<OBJECT CLASSID=\"CLSID:3BCDAA6A-7306-42FF-B8CF-BE5D3534C1E4\" \
		codebase=\"http://%s/activeX/ambaWeb.cab#version=%s\" align=\"absmiddle\" \
		ID=\"AmbaIPCmrWebPlugIn1\" style=\"width:510px;height:300px;position:absolute;\
		display:block\">\n\
			<PARAM NAME=\"_Version\" VALUE=\"65536\">\n\
			<PARAM NAME=\"_ExtentX\" VALUE=\"19045\">\n\
			<PARAM NAME=\"_ExtentY\" VALUE=\"11478\">\n\
			<PARAM NAME=\"_StockProps\" VALUE=\"0\">\n\
		</OBJECT>\n\
	</div>\n";


typedef enum
{
	PM_LEFT,
	PM_TOP,
	PM_W,
	PM_H,
	PM_COLOR,
	PM_ACTION,
	PM_PARAM_NUM
}PM_PARAMS;

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

#define MAX_OPTION_LEN 16
static ParamData pm_params[PM_PARAM_NUM];
static Label_Option options[MAX_OPTION_LEN];

static int _add_canvas (Page* currentPage, char* text)
{
	strcat(text,"<p align=\"center\" class=\"style1\">Main Encode Source Buffer</p>\n");
	strcat(text,"<div id=\"pm_canvas\" style=\"clear: both; position:relative; width:96%; \
				height:300px; margin:0 auto\">\n");
	char string[MAXSTRLEN] = {0};
	strcat(text,videoHtml);
	sprintf(string,videoHtmlforIE,currentPage->host,currentPage->version);
	strcat(text,string);
	strcat(text,"<div id=\"pm_mask\">\n</div>\n");
	strcat(text,"</div>");
	return 0;
}

static int _add_mask (Page* currentPage, char* text)
{
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

	strcat(text,"<br><br>\n");

	select_Label select_label;
	memset(&select_label, 0, sizeof(select_Label));
	select_label.label = "Privacy mask color :";
	select_label.name = "pm_color";
	select_label.options = options;
	select_label.option_len = COLOR_OPS_NUM;
	select_label.value = pm_params[PM_COLOR].value;
	select_label.action = "";
	(&virtual_PageOps)->create_select_label(currentPage, text, &select_label);
	strcat(text,"<br><br><label>Privacy mask rectangle : </label><br><br>\n");

	text_Entry text_entry;
	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Left (0 ~ 100%) :";
	text_entry.name = "pm_left";
	text_entry.value = pm_params[PM_LEFT].value;
	text_entry.maxlen = 2;
	text_entry.ro = "";
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"&nbsp; &nbsp; \n");

	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Top (0 ~ 100%) :";
	text_entry.name = "pm_top";
	text_entry.value = pm_params[PM_TOP].value;
	text_entry.maxlen = 2;
	text_entry.ro = "";
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br>\n");

	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Width (0 ~ 100%) :";
	text_entry.name = "pm_w";
	text_entry.value = pm_params[PM_W].value;
	text_entry.maxlen = 2;
	text_entry.ro = "";
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"&nbsp; &nbsp; \n");

	memset(&text_entry, 0, sizeof(text_Entry));
	text_entry.label = "Height (0 ~ 100%) :";
	text_entry.name = "pm_h";
	text_entry.value = pm_params[PM_H].value;
	text_entry.maxlen = 2;
	text_entry.ro = "";
	(&virtual_PageOps)->create_text_entry(currentPage, text, &text_entry);
	strcat(text,"<br><br>\n");

	return 0;
}

static int _create_params ()
{
	memset(pm_params,0,sizeof(ParamData)*PM_PARAM_NUM);
	strcat(pm_params[PM_LEFT].param_name, "pm_left");
	pm_params[PM_LEFT].value = 0;

	strcat(pm_params[PM_TOP].param_name, "pm_top");
	pm_params[PM_TOP].value = 0;

	strcat(pm_params[PM_W].param_name, "pm_w");
	pm_params[PM_W].value = 0;

	strcat(pm_params[PM_H].param_name, "pm_h");
	pm_params[PM_H].value = 0;

	strcat(pm_params[PM_COLOR].param_name, "pm_color");
	pm_params[PM_COLOR].value = 0;

	strcat(pm_params[PM_ACTION].param_name, "pm_action");
	pm_params[PM_ACTION].value = 0;
	return 0;
}


int AmbaPMPage_init (Page * currentPage)
{
	extern Page ConfigPage;
	memset(currentPage,0,sizeof(Page));

	AmbaConfig_init(&ConfigPage);
	*currentPage = ConfigPage;

	currentPage->superPage = &ConfigPage;
	currentPage->activeMenu = Camera_Setting;
	currentPage->activeSubMenu = Privacy_Mask;
	strcat(currentPage->name,"pm");

	memset(&PMPage_Ops,0,sizeof(Page_Ops));
	currentPage->page_ops = &PMPage_Ops;

	(&PMPage_Ops)->get_params = AmbaPMPage_get_params;
	(&PMPage_Ops)->add_params = AmbaPMPage_add_params;
	(&PMPage_Ops)->add_css_style = AmbaPMPage_add_css_style;
	(&PMPage_Ops)->add_body_JS = AmbaPMPage_add_body_JS;

	currentPage->streamId = 0;
	_create_params();
	return 0;
}


int AmbaPMPage_add_css_style (Page * currentPage, char * text)
{
	strcat(text,"\n\n");
	(PMPage.superPage)->page_ops->add_css_style(currentPage,text);
	strcat(text,"\n#content img { border: 1px solid #000000; }");
	return 0;
}


int AmbaPMPage_add_body_JS (Page * currentPage, char * text)
{
	char string[SHORTLEN] = {0};
	sprintf(string," onload=\"javascript: OnLoadActiveX('%s', %d, %d, %d, %d);\" ",\
		currentPage->host,currentPage->streamId,1,0,0);
	strcat(text,string);
	return 0;
}

int AmbaPMPage_get_params (Page * currentPage)
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
				section_param.sectionName = "PRIMASK";
				section_param.sectionPort = PRIMASK;
				section_param.paramData = pm_params;
				section_param.extroInfo = "";
				section_param.paramDataNum = PM_PARAM_NUM;
				if ((&virtual_PageOps)->get_section_param(currentPage, &section_param) == -1) {
					return -1;
				}
				return 1;
			} else {
				fprintf(stdout,"1:unexpected error %d",ret);
			}
		}
	}
	return 0;
}
int AmbaPMPage_add_params (Page* currentPage, char* text)
{
	strcat(text,"<fieldset><legend>Privacy Mask</legend><br>\n");
	_add_canvas(currentPage, text);
	_add_mask(currentPage, text);
	strcat(text,"<p align=\"center\" >\n");
	strcat(text,"<input type=\"button\" value=\"Add include\" onclick = \"\
					javascript:addRemovePrivacyMask(0)\" />&nbsp; &nbsp; \n");
	strcat(text,"<input type=\"button\" value=\"Add exclude\" onclick = \"\
						javascript:addRemovePrivacyMask(1)\"/>&nbsp; &nbsp; \n");
	strcat(text,"<input type=\"button\" value=\"Remove\" onclick = \"\
				javascript:clearPrivacyMask()\" />\n");
	strcat(text,"</p>\n");
	strcat(text,"</fieldset><br>\n");
	return 0;
}
