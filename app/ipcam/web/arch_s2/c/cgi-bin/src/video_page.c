

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "utils.h"
#include "page.h"
#include "base_page.h"
#include "video_page.h"


static char* const contentHtml = "<div id=\"sitename\" class=\"setting\">\n\
			<div id=\"title\"></div>\n\
		</div>\n\
		<div id=\"wrap\">\n\
			<div id=\"content\">%s\n\
			%s</div>\n\
			<div class=\"clearingdiv\">&nbsp; </div>\n\
		</div>\n";


static char* const cssStyle = "#container, #top { width:1280px; }\n\
	#content { margin:0px; border-left:0px; border-right:0px; padding-left:0px; padding-right:0px }\n\
	#display { width:100%; height:780px; align:center; padding:0px; }\n\
	#video {  margin:10px 0px 0px 0px; align:center;padding:0px; width:1280px; height:760px;\
	position:absolute;display:block;overflow:auto}\n\
	#action { margin:10px; width:100%; height:60px; align:left; }\n\
	.nav, .but { margin:3px; padding:5px; width:60px; vertical-align:middle; text-align:center;\
	display:inline; text-transform:capitalize; }\n\
	.but { width:auto; font-weight:bold; text-decoration:none; position:relative; }\n\
	.textbox { width:10%; text-align:center; }\n\
	.textinput { width:40px; }\n\
	.right { float:right; }\n";

static char* const cssStyleforIE = "#container, #top { width:1280px; }\n\
	#content { margin:0px; border-left:0px; border-right:0px; padding-left:0px; padding-right:0px }\n\
	#display { width:100%; height:780px; align:center; padding:0px; }\n\
	#videoforIE { margin:10px 0px 0px 0px;align:center;padding:0px;width:1280px;height:760px;\
	position:absolute;display:block}\n\
	#action { margin:10px; width:100%; height:60px; align:left; }\n\
	.nav, .but { margin:3px; padding:5px; width:60px; vertical-align:middle; text-align:center;\
	display:inline; text-transform:capitalize; }\n\
	.but { width:auto; font-weight:bold; text-decoration:none; position:relative; }\n\
	.textbox { width:10%; text-align:center; }\n\
	.textinput { width:40px; }\n\
	.right { float:right; }\n";

static char* const liveHtml="%s\n\
	<div id=\"display\">\n\
		<div id=\"video\">\n\
			<OBJECT CLASSID=\"clsid:9BE31822-FDAD-461B-AD51-BE1D1C159921\" WIDTH=%d \
			HEIGHT=%d style=\"margin:%dpx %dpx %dpx %dpx\" align=\"absmiddle\" ID=\"vlc\"\
			events=\"True\">\n\
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
				width=\"%d\"\n\
				height=\"%d\"\n\
				style=\"margin:%d 0 %d 0\"\n\
				toolbar=\"true\"\n\
				text=\"Waiting for video\"\n\
				name=\"vlc\">\n\
			</EMBED>\n\
			</OBJECT>\n\
		</div>\n\
	</div>\n";

static char* const liveHtmlforIE = "%s\n\
	<div id=\"display\">\n\
		<div id=\"videoforIE\">\n\
			<OBJECT CLASSID=\"CLSID:3BCDAA6A-7306-42FF-B8CF-BE5D3534C1E4\" \
			codebase=\"http://%s/activeX/ambaWeb.cab#version=%s\" WIDTH=1280 HEIGHT=760\
			align=\"absmiddle\" ID=\"AmbaIPCmrWebPlugIn1\">\n\
				<PARAM NAME=\"_Version\" VALUE=\"65536\">\n\
				<PARAM NAME=\"_ExtentX\" VALUE=\"19045\">\n\
				<PARAM NAME=\"_ExtentY\" VALUE=\"11478\">\n\
				<PARAM NAME=\"_StockProps\" VALUE=\"0\">\n\
			</OBJECT>\n\
		</div>\n\
	</div>\n";

static char* const actionHtml = "\n\
	<div id=\"action\">%s\n\
	</div>\n";

extern int videoWidth;
extern int videoHeight;


extern char postInfo[];
extern Page_Ops virtual_PageOps;



#define MAXSTRLEN 256
#define SHORTLEN 16
#define VMAX_WIDTH 1280
#define VMAX_HEIGHT 720
#define DISPLAY_MAX_HEIGHT 760
#define MIN_MARGIN ((760-720)/2)

