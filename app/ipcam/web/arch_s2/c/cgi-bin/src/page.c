#include <memory.h>
#include <stdio.h>

#include "utils.h"
#include "page.h"





int virtual_init(Page_Ops* p)
{

	memset(p,0,sizeof(Page_Ops));
	p->create_em_text = virtual_create_em_text;
	p->create_select_label = virtual_create_select_label;
	p->create_input_entry = virtual_create_input_entry;
	p->create_text_entry = virtual_create_text_entry;
	p->create_wide_text_entry = virtual_create_wide_text_entry;
	p->create_checkbox = virtual_create_checkbox;
	p->create_radio_input = virtual_create_radio_input;
	p->get_section_param = virtual_get_section_param;
	p->get_params = virtual_get_params;
	p->add_css_style = virtual_add_css_style;
	p->add_body_JS = virtual_add_body_JS;
	p->add_content = virtual_add_content;
	p->print_HTML = virtual_print_HTML;
	p->create_button = virtual_create_button;
	p->add_settings = virtual_add_settings;
	p->add_controls = virtual_add_controls;
	p->process_PostData = virtual_process_PostData;
	p->add_params = virtual_add_params;
	p->show_params = virtual_show_params;
	p->create_slider_bar = virtual_create_slider_bar;
	return 0;
}

int virtual_create_em_text (Page* currentPage, char* text, em_Text* em_text)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->create_em_text != NULL ) {
			p->page_ops->create_em_text(currentPage,text,em_text);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_create_select_label (Page* currentPage, char* text, select_Label* select_label)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->create_select_label != NULL) {
			p->page_ops->create_select_label(currentPage,text,select_label);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_create_input_entry (Page* currentPage, char* text, input_Entry* input_entry)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->create_input_entry != NULL) {
			p->page_ops->create_input_entry(currentPage,text,input_entry);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;

}

int virtual_create_text_entry (Page* currentPage, char* text, text_Entry* text_entry)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->create_text_entry != NULL) {
			p->page_ops->create_text_entry(currentPage,text,text_entry);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_create_wide_text_entry (Page* currentPage, char* text, wide_text_Entry* wide_text_entry)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->create_wide_text_entry != NULL) {
			p->page_ops->create_wide_text_entry(currentPage,text,wide_text_entry);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_create_checkbox (Page* currentPage, char* text, checkBox_Input* checkBox)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->create_checkbox != NULL) {
			p->page_ops->create_checkbox(currentPage,text,checkBox);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_create_radio_input (Page* currentPage, char* text, radio_Input* radio_input)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->create_radio_input != NULL) {
			p->page_ops->create_radio_input(currentPage,text,radio_input);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_get_section_param (Page* currentPage, section_Param* section_param)
{
	Page* p = currentPage;
	int ret = 0;
	do {
		if (p->page_ops->get_section_param != NULL) {
			ret = p->page_ops->get_section_param(currentPage,section_param);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return ret;
}

int virtual_get_params (Page* currentPage)
{
	Page* p = currentPage;
	int ret = 0;
	do {
		if (p->page_ops->get_params!= NULL) {
			ret = p->page_ops->get_params(currentPage);
			break;
		} else {
			p = p->superPage;
		}
	} while (p != NULL);
	return ret;
}

int virtual_add_css_style (Page* currentPage, char* text)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->add_css_style != NULL) {
			p->page_ops->add_css_style(currentPage,text);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_add_body_JS (Page* currentPage, char* text)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->add_body_JS != NULL) {
			p->page_ops->add_body_JS(currentPage,text);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_add_content (Page* currentPage, char* text)
{
	Page* p = currentPage;
	int ret = 0;
	do {
		if (p->page_ops->add_content != NULL) {
			ret = p->page_ops->add_content(currentPage,text);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return ret;
}

int virtual_print_HTML (Page* currentPage, char* HTML)
{
	Page* p = currentPage;
	int ret = 0;
	do {
		if (p->page_ops->print_HTML != NULL) {
			ret = p->page_ops->print_HTML(currentPage,HTML);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return ret;
}


int virtual_create_button (Page * currentPage, char * text, button_Input* button)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->create_button != NULL) {
			p->page_ops->create_button(currentPage,text,button);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
		return 0;
}


int virtual_add_settings (Page* currentPage, char* text)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->add_settings != NULL) {
			p->page_ops->add_settings(currentPage,text);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_add_controls (Page* currentPage, char* text)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->add_controls != NULL) {
			p->page_ops->add_controls(currentPage,text);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_process_PostData (Page* currentPage)
{
	Page* p = currentPage;
	int ret = 0;
	do {
		if (p->page_ops->process_PostData != NULL) {
			ret = p->page_ops->process_PostData(currentPage);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return ret;
}



int virtual_add_params (Page * currentPage, char* text)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->add_params != NULL) {
			p->page_ops->add_params(currentPage,text);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_show_params (Page * currentPage)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->show_params != NULL) {
			p->page_ops->show_params(currentPage);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

int virtual_create_slider_bar (Page * currentPage, char * text, slider_Bar* slider_bar)
{
	Page* p = currentPage;
	do {
		if (p->page_ops->create_slider_bar != NULL) {
			p->page_ops->create_slider_bar(currentPage,text,slider_bar);
			break;
		} else {
			p = p->superPage;
		}
	}while (p != NULL);
	return 0;
}

