#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>
#include <pthread.h>


#include "params.h"
#include "utils.h"
#include "page.h"
#include "config_page.h"
#include "sys_page.h"
#include "md5_check.h"

#define MAXSTRLEN 1024
#define SHORTLEN 128
#define PAGESIZE 40960

extern char content_type[];
extern char postInfo[];
extern int method;

static char* const contentHtml = "<fieldset><legend>Upload</legend><div>\n\
		<form name=\"form\" action=\"/cgi-bin/webdemo.cgi?page=sys\" method=\"POST\" enctype=\"multipart/form-data\">\n\
		<table cellpadding=\"0\" cellspacing=\"0\" class=\"tableForm\">\n\
		<thead>\n\
			<tr>\n\
			</tr>\n\
		</thead>\n\
		<tbody>\n\
			<tr>\n\
				<td><input id=\"fileToUpload\" type=\"file\" size=\"45\" name=\"fileToUpload\" class=\"input\"></td>\n\
			</tr>\n\
			<tr>\n\
				<td>Make sure the upgrade file name <font color=red>update.tar.bz2</font>, and be made of  a folder name <font color=red>update</font> which includes two files: <font color=red>update.bin</font> and <font color=red>update.md5</font></td>\n\
			</tr>\n\
		</tbody>\n\
			<tfoot>\n\
				<tr>\n\
					<td><button class=\"button\" id=\"buttonUpload\" onclick=\"return ajaxFileUpload();\">Upload</button></td>\n\
				</tr>\n\
			</tfoot>\n\
	</table>\n\
	</form>\n\
	</div></fieldset><br><br>";

char* const SyspageHtml = "<!DOCTYPE HTML PUBLIC  \"-//W3C//DTD HTML 4.0 Transitional//\
EN\"\"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\
	<html>\n\
	<head>\n\
		<title>%s</title>\n\
		<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\n\
		<script language=\"JavaScript\" src=\"../js/jquery.js\"></script>\n\
		<script language=\"JavaScript\" src=\"../js/AJAXFileUpload.js\"></script>\n\
		<script language=\"JavaScript\">\n\
		function ajaxFileUpload() {\n\
			var b=document.getElementById(\"fileToUpload\");\n\
			if(b.value==\"\")\n\
			{\n\
				alert(\"please select a file\");\n\
				return false;\n\
			}\n\
			else\n\
			{\n\
				var FileName=new String(b.value);\n\
				var extension=new String (FileName.substring(FileName.lastIndexOf(\".\")+1, FileName.length));\n\
				if(extension==\"bz2\")\n\
				{}\n\
				else\n\
				{\n\
					alert(\"unknown file type\");\n\
					return false;\n\
				}\n\
			}\n\
			$(\"status\")\n\
			.ajaxStart(function(){\n\
				$(this).innerHTML = \"loading...\";\n\
				$(this).style.visibility = \"visible\";\n\
			})\n\
			.ajaxComplete(function(){\n\
				$(this).innerHTML = \"i\";\n\
				$(this).style.visibility = \"hidden\";\n\
			});\n\
			$.ajaxFileUpload\n\
			(\n\
			{\n\
				url:'/cgi-bin/webdemo.cgi?page=sys',\n\
				secureuri:false,\n\
				fileElementId:'fileToUpload',\n\
				dataType: 'json',\n\
				beforeSend:function()\n\
				{\n\
					$(\"#loading\").show();\n\
				},\n\
				complete:function()\n\
				{\n\
					$(\"#loading\").hide();\n\
				},\n\
				success: function (data, status)\n\
				{\n\
					if(typeof(data.error) != 'undefined')\n\
					{\n\
						if(data.error != '')\n\
						{\n\
							alert(data.error);\n\
						}else\n\
						{\n\
							alert(data.msg);\n\
						}\n\
					}\n\
				},\n\
				error: function (data, status, e)\n\
				{\n\
					alert(e);\n\
				}\n\
			}\n\
		)\n\
		return false;\n\
	}\n\
	</script>\n\
		<link rel=\"stylesheet\" type=\"text/css\" href=\"../css/amba.css\"/>\n\
		<style type=\"text/css\"><!--\n\
		%s\n\
		--></style>\n\
	</head>\n\
	<body%s>\n\
		<div id=\"container\" class=\"subcont\">\n\
			<div id=\"top\">\n\
				<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" height=\"40\">\n\
					<tr>\n\
						<td align=\"center\" width=\"160\"><img src=\"../img/logo1.gif\" \
						alt=\"logo\" /></td>\n\
						<td align=\"center\" width=\"640\">&nbsp; &nbsp;<p class=\"style2\
						\">Ambarella IPCam</p></td>\n\
					</tr>\n\
				</table>\n\
			</div>\n\
			%s\n\
		</div>\n\
		<div id=\"footer\">\n\
			Copyright &copy 2012 Ambarella Inc. All right reserved.\n\
		</div>\n\
	</body>\n\
	</html>";

