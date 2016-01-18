

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"
#include "page.h"
#include "base_page.h"
#include "config_page.h"


static char* const contentHtml = "<div id=\"sitename\" class=\"setting\">\n\
				<div id=\"title\"></div>\n\
			</div>\n\
			<div id=\"wrap\">\n\
				<div id=\"menunav\"><p>\n\
				%s</p>\n\
				<p>&nbsp; </p>\n\
				</div>\n\
				<div id=\"content\">\n\
					<div id=\"status\">&nbsp; </div>\n\
					%s\n\
				</div>\n\
				<div class=\"clearingdiv\">&nbsp; </div>\n\
			</div>";

static char* const cssStyle = "#container, #top { width:800px; }\n\
		#content { margin:0 20px 0 200px; }";



#define MAXSTRLEN 512
#define SHORTLEN 16

#define MAP_STRING_LEN 16
#define PAGE_KEY_LEN 32
#define CFG_PAGE_NUM 6
#define STREAM_NUM 4
#define MENU_LEN 2048

#define TRUE 1
#define FALSE 0

typedef struct
{
	char name[MAP_STRING_LEN];
	char** subMenu;
}menuMap_Node;

typedef struct
{
	unsigned int key;
	char name[MAP_STRING_LEN];
}pageMap_Node;

extern Page_Ops virtual_PageOps;


static char* Cam_SubMenu[CAM_SUBMENU_NUM] = {0};			//cam_submenu[0] = null;
static char* Img_SubMenu[IMG_SUBMENU_NUM] = {0};				//Img_submenu[0] = null
static char* Sys_SubMenu[SYS_SUBMENU_NUM] = {0};				//Sys_submenu[0] = null
 menuMap_Node menuMap[MENU_NUM];			//menuMap[0] = null
static pageMap_Node pageMap[CFG_PAGE_NUM];
static char* const streamName[] = {"Main","Secondary","Third","Fourth"};
static char* const SubMenuString[] = {"Encode Setting","Privacy Mask","OSD Setting",\
"VinVout","Basic","Upgrade"};
extern char postInfo[];



static int _init_menu_info() {
	Cam_SubMenu[Encode_Setting] = SubMenuString[0];
	Cam_SubMenu[Privacy_Mask] = SubMenuString[1];
	Cam_SubMenu[OSD_Setting] = SubMenuString[2];
	Cam_SubMenu[VinVout] = SubMenuString[3];

	Img_SubMenu[Basic] = SubMenuString[4];

	Sys_SubMenu[Upgrade] = SubMenuString[5];

	strcat(menuMap[Camera_Setting].name,"Camera Setting");
	menuMap[Camera_Setting].subMenu = Cam_SubMenu;

	strcat(menuMap[Image_Quality].name,"Image Quality");
	menuMap[Image_Quality].subMenu = Img_SubMenu;

	strcat(menuMap[System_Setting].name,"System Setting");
	menuMap[System_Setting].subMenu = Sys_SubMenu;

	return 0;
}

static int _init_page_info() {
	pageMap[0].key = ((Camera_Setting << 8) | Encode_Setting);
	strcat(pageMap[0].name,"enc");

	pageMap[1].key = ((Camera_Setting << 8) | Privacy_Mask);
	strcat(pageMap[1].name,"pm");

	pageMap[2].key = ((Camera_Setting << 8) | OSD_Setting);
	strcat(pageMap[2].name,"osd");

	pageMap[3].key = ((Camera_Setting << 8) | VinVout);
	strcat(pageMap[3].name,"csb");

	pageMap[4].key = ((Image_Quality  << 8) | Basic);
	strcat(pageMap[4].name,"iqb");

	pageMap[5].key = ((System_Setting  << 8) | Upgrade);
	strcat(pageMap[5].name,"sys");

	return 0;
}

