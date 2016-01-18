

#ifndef AMBAPMPAGE_H
#define AMBAPMPAGE_H


Page PMPage;
Page_Ops PMPage_Ops;


int AmbaPMPage_init (Page* currentPage);
int AmbaPMPage_add_css_style (Page* currentPage, char* text);
int AmbaPMPage_get_params (Page* currentPage);
int AmbaPMPage_add_params (Page* currentPage, char* text);
int AmbaPMPage_add_body_JS (Page* currentPage, char* text);


#endif
