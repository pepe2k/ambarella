
#ifndef AMBABASEPAGE_H

#define AMBABASEPAGE_H



Page basePage;
Page_Ops basePage_Ops;



int AmbaBase_init (Page* currentPage);
int AmbaBase_create_em_text (Page* currentPage, char* text, em_Text* em_text);
int AmbaBase_create_select_label (Page* currentPage, char* text, select_Label* select_label);
int AmbaBase_create_input_entry (Page* currentPage, char* text, input_Entry* input_entry);
int AmbaBase_create_text_entry (Page* currentPage, char* text, text_Entry* text_entry);
int AmbaBase_create_wide_text_entry (Page* currentPage, char* text, wide_text_Entry* wide_text_entry);
int AmbaBase_create_checkbox (Page* currentPage, char* text, checkBox_Input* checkBox);
int AmbaBase_create_radio_input (Page* currentPage, char* text, radio_Input* radio_input);
int AmbaBase_get_section_param (Page* currentPage, section_Param* section_param);
int AmbaBase_get_params (Page* currentPage);
int AmbaBase_add_css_style (Page* currentPage, char* text);
int AmbaBase_add_body_JS (Page* currentPage, char* text);
int AmbaBase_add_content (Page* currentPage, char* text);
int AmbaBase_print_HTML (Page* currentPage, char* HTML);
int get_value (char * text, char * cmp_Text);


#endif




