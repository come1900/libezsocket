/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * platform_chiper.c - /home/wujj/src/libs/tutorial_ezsocket
 *
 * Copyright (C) 2003 Technologies, All Rights Reserved.
 *
 * $Id: platform_chiper.c, v 1.0.0.0 2008-7-10 11:16:18 WuJunjie Exp $
 *
 * test sys api on a new board
 * 
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include "ez_socket.h"
#include "./app_pub_def.h"

int main(int argc, char *argv[])
{
	ez_socket_t __sock;
	//int getsockname(int s, struct sockaddr *name, socklen_t *namelen);
	struct sockaddr_in __addr_in;
	SA __addr_sa;
	socklen_t addr_len;
	int iret;

//	struct addrinfo __addrinfo;
//	struct addrinfo hints;

	//char *_host = _D_SERV_IP;
	char *_port = _D_SERV_PORT_STR;
	memset(&__addr_in, 0, sizeof(struct sockaddr_in));
	memset(&__addr_sa, 0, sizeof(SA));
	addr_len = 0;

	__sock = ez_tcp_server((ez_port_t)atoi(_port), 6);

	if (__sock  < 0)
	{
		perror("ez_tcp_server");
		exit(1);
	}

	// --------------------------
	struct protoent *p = NULL;
	p = getprotobyname("tcp");
	if (p)
	{
		printf("p -> p_proto:%d\n", p->p_proto);
	}
	else
	{
		 perror("getprotobyname");
	}
	// --------------------------
	iret = getsockname(__sock, (SA *)&__addr_in, &addr_len);
	if (iret == 0)
	{
		printf("getsockname port:%d len:%d\n", ntohs(__addr_in.sin_port), addr_len);
	}
	else
	{
		 perror("getsockname");
	}

	// --------------------------

	return 0;
}
