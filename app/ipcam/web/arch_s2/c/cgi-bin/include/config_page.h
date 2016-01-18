

#ifndef AMBACONFIGPAGE_H
#define AMBACONFIGPAGE_H

typedef enum
{
	Camera_Setting = 0,
	Image_Quality,
	System_Setting,
	MENU_NUM
}menuMap_Index;

typedef enum
{
	Encode_Setting = 0,
	Privacy_Mask,
	OSD_Setting,
	VinVout,
	CAM_SUBMENU_NUM
}Cam_subMenu_Index;

typedef enum
{
	Basic = 0,
	IMG_SUBMENU_NUM
}Img_subMenu_Index;

typedef enum
{
	Upgrade = 0,
	SYS_SUBMENU_NUM
}Sys_subMenu_Index;

Page ConfigPage;
Page_Ops ConfigPage_Ops;



int AmbaConfig_init (Page* currentPage);
int AmbaConfig_add_params (Page* currentPage, char* text);
int AmbaConfig_show_params (Page* currentPage);
int AmbaConfig_create_slider_bar (Page* currentPage, char* text, slider_Bar* slider_bar);
int AmbaConfig_process_PostData (Page* currentPage);
int AmbaConfig_add_css_style (Page* currentPage, char* text);
int AmbaConfig_add_body_JS (Page* currentPage, char* text);
int AmbaConfig_add_content (Page* currentPage, char* text);

#endif
