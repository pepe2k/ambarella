#ifndef PAGE_H
#define PAGE_H


#define PAGE_NAME_LEN 12
#define PAGE_TITLE_LEN 32
#define PAGE_CSS_LEN 1024
#define PAGE_BODY_JS_LEN 64
#define SETTING_CONTENT_LEN 10240
#define PAGE_CONTENT_LEN 20480	// 20 * 1024
#define PAGE_CONTENT_EXT_LEN 40960
#define VERSION_LEN 32
#define HOST_LEN 32
#define PAGE_URL_LEN 32

#define ARR_SIZE(a) (sizeof((a))/sizeof((a[0])))
#define LOG_MESSG(MSG) do { \
	FILE* LOG; \
	LOG = fopen("debug","a+"); \
	fprintf(LOG,"%s\n", MSG); \
	fclose(LOG); } \
while(0)



#define LABEL_OPTION_LEN 32
typedef struct {
	unsigned int value;
	char option[LABEL_OPTION_LEN];
}Label_Option;

typedef struct page {
	struct page* superPage;
	struct page_Ops* page_ops;
	char name[PAGE_NAME_LEN];
	char title[PAGE_TITLE_LEN];
	char version[VERSION_LEN];
	char host[HOST_LEN];
	int agent;
	char pageURL[PAGE_URL_LEN];
	unsigned int streamId;
	unsigned int statSize;
	unsigned int recvType;
	int activeMenu;
	int activeSubMenu;
}Page;

typedef enum {
	MSIE,
	OTHER_BROWSER
}BROWSER_TYPE;

typedef enum {
	POST,
	GET
}METHODS;

typedef struct {
	char* content;
	char* style;
	char* name;
}em_Text;

typedef struct {
	char* label;
	char* name;
	Label_Option* options;
	int option_len;
	unsigned int value;
	char* action;
}select_Label;

typedef struct {
	char* label;
	char* name;
	int value;
	int maxlen;
	char* action;
}input_Entry;

typedef struct {
	char* label;
	char* name;
	int value;
	int maxlen;
	char* ro;
}text_Entry;

typedef struct {
	char* label;
	char* name;
	char* value;
	int maxlen;
	char* ro;
}wide_text_Entry;

typedef struct {
	char* label;
	char* name;
	int value;
	char* action;
}checkBox_Input;

typedef struct {
	char* label;
	char* name;
	Label_Option* options;
	int options_len;
	int value;
	char* action;
}radio_Input;


typedef struct {
	char* name;
	char* value;
	char* exprop;
	char* action;
}button_Input;

typedef struct {
	char* label;
	char* name;
	int value;
	int RangeMax;
	int RangeMin;
	char* outBox;
}slider_Bar;

typedef struct page_Ops {
	int (*create_em_text) (Page*, char*, em_Text*);
	int (*create_select_label) (Page*, char*, select_Label*);
	int (*create_input_entry) (Page*, char*, input_Entry*);
	int (*create_text_entry) (Page*, char*, text_Entry*);
	int (*create_wide_text_entry)(Page*, char*, wide_text_Entry*);
	int (*create_checkbox) (Page*, char*, checkBox_Input*);
	int (*create_radio_input) (Page*, char*, radio_Input*);
	int (*get_section_param) (Page*, section_Param*);
	int (*get_params) (Page*);
	int (*add_css_style) (Page*, char*);
	int (*add_body_JS) (Page*, char*);
	int (*add_content) (Page*, char*);
	int (*print_HTML) (Page*, char*);
	int (*create_button) (Page*, char*, button_Input*);
	int (*add_settings) (Page*, char*);
	int (*add_controls) (Page*, char*);
	int (*process_PostData) (Page*);

	int (*add_params) (Page*, char*);
	int (*show_params) (Page*);
	int (*create_slider_bar) (Page*, char*, slider_Bar*);
}Page_Ops;






Page_Ops virtual_PageOps;

int virtual_init (Page_Ops* p);
int virtual_create_em_text (Page* currentPage, char* text, em_Text* em_text);
int virtual_create_select_label (Page* currentPage, char* text, select_Label* select_label);
int virtual_create_input_entry (Page* currentPage, char* text, input_Entry* input_entry);
int virtual_create_text_entry (Page* currentPage, char* text, text_Entry* text_entry);
int virtual_create_wide_text_entry (Page* currentPage, char* text, wide_text_Entry* wide_text_entry);
int virtual_create_checkbox (Page* currentPage, char* text, checkBox_Input* checkBox);
int virtual_create_radio_input (Page* currentPage, char* text, radio_Input* radio_input);
int virtual_get_section_param (Page* currentPage, section_Param* section_param);
int virtual_get_params (Page* currentPage);
int virtual_add_css_style (Page* currentPage, char* text);
int virtual_add_body_JS (Page* currentPage, char* text);
int virtual_add_content (Page* currentPage, char* text);
int virtual_print_HTML (Page* currentPage, char* text);


int virtual_create_button (Page* currentPage, char* text, button_Input* button);
int virtual_add_settings (Page* currentPage, char* text);
int virtual_add_controls (Page* currentPage, char* text);
int virtual_process_PostData (Page* currentPage);

int virtual_add_params (Page* currentPage, char* text);
int virtual_show_params (Page* currentPage);
int virtual_create_slider_bar (Page* currentPage, char* text, slider_Bar* slider_bar);








#endif
