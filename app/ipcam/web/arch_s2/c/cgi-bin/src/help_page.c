#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"
#include "page.h"
#include "base_page.h"
#include "help_page.h"
#define MAXSTRLEN 512



static char* const contentHtml ="<div id=sitename class=setting>\n\
	<div id=title></div></div>\n\
	<div id=wrap>\n\
		<div id=content>%s</div>\n\
		<div class=clearingdiv>&nbsp; </div>\n\
	</div>";

static char* const liveViewHelp ="<div>\n\
	<div><b>If there is no video on live view page, and your browser is IE or IE \
	based browser, please check the below 3 points firstly:</b></div>\n\
	<br/>\n\
	<div>1. &nbsp;&nbsp;Make sure your OS is <font color=blue>Windows(XP, \
	Vista,Win7)</font> and your IE browser or IE based is <font color=blue>IE\
	(IE6,IE7,IE8).</font></div>\n\
	<br/>\n\
	<div>2. &nbsp;&nbsp;Make sure your site %s has been added to <font color\
	=blue>Trusted Site</font> and the <font color=blue>Security Level</font>\
	is <font color=blue>Low.</font><br/>\n\
	These settings can be found in IE's Menu: <font color=blue>Tool -> Internet\
	Option -> Security</font>.\n\
	</div>\n\
	<br/>\n\
	<div>3. &nbsp;&nbsp;Make sure <font color=blue> ffdshow </font>has been\
	installed on your PC, which can be get from \n\
	<a href=http://sourceforge.net/projects/ffdshow-tryout/><font color=blue>\
	http://sourceforge.net/projects/ffdshow-tryout/</font></a> \n\
	or from our SDK in the directory <font color=blue>thirdparty/decoder/ffdshow\
	-rev3498_20100704_clsid.zip</font><br/><br/>\n\
	Configure its video decoder enable <font color=blue>H.264</font> and <font\
	color=blue>MJPEG</font> use <font color=blue>libavcodec</font> library \
	under<br/>\n\
	<font color=blue> Windows Menu: Start -> All Programs -> FFdshow -> \
	Video Decoder Configuration -> Codec </font>\n\
	</div>\n\
	<div>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\
	<img src=../img/help_ffdshow1.jpeg> </div>\n\
	<br/>\n\
	<br/>\n\
	<div><b>If there is still no video on live view page, please download <a href\
	=../activeX/ambaWeb.cab>\n\
		<font color=blue>ambaWeb.cab </font></a> and follow the below steps:\
		</b> \n\
	</div>\n\
	<br/>\n\
	<div>1. &nbsp;&nbsp;<font color=blue>Extract ambaWeb.cab</font> to your\
	local directory.</div>\n\
	<br/>\n\
	<div>2. &nbsp;&nbsp;Run <font color=blue>manual_setup.bat</font>. <br/>\n\
		You can just double click the file, but it is recommended to open your \
		<font color=blue>Windows Command Prompt</font> , cd to the directory\
		and excute 'manual_setup.bat'.<br/>\n\
		If you are on <font color=blue>Vista</font> or <font color=blue>Win7\
		</font> OS, please notice you should <font color=blue>Run as Administrator\
		</font>:\n\
		right click the Command Prompt and choose <Run as Administrator> from\
		popped menu.\n\
	</div>\n\
	<br/>\n\
	<div>3. &nbsp;&nbsp;When you receive the message that the <font color=blue>\
	DllRegisterServer succeeded</font>, click <font color=blue>OK</font>.<br/>\n\
		There will be three plug-ins need to be registered:\n\
		<font color=blue>AmbaRTSPClient.ax, AmbaTCPClient.ax, AmbaIPCmrWebPlugIn.ocx</font>.\n\
		Setup succeed if all of the three are registered successfully. \n\
		<br/><br/>\n\
		If you receive the message like <font color=red> LoadLibrary (C:\\Program\
		files\\Ambarella\\AmbaRTSPClient.ax) failed</font>, \n\
		this may be caused by CRT lib 'msvr80.dll' not being found in system's \
		WinSxs folder, try to do step4, and then run <font color=blue>manual_setup.bat\
		</font> to setup again. \n\
	</div>\n\
	<br/>\n\
	<div>4. &nbsp;&nbsp;Run <font color=blue>vcredist_x86.exe</font> which is \
	<font color=blue>Microsoft Visual C++ 2005 SP1 Redistributable Package (x86)\
	</font>, and can be get from \n\
	<a href=http://www.microsoft.com/downloads/details.aspx?FamilyID=\
	200b2fd9-ae1a-4a14-984d-389c36f85647&DisplayLang=en>\n\
	<font color=blue>http://www.microsoft.com/downloads/details.aspx?FamilyID=\
	200b2fd9-ae1a-4a14-984d-389c36f85647&DisplayLang=en</font> </a>or \
	from our SDK in the directory\n\
	<font color=blue>thirdparty/windows/vcredist_x86.exe</font>.\n\
	<br/>&nbsp; <br/>\n\
	</div>\n\
	<div><b>If there is no video on live view page,  and your browser is Chrome\
	or Firefox,please check the below 3 points firstly:</b></div>\n\
	<br/>\n\
	<div>1. &nbsp;&nbsp;If your OS is <font color=blue>Windows(XP, Vista,Win7)\
	</font>.</div>\n\
	<br/>\n\
	<div>2. &nbsp;&nbsp;If you do not have <font color=blue> VLC media player\
	</font> installed on your PC, you can get the latetest version from\n\
	<a href=http://www.videolan.org/vlc/><font color=blue>http://www.videolan.org\
	/vlc/</font></a>. And make sure install the Mozzila\n\
	plugin, and the version of vlc mozzila plugin is greater than 2.0.0.<br/><br/>\n\
	</div>\n\
	<div>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\
	<img src=../img/help_vlc1.jpg> </div>\n\
	<br/>\n\
	<div>3. &nbsp;&nbsp;If you have installed <font color=blue> VLC media player\
	</font> on your PC, while its Mozzila plugin is lower than 2.0.0,\n\
	make sure to unistall it and download the greater version. we recommand you\
	to use the uninstall.exe in <font color=blue> the installation folder </font> \
	inorder to unintall\n\
	VLC clearly.\n\
	</div>\n\
	<br/>&nbsp; <br/>\n\
</div>";

