/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * http_cli.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: http_cli.c 5884 2017-11-13 04:45:34Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2017-11-13 04:45:34  Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include<stdio.h>

#include <ez_system_api.h>
#include "ez_fs.h"
#include "ez_socket.h"

#ifndef _DEBUG_THIS
    #define _DEBUG_THIS
#endif
#ifdef _DEBUG_THIS
	#define DEB(x) x
	#define DBG(x) x
#else
	#define DEB(x)
	#define DBG(x)
#endif

#ifndef __trip
	#define __trip printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
#endif
#ifndef __fline
	#define __fline printf("%s(%d)--", __FILE__, __LINE__);
#endif

#define ARG_USED(x) (void)&x;

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

//def_rtsp_url_pss
#define DEF_RTSP_URL "rtsp://61.160.149.159:18554/6fe237f5dd917e9b159dbcb9c1ecee705bc00c9e.sdp?s=a63d3c0edb2ad59c0f3ecad3216aeded1222316f"
//def_rtsp_url_ipc
//#define DEF_RTSP_URL "rtsp://61.160.149.159:18554/6fe237f5dd917e9b159dbcb9c1ecee705bc00c9e.sdp?s=a63d3c0edb2ad59c0f3ecad3216aeded1222316f"

#include <ezutil/ez_url_parser.h>
#define DEF_THE_BUFF_LEN 2*1024
#define DEF_TIMEOUT_SECOND 5*1000*1000

int main(int argc, char *argv[])
{
    char *p_rtsp_url = DEF_RTSP_URL;

    char str_host[128] = {0};
    char str_port[32] = {0};
    char str_scheme[8] = {0};

    ez_socket_t rtsp_cli_sock_fd = -1;

    // chk input url
    if (argc > 1)
    {
        p_rtsp_url = argv[1];

        printf("Url=%s\n", p_rtsp_url);
    }
    else
    {
        printf("Usage: %s [rtspurl=%s]\n", argv[0], p_rtsp_url);
    }

    // parse url
    ez_parsed_url_t * parsed_url = ez_parse_url(p_rtsp_url);

    if (parsed_url)
    {
        DBG(
            printf("scheme: '%s'\nHost: '%s'\nPort: '%s'\nPath: '%s'\nquery: '%s'\n",
                   parsed_url->scheme, parsed_url->host, parsed_url->port, parsed_url->path,
                   parsed_url->query);
        );
        strncpy(str_scheme, parsed_url->scheme, sizeof(str_scheme)-1);
        strncpy(str_host, parsed_url->host, sizeof(str_host)-1);
        strncpy(str_port, parsed_url->port, sizeof(str_port)-1);

        ez_parsed_url_free(parsed_url);
    }
    else
    {
        DBG(
            printf("failed ez_parse_url:%s\n", p_rtsp_url);
        );
        return -10;
    }

    rtsp_cli_sock_fd = ez_tcp_client_with_timeout(str_host, str_port, DEF_TIMEOUT_SECOND);
    if (rtsp_cli_sock_fd <= 0)
    {
        __fline;
        printf("ez_tcp_client_with_timeout failed\n");

        return -1;
    }

    printf("ez_tcp_client_with_timeout:%d\n", rtsp_cli_sock_fd);

//    int iret;
//    struct timeval my_tv =
//        {
//            1, 500
//        };
//    char sendBuff[DEF_THE_BUFF_LEN+1];
    char recvBuff[DEF_THE_BUFF_LEN+1];
    //int iSndLen = 0;
    //int iRcvLen = 0;
    
    int ii;
    
    do
    {
    	for (ii=0; ii<3; ii++)
    	{
        ssize_t ss_ret = ez_http_get(rtsp_cli_sock_fd
                                   , "host.com", "index.html", recvBuff, DEF_THE_BUFF_LEN, 0);

        printf("iret:%ld\n", ss_ret);
        if (ss_ret>0)
        {
            //printf("recvBuff:%s\n", recvBuff);
        }
        
        sleep(1);
    }

    }
    while(0);

    close (rtsp_cli_sock_fd);

    return 0;
}

