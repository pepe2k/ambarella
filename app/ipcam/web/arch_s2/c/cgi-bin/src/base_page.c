
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "utils.h"
#include "page.h"
#include "base_page.h"
#include "config.h"



#define MAXSTRLEN 512
#define SHORTLEN 32
#define PAGESIZE 40960

char* const pageHtml = "<!DOCTYPE HTML PUBLIC  \"-//W3C//DTD HTML 4.0 Transitional//\
EN\"\"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\
	<html>\n\
	<head>\n\
		<title>%s</title>\n\
		<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\n\
		<script language=\"JavaScript\" src=\"../js/AJAXInteraction.js\"></script>\n\
		<script language=\"JavaScript\" src=\"../js/CVerpage.js\"></script>\n\
		<script language=\"JavaScript\" src=\"../js/style.js\"></script>\n\
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



extern Page_Ops virtual_PageOps;
extern int method;
extern int indexhtml;

static inline char * gethost(void)
{
	return getenv("HTTP_HOST");
}

int AmbaBase_init (Page* currentPage) {
	currentPage->superPage = NULL;
	memset(currentPage, 0, sizeof(Page));

	strcat(currentPage->version, "1,0,0,33");
	strcpy(currentPage->pageURL, "/cgi-bin/webdemo.cgi");//pageURL: /cgi-bin/webdemo.cgi
	char* host_name = gethost();
	strncat(currentPage->host, host_name, strlen(host_name));
	memset(&basePage_Ops, 0, sizeof(Page_Ops));
	currentPage->page_ops = &basePage_Ops;

	(&basePage_Ops)->create_em_text = AmbaBase_create_em_text;
	(&basePage_Ops)->create_select_label = AmbaBase_create_select_label;
	(&basePage_Ops)->create_input_entry = AmbaBase_create_input_entry;
	(&basePage_Ops)->create_text_entry = AmbaBase_create_text_entry;
	(&basePage_Ops)->create_wide_text_entry = AmbaBase_create_wide_text_entry;
	(&basePage_Ops)->create_checkbox = AmbaBase_create_checkbox;
	(&basePage_Ops)->create_radio_input = AmbaBase_create_radio_input;
	(&basePage_Ops)->get_section_param = AmbaBase_get_section_param;
	(&basePage_Ops)->get_params = AmbaBase_get_params;
	(&basePage_Ops)->add_css_style = AmbaBase_add_css_style;
	(&basePage_Ops)->add_body_JS = AmbaBase_add_body_JS;
	(&basePage_Ops)->add_content = AmbaBase_add_content;
	(&basePage_Ops)->print_HTML = AmbaBase_print_HTML;

	return 0;
}



int AmbaBase_create_em_text (Page* currentPage, char * text, em_Text* em_text) {
	/* if(name != "" && style != "")  	string is : text_buffer*/
	char* string_format = "<p class=\"%s\" id=\"%s\">%s</p>\n";
	/* if( name == "" && style == "") 	string is : text_buffer_all_null*/
	char* string_format_all_null = "<p>%s</p>\n";
	/* if(name == "" && style != "") 	string is : text_buffer_name_null*/
	char* string_format_name_null = "<p class=\"%s\">%s</p>\n";
	/* if(name != "" && style == "")	string is : text_buffer_style_null*/
	char* string_format_style_null = "<p id=\"%s\">%s</p>\n";
	char text_format[MAXSTRLEN] = {0};
	if (strcmp(em_text->name, "") == 0) {
		if (strcmp(em_text->style, "") == 0) {
			sprintf(text_format, string_format_all_null, em_text->content);
		} else {
			sprintf(text_format, string_format_name_null, em_text->style, em_text->content);
		}
	} else {
		if (strcmp(em_text->style, "") == 0) {
			sprintf(text_format, string_format_style_null, em_text->name, em_text->content);
		} else {
			sprintf(text_format, string_format,em_text->style, em_text->name, em_text->content);
		}
	}
	strncat(text, text_format, strlen(text_format));
	return 0;
}


