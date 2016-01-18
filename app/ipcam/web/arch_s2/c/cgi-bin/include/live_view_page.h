

#ifndef AMBALIVEVIEWPAGE_H
#define AMBALIVEVIEWPAGE_H


Page LiveViewPage;
Page_Ops LiveViewPage_Ops;



int AmbaLiveView_init (Page* currentPage, int streamID, int browser);
int AmbaLiveView_get_params (Page* currentPage);
int AmbaLiveView_add_settings (Page* currentPage, char* text);
int AmbaLiveView_add_controls (Page* currentPage, char* text);
int AmbaLiveView_add_body_JS (Page* currentPage, char* text);


#endif
