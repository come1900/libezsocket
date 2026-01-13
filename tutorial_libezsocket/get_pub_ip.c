/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * get_pub_ip.c - _explain_
 *
 * Copyright (C) 2011 tiansu-china.com, All Rights Reserved.
 *
 * $Id: get_pub_ip.c 5884 2012-10-31 04:43:57Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2012-10-31 04:43:57  Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include "ez_socket.h"
#include <ez_system_api.h>

//#include "./app_pub_def.h"

#include<stdio.h>
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
// 内部函数
    int get_pub_ip_from_mb(char *buf, size_t count);
    // 2012-2-23 8:34:33 qq's url not valid
    //int get_pub_ip_from_qq(char *pPubIP, size_t count);
    int get_pub_ip_from_sina(char *pPubIP, size_t count);
    // 2014-09-05 14:17:28 replace by overhttp
    //int get_pub_ip_from_faceaip(char *pPubIP, size_t count);

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
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

void test_get_pub_ip();
void test_get_pub_ip_from_sina();
void test_get_pub_ip_from_mb();

void test_get_pub_ip_over_http();

int main(int argc, char *argv[])
{
    unsigned int iRunTimes=0;
    //test_get_pub_ip_from_sina();

    //test_get_pub_ip_from_faceaip();

    //test_get_pub_ip();

    while(1)
    {
        printf("iRunTimes:%20u\n", iRunTimes);

        //test_get_pub_ip_from_sina();

        test_get_pub_ip_from_mb();

        //test_get_pub_ip_over_http();

        //test_get_pub_ip();

        iRunTimes++;
        usleep(1000*50);

    }

    return 0;
}

void test_get_pub_ip_from_sina()
{
    int ret;

    char strPubIP[64] = {0};
    printf("\t%-26s", __FUNCTION__);

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

void test_get_pub_ip_from_mb()
{
    int ret;

    char strPubIP[64] = {0};
    printf("\t%-26s", __FUNCTION__);
    ret = get_pub_ip_from_mb(strPubIP, sizeof(strPubIP)-1);
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

void test_get_pub_ip()
{
    int ret;

    char strPubIP[64] = {0};

    printf("\t%-26s", __FUNCTION__);
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

// 实际
#define HTTP_STRING_LEN_FACEAIP	512
void test_get_pub_ip_over_http()
{
    int ret;

    char strPubIP[64] = {0};
    char buf_http[HTTP_STRING_LEN_FACEAIP+1] = {0}; // 

    printf("\t%-26s", __FUNCTION__);
    ret = get_pub_ip_over_http(strPubIP, sizeof(strPubIP)-1
                               , "www.faceaip.com"
                               , "60889"
                               , "ip.php"
                               , "MYIP="
                               , 5
                               ,buf_http
                               ,HTTP_STRING_LEN_FACEAIP);

    if (ret==0)
    {
        printf(":[%s]\n", strPubIP);
    }
    else
    {
        printf(": FAILED(%d)\n", ret);
        //printf(": FAILED\n");
    }
}
