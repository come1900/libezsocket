/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * porsts_cli.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: porsts_cli.c 5884 2018-08-10 05:43:14Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2018-08-10 05:43:14  Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/


#include<stdio.h>

#include "str_opr.h"
#include "ez_socket.h"

#ifndef _DEBUG_THIS
//    #define _DEBUG_THIS
#endif
#ifdef _DEBUG_THIS
	#define DEB(x) x
	#define DBG(x) x
#else
	#define DEB(x)
	#define DBG(x)
#endif

#ifndef __trip
	#define __trip  printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
#endif
#ifndef __fline
	#define __fline printf("%s(%d)--", __FILE__, __LINE__);
#endif

#define ARG_USED(x) (void)&x;

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int main(int argc, char *argv[])
{
    int iret = 9999;
    ez_socket_t sockfd = INVALID_SOCKET;

    do
    {
        //char recvline[HTTP_MAXLINE + 1];

        const char *hname = "192.168.9.66";
        const char *hport = "18900";
        const char *page = "/UniTrans/hello";
        char *p_post_string = "{\"DevID\":\"G1410503438\",\"Result\":\"0\",\"ResultMsg\":\"Ok\",\"Values\":[{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"999999999\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"1\",\"Type\":\"AlgaeConBlue\",\"Value\":\"4.2747\"},{\"Channel\":\"1\",\"Type\":\"AlgaeConGreen\",\"Value\":\"1.69\"},{\"Channel\":\"1\",\"Type\":\"AlgaeConDino\",\"Value\":\"1.098\"},{\"Channel\":\"1\",\"Type\":\"AlgaeConHide\",\"Value\":\"0\"},{\"Channel\":\"1\",\"Type\":\"AlgaeConYellow\",\"Value\":\"0.76607\"},{\"Channel\":\"1\",\"Type\":\"WindSpeed\",\"Value\":\"0\"},{\"Channel\":\"1\",\"Type\":\"WindDirection\",\"Value\":\"0\"},{\"Channel\":\"1\",\"Type\":\"AirPressure\",\"Value\":\"0\"},{\"Channel\":\"3\",\"Type\":\"Temperature\",\"Value\":\"-50\"},{\"Channel\":\"3\",\"Type\":\"Humidity\",\"Value\":\"0\"},{\"Channel\":\"1\",\"Type\":\"Rainfall\",\"Value\":\"0\"},{\"Channel\":\"1\",\"Type\":\"Temperature\",\"Value\":\"37.1\"},{\"Channel\":\"1\",\"Type\":\"PH\",\"Value\":\"7.36765\"},{\"Channel\":\"1\",\"Type\":\"DissolvedOxygen\",\"Value\":\"7.11346\"},{\"Channel\":\"1\",\"Type\":\"Conductivity\",\"Value\":\"241.579\"},{\"Channel\":\"1\",\"Type\":\"Turbidity\",\"Value\":\"253.168\"},{\"Channel\":\"1\",\"Type\":\"Transmittance\",\"Value\":\"-97.31\"},{\"Channel\":\"1\",\"Type\":\"Depth\",\"Value\":\"0.55753\"},{\"Channel\":\"2\",\"Type\":\"LiveUrlSmoothRtmp\",\"Value\":\"rtmp://61.160.149.164:19090/live/G1410503438VH2_1_2?s=4d377f2dc7ea79eca2eab78e773065db56950c72\"},{\"Channel\":\"3\",\"Type\":\"LiveUrlSmoothRtmp\",\"Value\":\"rtmp://61.160.149.164:19090/live/G1410503438VH3_1_2?s=c6f17d417845fb46b0de388444b9285cca828198\"}]}";

        int iFastMode = 0;

        //        char poststr[1024];
        //        snprintf(poststr
        //                 , sizeof(poststr),
        //                 "%s"
        //                 , p_post_string
        //                );
        //
        sockfd = ez_tcp_client_with_timeout(hname, hport, 1*1000*1000);

        if (sockfd==INVALID_SOCKET)
        {
            DBG(
                __fline;
                printf("ez_tcp_client_with_timeout failed.\n");
            );
            iret = -100;
            break;
        }

        // do not use fast mode
        char recvline[9024];
        int recvbuflen = 9023;
        int ret = 0;
        int ii;
        for (ii=0; ii<10000; ii++)
        {
            ret = ez_http_post(sockfd, hname, page, p_post_string, recvline, recvbuflen, iFastMode);

            printf("%03d == ez_http_post :%d, p_post_string:%ld\n", ii, ret, strlen(p_post_string));

            if (ret > 0)
            {
                //DBG(
                printf("recv(%d):\n--------------------\n%s\n---------------------\n", ret, recvline);
                //);

            }
            else
            {
                //DBG(
                printf("ez_http_post failed:%d\n", ret);
                //);
                iret = -102;
                break;
            }

            //sleep(1);
        }// for
    }
    while(0);

    if (sockfd!=INVALID_SOCKET)
    {
        close (sockfd);
    }

    return iret;
}