int AmbaVideo_init (Page* currentPage, int browser)
{
	extern Page basePage;
	memset(currentPage, 0, sizeof(Page));

	AmbaBase_init(&basePage);				// video page's super is base page
	*currentPage = basePage;

	(*currentPage).superPage = &basePage;
	strcat(currentPage->title, "Ambarella IPCAM Live View");
	currentPage->agent = browser;

	memset(&VideoPage_Ops, 0, sizeof(Page_Ops));
	currentPage->page_ops = &VideoPage_Ops;

	(&VideoPage_Ops)->create_button = AmbaVideo_create_button;
	(&VideoPage_Ops)->create_select_label = AmbaVideo_create_select_label;
	(&VideoPage_Ops)->create_text_entry = AmbaVideo_create_text_entry;
	(&VideoPage_Ops)->add_settings = AmbaVideo_add_settings;
	(&VideoPage_Ops)->add_controls = AmbaVideo_add_controls;
	(&VideoPage_Ops)->add_css_style = AmbaVideo_add_css_style;
	(&VideoPage_Ops)->add_body_JS = AmbaVideo_add_body_JS;
	(&VideoPage_Ops)->process_PostData = AmbaVideo_process_PostData;
	(&VideoPage_Ops)->add_content = AmbaVideo_add_content;
	return 0;
}


int AmbaVideo_create_button (Page* currentPage, char * text, button_Input* button)
{
	char* string_format = "		<input class=\"but\" id=\"%s\" type=\"button\" value=\"%s\"\
							style=\" visibility:%s\" onclick=\"javascript:%s\"/>\n";
	char text_buffer[MAXSTRLEN] = {0};
	sprintf(text_buffer, string_format, \
		button->name, button->value, button->exprop, button->action);
	strncat(text, text_buffer, strlen(text_buffer));
	return 0;
}


int AmbaVideo_create_select_label (Page* currentPage, char * text, select_Label* select_label)
{
	char* label_string_format = "<label for=\"%s\">%s</label><select id=\"%s\" onkeydown=\
							\"javascript:%s\">\n";
	char label_string[MAXSTRLEN] = {0};
	sprintf(label_string, label_string_format, \
		select_label->name, select_label->label, \
		select_label->name, select_label->action);
	strncat(text, label_string, strlen(label_string));

	int len = select_label->option_len;
	char* option_string_format = "<option value=%d%s>%s</option>\n";
	char option_string[MAXSTRLEN] = {0};
	int i;

	for (i = 0; i < len; i++) {
		if ((select_label->options + i)->value == select_label->value) {
			if (option_string[0] != 0) {
				memset(option_string, 0, sizeof(option_string));
			}
			sprintf(option_string, option_string_format, \
				(select_label->options + i)->value, " selected", \
				(select_label->options + i)->option);
			strncat(text, option_string, strlen(option_string));
			} else {
				if (option_string[0] != 0) {
				memset(option_string, 0, sizeof(option_string));
				}
				sprintf(option_string, option_string_format, \
					(select_label->options + i)->value, " ", \
					(select_label->options + i)->option);
				strncat(text, option_string, strlen(option_string));
		}
	}
	char* label_string_end = "</select>\n";
	strncat(text, label_string_end, strlen(label_string_end));
	return 0;
}


int AmbaVideo_create_text_entry (Page* currentPage, char * text, text_Entry* text_entry)
{
	char* string_format = "&nbsp; <label for=\"%s\">%s</label>\n\
		<input type=\"text\" class=\"textinput\" id=\"%s\" value=\"%d\" \
		maxlength=%d onkeypress=\"javascript:NumOnly()\" onkeydown=\"javascript:%s\" />\n";

	char text_buffer[MAXSTRLEN] = {0};
	sprintf(text_buffer, string_format, \
		text_entry->name, text_entry->label, text_entry->name, \
		text_entry->value, text_entry->maxlen, text_entry->ro);
	strncat(text, text_buffer, strlen(text_buffer));
	return 0;
}

int AmbaVideo_add_settings (Page* currentPage,char* text)
{
	return 0;
}

int AmbaVideo_add_controls (Page* currentPage,char* text)
{
	return 0;
}

int AmbaVideo_add_css_style (Page* currentPage,char* text)
{
	if (currentPage->agent == MSIE) {
		strncat(text, cssStyleforIE, strlen(cssStyleforIE));
	} else {
		strncat(text, cssStyle, strlen(cssStyle));
	}
	return 0;
}

