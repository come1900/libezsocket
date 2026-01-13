/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * rtsp_valid.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: rtsp_valid.c 5884 2016-09-29 03:28:34Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2016-09-29 03:28:34  Create
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

int chk_and_print(char *p_rtsp_url)
{
    int iret;

    printf("ez_rtsp_valid : %s\n", p_rtsp_url);

    iret = ez_rtsp_valid(p_rtsp_url, 1);

    char *p_error_str = \
                        iret==0    ? "OK"                     :
                        iret==-1   ? "input not valid"        :
                        iret==-2   ? "not rtsp"               :
                        iret==-3   ? "host not valid"         :
                        iret==-4   ? "host not valid"         :
                        iret==-5   ? "port not valid"         :
                        iret==-11  ? "network unreachable"    :
                        iret==-101 ? "failed pOPTIONS"        :
                        iret==-106 ? "rtsp program not valid" : "Unknow error";
    printf("\t(%d)\t: %s\n", iret, p_error_str);

    return iret;
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        char *p_rtsp_url = argv[1];

        chk_and_print(p_rtsp_url);

        return 0;
    }

    //good
    {
        char *p_rtsp_url = "rtsp://61.160.149.160:18554/jobs.mp4";

        chk_and_print(p_rtsp_url);
    }

    //good
    {
        char *p_rtsp_url = "rtsp://202.102.108.136:18554/812A931161E_0_1.sdp";

        chk_and_print(p_rtsp_url);
    }

    //good
    {
        char *p_rtsp_url = "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov";

        chk_and_print(p_rtsp_url);
    }

    //not exit
    {
        char *p_rtsp_url = "rtsp://202.102.108.136:18554/jobs_.mp4";

        chk_and_print(p_rtsp_url);
    }

    //port 18556 not exit
    {
        char *p_rtsp_url = "rtsp://202.102.108.136:18754/jobs.mp4";

        chk_and_print(p_rtsp_url);
    }
    //port 18556 not exit
    {
        char *p_rtsp_url = "rtsp://192.168.101.133:18754/jobs.mp4";

        chk_and_print(p_rtsp_url);
    }

    return 0;
}

