
#ifndef AMBAVIDEOPAGE_H

#define AMBAVIDEOPAGE_H

Page VideoPage;
Page_Ops VideoPage_Ops;


int AmbaVideo_init (Page* currentPage, int browser);
int AmbaVideo_create_button (Page* currentPage, char* text, button_Input* button);
int AmbaVideo_create_select_label (Page* currentPage, char* text, select_Label* select_label);
int AmbaVideo_create_text_entry (Page* currentPage, char* text, text_Entry* text_entry);
int AmbaVideo_add_settings (Page* currentPage, char* text);
int AmbaVideo_add_controls( Page* currentPage, char* text);
int AmbaVideo_add_css_style (Page* currentPage, char* text);
int AmbaVideo_add_body_JS (Page* currentPage, char* text);
int AmbaVideo_process_PostData (Page* currentPage);
int AmbaVideo_add_content (Page* currentPage, char* text);



#endif