static int _get_page_key (Page* currentPage, int menu_Index, int subMenu_Index) {
	if (subMenu_Index < 0) {
		return menu_Index << 8;
	} else {
		return ((menu_Index << 8) | subMenu_Index);
	}
}


static int _create_menu_item (Page* currentPage, char* text, char* page, char* menuName, char* status) {
	char* line = "				<a class=\"nav%s\" href=\"%s?page=%s\">%s</a><span class=\"hide\"> | </span>\n";
	char menuType[MAXSTRLEN];
	char text_buffer[MAXSTRLEN] = {0};
	memset(menuType, 0, MAXSTRLEN);
	if (strcmp(status,"") != 0) {
		sprintf(menuType," %s",status);
	} else {
		strcat(menuType,"");
	}
	sprintf(text_buffer, line, menuType, \
		currentPage->pageURL, page, menuName);
	strncat(text, text_buffer, strlen(text_buffer));
	return 0;
}

static int _create_sub_menu (Page* currentPage, char* subMenuString) {
	int i,j;
	int pageKey;
	char text[MAXSTRLEN] = {0};
	char status[SHORTLEN] = {0};
	char page[SHORTLEN] = {0};
	int subMenuNum = 0;
	int setting = 0;

	if (menuMap[currentPage->activeMenu].subMenu != NULL) {
		if (currentPage->activeMenu == Camera_Setting ) {
			subMenuNum = CAM_SUBMENU_NUM;
			setting = Camera_Setting;
		} else if (currentPage->activeMenu == Image_Quality) {
			subMenuNum = IMG_SUBMENU_NUM;
			setting = Image_Quality;
		} else if (currentPage->activeMenu == System_Setting) {
			subMenuNum = SYS_SUBMENU_NUM;
			setting = System_Setting;
		}
		if (subMenuNum > 0) {
			for (i = 0 ; i < subMenuNum; i++) {
				if (i == currentPage->activeSubMenu) {
					memset(status, 0, SHORTLEN);
					strcat(status, "sub active");
				} else {
					memset(status, 0, SHORTLEN);
					strcat(status, "sub");
				}
				pageKey = _get_page_key(currentPage,currentPage->activeMenu,i);
				for (j = 0; j < CFG_PAGE_NUM; j++) {
					if (pageKey == pageMap[j].key) {
						memset(page, 0, SHORTLEN);
						strncat(page, pageMap[j].name, strlen(pageMap[j].name));
					}
				}
				memset(text, 0, MAXSTRLEN);
				_create_menu_item(currentPage, text, page, \
					(menuMap[setting].subMenu)[i], status);
				strncat(subMenuString, text, strlen(text));
			}
		}
	}
	return 0;
}

static int _create_live_menu (Page* currentPage,char* menuString) {
	strcat(menuString,"<a class=\"nav\">Live View</a><span class=\"hide\"> | </span>\n");

	char* format = "				<a class=\"nav sub active\" href=\"/cgi-bin/webdemo.cgi?streamId=%d\">%s</a><span class=\"hide\">| </span>\n";
	char  text[MAXSTRLEN] = {0};

	int i;
	for (i = 0 ; i < STREAM_NUM; i++) {
		memset(text,0,MAXSTRLEN);
		sprintf(text, format, i, streamName[i]);
		strncat(menuString, text, strlen(text));
	}
	return 0;
}

