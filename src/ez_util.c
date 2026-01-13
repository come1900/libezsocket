/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_stdlib.c - ez_socket wrapper
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_util.c 16364 2008-04-25 03:59:51Z guan_shiyong $
 *
 * ez_stdlib .
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2005-04-02 17:19:24 WuJunjie 10221 Create
 *		wjj 2005-04-02 15:16:48
 *			Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include <time.h>
#include <pthread.h>
#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <ez_url_parser.h>
#include <ez_md5.h>

#include	"ez_socket.h"

#ifndef _DEBUG_THIS
//#define _DEBUG_THIS
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

#define DDNS_MAXLINE         256
#define JMS_DDNS_GET_IP_URL "admain.jmdvr.net"
#define JMS_DDNS_GET_IP_PORT "28583"
#define FNT_DDNS_GET_IP_URL "data002.cngame.org"
#define FNT_DDNS_GET_IP_PORT "28586"
int __get_pub_ip_from_mb(char *buf, size_t count, char * hostip, char *sname)
{
    int n;
    int iRet=-1;
    char	sendline[DDNS_MAXLINE], recvline[DDNS_MAXLINE + 1];
    int			sockfd;
    buf[0] = 0;

    sockfd = ez_udp_connect_host(hostip, sname);

    if (INVALID_SOCKET==sockfd)
    {
        return -100;
    }

    strcpy(sendline, "test&&&&&");
    /* read a line and send to server */
    n = write(sockfd, sendline, strlen(sendline)+1);
    if (n < 0)
    {
        close(sockfd);
        return -101;
    }

    //printf("sendline:%d:%s\n", n, sendline);

    struct timeval timeout =
        {
            1, 0
        };

    /* five seconds */
    n = ez_read_with_timeout(sockfd, recvline, DDNS_MAXLINE, &timeout);
    if (n > 2)
    {
        recvline[n] = 0;        /* terminate string */
        //printf("recvline:%d:%s\n", n, recvline);
        int i;
        for (i=2; i<n; i++)
        {
            if (recvline[i]=='&')
            {
                buf[i-2]=0;
                //printf("buf:%d:%s\n", i, buf);
                break;
            }
            else
            {
                buf[i-2] = recvline[i];
            }
        }
    }
    else
    {
        //printf("recverrorno:%d\n", n);
        close(sockfd);
        return -102;
    }
    if (ez_is_valid_ip4(buf))
    {
        iRet = 0;
    }
    else
    {
        iRet = -104;
    }

    close(sockfd);
    return iRet;
}


int get_pub_ip_from_mb(char *buf, size_t count)
{
    int iRet = -1;
#if 0

    iRet = __get_pub_ip_from_mb(buf, count, JMS_DDNS_GET_IP_URL, JMS_DDNS_GET_IP_PORT);
    DBG(
        printf("__get_pub_ip_from_mb(%d)(%s, %d, %s, %s)\n", iRet, buf, count, JMS_DDNS_GET_IP_URL, JMS_DDNS_GET_IP_PORT);
    )

    if (iRet != 0)
#endif // do not fit 2014-02-05 21:47:11

    {
        iRet = __get_pub_ip_from_mb(buf, count, FNT_DDNS_GET_IP_URL, FNT_DDNS_GET_IP_PORT);
        DBG(
            printf("__get_pub_ip_from_mb(%d)(%s, %ld, %s, %s)\n", iRet, buf, count, FNT_DDNS_GET_IP_URL, FNT_DDNS_GET_IP_PORT);
        )
    }

    return iRet;
}

//#define QQ_PUB_NET_ON_WEB "fw.qq.com"
//#define QQ_PUB_NET_ON_WEB_PAGE "ipaddress"
//#define QQ_SPLIT_STRING "(\""
//#define QQ_SPLIT_STRING_LEN 2
//
//int get_pub_ip_from_qq(char *pPubIP, size_t count)
//{
//	char	 recvline[DDNS_MAXLINE + 1];
//	int iRet = -1;
//	ez_socket_t sock ;
//	int n;
//
//	sock = ez_tcp_client(QQ_PUB_NET_ON_WEB, DEFAULT_HTTP_PORT);
//	if (sock==INVALID_SOCKET)
//	{
//		return -100;
//	}
//
//	n = ez_http_get(sock, QQ_PUB_NET_ON_WEB, QQ_PUB_NET_ON_WEB_PAGE, recvline, DDNS_MAXLINE);
//
//	if (n>0)
//	{
//		//printf("recvline:\n---------------------\n%s\n------------------\n", recvline);
//		char *ptmp = (char*)strstr(recvline, QQ_SPLIT_STRING);
//		int ii;
//
//		if (ptmp)
//		{
//			for (ii=0; *(ptmp+ii+QQ_SPLIT_STRING_LEN)!='\0'&&*(ptmp+ii+QQ_SPLIT_STRING_LEN)!='\n'&&ii<15; ii++)
//			{
//				if (isdigit(*(ptmp+ii+QQ_SPLIT_STRING_LEN)) || *(ptmp+ii+QQ_SPLIT_STRING_LEN)=='.')
//				{
//					pPubIP[ii] = *(ptmp+ii+QQ_SPLIT_STRING_LEN);
//				}
//				else
//				{
//					break;
//				}
//			}
//			pPubIP[ii] = '\0';
//
//			//printf("+++++++++pPubIP:%s\n", pPubIP);
//
//			if (ez_is_valid_ip4(pPubIP))
//			{
//				iRet = 0;
//			}
//			else
//			{
//				iRet = -102;
//			}
//
//		}
//		else
//		{
//			pPubIP[0] = '\0';
//			iRet = -103;
//		}
//
//	}
//	else
//	{
//		iRet = -104;
//	}
//
//	close(sock);
//	return iRet;
//}

