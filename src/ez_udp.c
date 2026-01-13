/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_udp.c - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_udp.c 8520 2007-04-25 05:42:25Z zhu_xiaoni $
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

ez_socket_t
ez_udp_server(const ez_port_t port)
{
	ez_socket_t listenfd;
	struct sockaddr_in my_addr;
	int yes;

	listenfd = ez_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == listenfd)
	{
		return INVALID_SOCKET;
	}

	yes = 1;
	if (INVALID_SOCKET
	    == ez_setsockopt(listenfd,
	                     SOL_SOCKET,
	                     SO_REUSEADDR,
	                     (char *) &yes,
	                     sizeof(int)) )
	{
		goto failure_ez_udp_listen_port_t;
	}

	ez_bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (INVALID_SOCKET
	    == ez_bind(listenfd, (SA *)&my_addr, sizeof(SA)) )
	{
		goto failure_ez_udp_listen_port_t;
	}
	else
	{
		return listenfd;
	}

failure_ez_udp_listen_port_t:
	ez_close(listenfd);
	return INVALID_SOCKET;
}

/**
 *  - 
 * @ : 
 * @ :
 * Desc
 */
ez_socket_t
ez_udp_client(const char * hostip, const ez_port_t port, SA *saptr, ez_socklen_t *lenp)
{
	ez_socket_t socketfd;
	struct sockaddr_in servaddr;

	socketfd = ez_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == socketfd)
	{
		//printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
		return INVALID_SOCKET;
	}

	ez_bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if(ez_inet_pton(AF_INET, hostip, &servaddr.sin_addr) <= 0)
	{
		//printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
		goto failure_ez_udp_client;
	}

	//*saptr = (SA *)malloc(sizeof(struct sockaddr_in));
	if (NULL == saptr)
	{
		goto failure_ez_udp_client;
	}

	memcpy(saptr, &servaddr, sizeof(servaddr));
	
	if (NULL != lenp)
	{
		*lenp = sizeof(servaddr);
	}
	
	return socketfd;

failure_ez_udp_client:
	//	printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
	ez_close(socketfd);
	return INVALID_SOCKET;
}

/**
 *  - 
 * @ : 
 * @ :
 * Desc
 *	For a connected udp socket:
 *	we can call sendto for a connected UDP socket, 
 *	 but we cannot specify a destination address. 
 *	 The fifth argument to sendto (the pointer to the 
 *	 socket address structure) must be a null pointer, 
 *	 and the sixth argument (the size of the socket 
 *	 address structure) should be 0
 */
ez_socket_t
ez_udp_connect(const char * hostip, const ez_port_t port)
{
	ez_socket_t socketfd;
	struct sockaddr_in servaddr;

	socketfd = ez_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == socketfd)
	{
		return INVALID_SOCKET;
	}

	ez_bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if(!ez_inet_pton(AF_INET, hostip, &servaddr.sin_addr))
	{
		goto failure_ez_udp_connect;
	}

	if (INVALID_SOCKET 
		== ez_connect(socketfd, (SA *)&servaddr, sizeof(servaddr)))
	{
		goto failure_ez_udp_connect;
	}

	return socketfd;

failure_ez_udp_connect:
	ez_close(socketfd);
	return INVALID_SOCKET;
}

int
ez_mcast_set_loop(ez_socket_t sockfd, unsigned char onoff)
{
	unsigned char flag;

	flag = onoff;
	return(setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
	                  &flag, sizeof(flag)));
}

/**
 *  - 
 * @ : 
 * @ :
 * Desc
 *	For a connected udp socket:
 *	we can call sendto for a connected UDP socket, 
 *	 but we cannot specify a destination address. 
 *	 The fifth argument to sendto (the pointer to the 
 *	 socket address structure) must be a null pointer, 
 *	 and the sixth argument (the size of the socket 
 *	 address structure) should be 0
 */
ez_socket_t
ez_udp_connect_host(char *hname, char *sname)
{
	struct sockaddr_in peer;
	ez_socket_t s;

	if(0 != ez_set_address( hname, sname, &peer, "udp" ))
	{
		return INVALID_SOCKET;
	}
		
	s = ez_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == s)
	{
		return INVALID_SOCKET;
	}


	if (INVALID_SOCKET 
		== ez_connect(s, (SA *)&peer, sizeof(peer)))
	{
		goto failure_ez_udp_connect_host;
	}

	return s;

failure_ez_udp_connect_host:
	ez_close(s);
	return INVALID_SOCKET;
}