static int _write_bin_file()
{
	char *p;
	//get boundary
	char boundary[64] = "--";
	p = strstr(getenv("CONTENT_TYPE"), "boundary=");
	strcpy(boundary + 2, p + strlen("boundary="));

	//get content length
	int len = atoi(getenv("CONTENT_LENGTH"));

	//read down to last file field
	char str[512];
	char fname[256];
	int pos = 0;
	while(1){
		if(fgets(str, 512, stdin)) {
			pos += strlen(str);
			if(strstr(str, "filename=")){
				//get filename
				memset(fname, 0, 256);
				strncpy(fname, strstr(str, "filename=")+strlen("filename="), 128);
				strncpy(fname, strchr(fname, '\"')+1, 128);
				p = strrchr(fname, '\"');
				*p= '\0';
				break;
			}
		}
	}
	fgets(str, 512, stdin);
	pos += strlen(str);
	fgets(str, 512, stdin);
	pos += strlen(str);

	int fSize = len - pos - strlen(boundary) - (strlen(boundary) + 2) - 6;		//6 means 6 "\n" in each line, I guess
	int bSize = 4096;

	FILE *f = fopen(fname, "wb");
	if(!f){
		LOG_MESSG("open file failure");
		return -1;
	}

	//chunk read
	int chunk = fSize / bSize;
	int lbSize = fSize % bSize;
	void* buf = malloc(bSize);
	if(buf == NULL) {
		LOG_MESSG("error buffer generate");
		return -1;
	}
	//open file for writing
	int i;
	for(i = 0; i < chunk; i++){
		if(fread(buf, bSize, 1, stdin)){
			if(!fwrite(buf, bSize, 1, f)){
				LOG_MESSG("error write chunk");
				free(buf);
				buf = NULL;
				return -1;
			}
		} else{
			LOG_MESSG("error read chunk");
			free(buf);
			buf = NULL;
			return -1;
		}
	}
	free(buf);
	buf = NULL;
	if(lbSize){
		buf = malloc(lbSize);
		if(buf == NULL) {
			LOG_MESSG("error buffer generate");
			return -1;
		}
		if(fread(buf, lbSize, 1, stdin)){
			if(!fwrite(buf, lbSize, 1, f)){
				LOG_MESSG("error write last chunk");
				free(buf);
				buf = NULL;
				return -1;
			}
		}else{
			LOG_MESSG("error read last chunk");
			free(buf);
			buf = NULL;
			return -1;
		}
		free(buf);
		buf = NULL;
	}
	if(fflush(f)){
		LOG_MESSG("error flush file");
		return -1;
	}
	fclose(f);
	return 0;
}

int _md5_check_file()
{
	FILE *pre_computed_stream;
	char *line;
	int count_total = 0;
	int count_failed = 0;
	int bSize = 4096;
	int return_value = 0;
	line = malloc(bSize);
	if(line == NULL) {
		LOG_MESSG("error buffer generate");
		return -1;
	}

	pre_computed_stream = fopen("update.md5","r");
	if (pre_computed_stream == NULL) {
		LOG_MESSG("md5 file doesn't exit");
		return -1;
	}
	if (fgets(line, bSize, pre_computed_stream) != NULL) {
		uint8_t *hash_value;
		char *filename_ptr;

		count_total++;
		filename_ptr = strstr(line, "  ");
		/* handle format for binary checksums */
		if (filename_ptr == NULL) {
			filename_ptr = strstr(line, " *");
		}
		if (filename_ptr == NULL) {
			LOG_MESSG("invalid format");
			count_failed++;
			return_value = -1;
			free(line);
			line = NULL;
			return return_value;
		}
		*filename_ptr = '\0';
		filename_ptr += 2;

		int i = strlen(filename_ptr);
		if (i && filename_ptr[--i] == '\n') {
			filename_ptr[i] = '\0';
		}
		hash_value = _hash_file(filename_ptr);

		if (hash_value && (strcmp((char*)hash_value, line) == 0)) {

		} else {
			count_failed++;
			return_value = -1;
		}
		/* possible free(NULL) */
		free(hash_value);
		hash_value =  NULL;
		free(line);
		line = NULL;
	}
	if (count_failed) {
		LOG_MESSG("WARNING:computed checksums did NOT match");
	}
	fclose(pre_computed_stream);
	return return_value;
}