// check 2013-02-01 9:23:02 ok
#define SINA_PUB_NET_ON_WEB "counter.sina.com.cn"
#define SINA_PUB_NET_ON_WEB_PAGE "ip"
#define SINA_SPLIT_STRING "Array(\""
#define SINA_SPLIT_STRING_LEN 7
#define HTTP_STRING_LEN_SINA	512
int get_pub_ip_from_sina(char *pPubIP, size_t count)
{
#if 1
    // 使用优化函数 2013-07-16 10:49:40
    char	 recvline[HTTP_STRING_LEN_SINA + 1];
    int iRet = -1;

    iRet = get_pub_ip_over_http(pPubIP, count
                                , SINA_PUB_NET_ON_WEB
                                , "80"
                                , SINA_PUB_NET_ON_WEB_PAGE
                                , SINA_SPLIT_STRING
                                , SINA_SPLIT_STRING_LEN
                                , recvline
                                , sizeof (recvline)-1);

    return iRet;
#else// 使用自定义函数

    char	 recvline[HTTP_STRING_LEN_SINA + 1];

    int iRet = -1;
    ez_socket_t sock ;
    int n;

    sock = ez_tcp_client(SINA_PUB_NET_ON_WEB, DEFAULT_HTTP_PORT);
    if (sock==INVALID_SOCKET)
    {
        return -100;
    }

    n = ez_http_get(sock, SINA_PUB_NET_ON_WEB, SINA_PUB_NET_ON_WEB_PAGE, recvline, HTTP_STRING_LEN_SINA, 1);
    DBG(printf("ez_http_get:%d\n", n););

    if (n>0)
    {
        DBG( printf("recvline:\n---------------------\n%s\n------------------\n", recvline); );
        char *ptmp = (char*)strstr(recvline, SINA_SPLIT_STRING);
        int ii;

        if (ptmp)
        {
            for (ii=0; *(ptmp+ii+SINA_SPLIT_STRING_LEN)!='\0'&&*(ptmp+ii+SINA_SPLIT_STRING_LEN)!='\n'&&ii<15; ii++)
            {
                if (isdigit(*(ptmp+ii+SINA_SPLIT_STRING_LEN)) || *(ptmp+ii+SINA_SPLIT_STRING_LEN)=='.')
                {
                    pPubIP[ii] = *(ptmp+ii+SINA_SPLIT_STRING_LEN);
                }
                else
                {
                    break;
                }
            }
            pPubIP[ii] = '\0';

            DBG( printf("+++++++++pPubIP:%s\n", pPubIP); );

            if (ez_is_valid_ip4(pPubIP))
            {
                iRet = 0;
            }
            else
            {
                iRet = -102;
            }

        }
        else
        {
            pPubIP[0] = '\0';
            DBG( printf("strstr(recvline, %s)failed.\n", SINA_SPLIT_STRING); );
            iRet = -103;
        }

    }
    else
    {
        iRet = -104;
        DBG(printf("http get failed\n"););
    }

    close(sock);
    return iRet;
#endif
}
// do not fit now 2013-07-16 10:57:28
#define PUB_NET_ON_WEB_FACEAIP "www.ezlibs.com"
#define FACEAIP_PUB_NET_ON_WEB_PAGE "ip.php"
#define SPLIT_STRING_FACEAIP_MYIP "MYIP="
#define SPLIT_STRING_LEN_FACEAIP_MYIP 5
//
//int get_pub_ip_from_faceaip(char *pPubIP, size_t count)
//{
//    char	recvline[DDNS_MAXLINE + 1];
//    int iRet = -1;
//    ez_socket_t sock ;
//    int n;
//
//    sock = ez_tcp_client(PUB_NET_ON_WEB_FACEAIP, DEFAULT_HTTP_PORT);
//    if (sock==INVALID_SOCKET)
//    {
//        return -100;
//    }
//
//    n = ez_http_get(sock, PUB_NET_ON_WEB_FACEAIP, FACEAIP_PUB_NET_ON_WEB_PAGE, recvline, DDNS_MAXLINE, 1);
//
//    if (n>0)
//    {
//        //printf("recvline:\n---------------------\n%s\n------------------\n", recvline);
//        char *ptmp = (char*)strstr(recvline, SPLIT_STRING_FACEAIP_MYIP);
//        int ii;
//
//        if (ptmp)
//        {
//            for (ii=0; *(ptmp+ii+SPLIT_STRING_LEN_FACEAIP_MYIP)!='\0'&&*(ptmp+ii+SPLIT_STRING_LEN_FACEAIP_MYIP)!='\n'&&ii<15; ii++)
//            {
//                if (isdigit(*(ptmp+ii+SPLIT_STRING_LEN_FACEAIP_MYIP)) || *(ptmp+ii+SPLIT_STRING_LEN_FACEAIP_MYIP)=='.')
//                {
//                    pPubIP[ii] = *(ptmp+ii+SPLIT_STRING_LEN_FACEAIP_MYIP);
//                }
//                else
//                {
//                    break;
//                }
//            }
//            pPubIP[ii] = '\0';
//
//            //printf("+++++++++pPubIP:%s\n", pPubIP);
//
//            if (ez_is_valid_ip4(pPubIP))
//            {
//                iRet = 0;
//            }
//            else
//            {
//                iRet = -102;
//            }
//
//        }
//        else
//        {
//            pPubIP[0] = '\0';
//            iRet = -103;
//        }
//
//    }
//    else
//    {
//        iRet = -104;
//    }
//
//    close(sock);
//    return iRet;
//}
//
// check 2013-07-16 10:58:39 ok
int get_pub_ip_from_pubservice(char *pPubIP, size_t count)
{
    char	 recvline[256 + 1];
    int iRet = -1;

    iRet = get_pub_ip_over_http(pPubIP, count-1
                                , "pubservice.ezlibs.com"
                                , "60089"
                                , "PubService/getPubIP4"
                                , "PUIP="
                                , 5
                                , recvline
                                , sizeof(recvline));

    return iRet;
}

int get_pub_ip(char *pPubIP, size_t count)
{
    int iret = -1;

    iret = get_pub_ip_from_mb(pPubIP, count);

    if (0 != iret)
    {
        iret = get_pub_ip_from_sina(pPubIP, count);

        if (0!=iret)
        {
            //printf("get_pub_ip_from_mb and get_pub_ip_from_http failed!\n");

            iret = get_pub_ip_from_pubservice(pPubIP, count);
        }
    }

    return iret;
}

//#define PUB_NET_OVER_HTTP "pubservice.ezlibs.com"
//#define FACEAIP_PUB_NET_ON_WEB_PAGE "ip.php"
//#define SPLIT_STRING_FACEAIP_MYIP "MYIP="
//#define SPLIT_STRING_LEN_FACEAIP_MYIP 5

int get_pub_ip_over_http(char *pPubIP
                         , size_t count
                         , const char *pUrl
                         , const char *pPort
                         , const char *pLocation
                         , const char *pSpliter
                         , const int SpliterLen
                         , char *pHttpBuffer
                         , const int iBufLen)
{
    //char	recvline[DDNS_MAXLINE + 1];
    int iRet = -1;
    ez_socket_t sock ;
    int n;

    DBG(
        printf("count:%ld, url:%s, port:%s, Loc:%s, Spli:%s, len:%d\n"
               ,  count
               , pUrl
               , pPort
               , pLocation
               , pSpliter
               , SpliterLen
              );

    );



    sock = ez_tcp_client(pUrl, pPort);
    if (sock==INVALID_SOCKET)
    {
        return -100;
    }

    n = ez_http_get(sock, pUrl, pLocation, pHttpBuffer, iBufLen, 0);

    if (n>0)
    {
        //printf("pHttpBuffer:\n---------------------\n%s\n------------------\n", pHttpBuffer);
        char *ptmp = (char*)strstr(pHttpBuffer, pSpliter);
        int ii;

        if (ptmp)
        {
            for (ii=0; *(ptmp+ii+SpliterLen)!='\0'&&*(ptmp+ii+SpliterLen)!='\n'&&ii<15; ii++)
            {
                if (isdigit(*(ptmp+ii+SpliterLen)) || *(ptmp+ii+SpliterLen)=='.')
                {
                    pPubIP[ii] = *(ptmp+ii+SpliterLen);
                }
                else
                {
                    break;
                }
            }
            pPubIP[ii] = '\0';

            //printf("+++++++++pPubIP:%s\n", pPubIP);

            if (ez_is_valid_ip4(pPubIP))
            {
                iRet = 0;
            }
            else
            {
                iRet = -102;
            }

        }
        else
        {
            pPubIP[0] = '\0';
            iRet = -103;
        }

    }
    else
    {
        iRet = -104;
        DBG(
            printf("ez_http_get failed:%d\n",  n      );

        );

    }

    close(sock);
    return iRet;
}

