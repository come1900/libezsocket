/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * test_ezsocket.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: test_ezsocket.c 5884 2014-08-25 05:17:12Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2014-08-25 05:17:12  Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/


#include "ez_socket.h"
#include <ez_system_api.h>

#include "./app_pub_def.h"

#include<stdio.h>


int get_pub_ip_from_mb(char *buf, size_t count);
// 2012-2-23 8:34:33 qq's url not valid
//int get_pub_ip_from_qq(char *pPubIP, size_t count);
int get_pub_ip_from_sina(char *pPubIP, size_t count);
int get_pub_ip_from_faceaip(char *pPubIP, size_t count);

// pPubIP 缓存获取到的公网ip
// count 缓存长度
// pUrl 网站链接
// pPort 获取端口
// pLocation 页面
// pSpliter 分隔符
// SpliterLen 分隔符长度
// pHttpBuffer http缓存
// iBufLen 缓存长度
int get_pub_ip_over_http(char *pPubIP
                         , size_t count
                         , const char *pUrl
                         , const char *pPort
                         , const char *pLocation
                         , const char *pSpliter
                         , const int SpliterLen
                         , char *pHttpBuffer
                         , const int iBufLen);
int get_pub_ip_from_pubservice(char *pPubIP, size_t count);

void test_get_pub_ip();
void test_get_pub_ip_from_mb();
void test_get_pub_ip_from_sina();
//void test_get_pub_ip_from_faceaip();
void test_get_pub_ip_over_http();
void test_get_pub_ip_from_pubservice();

int main(int argc, char *argv[])
{
    unsigned int iRunTimes=0;
    //test_get_pub_ip_from_sina();

    //test_get_pub_ip();
    while(1)
    {
        printf("iRunTimes:%20d\n", iRunTimes);

        test_get_pub_ip_from_mb();
        test_get_pub_ip_from_sina();
        test_get_pub_ip_over_http();

        test_get_pub_ip_from_pubservice();
        // use this pls
        test_get_pub_ip();

        iRunTimes++;
        usleep(1000*50);
    }

    return 0;
}

void test_get_pub_ip_from_sina()
{
    int ret;

    char strPubIP[64] = {0};
    printf("\t%-64s", __FUNCTION__);
    ret = get_pub_ip_from_sina(strPubIP, sizeof(strPubIP)-1);
    if (ret==0)
    {
        printf(":[%s]\n", strPubIP);
    }
    else
    {
        printf(": FAILED\n");
    }

    ez_sleep(0, 800);
}

//void test_get_pub_ip_from_faceaip()
//{
//    int ret;
//
//    char strPubIP[64] = {0};
//
//        printf("\t%-64s", __FUNCTION__);

//    ret = get_pub_ip_from_faceaip(strPubIP, sizeof(strPubIP)-1);
//    if (ret==0)
//    {
//        printf(":[%s]\n", strPubIP);
//    }
//    else
//    {
//        printf(": FAILED\n");
//    }
//
//    ez_sleep(0, 800);
//}

void test_get_pub_ip()
{
    int ret;

    char strPubIP[64] = {0};

    printf("\t%-64s", __FUNCTION__);

    ret = get_pub_ip(strPubIP, sizeof(strPubIP)-1);
    if (ret==0)
    {
        printf(":[%s]\n", strPubIP);
    }
    else
    {
        printf(": FAILED\n");
    }

}

void test_get_pub_ip_from_mb()
{
    int ret;

    char strPubIP[64] = {0};

    printf("\t%-64s", __FUNCTION__);

    ret = get_pub_ip_from_mb(strPubIP, sizeof(strPubIP)-1);
    if (ret==0)
    {
        printf(":[%s]\n", strPubIP);
    }
    else
    {
        printf(": FAILED\n");
    }
}

void test_get_pub_ip_from_pubservice()
{
    int ret;

    char strPubIP[64] = {0};

    printf("\t%-64s", __FUNCTION__);

    ret = get_pub_ip_from_pubservice(strPubIP, sizeof(strPubIP)-1);
    if (ret==0)
    {
        printf(":[%s]\n", strPubIP);
    }
    else
    {
        printf(": FAILED\n");
    }

}

void test_get_pub_ip_over_http()
{
    int ret;
    char http_str_buf[512];

    char strPubIP[64] = {0};

    printf("\t%-64s", __FUNCTION__);

#if 0
    // pubservice.ezlibs.com
    ret = get_pub_ip_over_http(strPubIP, sizeof(strPubIP)-1
                               , "pubservice.ezlibs.com"
                               , "60089"
                               , "PubService/getPubIP4"
                               , "PUIP="
                               , 5);
#endif
#if 1
#define SINA_PUB_NET_ON_WEB "counter.sina.com.cn"
#define SINA_PUB_NET_ON_WEB_PAGE "ip"
#define SINA_SPLIT_STRING "Array(\""
#define SINA_SPLIT_STRING_LEN 7
#define HTTP_STRING_LEN_SINA	512

    // pubservice.ezlibs.com
    ret = get_pub_ip_over_http(strPubIP, sizeof(strPubIP)-1
                               , "counter.sina.com.cn"
                               , "80"
                               , "ip"
                               , "Array(\""
                               , SINA_SPLIT_STRING_LEN
                               , http_str_buf
                               , sizeof (http_str_buf)-1);
#endif

    if (ret==0)
    {
        printf(":[%s]\n", strPubIP);
    }
    else
    {
        printf(": FAILED:%d\n", ret);
    }
}

