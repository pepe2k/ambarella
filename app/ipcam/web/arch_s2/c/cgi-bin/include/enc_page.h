

#ifndef AMBAENCPAGE_H
#define AMBAENCPAGE_H


Page EncodePage;
Page_Ops EncodePage_Ops;



int AmbaEncPage_init (Page* currentPage);
int AmbaEncPage_process_PostData (Page* currentPage);
int AmbaEncPage_get_params (Page* currentPage);
int AmbaEncPage_add_params (Page* currentPage, char* params);



#endif
