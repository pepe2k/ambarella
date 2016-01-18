

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>



#include "include.h"

#define PAGESIZE 40960 // define page size 40k
#define MAXPAGELEN 20
#define MAXINPUTLEN 2048


/*
	*Define Page Value
*/

typedef enum {
	VIEWPAGE = 1,		//liveView Page
	ENCPAGE,		//Encode Page
	PMPAGE,		//Privacy Mask Page
	OSDPAGE,		//OSD Page
	CSBPAGE,		//Camera Basic Setting Page
	IQBPAGE,		//Image Quality Basic Setting Page
	HELPPAGE,		//Help Page
	SYSPAGE,		//System Setting Page
	PAGE_NUM
}PAGES;

char postInfo[MAXINPUTLEN] = {0};
extern Page_Ops virtual_PageOps;
static unsigned int pageValue = ENCPAGE;
static int streamID;
int indexhtml = FALSE;
int method = -1;


static int parse_get_pageValue (char* data)
{
	int pageFlag = FALSE;
	int i = 0;

	char* page[] = {"streamId","enc","pm","osd","csb","iqb","help","sys"};
	char  pageName[MAXPAGELEN];
	char* pageBuffer = strstr(data,"page");
	memset(pageName,0,MAXPAGELEN);
	if(pageBuffer != NULL) 	{
			while(((*pageBuffer) != '&') && ((*pageBuffer) != '\0' )&&((*pageBuffer) != ' ')) {
				if(pageFlag) {
					pageName[i++] =  (*pageBuffer);
				}
				if((*pageBuffer) == '=') {
						pageFlag = TRUE;
				}
				pageBuffer++;

			}
			pageName[i] = '\0';
			for( i = 0; i < PAGE_NUM-1; i++) {
				if( strcmp(pageName, page[i]) == 0) {
					pageValue = (unsigned int) (i + 1);
				}
			}
	} else{
			pageBuffer = strstr(data,"streamId");
			while(((*pageBuffer) != '&') && ((*pageBuffer) != '\0' )&&((*pageBuffer) != ' ')) {
				if(pageFlag) {
					pageName[i++] =  (*pageBuffer);
				}
				if((*pageBuffer) == '=') {
						pageFlag = TRUE;
				}
				pageBuffer++;

			}
			pageName[i] = '\0';
			streamID = atoi(pageName);
			pageValue = VIEWPAGE;
	}
	return pageValue;
}

static int  get_Request ()
{
	int length = -1;
	char* live_data;

	memset(postInfo, 0, MAXINPUTLEN);
	length = atoi(getenv("CONTENT_LENGTH"));
	live_data = getenv("REQUEST_METHOD");
	if(strstr(live_data,"POST") != NULL) {
		method = POST;
	}else {
		method = GET;
	}

	if ((method == POST) && (length > 0)) 	{
		fgets(postInfo, length + 1, stdin);
		live_data = getenv("QUERY_STRING");
		if (live_data != NULL) {
			pageValue = parse_get_pageValue(live_data);
		} else {
			if (strstr(postInfo,"View") != NULL) {
				pageValue = VIEWPAGE;
				streamID = 0;
			}
			indexhtml = TRUE;
		}
	}else {
		live_data = getenv("QUERY_STRING");
		pageValue = parse_get_pageValue(live_data);
	}
	return 0;
}

static int switchPage (Page_Ops* p, char* page_HTML, int streamID)
{
	char* browser;
	browser = getenv("HTTP_USER_AGENT");
	switch(pageValue) {
		case VIEWPAGE:
			{
				extern Page LiveViewPage;
				if (strstr(browser, "MSIE")) {
					AmbaLiveView_init(&LiveViewPage, streamID, MSIE);
				} else {
					AmbaLiveView_init(&LiveViewPage, streamID, OTHER_BROWSER);
				}
				if (p->print_HTML(&LiveViewPage, page_HTML) < 0) {
					return -1;
				}
				break;
			}
		case ENCPAGE:
			{
				extern Page EncodePage;
				AmbaEncPage_init(&EncodePage);
				if (p->print_HTML(&EncodePage, page_HTML) < 0) {
					return -1;
				}
				break;
			}
		case PMPAGE:
			{
				extern Page PMPage;
				AmbaPMPage_init(&PMPage);
				if (p->print_HTML(&PMPage, page_HTML) < 0) {
					return -1;
				}
				break;
			}
		case OSDPAGE:
			{
				extern Page OSDPage;
				AmbaOSDPage_init(&OSDPage);
				if (p->print_HTML(&OSDPage, page_HTML) < 0) {
					return -1;
				}
				break;
			}
		case CSBPAGE:
			{
				extern Page CamBasicPage;
				AmbaCamBasicPage_init(&CamBasicPage);
				if (p->print_HTML(&CamBasicPage, page_HTML) < 0) {
					return -1;
				}
				break;
			}
		case IQBPAGE:
			{
				extern Page IQBasicPage;
				AmbaIQBasicPage_init(&IQBasicPage);
				if (p->print_HTML(&IQBasicPage, page_HTML) < 0) {
					return -1;
				}
				break;
			}
		case HELPPAGE:
			{
				extern Page HelpPage;
				AmbaHelpPage_init(&HelpPage);
				if (p->print_HTML(&HelpPage, page_HTML) < 0) {
					return -1;
				}
				break;
			}
		case SYSPAGE:
			{
				extern Page SysPage;
				AmbaSysPage_init(&SysPage);
				if (p->print_HTML(&SysPage, page_HTML) < 0) {
					return -1;
				}
				break;
			}
		default:
			break;
	}
	return 0;
}

static int init (Page_Ops* p)
{
	virtual_init(p);
	return 0;
}

int main ()
{
	init(&virtual_PageOps);

	char* HTML = NULL;
	HTML = (char *)malloc(PAGESIZE);

	if (HTML == NULL) {
		LOG_MESSG("HTML error");
		return -1;
	}

	get_Request();
	switchPage((&virtual_PageOps), HTML, streamID);

	free(HTML);
	HTML = NULL;

	return 0;
}

