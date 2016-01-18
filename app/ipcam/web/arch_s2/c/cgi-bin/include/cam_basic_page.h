

#ifndef AMBACAMBASICPAGE_H
#define AMBACAMBASICPAGE_H

Page CamBasicPage;
Page_Ops CamBasicPage_Ops;



int AmbaCamBasicPage_init (Page* currentPage);
int AmbaCamBasicPage_get_params (Page* currentPage);
int AmbaCamBasicPage_add_params (Page* currentPage, char* text);
int AmbaCamBasicPage_process_PostData (Page* currentPage);


#endif