int AmbaVideo_add_body_JS (Page* currentPage,char* text)
{
	return 0;
}


int AmbaVideo_process_PostData (Page* currentPage)
{
	int ret = 0;
	AmbaTransfer transfer;
	transfer_init(&transfer);
	int i = 0;
	int req = 0;
	int info = 0;
	int data = 0;
	char buffer[SHORTLEN] = {0};

	if (strstr(postInfo, "req_cnt") != NULL) {
		//req_cnt = 1;
		memset(buffer, 0, SHORTLEN);
		sprintf(buffer, "info%d", i);
		info = get_value(postInfo, buffer);

		memset(buffer, 0, SHORTLEN);
		sprintf(buffer, "data%d", i);
		data = get_value(postInfo, buffer);

		if (info || data) {
			if (strstr(postInfo, "REQ_SET_FORCEIDR") != NULL) {
				req = REQ_SET_FORCEIDR;
			}else if (strstr(postInfo, "REQ_CHANGE_BR") != NULL) {
				req = REQ_CHANGE_BR;
			}else if (strstr(postInfo, "REQ_CHANGE_FR") != NULL) {
				req = REQ_CHANGE_FR;
			}

			ret = transfer.send_fly_request(req, info, data);
			return ret;
		} else {
			return -1;
		}
	}
	return 1;
}

int AmbaVideo_add_content (Page* currentPage,char* text)
{
	char* settingText = NULL;
	settingText = (char *)malloc(SETTING_CONTENT_LEN);
	if (settingText == NULL) {
		LOG_MESSG("LiveView  add content error");
		return -1;
	}
	memset(settingText, 0, SETTING_CONTENT_LEN);

	char* controlText = NULL;
	controlText = (char *)malloc(SETTING_CONTENT_LEN);
	if (controlText == NULL) {
		LOG_MESSG("LiveView  add content error");
		return -1;
	}
	memset(controlText, 0, SETTING_CONTENT_LEN);


	char* LiveText = NULL;
	LiveText = (char *)malloc(PAGE_CONTENT_LEN);
	if (LiveText == NULL) {
		LOG_MESSG("LiveView add controls error");
		return -1;
	}
	memset(LiveText, 0, PAGE_CONTENT_LEN);

	if (currentPage->agent == MSIE) {
		(&virtual_PageOps)->add_settings(currentPage, settingText);
		sprintf(LiveText, liveHtmlforIE, settingText,\
			currentPage->host, currentPage->version);
	} else {
		(&virtual_PageOps)->add_settings(currentPage, settingText);
		if ( (videoWidth <= VMAX_WIDTH) & (videoHeight <= VMAX_HEIGHT)) {
			sprintf(LiveText, liveHtml, settingText,videoWidth, videoHeight, \
			(DISPLAY_MAX_HEIGHT-videoHeight)/2, (VMAX_WIDTH-videoWidth)/2, \
			(DISPLAY_MAX_HEIGHT-videoHeight)/2, (VMAX_WIDTH-videoWidth)/2,\
				videoWidth, videoHeight, (DISPLAY_MAX_HEIGHT-videoHeight)/2, \
				(DISPLAY_MAX_HEIGHT-videoHeight)/2);
		} else  {
			sprintf(LiveText, liveHtml, settingText, VMAX_WIDTH, VMAX_HEIGHT, \
				MIN_MARGIN, 0, MIN_MARGIN, 0, VMAX_WIDTH, VMAX_HEIGHT, MIN_MARGIN, MIN_MARGIN);
		}
	}

	memset(settingText, 0, SETTING_CONTENT_LEN);
	if ((&virtual_PageOps)->add_controls(currentPage, settingText) < 0) {
			return -1;
	}
	sprintf(controlText, actionHtml, settingText);

	char* contentText = NULL;
	contentText = (char *)malloc(PAGE_CONTENT_EXT_LEN);
	if (contentText == NULL) {
		LOG_MESSG("LiveView add controls error");
		return -1;
	}
	memset(contentText, 0, PAGE_CONTENT_EXT_LEN);
	sprintf(contentText, contentHtml, LiveText, controlText);
	strncat(text, contentText, strlen(contentText));

	free(settingText);
	settingText = NULL;
	free(LiveText);
	LiveText = NULL;
	free(LiveText);
	LiveText = NULL;
	free(contentText);
	contentText = NULL;
	return 0;
}