int AmbaSysPage_init (Page * currentPage)
{
	extern Page ConfigPage;
	memset(currentPage,0,sizeof(Page));

	AmbaConfig_init(&ConfigPage);
	*currentPage = ConfigPage;

	currentPage->superPage = &ConfigPage;
	currentPage->activeMenu = System_Setting;
	currentPage->activeSubMenu = Upgrade;
	strcat(currentPage->name,"sys");

	memset(&SysPage_Ops,0,sizeof(Page_Ops));
	currentPage->page_ops = &SysPage_Ops;

	(&SysPage_Ops)->process_PostData = AmbaSysPage_process_PostData;
	(&SysPage_Ops)->get_params = AmbaSysPage_get_params;
	(&SysPage_Ops)->add_params = AmbaSysPage_add_params;
	(&SysPage_Ops)->print_HTML = AmbaSysPage_print_HTML;

	return 0;
}
int AmbaSysPage_add_params (Page* currentPage, char* text)
{

	char* content = NULL;
	content = (char *)malloc(MAXSTRLEN);
	if (content == NULL) {
		LOG_MESSG("SystemPage add content error");
		return -1;
	}
	memset(content, 0, MAXSTRLEN);
	strcpy(content, contentHtml);
	strncat(text, content, strlen(content));

	free(content);
	content = NULL;
	return 0;
}

void *_unzip_bin(void *p)
{
	int rval = system("tar -jxf /webSvr/web/cgi-bin/update.tar.bz2");
	if (rval < 0) {
		LOG_MESSG("unzip upgrade file error");
		pthread_exit((void*) -1);
	}
	pthread_exit((void*) 0);
}

int AmbaSysPage_process_PostData (Page* currentPage)
{
	 if(getenv("CONTENT_TYPE")){
		if(!strncmp(getenv("CONTENT_TYPE"), "multipart/form-data", 19)){

			_write_bin_file();

			int tid1;
			void* result1;
			pthread_create((pthread_t *)&tid1, NULL, _unzip_bin, NULL);

			if(pthread_join(tid1, &result1) == 0) {
				chdir("/webSvr/web/cgi-bin/update/");
				int rval = _md5_check_file();
				if(rval == 0) {
					return 0;
				}else {
					LOG_MESSG("md5 check error");
					return -1;
				}
			}else {
				LOG_MESSG("unzip upgrade file error");
				return -1;
			}
		}
		return 1;
	}
	return 1;
}


int AmbaSysPage_get_params (Page* currentPage)
{
	int ret = -1;
	ret = (&virtual_PageOps)->process_PostData(currentPage);
	fflush(stdout);
	if (ret < 0) {
		fprintf(stdout,"{error: 'upload file failed, please reupload',msg: ''}");
	} else {
		if (ret == 0) {

			fprintf(stdout, "{error: '',msg: 'upload file succeeded'}");
		} else {
			if (ret == 1) {
				return 0;
			}
		}
	}
	return ret;
}
static int Print_HTML_Header () {
	fprintf(stdout,"Content-type: text/html\n\n");
	return 0;
}

int AmbaSysPage_print_HTML (Page* currentPage, char* HTML) {
	if(method == POST) {
		if ((&virtual_PageOps)->get_params(currentPage) == 0){
			return 0;
		}
		LOG_MESSG("get params error");
		return -1;
	}else {
		int len;
		char css[PAGE_CSS_LEN] = {0};
		char body_JS[PAGE_BODY_JS_LEN] = {0};

		Print_HTML_Header();
		(&virtual_PageOps)->add_css_style(currentPage, css);
		(&virtual_PageOps)->add_body_JS(currentPage, body_JS);
		if ((&virtual_PageOps)->get_params(currentPage) < 0) {
			return -1;
		}

		char* content = NULL;
		content = (char *)malloc(PAGE_CONTENT_LEN);
		if (content == NULL) {
			LOG_MESSG("print HTML error");
			return -1;
		}
		memset(content, 0, PAGE_CONTENT_LEN);

		if ((&virtual_PageOps)->add_content(currentPage, content) < 0) {
			LOG_MESSG("add content error");
			free(content);
			content = NULL;
			return -1;
		}

		len = sprintf(HTML, SyspageHtml, currentPage->title, css, body_JS, content);
		free(content);
		content =  NULL;

		if( len <= PAGESIZE) {
			fputs(HTML,stdout);
		} else {
			LOG_MESSG("HTML oversize error");
			return -1;
		}
		return 0;
	}
}

