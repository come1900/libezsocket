/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * faceaip.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: faceaip.c 5884 2014-09-05 04:07:54Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2014-09-05 04:07:54  Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include<stdio.h>

#include "ez_socket.h"
#include <ez_system_api.h>

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
	#define __trip printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
#endif
#ifndef __fline
	#define __fline printf("%s(%d)--", __FILE__, __LINE__);
#endif

#define ARG_USED(x) (void)&x;
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

void my_faceaip();

int main(int argc, char *argv[])
{
    unsigned int iRunTimes=1;

    while(1)
    {
        printf("iRunTimes:%20u\n", iRunTimes);

        my_faceaip();

        iRunTimes++;
        usleep(1000*50);
    }

    return 0;
}

void my_faceaip()
{
    int ret;

    //char strPubIP[64] = {0};

    char recvline[512] = {0};/*460*/

    printf("\t%-26s\n", __FUNCTION__);

    printf("\t%-26s", "ez_ivs_updateip");

    ret = ez_ivs_updateip(
              "demo", //const char *pLogin		   ,
              "demo", //const char *pPasswd        ,
              "216112114781954411T1", //const char *pPuid          ,
              "192.168.0.225", //const char *pPuip          ,
              "37777", //const char *pPuPort        ,
              "80", //const char *pHttpPort      ,
              "IP Camera", //const char *pPuname        ,
              "00:0C:29:D1:F2:82", //const char *pMac		   ,
              "attr of demo", //const char *pAttr		   ,
              "www.faceaip.com", //const char *pServiceHost   , //
              "60889",//const unsigned short iPort ,
              "/devupdae.php", //const char *pServiceLoc	   ,
              recvline, //char *recvline			   ,
              sizeof(recvline)/*size_t count*/);

    if (ret==0)
    {
        printf(":[%s]\n", "SUCCEED");
    }
    else
    {
        printf("![%s]\n", "FAILED");
    }

    ez_sleep(0, 800);
}