#define IVS_UPDATEIP_KEY_STRING "Succeeded"
#define UPDATEIP_SUCCESS 1
#define UPDATEIP_FAILED  0
int __g_UpdateIP=UPDATEIP_FAILED;
#define DEFAULT_FACEAIP_UPDATESTRING 	 "/devupdae.php"
int ez_ivs_updateip(
    const char *pLogin		   ,
    const char *pPasswd        ,
    const char *pPuid          ,
    const char *pPuip          ,
    const char *pPuPort        ,
    const char *pHttpPort      ,
    const char *pPuname        ,
    const char *pMac		   ,
    const char *pAttr		   ,
    const char *pServiceHost   , //
    const char *pServicePort   , //
    const char *pServiceLoc	   ,
    char *recvline			   ,
    size_t count)
{
    //char postline[HTTP_MAXSUB + 1];
    // 根据实际计算 151 左右
#define FACEAIP_POST_STRING_LEN 256
    char postline[FACEAIP_POST_STRING_LEN + 1];

    int iRet = -1;
    ez_socket_t sock ;
    int n;
    if ( (pLogin	      == NULL)  \
         || (pPasswd      == NULL)  \
         || (pPuid        == NULL)  \
         || (pPuip        == NULL)  \
         || (pPuPort      == NULL)  \
         || (pHttpPort    == NULL)  \
         || (pPuname      == NULL)  \
         || (pMac	      == NULL)  \
         || (pAttr        == NULL)  \
         || (pServiceHost == NULL)  \
         || (pServiceLoc  == NULL)  \
       )
    {
        return -1;
    }

    // 2014-09-05 16:26:13 use para wujj
    sock = ez_tcp_client(pServiceHost, pServicePort);
    //sock = ez_tcp_client(PUB_NET_ON_WEB_FACEAIP, DEFAULT_HTTP_PORT);
    if (sock==INVALID_SOCKET)
    {
        return -100;
    }

    snprintf(postline, FACEAIP_POST_STRING_LEN, "login=%s&password=%s&puid=%s&puname=%s&puip=%s&puport=%s&httpport=%s&mac=%s&attr=%s"
             , pLogin
             , pPasswd
             , pPuid
             , pPuname
             , pPuip
             , pPuPort
             , pHttpPort
             , pMac
             , pAttr);
    // use fast mode
    //n = ez_http_post(sock, PUB_NET_ON_WEB_FACEAIP, DEFAULT_FACEAIP_UPDATESTRING, postline, recvline, count, 1);
    DBG(
        printf("postline(%ld):\n---------------------\n%s\n------------------\n", strlen(postline), postline);
    );

    n = ez_http_post(sock, pServiceHost, pServiceLoc, postline, recvline, count, 1);
    DBG(
        printf("postline ret(%d)\n", n);
    );

    if (n>0)
    {
        DBG(
            printf("recvline(%ld):\n---------------------\n%s\n------------------\n", strlen(recvline), recvline);
        );
        char *ptmp = (char*)strstr(recvline, IVS_UPDATEIP_KEY_STRING);
        if (ptmp)
        {
            iRet = 0;
            __g_UpdateIP = UPDATEIP_SUCCESS;
            //printf("updateIP success\n");
        }// if (ptmp)
        else
        {
            iRet = -101;
            __g_UpdateIP = UPDATEIP_FAILED;
            //printf("updateIP UPDATEIP_FAILED\n");
        }
    }
    else
    {
        iRet = -104;
    }

    close(sock);
    return iRet;
}

#define FACEAIP_PUB_NET_ON_SERVICE_PAGE "/service_list.php"
#define SPLIT_STRING_FACEAIP_SERVICE "<?xml"
#define END_STRING_FACEAIP_SERVICE "</ServiceList>"
int ez_get_ivs_service(
    const char * const pPuid ,
    const char * const pService,
    char *recvline,
    size_t count)
{
    ////////////////////////////////////////////////////
    // 减轻服务器压力
    int ret = -1;// 默认失败
    char strPubIP[64] = {0};

    //printf("__g_UpdateIP:%d\n", __g_UpdateIP);

    if (UPDATEIP_SUCCESS != __g_UpdateIP)
    {
        //printf("111 ++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        ret = get_pub_ip(strPubIP, sizeof(strPubIP)-1);

        if (0 == ret)
        {
            strcpy(recvline, "<?xml version=\"1.0\"?>\n<ServiceList>\n\t<ReturnMessage>\n\t\t<Code>101</Code>\n\t\t<Message>Skip Now</Message>\n\t\t<RemotIP>");
            strcat(recvline, strPubIP);
            strcat(recvline, "</RemotIP>\n\t\t<PuID>");
            strcat(recvline, pPuid);
            strcat(recvline, "</PuID>\n\t</ReturnMessage>\n</ServiceList>\n");
        }

        return ret;
    }
    ////////////////////////////////
    char postline[HTTP_MAXSUB + 1];

    int iRet = -1;
    ez_socket_t sock ;
    int n;
    if (pPuid==NULL)
    {
        return -1;
    }

    sock = ez_tcp_client(PUB_NET_ON_WEB_FACEAIP, DEFAULT_HTTP_PORT);
    if (sock==INVALID_SOCKET)
    {
        return -100;
    }

    if (pService)
    {
        snprintf(postline, HTTP_MAXSUB, "puid=%s&service=%s", pPuid, pService);
    }
    else
    {
        snprintf(postline, HTTP_MAXSUB, "puid=%s", pPuid);
    }

    n = ez_http_post(sock
                     , PUB_NET_ON_WEB_FACEAIP
                     , FACEAIP_PUB_NET_ON_SERVICE_PAGE
                     , postline
                     , recvline
                     , count
                     , 1);

    if (n>0)
    {
        //printf("recvline:\n---------------------\n%s\n------------------\n", recvline);
        char *ptmp = (char*)strstr(recvline, SPLIT_STRING_FACEAIP_SERVICE);
        if (ptmp)
        {
            //printf("ptmp:\n---------------------\n%s\n------------------\n", ptmp);
            char *ptail = (char*)strstr(ptmp, END_STRING_FACEAIP_SERVICE);
            if (ptail)
            {
                *(ptail+strlen(END_STRING_FACEAIP_SERVICE)) = '\0';

                strcpy(recvline, ptmp);
                //printf("recvline:\n---------------------\n%s\n------------------\n", recvline);
                iRet = 0;
                __g_UpdateIP = UPDATEIP_FAILED;
            }// if (ptail)
        }// if (ptmp)
    }
    else
    {
        iRet = -104;
    }

    close(sock);
    return iRet;
}
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
// 0 - ok
// < 0 NOT ok

//    char *p_error_str =
//                        iret==0    ? "OK"                     :
//                        iret==-1   ? "input not valid"        :
//                        iret==-2   ? "not rtsp"               :
//                        iret==-3   ? "host not valid"         :
//                        iret==-4   ? "host not valid"         :
//                        iret==-5   ? "port not valid"         :
//                        iret==-11  ? "network unreachable"    :
//                        iret==-101 ? "not rtsp service"       :
//                        iret==-106 ? "rtsp program not valid" : "Unknow error";

//  others, unknow error

#if 1
#define DEF_THE_BUFF_LEN 2*1024
#define DEF_TIMEOUT_SECOND 5*1000*1000

