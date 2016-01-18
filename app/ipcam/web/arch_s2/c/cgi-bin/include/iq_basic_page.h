
#ifndef AMBAIQBASIC_PAGE_H
#define AMBAIQBASIC_PAGE_H

Page IQBasicPage;
Page_Ops IQBasicPage_Ops;


int AmbaIQBasicPage_init (Page* currentPage);
int AmbaIQBasicPage_get_params (Page* currentPage);
int AmbaIQBasicPage_add_params (Page* currentPage, char* text);

#endif
