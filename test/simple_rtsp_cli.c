/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * simple_rtsp_cli.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: simple_rtsp_cli.c 5884 2015-11-11 05:00:12Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *     ./simple_rtsp_cli-linux rtsp://219.150.56.139:18554/fca859970eccbe7a634363540fa33126e5c2b598.sdp
 *
 *  Update:
 *     2015-11-11 05:00:12  Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 
简单的RTSP交互过程：
C表示RTSP客户端，S表示RTSP服务端
1.C->S：OPTIONS request    //询问S有哪些方法可用
1.S->C：OPTIONS response   //S回应信息中包括提供的所有可用方法
 
2.C->S：DESCRIBE request   //要求得到S提供的媒体初始化描述信息
2.S->C：DESCRIBE response  //S回应媒体初始化描述信息，主要是sdp
 
3.C->S：SETUP request      //设置会话的属性，以及传输模式，提醒S建立会话
3.S->C：SETUP response     //S建立会话，返回会话标识符，以及会话相关信息
 
4.C->S：PLAY request       //C请求播放
4.S->C：PLAY response      //S回应该请求的信息
 
S->C：发送流媒体数据
5.C->S：TEARDOWN request   //C请求关闭会话
5.S->C：TEARDOWN response  //S回应该请求
 
*/

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

#include <ez_url_parser.h>
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

        printf("Rtspurl=%s\n", p_rtsp_url);
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

    int iret;
    struct timeval my_tv =
        {
            1, 500
        };
    char sendBuff[DEF_THE_BUFF_LEN+1];
    char recvBuff[DEF_THE_BUFF_LEN+1];
    int iSndLen = 0;
    //int iRcvLen = 0;
    do
    {
        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
        char *pOPTIONS =
            "OPTIONS %s RTSP/1.0\r\n"
            "CSeq: 2\r\n"
            "User-Agent: LibVLC/2.2.1 (LIVE555 Streaming Media v2014.07.25)\r\n"
            "\r\n"
            ;

        snprintf(sendBuff, DEF_THE_BUFF_LEN, pOPTIONS, p_rtsp_url);
        sendBuff[DEF_THE_BUFF_LEN] = '\0';

        pOPTIONS = sendBuff;

        printf("1. C-S pOPTIONS:\n-------------\n%s\n-------------\n", pOPTIONS);
        iSndLen = strlen(pOPTIONS);
        iret = ez_writen(rtsp_cli_sock_fd, pOPTIONS, iSndLen);
        //printf("ez_writen:%d\n", iret);
        if (iret!=iSndLen)
        {
            __trip;
            printf("ez_writen(%d) failed.\n", iret);

            break;
        }
        my_tv.tv_usec = 500;
        my_tv.tv_sec = 5;
        iret = ez_read_with_timeout(rtsp_cli_sock_fd, recvBuff, sizeof(recvBuff), &my_tv);
        //printf("ez_read_with_timeout:%d\n", iret);
        if (iret<=0)
        {
            __trip;
            printf("ez_read_with_timeout(%d) failed.\n", iret);

            break;
        }
        recvBuff[iret] = '\0';
        printf("   S-C pOPTIONS ACK:\n-------------\n%s\n-------------\n", recvBuff);

        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
        char *pDESCRIBE =
            "DESCRIBE %s RTSP/1.0\r\n"
            "CSeq: 3\r\n"
            "User-Agent: LibVLC/2.2.1 (LIVE555 Streaming Media v2014.07.25)\r\n"
            "Accept: application/sdp\r\n"
            "\r\n"
            ;

        snprintf(sendBuff, DEF_THE_BUFF_LEN, pDESCRIBE, p_rtsp_url);
        sendBuff[DEF_THE_BUFF_LEN] = '\0';

        pDESCRIBE = sendBuff;

        printf("2. C-S pDESCRIBE:\n-------------\n%s\n-------------\n", pDESCRIBE);
        iSndLen = strlen(pDESCRIBE);
        iret = ez_writen(rtsp_cli_sock_fd, pDESCRIBE, iSndLen);
        //printf("ez_writen:%d\n", iret);
        if (iret!=iSndLen)
        {
            __trip;
            printf("ez_writen(%d) failed.\n", iret);

            break;
        }
        my_tv.tv_usec = 500;
        my_tv.tv_sec = 5;
        iret = ez_read_with_timeout(rtsp_cli_sock_fd, recvBuff, sizeof(recvBuff), &my_tv);
        //printf("ez_read_with_timeout:%d\n", iret);
        if (iret<=0)
        {
            __trip;
            printf("ez_read_with_timeout(%d) failed.\n", iret);

            break;
        }
        recvBuff[iret] = '\0';
        printf("   S-C pDESCRIBE ACK:\n-------------\n%s\n-------------\n", recvBuff);
        
        //break;
        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
        // udp 方式 ok
#if 1
                char *pSETUP =
                    "SETUP %s/trackID=0 RTSP/1.0\r\n"
                    "CSeq: 5\r\n"
                    "User-Agent: LibVLC/2.2.1 (LIVE555 Streaming Media v2014.07.25)\r\n"
                    "Transport: RTP/AVP;unicast;client_port=56964-56965\r\n"
                    "Session: 938576693\r\n"
                    "\r\n"
                    ;
#else
        // tcp方式 待测试
        char *pSETUP =
            "SETUP %s/trackID=0 RTSP/1.0\r\n"
            "CSeq: 5\r\n"
            "User-Agent: LibVLC/2.2.1 (LIVE555 Streaming Media v2014.07.25)\r\n"
            "Transport: RTP/AVP/TCP;interleaved=0-1\r\n"
            //"Session: 938576693\r\n"
            "\r\n"
            ;
#endif
        snprintf(sendBuff, DEF_THE_BUFF_LEN, pSETUP, p_rtsp_url);
        sendBuff[DEF_THE_BUFF_LEN] = '\0';

        pSETUP = sendBuff;

        printf("3. C-S pSETUP:\n-------------\n%s\n-------------\n", pSETUP);
        iSndLen = strlen(pSETUP);
        iret = ez_writen(rtsp_cli_sock_fd, pSETUP, iSndLen);
        //printf("ez_writen:%d\n", iret);
        if (iret!=iSndLen)
        {
            __trip;
            printf("ez_writen(%d) failed.\n", iret);

            break;
        }
        my_tv.tv_usec = 500;
        my_tv.tv_sec = 1;
        iret = ez_read_with_timeout(rtsp_cli_sock_fd, recvBuff, sizeof(recvBuff), &my_tv);
        //printf("ez_read_with_timeout:%d\n", iret);
        if (iret<=0)
        {
            __trip;
            printf("ez_read_with_timeout(%d) failed.\n", iret);

            break;
        }
        recvBuff[iret] = '\0';
        printf("   S-C pSETUP ACK:\n-------------\n%s\n-------------\n", recvBuff);
        char *pSession = strstr(recvBuff, "Session: ");
        int iSession = 0;
        if (pSession!=NULL)
        {
            pSession+=strlen("Session: ");
            iSession = atoi(pSession);
            printf("iSession(%d).\n", iSession);
        }
        else
        {
            iSession = 0;
        }
        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

        break;

        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
        char *pPLAY =
            "PLAY rtsp://184.72.239.149/vod/mp4:BigBuckBunny_115k.mov/ RTSP/1.0\r\n"
            "CSeq: 6\r\n"
            "User-Agent: LibVLC/2.2.1 (LIVE555 Streaming Media v2014.07.25)\r\n"
            "Session: %d\r\n"
            //"Range: npt=0.000-\r\n"
            "\r\n"
            ;

        snprintf(recvBuff, sizeof(recvBuff)-1, pPLAY, iSession);
        pPLAY = recvBuff;
        printf("3. C-S pPLAY:\n-------------\n%s\n-------------\n", pPLAY);
        iSndLen = strlen(pPLAY);
        iret = ez_writen(rtsp_cli_sock_fd, pPLAY, iSndLen);
        //printf("ez_writen:%d\n", iret);
        if (iret!=iSndLen)
        {
            __trip;
            printf("ez_writen(%d) failed.\n", iret);

            break;
        }
        my_tv.tv_usec = 500;
        my_tv.tv_sec = 1;
        iret = ez_read_with_timeout(rtsp_cli_sock_fd, recvBuff, sizeof(recvBuff), &my_tv);
        //printf("ez_read_with_timeout:%d\n", iret);
        if (iret<=0)
        {
            __trip;
            printf("ez_read_with_timeout(%d) failed.\n", iret);

            break;
        }
        recvBuff[iret] = '\0';
        printf("   S-C pPLAY ACK:\n-------------\n%s\n-------------\n", recvBuff);
        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

        while(1)
        {
            my_tv.tv_usec = 500;
            my_tv.tv_sec = 1;
            iret = ez_read_with_timeout(rtsp_cli_sock_fd, recvBuff, sizeof(recvBuff), &my_tv);
            if (iret<=0)
            {
                __trip;
                printf("ez_read_with_timeout:%d\n", iret);
                break;
            }
            else
            {
                printf("ez_read_with_timeout:%d\n", iret);
                iret = ez_append_file_from_memory("buff.264", recvBuff, iret);
                printf("ez_append_file_from_memory:%d\n", iret);
            }
        }
    }
    while(0);

    close (rtsp_cli_sock_fd);

    return 0;
}