static int _create_menu (Page* currentPage,char* menuString)
{
	_create_live_menu(currentPage,menuString);
	int index;
	int j;
	int isActive = FALSE;
	int sub;
	int pageKey;
	char page[SHORTLEN] = {0};
	char text[MAXSTRLEN] = {0};

	for (index = 0; index < MENU_NUM; index++) {
		if (index == currentPage->activeMenu) {
			isActive = TRUE;
		} else {
			isActive = FALSE;
		}

		if (menuMap[index].subMenu != NULL) {
			sub = 0;
		} else {
			sub = -1;
		}

		pageKey = _get_page_key(currentPage, index, sub);
		for (j = 0; j < CFG_PAGE_NUM; j++) {
			if (pageMap[j].key == pageKey) {
				memset(page, 0, SHORTLEN);
				strcat(page, pageMap[j].name);
				break;
			}
		}

		if (isActive) {
			memset(text, 0, MAXSTRLEN);
			_create_menu_item(currentPage, text, page,menuMap[index].name, "active");
			strncat(menuString, text, strlen(text));
			memset(text, 0, MAXSTRLEN);
			_create_sub_menu(currentPage, text);
			strncat(menuString, text, strlen(text));
		} else {

			memset(text, 0, MAXSTRLEN);
			_create_menu_item(currentPage, text, page, \
				menuMap[index].name, "");
			strncat(menuString, text, strlen(text));
		}

	}
	return 0;
}

int AmbaConfig_init (Page* currentPage) {
	extern Page basePage;
	memset(currentPage, 0, sizeof(Page));

	AmbaBase_init(&basePage);
	*currentPage = basePage;

	currentPage->superPage = &basePage;
	strcat(currentPage->title, "Ambarella IPCAM Setting");

	memset(&ConfigPage_Ops, 0, sizeof(Page_Ops));
	currentPage->page_ops = &ConfigPage_Ops;

	(&ConfigPage_Ops)->add_params = AmbaConfig_add_params;
	(&ConfigPage_Ops)->show_params = AmbaConfig_show_params;
	(&ConfigPage_Ops)->create_slider_bar = AmbaConfig_create_slider_bar;
	(&ConfigPage_Ops)->process_PostData = AmbaConfig_process_PostData;
	(&ConfigPage_Ops)->add_css_style = AmbaConfig_add_css_style;
	(&ConfigPage_Ops)->add_body_JS = AmbaConfig_add_body_JS;
	(&ConfigPage_Ops)->add_content = AmbaConfig_add_content;

	_init_menu_info();
	_init_page_info();

	return 0;
}


int AmbaConfig_add_params (Page* currentPage, char* text) {
	return 0;
}

int AmbaConfig_show_params (Page* currentPage) {
	return 0;
}

int AmbaConfig_create_slider_bar (Page* currentPage, char* text, slider_Bar* slider_bar) {
	int width = (slider_bar->RangeMax) - (slider_bar->RangeMin);
	int SLIDERWIDTH = 350;
	int left = ((slider_bar->value) - (slider_bar->RangeMin)) * SLIDERWIDTH /width;
	char buffer[MAXSTRLEN] = {0};
	char action[MAXSTRLEN] = {0};
	sprintf(buffer,"				<label>%s:</label>\n", slider_bar->label);
	strncat(text, buffer, strlen(buffer));
	memset(buffer, 0, MAXSTRLEN);
	sprintf(action, "<input type=\"text\" class=\"textinput\" id=\"%s\" value=\"%d\" maxlength=%d readonly=\"readonly\" />",\
		slider_bar->outBox, slider_bar->value, 4);
	sprintf(buffer, "				%s<br><br>\n", action);
	strncat(text, buffer, strlen(buffer));
	strcat(text,"					<div class=\"left barholder\"></div>\n");
	memset(buffer, 0, MAXSTRLEN);
	memset(action, 0, MAXSTRLEN);
	sprintf(action, "onclick=\"javascript:AddSubOneStep('%s', '%s', %d, %d, 1)\"", \
		slider_bar->name, slider_bar->outBox, width, slider_bar->RangeMin);
	sprintf(buffer, "					<div class=\"left\"><img src=\"../img/left.gif\" %s />%d&nbsp; </div>\n", \
		action, slider_bar->RangeMin);
	strncat(text, buffer, strlen(buffer));
	memset(buffer, 0, MAXSTRLEN);
	memset(action, 0, MAXSTRLEN);
	sprintf(action, "onmouseover=\"javascript:MoveBar('%s', '%s', %d, %d)\"", \
		slider_bar->name, slider_bar->outBox, width, slider_bar->RangeMin);
	strcat(text, "					<div class=\"left slider\">");
	sprintf(buffer, "<div class=\"bar\" id=\"%s\" %s style=\"left: %dpx; \"></div></div>\n", \
		slider_bar->name, action, left);
	strncat(text, buffer, strlen(buffer));
	memset(buffer, 0, MAXSTRLEN);
	memset(action, 0, MAXSTRLEN);
	sprintf(action, "onclick=\"javascript:AddSubOneStep('%s', '%s', %d, %d, 0)\"", \
		slider_bar->name, slider_bar->outBox, width, slider_bar->RangeMin);
	sprintf(buffer, "					<div class=\"left\">&nbsp; %d<img src=\"../img/right.gif\" %s /></div>\n",\
		slider_bar->RangeMax, action);
	strncat(text, buffer, strlen(buffer));

	return 0;

}