int AmbaBase_create_select_label (Page* currentPage, char * text, select_Label* select_label) {
	char* label_format_title = "<label for=\"%s\">%s</label><select id=\"%s\" %s>\n";
	int len = select_label->option_len;


	char label_string[MAXSTRLEN] = {0};
	sprintf(label_string, label_format_title, \
		select_label->name, select_label->label, select_label->name, select_label->action);
	strncat(text, label_string, strlen(label_string));

	char* option_string_format = "<option value=%d%s>%s</option>\n";
	char option_string[MAXSTRLEN] = {0};
	int i = 0;
	for (i = 0; i < len; i++) 	{
		memset(option_string, 0, sizeof(option_string));
		if (select_label->options[i].value == select_label->value) {
			sprintf(option_string, option_string_format, select_label->options[i].value, \
				" selected", select_label->options[i].option);
		} else {
			sprintf(option_string, option_string_format, select_label->options[i].value, \
				" ", select_label->options[i].option);
		}
		strncat(text, option_string, strlen(option_string));
	}
	char* label_format_end = "</select>\n";
	strncat(text, label_format_end, strlen(label_format_end));

	return 0;
}

int AmbaBase_create_input_entry (Page* currentPage, char * text, input_Entry* input_entry) {
	char* string_format = "<input type=\"%s\" class=\"textinput\" id=\"%s\" value=\"%d\" maxlength=%d %s />\n";
	char text_buffer[MAXSTRLEN] = {0};
	if (strcmp(input_entry->action, "hidden") == 0) {
		sprintf(text_buffer, string_format, "hidden", \
			input_entry->name, input_entry->value, input_entry->maxlen, "");
	}
	else {
		sprintf(text_buffer, string_format, "text", \
			input_entry->name, input_entry->value, input_entry->maxlen, input_entry->action);
	}
	strncat(text, text_buffer, strlen(text_buffer));
	return 0;
}

int AmbaBase_create_text_entry (Page* currentPage, char * text, text_Entry* text_entry) {
	char* string_format = "<label for=\"%s\" id=\"%s_l\">%s</label>\n\
						<input type=\"text\" class=\"textinput\" id=\"%s\" value=\"%d\" maxlength=%d %s />\n";
	char text_buffer[MAXSTRLEN] = {0};
	sprintf(text_buffer, string_format, \
		text_entry->name, text_entry->name, text_entry->label,
		text_entry->name, text_entry->value, text_entry->maxlen, text_entry->ro);
	strncat(text, text_buffer, strlen(text_buffer));
	return 0;
}


int AmbaBase_create_wide_text_entry (Page* currentPage, char * text, wide_text_Entry* wide_text_entry) {
	char* string_format = "<label for=\"%s\" id=\"%s_l\">%s</label>\n\
						<input type=\"text\" class=\"widetextinput\" id=\"%s\" value=\"%s\" maxlength=%d %s />\n";
	char text_buffer[MAXSTRLEN] = {0};
	sprintf(text_buffer, string_format, \
		wide_text_entry->name, wide_text_entry->name, wide_text_entry->label,\
		wide_text_entry->name, wide_text_entry->value, wide_text_entry->maxlen, wide_text_entry->ro);
	strncat(text, text_buffer, strlen(text_buffer));
	return 0;
}


int AmbaBase_create_checkbox (Page* currentPage, char * text, checkBox_Input* checkBox) {
	char* string_format = "<input type=\"checkbox\" id=\"%s\" value=\"%d\" %s %s />\n\
						<label for=\"%s\" id=\"%s_l\">%s</label>\n";
	char text_buffer[MAXSTRLEN] = {0};
	if (checkBox->value == 1) {
		sprintf(text_buffer, string_format, \
		checkBox->name, checkBox->value, checkBox->action, \
		"checked", checkBox->name, checkBox->name, checkBox->label);
	} else {
		sprintf(text_buffer,string_format, \
			checkBox->name, checkBox->value, checkBox->action, \
			"", checkBox->name, checkBox->name, checkBox->label);
	}
	strncat(text, text_buffer, strlen(text_buffer));
	return 0;
}