#define RTSP_LIBS "LibVLC/2.2.1 (LIVE555 Streaming Media v2014.07.25)"
//#define RTSP_LIBS "ezlibs.com/355 (ezlibs.com v2016.09.29)"
int ez_rtsp_valid(const char *p_rtsp_url, int i_step)
{
    int ret_ez_rtsp_valid = -100;

    int iret;
    ez_socket_t rtsp_cli_sock_fd = -1;
    char str_host[128] = {0};
    char str_port[32] = {0};
    char str_scheme[8] = {0};

    //int iport=0;

    if (p_rtsp_url==NULL || *p_rtsp_url=='\0')
    {
        return em_ez_rtsp_valid_PlayUrlEmpty;
    }
    DBG(
        printf("%s ...\n", "ez_parse_url");
    );

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

        if (0 != strcasecmp(str_scheme, "rtsp"))
        {
            DBG(
                printf("scheme:%s not valid.\n", str_scheme);
            );

            return em_ez_rtsp_valid_PlayUrlCanNotSupport;
        }

        if (en_true != ez_is_valid_ip4(str_host))
        {
            DBG(
                printf("host:%s not valid.\n", str_host);
            );

            return em_ez_rtsp_valid_PlayUrlIPNotValid;
        }
        // 此地址也是无效
        if (0 == strcasecmp(str_host, "0.0.0.0"))
        {
            DBG(
                printf("host:%s not valid.\n", str_host);
            );

            return em_ez_rtsp_valid_PlayUrlIPNotValid;
        }

        if (0 == atoi(str_port))
        {
            DBG(
                printf("port:%s not valid.\n", str_port);
            );

            return em_ez_rtsp_valid_PlayUrlPortNotValid;
        }
    }
    else
    {
        DBG(
            printf("failed ez_parse_url:%s\n", p_rtsp_url);
        );

        return em_ez_rtsp_valid_PlayUrlCanNotSupport;
    }

    //rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov
    rtsp_cli_sock_fd = ez_tcp_client_with_timeout(str_host, str_port, DEF_TIMEOUT_SECOND);
    //rtsp_cli_sock_fd = ez_tcp_connect(str_host, str_port, 5*1000*1000);
    if (rtsp_cli_sock_fd <= 0)
    {
        DBG(
            printf("ez_tcp_client_with_timeout failed\n");
        );

        return em_ez_rtsp_valid_PlayUrlNetworkNotFit;
    }

    DBG(
        __fline;printf("ez_tcp_client_with_timeout:%d\n", rtsp_cli_sock_fd);
    );

    //int iret;
    struct timeval my_tv =
        {
            1, 500
        };

    char recv_send_buff[DEF_THE_BUFF_LEN];
    int iSndLen = 0;
    //int iRcvLen = 0;
    do
    {
        if (i_step < 1)
        {
            ret_ez_rtsp_valid = 0;
            break;
        }

        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
        //char str_send[1024];
        char *pOPTIONS =
            "OPTIONS %s RTSP/1.0\r\n"
            "CSeq: 2\r\n"
            "User-Agent: "RTSP_LIBS"\r\n"
            "\r\n"
            ;

        snprintf(recv_send_buff, sizeof(recv_send_buff)-1, pOPTIONS, p_rtsp_url);

        DBG(
            printf("1. C-S pOPTIONS:\n-------------\n%s\n-------------\n", recv_send_buff);
        );
        iSndLen = strlen(recv_send_buff);
        iret = ez_writen(rtsp_cli_sock_fd, recv_send_buff, iSndLen);
        DBG(
            printf("ez_writen:%d\n", iret);
        );
        if (iret!=iSndLen)
            //if (iret<=0)
        {
            DBG(
                printf("ez_writen(%d) failed:%d.\n", iSndLen, iret);
            );
            ret_ez_rtsp_valid=em_ez_rtsp_valid_PlayUrlNetworkNotFit;

            break;
        }
        my_tv.tv_usec = 500;
        my_tv.tv_sec = 1;
        iret = ez_read_with_timeout(rtsp_cli_sock_fd, recv_send_buff, sizeof(recv_send_buff), &my_tv);
        DBG(
            printf("ez_read_with_timeout:%d\n", iret);
        );
        if (iret<=0)
        {
            DBG(
                printf("ez_read_with_timeout(%d) failed.\n", iret);
            );

            break;
        }
        recv_send_buff[iret] = '\0';
        DBG(
            printf("   S-C pOPTIONS ACK:\n-------------\n%s\n-------------\n", recv_send_buff);
        );

        if (strstr(recv_send_buff, "RTSP/1.0 200 OK")==NULL)
        {
            ret_ez_rtsp_valid=em_ez_rtsp_valid_PlayUrlNull;
            break;
        }
        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
        if (i_step < 2)
        {
            ret_ez_rtsp_valid = 0;
            break;
        }

        char *pDESCRIBE =
            "DESCRIBE %s RTSP/1.0\r\n"
            "CSeq: 3\r\n"
            "User-Agent: "RTSP_LIBS"\r\n"
            "Accept: application/sdp\r\n"
            "\r\n"
            ;

        snprintf(recv_send_buff, sizeof(recv_send_buff)-1, pDESCRIBE, p_rtsp_url);
        DBG(
            printf("2. C-S pDESCRIBE:\n-------------\n%s\n-------------\n", recv_send_buff);
        );
        iSndLen = strlen(recv_send_buff);
        iret = ez_writen(rtsp_cli_sock_fd, recv_send_buff, iSndLen);
        DBG(
            printf("ez_writen:%d\n", iret);
        );
        if (iret!=iSndLen)
        {
            DBG(
                printf("ez_writen(%d) failed.\n", iret);
            );

            ret_ez_rtsp_valid=em_ez_rtsp_valid_PlayUrlNull;
            break;
        }
        my_tv.tv_usec = 500;
        my_tv.tv_sec = 1;
        iret = ez_read_with_timeout(rtsp_cli_sock_fd, recv_send_buff, sizeof(recv_send_buff), &my_tv);
        DBG(
            printf("ez_read_with_timeout:%d\n", iret);
        );
        if (iret<=0)
        {
            DBG(
                printf("ez_read_with_timeout(%d) failed.\n", iret);
            );

            ret_ez_rtsp_valid=em_ez_rtsp_valid_PlayUrlNull;

            break;
        }
        recv_send_buff[iret] = '\0';
        DBG(
            printf("   S-C pDESCRIBE ACK:\n-------------\n%s\n-------------\n", recv_send_buff);
        );

        if (strstr(recv_send_buff, "RTSP/1.0 200 OK")==NULL)
        {
            ret_ez_rtsp_valid=em_ez_rtsp_valid_PlayUrlNull;
            break;
        }

        /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
        ret_ez_rtsp_valid = 0;
    }
    while(0);

    close (rtsp_cli_sock_fd);

    return ret_ez_rtsp_valid;
}
#else

#endif
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
//2017-04-18 17:13:41
ssize_t ez_xml_post(ez_socket_t sockfd, const char *host, const char *page, const char *poststr, char *recvline, int recvbuflen, int iFastMode)
{
    char sendline[HTTP_MAXSUB + 1];
    char __recvbuf[HTTP_MAXSUB+1];
    int ret;
    ssize_t n;
    ssize_t recvedbytes;
    snprintf(sendline, HTTP_MAXSUB,
             "POST %s HTTP/1.0\r\n"
             "User-Agent:Mozilla/4.0\r\n"
             "Host: %s\r\n"
             "Accept: */*\r\n"
             "Content-Type: text/xml;charset=UTF-8\r\n"
             "Content-length: %lu\r\n\r\n"
             "%s"
             , page, host, strlen(poststr), poststr);

    //DBG( printf("ez_writen:\n-----------\n%s\n---------------\n", sendline); );
    ret = ez_writen(sockfd, sendline, strlen(sendline));
    if (ret<=0)
    {
        DBG(
            printf("ez_writen error:%d\n", ret);
        );
        return ret;
    }

    *recvline = '\0';
    n = 0;
    recvedbytes = 0;
    do
    {
        struct timeval timeout =
            {
                1, 500
            };
        n = ez_read_with_timeout(sockfd, __recvbuf, HTTP_MAXSUB, &timeout);
        __recvbuf[n] = '\0';

        DBG(
            printf("recv:%ld\n", n);
        );

        // check buffer
        if (recvedbytes+n>=recvbuflen)
        {
            break;
        }

        if (n>0)
        {
            strcat (recvline, __recvbuf);
            recvedbytes+=n;
        }

        if(iFastMode==1
           && n < HTTP_MAXSUB/*加上这个让快速返回， 不必等待超时*/)
        {
            break;
        }
    }
    while (n > 0);

    return recvedbytes;
}

