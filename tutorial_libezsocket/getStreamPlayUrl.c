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
    if (0)
    {
    	const int iType = ezDef_StreamType_IPC_hikvision/*2*/;
        //const char *pType    = "2"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "rtsp://222.189.3.205:54246"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "chw"; /* 账号、用户名                                                           */
        const char *pPasswd  = "chw123457"; /* 密码                                                                   */
        const char *pChannel = "1"; /* 通道号                                                                 */
        const char *pSubtype = "2"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("StreamType_IPC_hikvision -- ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }
    // 海康IP摄像机
    if (0)
    {
    	const int iType = ezDef_StreamType_IPC_hikvision/*2*/;
        //const char *pType    = "2"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "rtsp://222.189.3.205:54246"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "chw"; /* 账号、用户名                                                           */
        const char *pPasswd  = "chw123457"; /* 密码                                                                   */
        const char *pChannel = "1"; /* 通道号                                                                 */
        const char *pSubtype = "2"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("StreamType_IPC_hikvision -- ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }

    // 大华IP摄像机
    if (0)
    {
    	const int iType = ezDef_StreamType_IPC_dahua/*3*/;
        //const char *pType    = "3"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "rtsp://192.168.5.176:554"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin"; /* 账号、用户名                                                           */
        const char *pPasswd  = "B5DD4F86"; /* 密码                                                                   */
        const char *pChannel = "1"; /* 通道号                                                                 */
        const char *pSubtype = "1"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        //printf("ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);
        printf("StreamType_IPC_dahua -- ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }
    // 海康7600平台
    if (0)
    {
    	const int iType = ezDef_StreamType_hk7600/*7*/;
        const char *pUrl     = "rtsp://218.92.70.210:554"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin"; /* 账号、用户名                                                           */
        const char *pPasswd  = "hik12345"; /* 密码                                                                   */
        const char *pChannel = "192.168.1.4:8000"; /* 通道号                                                                 */
        const char *pSubtype = "1"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ezDef_StreamType_hk7600 -- ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }
    // 天地伟业
    if (0)
    {
    	const int iType = ezDef_StreamType_IPC_tdwy/*8*/;
        const char *pUrl     = "rtsp://222.189.3.238:554"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin"; /* 账号、用户名                                                           */
        const char *pPasswd  = "1111"; /* 密码                                                                   */
        const char *pChannel = "2"; /* 通道号                                                                 */
        const char *pSubtype = "1"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ezDef_StreamType_IPC_tdwy -- ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }
    // 全球眼
    // 龙蟠路太平门
    if (0)
    {
    	const int iType = 4;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "tykjt_njjj"; /* 账号、用户名                                                           */
        const char *pPasswd  = "njjj@12345"; /* 密码                                                                   */
        const char *pChannel = "010175010501420001";

        const char *pSubtype = ""; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("MEYE ez_getStreamPlayUrl(%d,%s,%s,%s ...):%d\n", iType, pAccount , pPasswd, pChannel, ret);

        if (ret == 0)
        {
        	// 接口中不处理
        	replace_chars_with_chars(pPlayUrl, "&amp;", "&");
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }
    // 全球眼
    // 扬子江药业
    if (0)
    {
    	const int iType = 4;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "tzyzjzy_zpzd"; /* 账号、用户名                                                           */
        const char *pPasswd  = "tzyzjzy_zpzd"; /* 密码                                                                   */
        const char *pChannel = "012307010563395804";

        const char *pSubtype = ""; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("MEYE yangzijiangyaoye ez_getStreamPlayUrl(%d,%s,%s,%s ...):%d\n", iType, pAccount , pPasswd, pChannel, ret);

        if (ret == 0)
        {
        	// 接口中不处理
        	replace_chars_with_chars(pPlayUrl, "&amp;", "&");
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }
    // 全球眼
    // 交警四队-汉中门大街-莫愁湖东路
    if (0)
    {
    	const int iType = 4;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "tykjt_njjj"; /* 账号、用户名                                                           */
        const char *pPasswd  = "njjj12345"; /* 密码                                                                   */
        const char *pChannel = "010175010501420005"; /* DeviceId                                                                 */
        const char *pSubtype = ""; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        printf("ez_getStreamPlayUrl(iType:%d, pUrl:%s, pAccount:%s , pPasswd:%s  , pChannel:%s)"
                                  , iType, pUrl, pAccount , pPasswd  , pChannel);
        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf(":%d\n", ret);

        if (ret == 0)
        {
        	// 接口中不处理
            replace_chars_with_chars(pPlayUrl, "&amp;", "&");
            printf("pPlayUrl:%s\n", pPlayUrl);
        }
    }

    // 全球眼-江宁综治
    if (1)
    {
        const int iType = ezDef_StreamType_megaeye/*4*/;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin@jnkfqysbz.nj"; /* 账号、用户名                                                           */
        const char *pPasswd  = "JSdx@2017"; /* 密码                                                                   */
        //const char *pChannel = "010119010564489002";
        const char *pChannel = "010119010505513802";

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
    // vlc播放异常摄像机
    if (0)
    {
        const int iType = 4;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "hongxintest"; /* 账号、用户名                                                           */
        const char *pPasswd  = "hongxintest@123"; /* 密码                                                                   */
        const char *pChannel = "010150010505065003";

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
    // 全球眼，最美泰州
    if (0)
    {
        const int iType = ezDef_StreamType_megaeye/*4*/;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "tzlyys"; /* 账号、用户名                                                           */
        const char *pPasswd  = "12345@qwe"; /* 密码                                                                   */
        const char *pChannel = "012301010563352001";
        //const char *pChannel = "012301000263498000";

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
    if (1)
    {
        const int iType = ezDef_StreamType_megaeye/*4*/;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "tz12345"; /* 账号、用户名                                                           */
        const char *pPasswd  = "12345@qwe"; /* 密码                                                                   */
        const char *pChannel = "012302010562359601";

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
    if (1)
    {
        const int iType = ezDef_StreamType_megaeye/*4*/;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "tz12345"; /* 账号、用户名                                                           */
        const char *pPasswd  = "12345@qwe"; /* 密码                                                                   */
        const char *pChannel = "012302010562359601";

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
    if (1)
    {
        const int iType = ezDef_StreamType_megaeye/*4*/;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "zjwwjk"; //镇江文旅                                              */
        const char *pPasswd  = "JSdx@2017"; /* 密码                                                                   */
        const char *pChannel = "026523010504727201"; //英国领事馆旧址

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
    if (0)
    {
        const int iType = ezDef_StreamType_megaeye/*4*/;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "tzhlhbj"; /* 账号、用户名                                                           */
        const char *pPasswd  = "JSdx@2017"; /* 密码                                                                   */
        const char *pChannel = "012301110566162272"; // 012301110566162272, 012302110566162221

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
    // 全球眼，最美泰州
    if (1)
    {
        const int iType = ezDef_StreamType_megaeye/*4*/;
        //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
        const char *pUrl     = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "xcqajj"; /* 账号、用户名                                                           */
        const char *pPasswd  = "xcqajj@111111"; /* 密码                                                                   */
        const char *pChannel = "011245010503952202";

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
    // 睿颖
    if (0)
    {
    	const int iType = ezDef_StreamType_IPC_ryht/*8*/;
        const char *pUrl     = "rtsp://221.226.192.193:7554"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin"; /* 账号、用户名                                                           */
        const char *pPasswd  = "00001838"; /* 密码                                                                   */
        const char *pChannel = "1"; /* 通道号                                                                 */
        const char *pSubtype = "1"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ezDef_StreamType_IPC_ryht -- ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("\t%s\n", pPlayUrl);
        }
    }
    // 雄迈
    if (0)
    {
    	const int iType = ezDef_StreamType_IPC_xm/*雄迈*/;
        const char *pUrl     = "pss://192.168.5.66:18088/sreq?http=80&rtsp=7554"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin"; /* 账号、用户名                                                           */
        const char *pPasswd  = "00001838"; /* 密码                                                                   */
        const char *pChannel = "1"; /* 通道号                                                                 */
        const char *pSubtype = "1"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ezDef_StreamType_IPC_xm -- ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("\t%s\n", pPlayUrl);
        }
    }
    // tp
    if (0)
    {
    	const int iType = ezDef_StreamType_IPC_tp/*tp-link*/;
        const char *pUrl     = "pss://192.168.5.192:18088/sreq?http=80&rtsp=554"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin"; /* 账号、用户名                                                           */
        const char *pPasswd  = "S8GAJ2SP"; /* 密码                                                                   */
        const char *pChannel = "1"; /* 通道号                                                                 */
        const char *pSubtype = "2"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ezDef_StreamType_IPC_tp -- ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("\t%s\n", pPlayUrl);
        }
    }
    // 集光
    if (0)
    {
    	const int iType = ezDef_StreamType_IPC_jiguang;
        const char *pUrl     = "pss://192.168.5.192:18088/sreq?http=80&rtsp=554"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin"; /* 账号、用户名                                                           */
        const char *pPasswd  = "S8GAJ2SP"; /* 密码                                                                   */
        const char *pChannel = "1"; /* 通道号                                                                 */
        const char *pSubtype = "1"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ezDef_StreamType_IPC_jiguang -- ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("\t%s\n", pPlayUrl);
        }
    }
    // 视频压缩A 
    if (0)
    {
    	const int iType = ezDef_StreamType_XXXX_001;
        const char *pUrl     = "pss://192.168.5.10:18088/sreq?http=80&rtsp=554"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin"; /* 账号、用户名                                                           */
        const char *pPasswd  = "S8GAJ2SP"; /* 密码                                                                   */
        const char *pChannel = "1"; /* 通道号                                                                 */
        const char *pSubtype = "1"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ezDef_StreamType_IPC_jiguang -- ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("\t%s\n", pPlayUrl);
        }
    }
    // 图鸭
    if (0)
    {
    	const int iType = ezDef_StreamType_TUYA_001;
        const char *pUrl     = "pss://192.168.5.13:18088/sreq?http=80&rtsp=554"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount = "admin"; /* 账号、用户名                                                           */
        const char *pPasswd  = "S8GAJ2SP"; /* 密码                                                                   */
        const char *pChannel = "1"; /* 通道号                                                                 */
        const char *pSubtype = "1"; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char        pPlayUrl[1024];   /* 播放地址 */

        ret = ez_getStreamPlayUrl(iType, pUrl, pAccount , pPasswd  , pChannel , pSubtype , pPlayUrl, sizeof(ez_getStreamPlayUrl)-1);
        printf("ezDef_StreamType_IPC_jiguang -- ez_getStreamPlayUrl(%d ...):%d\n", iType, ret);

        if (ret == 0)
        {
            printf("\t%s\n", pPlayUrl);
        }
    }

    return 0;
}

