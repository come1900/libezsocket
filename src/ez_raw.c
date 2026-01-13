/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_raw.c - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_raw.c 5901 2006-12-12 03:19:09Z wu_junjie $
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

ez_socket_t ez_ip_snif_socket(const char * device)
{
#ifdef LINUX
	ez_socket_t sock;
	struct ifreq ethreq;
	if ( (sock=socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0)
	{
		ez_err_sys("socket");
		return INVALID_SOCKET;
	}

	/* Set the network card in promiscuos mode */
	strncpy(ethreq.ifr_name, device, IFNAMSIZ);
	if (-1 == ioctl(sock,SIOCGIFFLAGS, &ethreq))
	{
		ez_err_sys("ioctl");
		ez_close(sock);
		return INVALID_SOCKET;
	}

	ethreq.ifr_flags |= IFF_PROMISC;
	if (-1 == ioctl(sock, SIOCSIFFLAGS, &ethreq))
	{
		ez_err_sys("ioctl");
		ez_close(sock);
		return INVALID_SOCKET;
	}

	return sock;
#else
	return INVALID_SOCKET;
#endif
}