int ez_nvp_add_account(const char *mobilePhone)
{
    int iret = 9999;
    ez_socket_t sockfd = INVALID_SOCKET;

    if (mobilePhone==NULL || strlen(mobilePhone)!= 11 )
    {
        return -1;
    }

    do
    {
        char recvline[HTTP_MAXLINE + 1];

        char *hname = "www.189eyes.com";
        char *hport = "9000";
        char *page = "/cxf/InternalExtServices";

        //char *poststr = "<v:Envelope xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:d=\"http://www.w3.org/2001/XMLSchema\" xmlns:c=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:v=\"http://schemas.xmlsoap.org/soap/envelope/\"><v:Header /><v:Body><n0:addCustomerExtReq id=\"o0\" c:root=\"1\" xmlns:n0=\"http://www.sttri.com.cn/ns1MobileServices/\"><n0:Deptid i:type=\"d:string\">ff80808155a2635f0155b9d26725299b</n0:Deptid><n0:Mcunum i:type=\"d:int\">0</n0:Mcunum><n0:CompanyName i:type=\"d:string\">018963609588</n0:CompanyName><n0:CompanyCode i:type=\"d:string\">18963609588</n0:CompanyCode><n0:Mobilephone i:type=\"d:string\">18963609574</n0:Mobilephone><n0:Imsi i:type=\"d:string\">A00000396CB6ED</n0:Imsi><n0:TerminalCount i:type=\"d:int\">0</n0:TerminalCount></n0:addCustomerExtReq></v:Body></v:Envelope>";

        const char *mp = mobilePhone;//"18963609574";
        const char *account = mobilePhone;//"11963609589";
        const char *name = mobilePhone;//"011963609589";
        //ff80808155a2635f0155b9d26725299b //qiqihaer
        const char *area = "402881e43ecf58e2013ecf61194f0004";//suzhoushiqu
        //char name[16] = {'\0'};
        //snprintf(name, sizeof(name), "211-%s", mobilePhone);

        //char poststr[HTTP_MAXLINE + 1];
        char poststr[1024];
        snprintf(poststr
                 , sizeof(poststr),
                 "<v:Envelope xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:d=\"http://www.w3.org/2001/XMLSchema\" xmlns:c=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:v=\"http://schemas.xmlsoap.org/soap/envelope/\"><v:Header /><v:Body><n0:addCustomerExtReq id=\"o0\" c:root=\"1\" xmlns:n0=\"http://www.sttri.com.cn/ns1MobileServices/\"><n0:Deptid i:type=\"d:string\">%s</n0:Deptid><n0:Mcunum i:type=\"d:int\">0</n0:Mcunum><n0:CompanyName i:type=\"d:string\">%s</n0:CompanyName><n0:CompanyCode i:type=\"d:string\">%s</n0:CompanyCode><n0:Mobilephone i:type=\"d:string\">%s</n0:Mobilephone><n0:Imsi i:type=\"d:string\">A00000396CB6ED</n0:Imsi><n0:TerminalCount i:type=\"d:int\">0</n0:TerminalCount></n0:addCustomerExtReq></v:Body></v:Envelope>"
                 , area
                 , name
                 , account
                 , mp
                );

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
        int ret = ez_xml_post(sockfd, hname, page, poststr, recvline, HTTP_MAXLINE, 1);

        if (ret > 0)
        {
            DBG(
                printf("recv(%d):\n--------------------\n%s\n---------------------\n", ret, recvline);
            );
            char *code=strstr(recvline, "<Code>");
            if (code==NULL || strlen(code)==0)
            {
                DBG(
                    printf("ret data not valid.\n");
                );
                iret = -101;
                break;
            }

            code+=strlen("<Code>");

            DBG(
                printf("code:%s\n", code);
            );

            if (isdigit(code[0]))
            {
                iret = atoi(code);
            }
        }
        else
        {
            DBG(
                printf("ez_http_post failed:%d\n", ret);
            );
            iret = -102;
            break;
        }
    }
    while(0);

    if (sockfd!=INVALID_SOCKET)
    {
        close (sockfd);
    }
    return iret;
}

//int ez_nvp_add_account(const char *mobilePhone)
int ez_nvp_add_device(const char *p_devid, const char *p_key)
{
    int iret = 9999;
    ez_socket_t sockfd = INVALID_SOCKET;

    if (NULL==p_devid || strlen(p_devid)!= 11
        ||NULL==p_key)
    {
        return -1;
    }

    do
    {
        char recvline[HTTP_MAXLINE + 1];

        //char *hname = "www.189eyes.com";
        char *hname = "www.189eyes.com";
        char *hport = "9000";
        char *page = "/cxf/InternalExtServices";

        //char *poststr = "<v:Envelope xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:d=\"http://www.w3.org/2001/XMLSchema\" xmlns:c=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:v=\"http://schemas.xmlsoap.org/soap/envelope/\"><v:Header /><v:Body><n0:addCustomerExtReq id=\"o0\" c:root=\"1\" xmlns:n0=\"http://www.sttri.com.cn/ns1MobileServices/\"><n0:Deptid i:type=\"d:string\">ff80808155a2635f0155b9d26725299b</n0:Deptid><n0:Mcunum i:type=\"d:int\">0</n0:Mcunum><n0:CompanyName i:type=\"d:string\">018963609588</n0:CompanyName><n0:CompanyCode i:type=\"d:string\">18963609588</n0:CompanyCode><n0:Mobilephone i:type=\"d:string\">18963609574</n0:Mobilephone><n0:Imsi i:type=\"d:string\">A00000396CB6ED</n0:Imsi><n0:TerminalCount i:type=\"d:int\">0</n0:TerminalCount></n0:addCustomerExtReq></v:Body></v:Envelope>";

        const char *p_dev_name = p_devid;//"011963609589";
        //ff80808155a2635f0155b9d26725299b //qiqihaer
        const char *area = "402881e43ecf58e2013ecf61194f0004";//suzhoushiqu
        //char name[16] = {'\0'};
        //snprintf(name, sizeof(name), "211-%s", mobilePhone);

        //char poststr[HTTP_MAXLINE + 1];
        char poststr[1024];
        snprintf(poststr
                 , sizeof(poststr),
                 "<v:Envelope xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:d=\"http://www.w3.org/2001/XMLSchema\" xmlns:c=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:v=\"http://schemas.xmlsoap.org/soap/envelope/\"><v:Header /><v:Body><n0:addDevExtReq id=\"o0\" c:root=\"1\" xmlns:n0=\"http://www.sttri.com.cn/ns1MobileServices/\"><n0:Tname i:type=\"d:string\">%s</n0:Tname><n0:DeviceId i:type=\"d:string\">4ae6eca6455aae7701456eb96c3504a6</n0:DeviceId><n0:AreaId i:type=\"d:string\">%s</n0:AreaId><n0:Key i:type=\"d:string\">%s</n0:Key><n0:Imsi i:type=\"d:string\">A00000396CB6ED</n0:Imsi><n0:MediaServerType i:type=\"d:string\">0</n0:MediaServerType><n0:Code i:type=\"d:string\">%s</n0:Code><n0:ChannelNum i:type=\"d:string\">1</n0:ChannelNum></n0:addDevExtReq></v:Body></v:Envelope>"
                 , p_dev_name
                 , area
                 , p_key
                 , p_devid
                );

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
        int ret = ez_xml_post(sockfd, hname, page, poststr, recvline, HTTP_MAXLINE, 1);

        if (ret > 0)
        {
            DBG(
                printf("recv(%d):\n--------------------\n%s\n---------------------\n", ret, recvline);
            );
            char *code=strstr(recvline, "<Code>");
            if (code==NULL || strlen(code)==0)
            {
                DBG(
                    printf("ret data not valid.\n");
                );
                iret = -101;
                break;
            }

            code+=strlen("<Code>");

            DBG(
                printf("code:%s\n", code);
            );

            if (isdigit(code[0]))
            {
                iret = atoi(code);
            }
        }
        else
        {
            DBG(
                printf("ez_http_post failed:%d\n", ret);
            );
            iret = -102;
            break;
        }
    }
    while(0);

    if (sockfd!=INVALID_SOCKET)
    {
        close (sockfd);
    }
    return iret;
}

