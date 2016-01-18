#ifndef AMBASYSPAGE_H
#define AMBASYSPAGE_H


Page SysPage;
Page_Ops SysPage_Ops;


int AmbaSysPage_init (Page* currentPage);
int AmbaSysPage_add_params(Page* currentPage, char* text);
int AmbaSysPage_process_PostData (Page* currentPage);
int AmbaSysPage_get_params (Page* currentPage);
int AmbaSysPage_print_HTML (Page* currentPage, char* HTML) ;


#endif