static char helpItem[MAXSTRLEN] = {0};

static char* const cssStyle = "#container, #top { width:640px; }\n\
	#content { margin:0px; border-left:0px; border-right:0px; padding-left:20px; padding-right:20px }\n\
	#video { margin:60px 0px 0px 0px; width:100%; height:720px; align:center;	padding:0px; }\n\
	#action { margin:10px; width:100%; height:60px; align:left; }\n\
	.nav, .but { margin:10px; padding:5px; width:10%; text-align:center; \
	display:inline; text-transform:capitalize; }\n\
	.but { width:auto; font-weight:bold; text-decoration:none; }\n\
	.textbox { width:10%; text-align:center; }\n\
	.right { float:right; }";


int AmbaHelpPage_init (Page* currentPage)
{
	extern Page basePage;
	memset(currentPage, 0, sizeof(Page));

	AmbaBase_init(&basePage);
	*currentPage = basePage;

	currentPage->superPage = &basePage;
	strcat(currentPage->title, "Amba IPCAM HELP");

	memset(&HelpPage_Ops, 0, sizeof(Page_Ops));
	currentPage->page_ops = &HelpPage_Ops;

	(&HelpPage_Ops)->add_css_style = AmbaHelpPage_add_css_style;
	(&HelpPage_Ops)->add_content = AmbaHelpPage_add_content;
	(&HelpPage_Ops)->get_params = AmbaHelpPage_get_params;

	return 0;
}

int AmbaHelpPage_add_css_style (Page* currentPage, char* text)
{
	strncat(text, cssStyle, strlen(cssStyle));
	return 0;
}
int AmbaHelpPage_add_help_item (Page* currentPage)
{
	sprintf(helpItem,liveViewHelp,currentPage->host);
	return 0;
}

int AmbaHelpPage_add_content (Page* currentPage, char* text)
{
	AmbaHelpPage_add_help_item(currentPage);

	char* content = NULL;
	content = (char *)malloc(PAGE_CONTENT_LEN);
	if (content == NULL) {
		LOG_MESSG("HelpPage add content error");
		return -1;
	}
	memset(content, 0, PAGE_CONTENT_LEN);
	sprintf(content, contentHtml, helpItem);
	strncat(text, content, strlen(content));

	free(content);
	content = NULL;
	return 0;
}

int AmbaHelpPage_get_params (Page* currentPage)
{
	return 0;
}