int ez_nvp_device_reg_2t(const char *p_reg_string, char *p_ret_session, char *p_ret_body)
{
    int iret = 9999;
    ez_socket_t sockfd = INVALID_SOCKET;

    if (NULL==p_reg_string
        || NULL==p_ret_session
        || NULL==p_ret_body
       )
    {
        return -1;
    }

    do
    {
        char recvline[HTTP_MAXLINE + 1];

        //char *hname = "www.189eyes.com";
        char *hname = "202.102.108.45";
        char *hport = "9000";
        char *page = "/DeviceRegisterServlet";

        //char *poststr = "<v:Envelope xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:d=\"http://www.w3.org/2001/XMLSchema\" xmlns:c=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:v=\"http://schemas.xmlsoap.org/soap/envelope/\"><v:Header /><v:Body><n0:addCustomerExtReq id=\"o0\" c:root=\"1\" xmlns:n0=\"http://www.sttri.com.cn/ns1MobileServices/\"><n0:Deptid i:type=\"d:string\">ff80808155a2635f0155b9d26725299b</n0:Deptid><n0:Mcunum i:type=\"d:int\">0</n0:Mcunum><n0:CompanyName i:type=\"d:string\">018963609588</n0:CompanyName><n0:CompanyCode i:type=\"d:string\">18963609588</n0:CompanyCode><n0:Mobilephone i:type=\"d:string\">18963609574</n0:Mobilephone><n0:Imsi i:type=\"d:string\">A00000396CB6ED</n0:Imsi><n0:TerminalCount i:type=\"d:int\">0</n0:TerminalCount></n0:addCustomerExtReq></v:Body></v:Envelope>";

        //const char *p_dev_name = p_devid;//"011963609589";
        //ff80808155a2635f0155b9d26725299b //qiqihaer
        //const char *area = "402881e43ecf58e2013ecf61194f0004";//suzhoushiqu
        //char name[16] = {'\0'};
        //snprintf(name, sizeof(name), "211-%s", mobilePhone);

        //char poststr[HTTP_MAXLINE + 1];
        char poststr[1024];
        snprintf(poststr
                 , sizeof(poststr),
                 "%s"
                 , p_reg_string
                );

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
        int ret = ez_xml_post(sockfd, hname, page, poststr, recvline, HTTP_MAXLINE, 1);

        if (ret > 0)
        {
            DBG(
                printf("recv(%d):\n--------------------\n%s\n---------------------\n", ret, recvline);
            );
            // session
            char *__session = strstr(recvline, "JSESSIONID=");
            if (__session==NULL || strlen(__session)==0)
            {

                DBG(
                    printf("ret data not valid.\n");
                );
                iret = -101;
                break;
            }

            char *__session_end = strstr(__session, "\r\n");
            if (__session_end==NULL)
            {
                DBG(
                    printf("ret data not valid.\n");
                );
                iret = -102;
                break;
            }

            memcpy(p_ret_session, __session, __session_end-__session);
            p_ret_session[__session_end-__session] = '\0';
            printf("p_ret_session:[%s]\n", p_ret_session);

            // body
            char *__body = strstr(recvline, "<?xml");
            if (__body==NULL || strlen(__body)==0)
            {
                DBG(
                    printf("ret data not valid.\n");
                );
                iret = -103;
                break;
            }

            strcpy(p_ret_body, __body);

            DBG(
                printf("p_ret_body:%s\n", p_ret_body);
            );

            //            if (isdigit(code[0]))
            //            {
            //                iret = atoi(code);
            //            }
            iret = 0;

        }
        else
        {
            DBG(
                printf("ez_http_post failed:%d\n", ret);
            );
            iret = -104;
            break;
        }
    }
    while(0);

    if (sockfd!=INVALID_SOCKET)
    {
        close (sockfd);
    }
    return iret;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
//
/*inline */int ez_megaeye_GetMonitorPlayUrl(
    const char *pHost    , /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
    const char *pPort    , /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
    const char *pAccount , /* 账号、用户名                                                           */
    const char *pPasswd  , /* 密码                                                                   */
    const char *pChannel , /* 通道号, ID号                                                           */
    char       *pPlayUrl   /* 码流类型，主码流为0（即subtype=0），辅码流为1（即subtype=1），以此类推 */
)
{
    int iret = 9999;
    ez_socket_t sockfd = INVALID_SOCKET;

    if (pHost==NULL
        || pPort==NULL
        || pAccount==NULL
        || pPasswd==NULL
        || pChannel==NULL
        || pPlayUrl==NULL
       )
    {
        return -1;
    }

    do
    {
        char recvline[HTTP_MAXLINE + 1];

        char *hname = (char *)pHost;
        char *hport = (char *)pPort;
        char *page = "/";

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
        /*
        <soapenv:Envelope xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ao="http://tempuri.org/AO.xsd">
           <soapenv:Header/>
           <soapenv:Body>
              <ao:GetMonitorPlayUrl>
                 <WSRequest>
                    <IdentityInfo>
                       <UserAccount>ckgl_wx</UserAccount>
                       <PassWord>044B383F001BB83EE435E378C350ADC6</PassWord>
                    </IdentityInfo>
                    <DeviceId>011001010561160004</DeviceId>
                    <Target>pu</Target>
                    <PID></PID>
                 </WSRequest>
              </ao:GetMonitorPlayUrl>
           </soapenv:Body>
        </soapenv:Envelope>
        */

        char out_pPasswd[EZ_STD_MD5_STRING_LEN_UP_32B+1] = {'\0'};
        int ret = ez_cal_std_md5_32b_up_string(out_pPasswd, sizeof(out_pPasswd), pPasswd, strlen(pPasswd));
        if (ret < 0)
        {
            iret = -102;
            break;
        }

        char *pPostStrModel = \
                              "<soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:ao=\"http://tempuri.org/AO.xsd\">"
                              "   <soapenv:Header/>"
                              "   <soapenv:Body>"
                              "      <ao:GetMonitorPlayUrl>"
                              "         <WSRequest>"
                              "            <IdentityInfo>"
                              "               <UserAccount>%s</UserAccount>"
                              "               <PassWord>%s</PassWord>"
                              "            </IdentityInfo>"
                              "            <DeviceId>%s</DeviceId>"
                              "            <Target>pu</Target>"
                              "            <PID></PID>"
                              "         </WSRequest>"
                              "      </ao:GetMonitorPlayUrl>"
                              "   </soapenv:Body>"
                              "</soapenv:Envelope>"
                              ;

        char poststr[1024];
        snprintf(poststr
                 , sizeof(poststr)
                 , pPostStrModel /*    ........ */
                 , pAccount
                 , out_pPasswd /* md5 enc*/
                 , pChannel
                );
DBG(
        printf("poststr------------:\n%s\n", poststr);
);
        ret = ez_xml_post(sockfd, hname, page, poststr, recvline, HTTP_MAXLINE, 1);

        if (ret > 0)
        {
            DBG(
                printf("recv(%d):\n--------------------\n%s\n---------------------\n", ret, recvline);
            );

            char *code=strstr(recvline, "rtsp://");
            if (code==NULL)
            {
                DBG(
                    printf("ret data not valid.\n");
                );
                iret = -102;
                break;
            }
            char *code_end=strstr(recvline, "</Url>");
            if (code_end==NULL)
            {
                DBG(
                    printf("ret data not valid.\n");
                );
                iret = -103;
                break;
            }
            // 截至
            code_end[0] = '\0';

            strcpy(pPlayUrl, code);

            iret = 0;
            break;
        }
        else
        {
            DBG(
                printf("ez_xml_post failed:%d\n", ret);
            );

            iret = -104;
            break;
        }
    }
    while(0);

    if (sockfd!=INVALID_SOCKET)
    {
        close (sockfd);
    }
    return iret;
}

/*inline */int ez_onvif_GetMonitorPlayUrl(
    const char *pHost    , /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
    const char *pPort    , /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
    const char *pAccount , /* 账号、用户名                                                           */
    const char *pPasswd  , /* 密码                                                                   */
    const char *pChannel , /* 通道号, ID号                                                           */
    char       *pPlayUrl   /* 码流类型，主码流为0（即subtype=0），辅码流为1（即subtype=1），以此类推 */
)
{
    return -1;
}
int ez_getStreamPlayUrl(
    const int iType      , /* 流类型，代号定义：1-流媒体  2-海康IPC 3-大华IPC  ezDef_StreamType_     */
    const char *pUrl     , /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
    const char *pAccount , /* 账号、用户名                                                           */
    const char *pPasswd  , /* 密码                                                                   */
    const char *pChannel , /* 通道号, ID号                                                           */
    const char *pSubtype , /* 码流类型，主码流为0（即subtype=0），辅码流为1（即subtype=1），以此类推 */
    char       *pPlayUrl , /* 码流类型，主码流为0（即subtype=0），辅码流为1（即subtype=1），以此类推 */
    const size_t PlayUrlBufLen
)
{
    //必须参数
    if (pPlayUrl==NULL)
    {
        return -1;
    }

    //int iType = atoi(pType);
    int iret = 0;

    char *__phost = NULL;
    char *__pport = NULL;//防止后面直接用parsed_url->port 挂掉
    int __port_http = 0;
    int __port_rtsp = 0;

    ez_parsed_url_t *parsed_url = NULL;

    ARG_USED(__port_http);

    if (pUrl && *pUrl!='\0')
    {
        //
        parsed_url = ez_parse_url(pUrl);

        do
        {
            if (NULL == parsed_url)
            {
                break;
            }
            if (NULL == parsed_url->scheme)
            {
                break;
            }
            if (NULL == parsed_url->host)
            {
                break;
            }

            __phost = parsed_url->host;
            __pport = parsed_url->port;

            // use the port
            if (!strcasecmp(parsed_url->scheme, "rtsp"))
            {
                //__port_rtsp = atoi(parsed_url->port);
                if (parsed_url->port)
                {
                    __port_rtsp = atoi(parsed_url->port);
                }
                else
                {
                    __port_rtsp = 554;
                }
            }
            else if (!strcasecmp(parsed_url->scheme, "http"))
            {
                //__port_http = atoi(parsed_url->port);
                if (parsed_url->port)
                {
                    __port_http = atoi(parsed_url->port);
                }
                else
                {
                    __port_http = 80;
                }
            }
            else if (!strcasecmp(parsed_url->scheme, "pss"))
            {
                // 防止：pss://192.168.5.66:18088?http=80&rtsp=554
                if (NULL == parsed_url->path
                    || NULL == parsed_url->query)
                {
                    break;
                }

                char *pFindPort = NULL;

                pFindPort = strstr(parsed_url->query, "http=");
                if (pFindPort)
                {
                    __port_http = atoi(pFindPort+strlen("http="));
                }

                pFindPort = strstr(parsed_url->query, "rtsp=");
                if (pFindPort)
                {
                    __port_rtsp = atoi(pFindPort+strlen("rtsp="));
                }
            }
            else // unknow
            {
                __phost = NULL;
                __pport = NULL;
                __port_http = 0;
                __port_rtsp = 0;
            }
        }
        while(0);
    }

    //内部不要直接返回， 最后需要释放资源
    switch ( iType )
    {
        case ezDef_StreamType_Streamming:
        case ezDef_StreamType_ppp:
        {
            strncpy(pPlayUrl, pUrl, PlayUrlBufLen);

            break;
        }
        case ezDef_StreamType_IPC_hikvision:
        {
            int iChn = atoi(pChannel);
            int iSubStream = atoi(pSubtype);

            if (NULL==pAccount
                || NULL==pPasswd
                || iChn<1
                || iSubStream<1 )
            {
                iret = -1*ezDef_StreamType_IPC_hikvision*100;
                break;
            }
#if 0
            sprintf(pPlayUrl
                    , "rtsp://%s:%s@%s:%d/h264/ch%d/%s/av_stream"
                    , pAccount
                    , pPasswd
                    , __phost
                    , port_rtsp
                    , iChn
                    , iSubStream>1?"sub":"main");
#else

            snprintf(pPlayUrl
                    , PlayUrlBufLen
                    , "rtsp://%s:%s@%s:%d/Streaming/Channels/%d%02d?transportmode=unicast&profile=Profile_1"
                    , pAccount
                    , pPasswd
                    , __phost
                    , __port_rtsp
                    , iChn
                    , iSubStream);
#endif

            iret = 0;
            break;
        }
        case ezDef_StreamType_hk7600:
        {
            //int iChn = atoi(pChannel);
            int iSubStream = atoi(pSubtype);

            if (NULL==pAccount
                || NULL==pPasswd
                /*|| iChn<1*/
                || iSubStream<1 )
            {
                iret = -1*ezDef_StreamType_hk7600*100;
                break;
            }

            snprintf(pPlayUrl
                    , PlayUrlBufLen
                    , "rtsp://%s:%d/hikvision://%s:1:%d?streamform=rtp&username=%s&password=%s"
                    , __phost
                    , __port_rtsp
                    , pChannel
                    , iSubStream-1 /*取用子码流和主码流*/
                    , pAccount
                    , pPasswd
                   );

            iret = 0;
            break;
        }
        case ezDef_StreamType_IPC_dahua:
        {
            int iChn = atoi(pChannel);
            int iSubStream = atoi(pSubtype);

            if (NULL==pAccount
                || NULL==pPasswd
                || iChn<1
                || iSubStream<1 )
            {
                iret = -1*ezDef_StreamType_IPC_dahua*100;
                break;
            }

            snprintf(pPlayUrl
                    , PlayUrlBufLen
                    , "rtsp://%s:%s@%s:%d/cam/realmonitor?channel=%d&subtype=%d"
                    , pAccount
                    , pPasswd
                    , __phost
                    , __port_rtsp
                    , iChn
                    , iSubStream-1/*>2?1:0*//*取用子码流和主码流*/);

            iret = 0;
            break;
        }
        case ezDef_StreamType_IPC_tdwy:
        {
            int iChn = atoi(pChannel);
            int iSubStream = atoi(pSubtype);

            if (NULL==pAccount
                || NULL==pPasswd
                || iChn<1
                || iSubStream<1 )
            {
                iret = -1*ezDef_StreamType_IPC_tdwy*100;
                break;
            }

            snprintf(pPlayUrl
                    , PlayUrlBufLen
                    , "rtsp://%s:%s@%s:%d/%d/%d"
                    , pAccount
                    , pPasswd
                    , __phost
                    , __port_rtsp
                    , iChn
                    , iSubStream);

            iret = 0;
            break;
        }
        case ezDef_StreamType_megaeye:
        {
        	if (NULL == __phost
        		|| NULL == pAccount
        		|| NULL == pPasswd
        		)
            {
DBG(
                __trip;
);

                iret = -1*ezDef_StreamType_megaeye*100;
                break;
            }

            int ret = ez_megaeye_GetMonitorPlayUrl(
                          __phost
                          , __pport//parsed_url->port
                          , pAccount
                          , pPasswd
                          , pChannel
                          , pPlayUrl
                      );

            if (ret<0)
            {
                iret = -1*ezDef_StreamType_megaeye*100;
            }
            else
            {
                iret = 0;
            }

            break;
        }
        // 睿颖
        //0--rtsp://221.226.192.193:7554/dev=JXJ-IPC-00000000/media=0/channel=0&level=0
        //1--rtsp://221.226.192.193:7554/dev=JXJ-IPC-00000000/media=0/channel=0&level=1
        case ezDef_StreamType_IPC_ryht:
        {
            int iChn = atoi(pChannel);
            int iSubStream = atoi(pSubtype);

            if (NULL==pAccount
                || NULL==pPasswd
                || iChn<1
                || iSubStream<1 )
            {
                iret = -1*ezDef_StreamType_IPC_ryht*100;
                break;
            }

            snprintf(pPlayUrl
                    , PlayUrlBufLen
                    , "rtsp://%s:%s@%s:%d/dev=JXJ-IPC-00000000/media=0/channel=%d&level=%d"
                    , pAccount
                    , pPasswd
                    , __phost
                    , __port_rtsp
                    , iChn-1//
                    , iSubStream-1/*>2?1:0*//*取用子码流和主码流*/);

            iret = 0;
            break;
        }
        // 雄迈
        // rtsp://192.168.5.74:554/user=admin_password=tlJwpbo6_channel=1_stream=0.sdp?real_stream
        // rtsp://192.168.5.74:554/user=admin_password=tlJwpbo6_channel=1_stream=1.sdp?real_stream
        case ezDef_StreamType_IPC_xm:
        {
            int iChn = atoi(pChannel);
            int iSubStream = atoi(pSubtype);

            if (NULL==pAccount
                || NULL==pPasswd
                || iChn<1
                || iSubStream<1 )
            {
                iret = -1*ezDef_StreamType_IPC_xm*100;
                break;
            }

            snprintf(pPlayUrl
                    , PlayUrlBufLen
                    , "rtsp://%s:%s@%s:%d/user=admin_password=tlJwpbo6_channel=%d_stream=%d.sdp?real_stream"
                    , pAccount
                    , pPasswd
                    , __phost
                    , __port_rtsp
                    , iChn//
                    , iSubStream-1/*>2?1:0*//*取用子码流和主码流*/);

            iret = 0;
            break;
        }
        // tp-link
        //rtsp://admin:S8GAJ2SP@192.168.5.192:554/stream1
        //rtsp://admin:S8GAJ2SP@192.168.5.192:554/stream2
        case ezDef_StreamType_IPC_tp:
        {
            int iChn = atoi(pChannel);
            int iSubStream = atoi(pSubtype);

            if (NULL==pAccount
                || NULL==pPasswd
                || iChn<1
                || iSubStream<1 )
            {
                iret = -1*ezDef_StreamType_IPC_tp*100;
                break;
            }

            snprintf(pPlayUrl
                    , PlayUrlBufLen
                    , "rtsp://%s:%s@%s:%d/stream%d"
                    , pAccount
                    , pPasswd
                    , __phost
                    , __port_rtsp
                    , iSubStream/*>2?1:0*//*取用子码流和主码流*/);

            iret = 0;
            break;
        }
        // 集光
        //rtsp://vsa.shp.ezlibs.com:16368/id=1&type=0
        //rtsp://vsa.shp.ezlibs.com:16368/id=1&type=1
        case ezDef_StreamType_IPC_jiguang:
        {
            int iChn = atoi(pChannel);
            int iSubStream = atoi(pSubtype);

            if (NULL==pAccount
                || NULL==pPasswd
                || iChn<1
                || iSubStream<1 )
            {
                iret = -1*ezDef_StreamType_IPC_jiguang*100;
                break;
            }

            snprintf(pPlayUrl
                    , PlayUrlBufLen
                    , "rtsp://%s:%s@%s:%d/id=%d&type=%d"
                    , pAccount
                    , pPasswd
                    , __phost
                    , __port_rtsp
                    , iChn
                    , iSubStream-1/*>2?1:0*//*取用子码流和主码流*/);

            iret = 0;
            break;
        }
        case ezDef_StreamType_XXXX_001:
        {
            int iChn = atoi(pChannel);
            //int iSubStream = atoi(pSubtype);

            if (/*NULL==pAccount
                || NULL==pPasswd
                || */iChn<1
                /*|| iSubStream<1 */)
            {
                iret = -1*ezDef_StreamType_XXXX_001*100;
                break;
            }

            snprintf(pPlayUrl
                    , PlayUrlBufLen
                    , "rtsp://%s:%d/%d"
                    , __phost
                    , __port_rtsp
                    , iChn);

            iret = 0;
            break;
        }
        case ezDef_StreamType_TUYA_001:
        {
            int iChn = atoi(pChannel);
            //int iSubStream = atoi(pSubtype);

            if (/*NULL==pAccount
                || NULL==pPasswd
                || */iChn<1
                /*|| iSubStream<1 */)
            {
                iret = -1*ezDef_StreamType_TUYA_001*100;
                break;
            }

            snprintf(pPlayUrl
                    , PlayUrlBufLen
                    , "rtsp://%s:%d/stream%d"
                    , __phost
                    , __port_rtsp
                    , iChn);

            iret = 0;
            break;
        }
        case ezDef_StreamType_onvif:
        {
            int ret = ez_onvif_GetMonitorPlayUrl(
                          __phost
                          , parsed_url->port
                          , pAccount
                          , pPasswd
                          , pChannel
                          , pPlayUrl
                      );
            if (ret<0)
            {
                iret = -1*ezDef_StreamType_megaeye*100;
            }
            else
            {
                iret = 0;
            }

            break;
        }
        default:
        {
            iret = -99999;

            break;
        }
    }// switch

    if (parsed_url)
    {
        ez_parsed_url_free(parsed_url);
    }

    return iret;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
//2017-06-29 8:24:29
ssize_t ez_http_post_2(ez_socket_t sockfd
                       , const char *host, const char *page, const char *poststr, char *recvline, int recvbuflen, int iFastMode)
{
    char sendline[HTTP_MAXSUB + 1];
    char __recvbuf[HTTP_MAXSUB+1];
    int ret;
    ssize_t n;
    ssize_t recvedbytes;
    snprintf(sendline, HTTP_MAXSUB,
             "POST %s HTTP/1.0\r\n"
             "User-Agent:Mozilla/4.0\r\n"
             "Host: %s\r\n"
             "Accept: */*\r\n"
             "Content-Type: text/xml;charset=UTF-8\r\n"
             "Content-length: %lu\r\n\r\n"
             "%s"
             , page, host, strlen(poststr), poststr);

    //DBG( printf("ez_writen:\n-----------\n%s\n---------------\n", sendline); );
    ret = ez_writen(sockfd, sendline, strlen(sendline));
    if (ret<=0)
    {
        DBG(
            printf("ez_writen error:%d\n", ret);
        );
        return ret;
    }

    *recvline = '\0';
    n = 0;
    recvedbytes = 0;
    do
    {
        struct timeval timeout =
            {
                1, 500
            };
        n = ez_read_with_timeout(sockfd, __recvbuf, HTTP_MAXSUB, &timeout);
        __recvbuf[n] = '\0';

        DBG(
            printf("recv:%ld\n", n);
        );

        // check buffer
        if (recvedbytes+n>=recvbuflen)
        {
            break;
        }

        if (n>0)
        {
            strcat (recvline, __recvbuf);
            recvedbytes+=n;
        }

        if(iFastMode==1
           && n < HTTP_MAXSUB/*加上这个让快速返回， 不必等待超时*/)
        {
            break;
        }
    }
    while (n > 0);

    return recvedbytes;
}

int ez_http_post_to_host(const char *p_host, const char *p_port, const char *p_page, const char *p_post_string, char *recvline, int recvbuflen, int iFastMode)
{
    int iret = 9999;
    ez_socket_t sockfd = INVALID_SOCKET;

    if (p_post_string==NULL || *p_post_string=='\0' )
    {
        return -1;
    }

    do
    {
        //char recvline[HTTP_MAXLINE + 1];

        const char *hname = p_host;
        const char *hport = p_port;
        const char *page = p_page;

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
        int ret = ez_http_post(sockfd, hname, page, p_post_string, recvline, recvbuflen, iFastMode);

        if (ret > 0)
        {
            DBG(
                printf("recv(%d):\n--------------------\n%s\n---------------------\n", ret, recvline);
            );

        }
        else
        {
            DBG(
                printf("ez_http_post failed:%d\n", ret);
            );
            iret = -102;
            break;
        }
    }
    while(0);

    if (sockfd!=INVALID_SOCKET)
    {
        close (sockfd);
    }

    return iret;
}