int AmbaConfig_process_PostData (Page* currentPage) {
	AmbaTransfer transfer;
	Message msg;
	transfer_init(&transfer);

	char* req = strstr(postInfo,"req_cnt");
	int port = PORT;
	int i = 0;
	int req_cnt = -1;

	if (req != NULL) {
		req_cnt = get_value(postInfo, "req_cnt");
		for(i = 0; i < req_cnt; i++) {
			if ((strstr(postInfo,"sec") != NULL)&&(strstr(postInfo,"data") != NULL)) {
				char* string_buffer = NULL;
				string_buffer = (char *)malloc(MSG_INFO_LEN);
				if (string_buffer == NULL) {
					LOG_MESSG("print Config error");
					return -1;
				}
				memset(string_buffer, 0, MSG_INFO_LEN);
				url_decode(string_buffer, postInfo, strlen(postInfo));
				memset(&msg, 0, sizeof(Message));
				parse_postSec(string_buffer,&msg ,i);
				parse_postData(string_buffer,&msg ,i);
				if (strcmp(msg.section_Name,"IMAGE") == 0) {
					port = IMAGE;
				}
				free(string_buffer);
				string_buffer = NULL;
				if ((transfer.send_set_request(REQ_SET_PARAM, port, msg) < 0)) {
					return -1;
				}
			} else {
				return -1;
			}
		}
		return 0;
	} else {
			return 1;
	}
}
int AmbaConfig_add_css_style (Page* currentPage, char* text) {
	strncat(text, cssStyle, strlen(cssStyle));
	return 0;
}
int AmbaConfig_add_body_JS (Page* currentPage, char* text) {
	char string[MAXSTRLEN] = {0};
	sprintf(string, " onload=\"javascript: getData('%s')\" ", currentPage->name);
	strncat(text, string, strlen(string));
	return 0;
}
int AmbaConfig_add_content (Page* currentPage, char* text) {
	char* menu = NULL;
	menu = (char *)malloc(MENU_LEN);
	if (menu == NULL) {
		LOG_MESSG("ConfigPage add content error");
		return -1;
	}
	memset(menu, 0, MENU_LEN);
	_create_menu(currentPage, menu);

	char* params = NULL;
	params = (char *)malloc(PAGE_CONTENT_LEN);
	if (params == NULL) {
		LOG_MESSG("ConfigPage add content error");
		return -1;
	}
	memset(params, 0, PAGE_CONTENT_LEN);
	(&virtual_PageOps)->add_params(currentPage, params);

	char* content = NULL;
	content = (char *)malloc(PAGE_CONTENT_LEN);
	if (content == NULL) {
		LOG_MESSG("ConfigPage add content error");
		return -1;
	}
	memset(content, 0, PAGE_CONTENT_LEN);
	sprintf(content, contentHtml, menu, params);
	strncat(text, content, strlen(content));
	free(menu);
	menu = NULL;
	free(params);
	params = NULL;
	free(content);
	content = NULL;
	return 0;
}
