/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * getStreamPlayUrl.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: getStreamPlayUrl.c 5884 2017-05-18 10:33:34Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2017-05-18 10:33:34  Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include "str_opr.h"

#include "ez_socket.h"

#include "./app_pub_def.h"

#include<stdio.h>

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

//#define ARG_USED(x) (void)&x;

int main(int argc, char *argv[])
{
    int ret;
 
    // 海康IP摄像机
    {
    	const int iType = 2;
        //const char *pType    = "2"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "rtsp://110.167.76.80:554"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin"; /* 账号、用户名                                                           */
        const char *pPasswd  = "12345"; /* 密码                                                                   */
        const char *pChannel = "1"; /* 通道号                                                                 */
        const char *pSubtype = "1"; /* 码流类型，主码流为0（即subtype=0），辅码流为1（即subtype=1），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }

    // 大华IP摄像机
    {
    	const int iType = 3;
        //const char *pType    = "3"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "rtsp://192.168.5.176:554"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin"; /* 账号、用户名                                                           */
        const char *pPasswd  = "B5DD4F86"; /* 密码                                                                   */
        const char *pChannel = "1"; /* 通道号                                                                 */
        const char *pSubtype = "1"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }

    // 全球眼
    // 电信仓库
    {
    	const int iType = 4;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "njdxABB"; /* 账号、用户名                                                           */
        const char *pPasswd  = "12345@qwe"; /* 密码                                                                   */
        const char *pChannel = "010150010560752002";
        //const char *pAccount = "njdxABB";
        //const char *pPasswd  = "12345@qwe";
        //const char *pChannel = "010150010560752002";

	pAccount = "xqyanshi";
	pPasswd  = "xqyanshi@1";
	pChannel = "010011010505499207"; // nok
	//pChannel = "010011010504302601"; // ok
	//pChannel = "010011010501243002"; // ok 
	//pChannel = "010011010501242804"; // ok
	//pChannel = "010011010505499401"; // ok 

        const char *pSubtype = ""; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ez_getStreamPlayUrl(%d,%s,%s,%s ...):%d\n", iType, pAccount , pPasswd, pChannel, ret);

        if (ret == 0)
        {
        	// 接口中不处理
        	replace_chars_with_chars(pPlayUrl, "&amp;", "&");
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }

    // 全球眼
    // 交警四队-汉中门大街-莫愁湖东路
    do
    {
    	const int iType = 4;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "tykjt_njjj"; /* 账号、用户名                                                           */
        const char *pPasswd  = "njjj12345"; /* 密码                                                                   */
        const char *pChannel = "010175010501420005"; /* DeviceId                                                                 */
        const char *pSubtype = ""; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
        	// 接口中不处理
            replace_chars_with_chars(pPlayUrl, "&amp;", "&");
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }
    while(0);

    return 0;
}

