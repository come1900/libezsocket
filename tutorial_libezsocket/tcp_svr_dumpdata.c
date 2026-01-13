/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * tcp_svr_dumpdata.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: tcp_svr_dumpdata.c 5884 2015-11-23 13:23:22Z WuJunjie $
 *
 *  Explain:
 *     -
 *      tcp svr rcv anddumpbuffer
 *     -
 *
 *  Update:
 *     2003-08-15 08:49:18  Create
 *     2015-11-23 13:22:48  修改为一般用服务器， 接收数据并dumpbuffer
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include <sys/socket.h>
//#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>

#include "str_opr.h"

#include "./app_pub_def.h"

int setnonblocking(int sock)
{
    int opts;
    opts=fcntl(sock, F_GETFL);
    if(opts<0)
    {
        perror("fcntl(sock,GETFL)");
        return -1;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock, F_SETFL, opts)<0)
    {
        perror("fcntl(sock,SETFL,opts)");
        return -2;
    }

    return 0;
}
/*!
 * \prama 
 */
int req_proc(int iSocketFd, char *pPacket, int iPacketLen);

int
main(int argc, char *argv[])
{
    struct servent *servp;
    struct sockaddr_in server, remote;
    int request_sock, new_sock;
    int nfound, fd, maxfd, bytesread;
    socklen_t addrlen;
    fd_set rmask, mask;
    static struct timeval timeout =
        {
            0, 500000
        }
        ; /* one half second */
    char buf[BUFSIZ];
    char *_port = "60000";//_D_SERV_PORT_STR;

    if (argc != 2)
    {
        printf("usage: %s [port]\n", argv[0]);
        printf("default is:%s\n", _port);
    }
    _port = argc>1 ? argv[1] : _port;


    if ((request_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("socket");
        exit(1);
    }

    if (isdigit(_port[0]))
    {
        static struct servent s;
        servp = &s;
        s.s_port = htons((u_short)atoi(_port));
    }
    else if ((servp = getservbyname(_port, "tcp")) == 0)
    {
        fprintf(stderr,"%s: unknown service\n", argv[1]);
        exit(1);
    }

    int yes = 1;
    if (setsockopt(request_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }

    memset((void *) &server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = servp->s_port;
    if (bind(request_sock, (struct sockaddr *)&server, sizeof server) < 0)
    {
        perror("bind");
        exit(1);
    }
    if (listen(request_sock, SOMAXCONN) < 0)
    {
        perror("listen");
        exit(1);
    }

    setnonblocking(request_sock);

    FD_ZERO(&mask);
    FD_SET(request_sock, &mask);
    maxfd = request_sock;

    for (;;)
    {
        rmask = mask;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        nfound = select(maxfd+1, &rmask, (fd_set *)0, (fd_set *)0, &timeout);
        if (nfound < 0)
        {
            if (errno == EINTR)
            {
                printf("interrupted system call\n");
                continue;
            }
            /* something is very wrong! */
            perror("select");
            exit(1);
        }
        else if (nfound == 0)
        {
            /* timeout */
            /* printf("."); fflush(stdout);*/
            continue;
        }
        // else {}

        if (FD_ISSET(request_sock, &rmask))
        {
            /* a new connection is available on the connetion socket */
            addrlen = sizeof(remote);
            new_sock = accept(request_sock,
                              (struct sockaddr *)&remote,
                              &addrlen);
            if (new_sock < 0)
            {
                perror("accept");
                exit(1);
            }
            printf("connection from host %s, port %d, socket %d\n",
                   inet_ntoa(remote.sin_addr),
                   ntohs(remote.sin_port),
                   new_sock);

            FD_SET(new_sock, &mask);
            if (new_sock > maxfd)
            {
                maxfd = new_sock;
            }
            FD_CLR(request_sock, &rmask);
        }

        for (fd=0; fd <= maxfd ; fd++)
        {
            /* look for other sockets that have data available */
            if (FD_ISSET(fd, &rmask))
            {
                /* process the data */
                bytesread = read(fd, buf, sizeof(buf) - 1);
                if (bytesread<0)
                {
                    perror("read");
                    /* fall through */
                }
                if (bytesread<=0)
                {
                    printf("conn brocken, fd:%d\n",fd);
                    FD_CLR(fd, &mask);
                    if (close(fd))
                    {
                        perror("close");
                    }

                    continue;
                }
                buf[bytesread] = '\0';

                //printf("%d bytes:[%s]\n from %d\n",
                //       bytesread, buf, fd);

                /* echo it back */
                if( req_proc(fd, buf, bytesread) < 0)
                {
                    printf("req_proc not success.\n");
                }

            } // if (FD_ISSET(fd, &rmask))
        } //for (fd=0; fd <= maxfd ; fd++)
    } //for (;;)

    return 0;
} /* main - server.c */

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/**
 *  req_proc- 
 * @ iSocketIdx: 
 * @ pPacket:
 * @ iPacketLen:
 * Returns 0 if succeeded
 */
int req_proc(const int iFd, char * const pPacket, const int iPacketLen)
{
    int iResult=-1;

    dumpBuffer(stdout,
               (unsigned char *)pPacket,
               iPacketLen,
               SHOW_ASCII | SHOW_BINAR | SHOW_HEXAD);
    iResult = 0;


    return iResult;
}

