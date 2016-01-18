/*******************************************************************************
 * router.c
 *
 * Histroy:
 *  2012-09-06 2012 - [Zhikan Yang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ClearSilver.h"

#define BUFFERLENTH (1024)

#define CGIPATH "/webSvr/web/cgi-bin/"
#define CONFPATH "/webSvr/web/cgi-bin/router.conf"
//#define CGIPATH "./"


static char * get_realcgi(char *page,char *action,char *conf)
{
    FILE *conf_fd=NULL;
    char *buffer=NULL;
    char *page_action=NULL;
    char *seek=NULL;
    char *tmp=NULL;
    if (page == NULL ||action == NULL || conf == NULL){
        return NULL;
    }

    conf_fd = fopen(conf,"r");

    if (conf_fd == NULL){
        perror("fopen");
        return NULL;
    }

    buffer = (char *)malloc(BUFFERLENTH);
    page_action = (char *)malloc(strlen(page) + strlen(action) + 2);
    strncpy(page_action,page,strlen(page) + 1);
    strcat(page_action,"_");
    strcat(page_action,action);

    while (fgets(buffer,BUFFERLENTH,conf_fd)){
        seek = strstr(buffer,page_action);
        if (seek == NULL){
            continue;
        }
        seek = strstr(buffer,"=");
        if (seek  == NULL){
            continue;
        }
        seek++;
        while (*seek == ' ' || *seek == '\t'){
            seek++;
        }
        break;
    }

    if (seek == NULL){
        free(buffer);
        free(page_action);
        fclose(conf_fd);
        return NULL;
    }
    else {
        free(page_action);
        fclose(conf_fd);
        tmp = seek;
        while (*tmp != ' ' && *tmp != '\t' && *tmp != '\n' && *tmp != '\0'){
            tmp++;
        }
        *tmp = '\0';
        tmp = (char *)malloc(strlen(seek) + 1);
        strcpy(tmp,seek);
        free(buffer);
        return tmp;
    }

}

static char *parse_absulute_path(char *cginame)
{
    char *buffer;
    buffer = (char *)malloc(strlen(cginame) + strlen(CGIPATH) +1);
    strcpy(buffer,CGIPATH);
    strcat(buffer,cginame);
    return buffer;
}

int main(int argc,char **agrs,char **env)
{
    char *realcgi;
    char *realcgi_path;
    FILE *fp_read;
    char *buffer;
    char *page = NULL;
    char *action = NULL;
    CGI *cgi = NULL;
    HDF *hdf = NULL;

    hdf_init(&hdf);
    cgi_init(&cgi, hdf);

    page = hdf_get_value(cgi->hdf,"Query.page",NULL);
    action = hdf_get_value(cgi->hdf,"Query.action","query");

    buffer = (char *)malloc(BUFFERLENTH);

    realcgi = get_realcgi(page,action,CONFPATH);
    if (realcgi == NULL){
        printf("Content-type: text/html\n\n");
        printf("CGI Not Found\n");
        return 1;
    }
    realcgi_path = parse_absulute_path(realcgi);
    fp_read=popen(realcgi_path,"r");
    while(fgets(buffer,BUFFERLENTH,fp_read)){
        printf("%s",buffer);
    }

    free(buffer);
    pclose(fp_read);
    return 0;

}

