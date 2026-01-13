/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_telnet.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_telnet.c 5884 2019-10-18 03:25:12Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2019-10-18 03:25:12  Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include<stdio.h>

#include <ez_system_api.h>
#include <ez_fs.h>
#include <ez_socket.h>

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
#define def_Server_Host "61.147.2.34"
#define def_Server_Port "3478"

int main(int argc, char *argv[])
{
    char *pHost = argc>1?argv[1]:def_Server_Host;
    char *pPort = argc>2?argv[2]:def_Server_Port;

    ez_socket_t rtsp_cli_sock_fd = -1;

    if (argc<2)
    {
        printf("usage:%s [host=%s] [port=%s]\n", argv[0], def_Server_Host, def_Server_Port) ;
    }

    rtsp_cli_sock_fd = ez_tcp_client_with_timeout(pHost, pPort,  3*1000*1000);

    if (rtsp_cli_sock_fd <= 0)
    {
        printf("%s failed\n", argv[0]);

        return -1;
    }

    //printf("ez_tcp_client_with_timeout:%d\n", rtsp_cli_sock_fd);
    printf("%s %s:%s succeeded.\n", argv[0], pHost, pPort);

    close (rtsp_cli_sock_fd);

    return 0;
}

