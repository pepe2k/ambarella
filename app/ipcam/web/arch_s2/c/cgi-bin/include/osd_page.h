

#ifndef AMBAOSDPAGE_H
#define AMBAOSDPAGE_H

Page OSDPage;
Page_Ops OSDPage_Ops;


int AmbaOSDPage_init (Page* currentPage);
int AmbaOSDPage_get_params (Page* currentPage);
int AmbaOSDPage_add_params (Page* currentPage, char* text);



#endif
