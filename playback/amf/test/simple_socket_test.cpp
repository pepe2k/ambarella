/*******************************************************************************
 * simple_socket_test.cpp
 *
 * Histroy:
 *  2012-09-05 2012 - [Zhi He] Created file
 *
 * Copyright (C) 2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>

#define ERROR(format,args...)  do { \
  fprintf(stdout, "!!! %s:%d,  " __FILE__, __LINE__);\
  fprintf(stdout, format,##args); \
} while (0)

//-----------------------------------------------------------------------
//
// Parse Customized Protocol
//
//-----------------------------------------------------------------------
#define DStrCPStartTag "[CPStart] "
#define DStrCPEndTag " [CPEnd]\r\n"

#define DStrCPName "[CPName]:"
//ip address is unique id for each device
#define DStrCPIPAddress "[CPIPAddress]:"
#define DStrCPRTSPAddress "[CPRTSPAddress]:"

static int _get_local_ip_address (char* addressBuffer) {
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!(ifa->ifa_flags & IFF_LOOPBACK)) {
            if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
                // is a valid IP4 Address
                tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
                inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
                printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
                if (strcmp(addressBuffer, "127.0.0.1")) {
                    //return first non-127.0.0.1
                    return 0;
                }
            } else if (ifa->ifa_addr->sa_family==AF_INET6) { // check it is IP6
                // is a valid IP6 Address
                tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
                inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
                printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
            }
        } else {
            printf("Find loopback device: %s\n", ifa->ifa_name);
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return 0;
}

static void _buildup_rtsp_url(char* ipaddr, char* rtsp_url, int dest_len,
                              const char* tag)
{
    if (!ipaddr || !rtsp_url) {
        ERROR("bad(null) input params.\n");
    }
    if (tag) {
        snprintf(rtsp_url, dest_len, "rtsp://%s/%s", ipaddr, tag);
    } else {
        snprintf(rtsp_url, dest_len, "rtsp://%s", ipaddr);
    }
}

static int _setup_datagram_socket(unsigned int local_addr,
                                  unsigned short local_port, bool non_blocking)
{
    struct sockaddr_in  servaddr;
    int reuse_flag = 1;
    int new_socket = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family =   AF_INET;
    servaddr.sin_addr.s_addr    =  htonl(local_addr);
    servaddr.sin_port   =   htons(local_port);

    if (new_socket < 0) {
        ERROR("unable to create dgram socket\n");
        return new_socket;
    }

    if (setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_flag, sizeof reuse_flag) < 0) {
        ERROR("setsockopt(SO_REUSEADDR) error\n");
        close(new_socket);
        return -1;
    }

    if (bind(new_socket, (struct sockaddr*)&servaddr, sizeof servaddr) != 0) {
        ERROR("bind() error (port number: %d)\n", local_port);
        close(new_socket);
        return -1;
    }

    if (non_blocking) {
        int cur_flags = fcntl(new_socket, F_GETFL, 0);
        if (fcntl(new_socket, F_SETFL, cur_flags|O_NONBLOCK) != 0) {
            ERROR("failed to make non-blocking\n");
            close(new_socket);
            return -1;
        }
    }

    return new_socket;
}

static int _build_addr_from_url(struct sockaddr_in* dest_addr, char* url, unsigned short default_port)
{
    char* tmp;
    if (!dest_addr || !url) {
        ERROR("bad(null) input params.\n");
        return -1;
    }

    memset(dest_addr, 0x0, sizeof(*dest_addr));
    dest_addr->sin_family = AF_INET;

    tmp = strchr(url, ':');
    if (!tmp) {
        //no port number
        dest_addr->sin_port = htons(default_port);
        dest_addr->sin_addr.s_addr = inet_addr(url);
    } else {
        //have port number
        default_port = atoi(tmp + 1);
        *tmp = 0x0;
        dest_addr->sin_port = htons(default_port);
        dest_addr->sin_addr.s_addr = inet_addr(url);
        *tmp = ':';
    }

    return 0;
}

static void _post_msg_to_server(char* rtsp_url, struct sockaddr_in* dest_addr, int local_socket)
{
    char cp_buffer[256] = {0};//assume 256 is big enough
    unsigned int size = 0;

    //build up cp string
    strcat(cp_buffer, DStrCPStartTag);
    strcat(cp_buffer, DStrCPRTSPAddress);
    strcat(cp_buffer, rtsp_url);
    strcat(cp_buffer, DStrCPEndTag);

    //send
    size = strlen(cp_buffer);
    if (size >= (sizeof(cp_buffer) - 1)) {
        ERROR("fatal error!!! cp_buffer over flow, size %d, feeding size %d.\n", sizeof(cp_buffer), size);
        return;
    }

    sendto(local_socket, cp_buffer, size, 0, (struct sockaddr*)dest_addr, sizeof(struct sockaddr));
    return;
}

int main(int argc, char *argv[])
{
    unsigned short server_msg_port = 4848;//hard default code here
    unsigned short local_msg_port = 8484;//hard default code here
    int i = 0;
    int dgram_socket = -1;
    struct sockaddr_in dest_addr;
    char server_ip_addr_buffer[64] = "10.0.0.2";
    char local_ip_addr_buffer[INET6_ADDRSTRLEN + 4] = {0};//plus 4 for safe
    char local_rtsp_url_buffer[128] = "default";
    int specify_debug_string = 0;

  //simple check arguments
    if (argc > 1) {
        for (i = 0; i < argc; ++ i) {
            if (!strcmp("-s", argv[i])) {
                snprintf(local_rtsp_url_buffer, 128 - 1, "%s", argv[i + 1]);
                specify_debug_string = 1;
                ++ i;
            } else if (!strcmp("--msgdst", argv[i])) {
                snprintf(server_ip_addr_buffer, 64 - 1, "%s", argv[i + 1]);
                ++ i;
            } else if (!strcmp("--msgport", argv[i])) {
                ++ i;
                server_msg_port = atoi(argv[i]);
                fprintf(stdout, "\tparams: msg port %hu.\n", server_msg_port);
            } else if (!strcmp("--localmsgport", argv[i])) {
                ++ i;
                local_msg_port = atoi(argv[i]);
                fprintf(stdout, "\tparams: local msg port %hu.\n", local_msg_port);
            }
        }
    }

    //setup datagram socket
    dgram_socket = _setup_datagram_socket(INADDR_ANY, local_msg_port, false);
    fprintf(stdout, "\t[flow]: _setup_datagram_socket, local msg port %hu, ret socket %d.\n", local_msg_port, dgram_socket);

    if (_build_addr_from_url(&dest_addr, server_ip_addr_buffer, server_msg_port) < 0) {
        ERROR("build addr fail, url %s\n", server_ip_addr_buffer);
    } else {
        if (!specify_debug_string) {
            //send rtsp url to server
            _get_local_ip_address(local_ip_addr_buffer);
            _buildup_rtsp_url(local_ip_addr_buffer, local_rtsp_url_buffer, sizeof(local_rtsp_url_buffer) - 1, "stream1");
            _post_msg_to_server(local_rtsp_url_buffer, &dest_addr, dgram_socket);
        } else {
            //direct send
            _post_msg_to_server(local_rtsp_url_buffer, &dest_addr, dgram_socket);
        }
    }

    if (dgram_socket >= 0) {
        close(dgram_socket);
    }

    return 0;
}
