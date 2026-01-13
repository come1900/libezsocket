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

//#define _DEBUG_THIS
#include <ezutil/ez_def_devel_debug.h>

#define def_defaultDevId "026523010504727201"
#define def_defaultAccount "zjwwjk"
#define def_defaultPasswd "JSdx@2017"

int main(int argc, char *argv[])
{

    const char *pDevId = argc > 1 ? argv[1] : def_defaultDevId;
    const char *pAccount = argc > 2 ? argv[2] : def_defaultAccount;
    const char *pPasswd = argc > 3 ? argv[3] : def_defaultPasswd;

    printf("%20s:%s\n", "strAccount", pAccount);
    printf("%20s:%s\n", "strPasswd", pPasswd);
    printf("%20s:%s\n", "strDevId", pDevId);

    int ret;

    const int iType = ezDef_StreamType_megaeye /*4*/;
    //const char *pType    = "4"; /* 流类型，代号如下：1-流媒体      2-海康IPC     3-大华IPC     4-全球眼   */
    const char *pUrl = "http://58.223.251.12:9001"; /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
                                                    //        const char *pAccount = pAccount; //镇江文旅                                              */
                                                    //        const char *pPasswd  = pPasswd; /* 密码                                                                   */
    const char *pChannel = pDevId;                  //英国领事馆旧址

    const char *pSubtype = ""; /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
    char pPlayUrl[1024];       /* 播放地址 */

    ret = ez_getStreamPlayUrl(iType, pUrl, pAccount, pPasswd, pChannel, pSubtype, pPlayUrl, sizeof(ez_getStreamPlayUrl) - 1);
    printf("ez_getStreamPlayUrl(%d,%s,%s,%s ...):%d\n", iType, pAccount, pPasswd, pChannel, ret);

    if (ret == 0)
    {
        // 接口中不处理
        replace_chars_with_chars(pPlayUrl, "&amp;", "&");
        printf("pPlayUrl:%s\n", pPlayUrl);
    }

    return 0;
}
