/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_http.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_http.c 5884 2016-09-22 04:58:52Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2016-09-22 04:58:52  Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include <time.h>
#include	"ez_socket.h"

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

// if recvline NULL, do not return data
ssize_t ez_http_get(ez_socket_t sockfd, const char *host, const char *page, char *recvline, int recvbuflen, int iFastMode)
{
	char sendline[HTTP_MAXSUB + 1];
	char __recvbuf[HTTP_MAXSUB+1];
	int ret;
	ssize_t n;
	ssize_t recvedbytes;

	//snprintf(sendline, HTTP_MAXSUB,"GET http://%s/%s HTTP/1.0\r\nHost:%s\r\nUser-Agent:Mozilla/4.0\r\n\r\n", host, page, host);
	snprintf(sendline, HTTP_MAXSUB,"GET /%s HTTP/1.0\r\nHost:%s\r\nUser-Agent:Mozilla/4.0\r\n\r\n", page, host);

	DBG(printf("ez_writen:\n-----------\n%s\n---------------\n", sendline); );
	ret = ez_writen(sockfd, sendline, strlen(sendline));
	if (ret<=0)
	{
		printf("ez_writen error:%d\n", ret);
		return ret;
	}

    if (NULL != recvline)
        *recvline = '\0';

	n = 0;
	recvedbytes = 0;
	do
	{
		struct timeval timeout = {1, 500};
		n = ez_read_with_timeout(sockfd, __recvbuf, HTTP_MAXSUB, &timeout);
		__recvbuf[n] = '\0';
		DBG( printf("recv:%d\n", n); );

		// check buffer
		if (recvedbytes+n >= recvbuflen)
		{
			DBG(__fline;);
			break;
		}
		
		if (n>0)
		{
            if (NULL != recvline)
		        strcat (recvline, __recvbuf);

			recvedbytes+=n;
		}
		
		if(iFastMode==1
			&& n < HTTP_MAXSUB/*加上这个让快速返回， 不必等待超时*/)
		{
			break;
		}
	} while (n > 0);

	return recvedbytes;
}

// if recvline NULL, do not return data
ssize_t ez_http_post(ez_socket_t sockfd, const char *host, const char *page, const char *poststr, char *recvline, int recvbuflen, int iFastMode)
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
		 "Content-Type: application/x-www-form-urlencoded\r\n"
		 "Content-length: %lu\r\n\r\n"
		 "%s", page, host, strlen(poststr), poststr);

	DBG( printf("ez_writen:\n-----------\n%s\n---------------\n", sendline); );
	ret = ez_writen(sockfd, sendline, strlen(sendline));
	if (ret<=0)
	{
		printf("ez_writen error:%d\n", ret);
		return ret;
	}

    if (NULL != recvline)
	    *recvline = '\0';

	n = 0;
	recvedbytes = 0;

	do
	{
		struct timeval timeout = {1, 500};
		n = ez_read_with_timeout(sockfd, __recvbuf, HTTP_MAXSUB, &timeout);
		__recvbuf[n] = '\0';
		DBG( printf("recv:%d\n", n); );

		// check buffer
		if (recvedbytes+n>=recvbuflen)
		{
			break;
		}
		
		if (n>0)
		{
			if (NULL != recvline)
			    strcat (recvline, __recvbuf);

			recvedbytes+=n;
		}

		if(iFastMode==1
			&& n < HTTP_MAXSUB/*加上这个让快速返回， 不必等待超时*/)
		{
			break;
		}

	} while (n > 0);

	return recvedbytes;
}

