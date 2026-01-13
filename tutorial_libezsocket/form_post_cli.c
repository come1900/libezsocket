/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * form_post_cli.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: form_post_cli.c 5884 2018-08-09 10:07:42Z WuJunjie $
 *
 *  Explain:
 *     -
 *      按表单的方式提交
 *     -
 *
 *  Update:
 *     2018-08-09 10:07:42  Create
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
    int iret=0;

    //http://192.168.5.103:8080/bankCloud/updateDevice
    {
        char *hname = "180.109.118.37";
        char *hport = "18080";
        char *page = "/bankCloud/updateDevice";

        char *p_post_string = "DevId=18115886801&DevStatus=Y";
        char recvline[HTTP_MAXLINE + 1];
        int iFastMode = 1;

        iret = ez_http_post_to_host(hname, hport, page, p_post_string, recvline, HTTP_MAXLINE, iFastMode);

        printf("recv(%d):\n--------------------\n%s\n---------------------\n", iret, recvline);
    }

    return 0;
}