int AmbaBase_create_radio_input (Page* currentPage, char * text, radio_Input* radio_input) {
	char* label_string_format = "<label for=\"%s\">%s</label>\n\
							<input type=\"radio\" id=\"%s\" style=\"visibility:hidden\" />";
	char label_string[MAXSTRLEN] = {0};
	sprintf(label_string, label_string_format, \
		radio_input->name, radio_input->label, radio_input->name);

	int len = radio_input->options_len;
	int i ;
	char* option_string_format = "<input type=\"radio\" name=\"n_%s\" value=%d%s/>%s &nbsp; &nbsp; \n";
	char option_string[MAXSTRLEN] = {0};
	for(i = 0; i < len; i++) {
		if ((radio_input->options + i)->value == radio_input->value) {
			if (option_string[0] != 0) {
				memset(option_string, 0, sizeof(option_string));
			}
			sprintf(option_string, option_string_format, \
				radio_input->name, (radio_input->options + i)->value, \
				" checked", (radio_input->options + i)->option);
			strncat(text, option_string, strlen(option_string));
		} else {
			if (option_string[0] != 0) {
				memset(option_string, 0, sizeof(option_string));
			}
			sprintf(option_string, option_string_format, \
				radio_input->name, (radio_input->options + i)->value, \
				" ", (radio_input->options + i)->option);
			strncat(text, option_string, strlen(option_string));
		}
	}
	return 0;
}


int AmbaBase_get_section_param (Page* currentPage,section_Param* section_param) {
	AmbaTransfer transfer;
	int ret = 0;
	transfer_init(&transfer);
	Message msg;
	AmbaPack pack;
	pack_init(&pack);

	pack.pack_msg(0, section_param->sectionName, \
		section_param->extroInfo, &msg);
	ret = transfer.send_get_request(section_param, REQ_GET_PARAM,msg);
	if (ret < 0) {
		return -1;
	}
	return 0;
}

int AmbaBase_get_params (Page* currentPage) {
	return 0;
}

int AmbaBase_add_css_style (Page* currentPage,char* text) {
	return 0;
}

int AmbaBase_add_body_JS (Page* currentPage,char* text) {
	return 0;
}

int AmbaBase_add_content (Page* currentPage,char* text) {
	return 0;
}

static int Print_HTML_Header () {
	fprintf(stdout,"Content-type: text/html\n\n");
	return 0;
}

int AmbaBase_print_HTML (Page* currentPage, char* HTML) {
	if((method == POST) && (indexhtml == FALSE)) {
		/*Apply Operations*/
		if ((&virtual_PageOps)->get_params(currentPage) != 1) {
			return 0;
		}
		/*Cancel Operations*/
		fprintf(stdout,"<div id=\"status\">&nbsp; </div>\n");

		char* params = NULL;
		params = (char *)malloc(PAGE_CONTENT_LEN);
		if (params == NULL) {
			LOG_MESSG("print params error");
			return -1;
		}
		memset(params, 0, PAGE_CONTENT_LEN);
		(&virtual_PageOps)->add_params(currentPage, params);
		fputs(params,stdout);

		free(params);
		params = NULL;
		return 0;
	}else {
		int len;
		char css[PAGE_CSS_LEN] = {0};
		char body_JS[PAGE_BODY_JS_LEN] = {0};

		Print_HTML_Header();
		(&virtual_PageOps)->add_css_style(currentPage, css);
		(&virtual_PageOps)->add_body_JS(currentPage, body_JS);
		(&virtual_PageOps)->get_params(currentPage);


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

		len = sprintf(HTML,pageHtml,currentPage->title,css,body_JS,content);
		free(content);
		content =  NULL;

		if( len <= PAGESIZE) {
			fputs(HTML,stdout);
		} else {
			LOG_MESSG("oversize HTML error");
			return -1;
		}
		return 0;
	}
}

int get_value (char* text,char* cmp_Text)
{
	char* buffer = strstr(text, cmp_Text);
	char value[SHORTLEN] = {0};
	int i = 0;
	int j = 0;
	int flag = FALSE;

	memset(value, 0, SHORTLEN);
	if (buffer != NULL) {
		int len = strlen(buffer);
		for (i = 0; i < len; i++) {
			if (flag) {
				if ((*buffer) == '&') {
					break;
				}
				value[j++] = (*buffer);
			}
			if ((*buffer) == '=') {
				flag = TRUE;
			}
			buffer++;
		}
		value[j] = '\0';
		return atoi(value);
	} else {
		return 0;
	}
}
