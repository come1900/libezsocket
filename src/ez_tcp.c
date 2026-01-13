/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_tcp.c - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_tcp.c 10760 2007-08-08 06:18:37Z xu_geng $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2005-04-02 17:19:24 WuJunjie 10221 Create
 *		wjj 2005-04-02 15:16:48
 *			Create
 * 
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

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

ez_socket_t
ez_tcp_server(const ez_port_t port, const int listenque)
{
    ez_socket_t listenfd;
    struct sockaddr_in my_addr;
    const int yes = 1;

    listenfd = ez_socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == listenfd)
    {
        return INVALID_SOCKET;
    }

    if (INVALID_SOCKET
        == ez_setsockopt(listenfd,
                         SOL_SOCKET,
                         SO_REUSEADDR,
                         (char *) &yes,
                         sizeof(yes)) )
    {
        goto failure_ez_tcp_listen_port_t;
    }

    ez_bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (INVALID_SOCKET
        == ez_bind(listenfd, (SA *)&my_addr, sizeof(SA)) )
    {
        goto failure_ez_tcp_listen_port_t;
    }

    if (INVALID_SOCKET
        == ez_listen(listenfd, listenque) )
    {
        goto failure_ez_tcp_listen_port_t;
    }

    return listenfd;

failure_ez_tcp_listen_port_t:
    ez_close(listenfd);
    return INVALID_SOCKET;
}

int
ez_connect_nonblock(ez_socket_t fd, const SA *sa, ez_socklen_t salen, const long usec_timeout)
{
    int ret;
//printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
#ifndef WIN32
    //{
    int on = 1;
    setsockopt(fd, SOL_SOCKET,SO_REUSEADDR,(void *)&on,sizeof(on));

    //}

    // Get old flags
    int flags = 0;
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
    {
        ez_err_sys("fcntl(F_GETFL, O_NONBLOCK)");
        return -1;
    }
    // set nonblock
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        ez_err_sys("fcntl(F_SETFL, O_NONBLOCK)");
        return -1;
    }
#endif
//printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
    if (INVALID_SOCKET
        == connect(fd, sa, salen))
    {
//printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
        if (errno == EINPROGRESS)
        {
//printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
            DBG( fprintf(stderr, "EINPROGRESS in connect() - selecting\n"); )

            fd_set wds;
            FD_ZERO(&wds);
            FD_SET(fd, &wds);

            struct timeval tv;
            tv.tv_sec = usec_timeout/1000000;
            tv.tv_usec = usec_timeout%1000000;
            DBG(
                printf("conn in:%u sec %u usec ", (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);
            );
//printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
            int iRet = select(fd + 1, NULL, &wds, NULL, &tv);
//printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
            if(iRet > 0 && FD_ISSET(fd, &wds))
            {
                DBG(
                    printf("ok.\n");
                );
                ret = 0;
                goto end_of_ez_connect_nonblock;
            }
            else
            {
                DBG(
                    printf("timeout.\n");
                );
                // conn timeout
                ret = -2;
                goto end_of_ez_connect_nonblock;
                //ez_close_socket(&fd);
            }
        }
        else
        {
            //ez_close_socket(&fd);
            // conn error
            ret = -3;
            goto end_of_ez_connect_nonblock;
//printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
        }
    }
    //else
    ret = 0;
end_of_ez_connect_nonblock:
//printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
    if (fcntl(fd, F_SETFL, flags/* | O_NONBLOCK*/) == -1)
    {
        ez_err_sys("fcntl(F_SETFL, O_NONBLOCK)");
        //return en_false;
    }
//printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);

    return ret;
}

ez_socket_t
ez_tcp_connect(const char * hostip, const ez_port_t port, const long usec_timeout)
{
    ez_socket_t socketfd;
    struct sockaddr_in servaddr;


    socketfd = ez_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == socketfd)
    {
        return INVALID_SOCKET;
    }

    ez_bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if(!ez_inet_pton(AF_INET, hostip, &servaddr.sin_addr))
    {
        goto failure_ez_tcp_connect;
    }

#ifndef WIN32
    {
        int on = 1;
        setsockopt(socketfd, SOL_SOCKET,SO_REUSEADDR,(void *)&on,sizeof(on));
        ez_set_nonblock(en_true, socketfd);
    }
#endif

    if (INVALID_SOCKET
        == connect(socketfd, (SA *)&servaddr, sizeof(servaddr)))
    {
        if (errno == EINPROGRESS)
        {
            DBG( fprintf(stderr, "EINPROGRESS in connect() - selecting\n"); )

            fd_set wds;
            FD_ZERO(&wds);
            FD_SET(socketfd, &wds);

            struct timeval tv;
            tv.tv_sec = usec_timeout/1000000;
            tv.tv_usec = usec_timeout%1000000;
            DBG(
                printf("conn in:%u sec %u usec ", (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);
            );
            int iRet = select(socketfd + 1, NULL, &wds, NULL, &tv);
            if(iRet > 0 && FD_ISSET(socketfd, &wds))
            {
                DBG(
                    printf("ok.\n");
                );
            }
            else
            {
                DBG(
                    printf("timeout.\n");
                );
                ez_close_socket(&socketfd);
            }
        }
        else
        {
            ez_close_socket(&socketfd);
        }
    }
    //
    if (INVALID_SOCKET != socketfd)
    {
        ez_set_nonblock(en_false, socketfd);
    }

    return socketfd;

failure_ez_tcp_connect:
    ez_close(socketfd);
    return INVALID_SOCKET;
}

ez_socket_t ez_tcp_client(const char *hname, const char *sname)
{
    struct sockaddr_in peer;
    ez_socket_t s;
    int iret = 0;
    iret = ez_set_address( hname, sname, &peer, "tcp" );
    if (iret != 0 )
    {
        return INVALID_SOCKET;
    }
    s = ez_socket( AF_INET, SOCK_STREAM, 0 );
    if (INVALID_SOCKET == s)
    {
        return INVALID_SOCKET;
    }
    if ( 0 != ez_connect( s, ( SA * )&peer, sizeof( peer ) ) )
    {
        ez_close(s);
        return INVALID_SOCKET;
    }
    return s;
}

ez_socket_t ez_tcp_client_with_timeout(const char *hname, const char *sname, const long usec_timeout)
{
    struct sockaddr_in peer;
    ez_socket_t s;
    int iret = 0;
    iret = ez_set_address( hname, sname, &peer, "tcp" );
    if (iret != 0 )
    {
        return INVALID_SOCKET;
    }
    s = ez_socket( AF_INET, SOCK_STREAM, 0 );
    if (INVALID_SOCKET == s)
    {
        return INVALID_SOCKET;
    }
    //printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
    if ( 0 != ez_connect_nonblock( s, ( SA * )&peer, sizeof( peer ), usec_timeout) )
    {
        ez_close(s);
        return INVALID_SOCKET;
    }
    //printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
    return s;
}
