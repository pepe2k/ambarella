
#ifndef AMBAHELPPAGE_H
#define AMBAHELPPAGE_H


Page HelpPage;
Page_Ops HelpPage_Ops;


int AmbaHelpPage_init (Page* currentPage);
int AmbaHelpPage_add_css_style (Page* currentPage, char* text);
int AmbaHelpPage_add_help_item (Page* currentPage);
int AmbaHelpPage_add_content (Page* currentPage, char* text);
int AmbaHelpPage_get_params (Page* currentPage);


#endif