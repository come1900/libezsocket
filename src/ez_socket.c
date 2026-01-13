/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_socket.c - ez_socket wrapper
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_socket.c 14059 2008-01-21 06:03:21Z zhang_yaozhen $
 *
 * ez_socket wrapper functions.
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2005-04-02 17:19:24 WuJunjie 10221 Create
 *		wjj 2005-04-02 15:16:48
 *			Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include	"ez_socket.h"
#include <string.h>

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

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

ez_socket_t
ez_socket(int family, int type, int protocol)
{
	ez_socket_t		n;

	n = socket(family, type, protocol);

	if (INVALID_SOCKET == n)
	{
		ez_err_sys("socket error");
	}

	return n;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int
ez_bind(ez_socket_t fd, const SA *sa, ez_socklen_t salen)
{
	ez_socket_t n;

	n = bind(fd, sa, salen);

	if (SOCKET_ERROR == n)
	{
		ez_err_sys("bind error");
	}

	return n;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int
ez_listen(ez_socket_t fd, int backlog)
{
	int n;

	n = listen(fd, backlog);

	if (SOCKET_ERROR == n)
	{
		ez_err_sys("listen error");
	}

	return n;
}


/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

ez_socket_t
ez_accept(ez_socket_t fd, SA *sa, ez_socklen_t *salenptr)
{
	ez_socket_t		n;

	n = accept(fd, sa, salenptr);

	if (SOCKET_ERROR == n)
	{
		ez_err_sys("accept error");
	}

	return n;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int
ez_connect(ez_socket_t fd, const SA *sa, ez_socklen_t salen)
{
	int n;

	n = connect(fd, sa, salen);
	if (SOCKET_ERROR == n)
	{
		ez_err_sys("connect error ");
	}

	return n;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int 
ez_read_with_timeout(int sockfd, void *buf, size_t count, struct timeval *ptime_out)
{
	int bytesread = 0;
	int nfound;

	fd_set rmask;
	FD_ZERO(&rmask);
	FD_SET(sockfd, &rmask);

	nfound = select(FD_SETSIZE, &rmask, (fd_set *)0, (fd_set *)0, ptime_out);
	if (nfound < 0)
	{
		if (errno == EINTR)
		{
			printf("interrupted system call\n");
			return -2;
		}
		perror("select");
		return -3;
	}
	else if (nfound == 0)
	{
		/* timer expired */
		//printf("time out!\n");
		//continue;
		return -1;
	}

	if (FD_ISSET(sockfd,&rmask))
	{
		/* data from network */
		bytesread = read(sockfd, buf, count);
		if (0 == bytesread)
		{
			DBG(printf("conn brocken.\n"););
			return -4;
		}
		else if (bytesread < 0)
		{
			DBG(printf("read:%d, count:%d, errno %d.\n", bytesread, count, errno ););
			//__trip;
			return -5;
		}
		else
		{
			//recvline[bytesread] = '\0';
			//printf("got %d bytes: %s\n", bytesread, recvline);
		} //if (0 == bytesread)
	}//if (FD_ISSET(sock,&rmask))

	return bytesread;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int ez_socket_init(void)
{
#if (defined (WINCE) || defined (WIN32))
	WSADATA 	wsaData;
	WORD		wVersionRequested;

	//Modified by yehao(10857) 2007-3-2:启动的版本被修改。
#ifdef WINCE
	wVersionRequested = MAKEWORD(1,1);
#else
	wVersionRequested = MAKEWORD(2,2);
#endif;

	if (__socket_initialized == __true)
	{
		return 0;
	}

	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		return -1;
	}
	if (wsaData.wVersion != wVersionRequested)
	{
		WSACleanup();
		return -1;
	}

	__socket_initialized = __true;
#endif

	return 0;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int ez_socket_cleanup(void)
{

#if (defined (WINCE) || defined (WIN32))
	if (__socket_initialized == __false)
	{
		return 0;
	}

	if (WSACleanup () != 0)
	{
		return -1;
	}

	__socket_initialized = __false;
#endif

	return 0;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int ez_close(ez_socket_t fd)
{
#if (defined (WINCE) || defined (WIN32) || defined(PSOS))
	return closesocket(fd);
#else

	return close(fd);
#endif
}

int ez_close_socket(ez_socket_t *fd)
{
	int i;

	i = ez_close(*fd);
	*fd = INVALID_SOCKET;

	return i;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

struct hostent *
			ez_gethostbyname(const char * name)
{
	struct hostent * sh;

	sh = gethostbyname(name);
	if (NULL == sh)
	{
		ez_err_sys("gethostbyname error");
	}

	return sh;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int
ez_getpeername(ez_socket_t fd, SA *sa, ez_socklen_t *salenptr)
{
	int n;

	n = getpeername(fd, sa, salenptr);

	if (SOCKET_ERROR == n)
	{
		ez_err_sys("getpeername error");
	}

	return n;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int
ez_getsockname(ez_socket_t fd, SA *sa, ez_socklen_t *salenptr)
{
	int n;

	n = getsockname(fd, sa, salenptr);

	if (SOCKET_ERROR == n)
	{
		ez_err_sys("getsockname error");
	}

	return n;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int
ez_gethostname(char *name, size_t len)
{
	return gethostname(name, len);
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int
ez_getsockopt(ez_socket_t fd, 
				int level, 
				int optname, 
				void *optval, 
				ez_socklen_t *optlenptr)
{
	int n;

	n = getsockopt(fd, level, optname, optval, optlenptr);

	if (SOCKET_ERROR == n)
	{
		ez_err_sys("getsockopt error");
	}

	return n;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int
ez_setsockopt(ez_socket_t fd, 
				int level, 
				int optname, 
				const void *optval, 
				ez_socklen_t optlen)
{
	int n;

	n = setsockopt(fd, level, optname, optval, optlen);

	if (SOCKET_ERROR == n)
	{
		ez_err_sys("setsockopt error");
	}

	return n;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int
ez_shutdown(ez_socket_t fd, int how)
{
	int n;

	n = shutdown(fd, how);

	if (SOCKET_ERROR == n)
	{
		ez_err_sys("shutdown error");
	}

	return n;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int
ez_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
          struct timeval *timeout)
{
	int		n;

	n = select(nfds, readfds, writefds, exceptfds, timeout);

	if (SOCKET_ERROR == n)
	{
		ez_err_sys("select error");
	}

	return n;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

en_bool ez_set_nodelay(ez_socket_t s)
{
	const int optval = 1;

	if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
	               (char *)&optval, sizeof(optval)) < 0)
	{
		ez_err_sys("error setsockopt nodelay");
		return en_false;
	}

	return en_true;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

en_bool ez_set_nonblock(en_bool bNb, ez_socket_t s)
{
#ifdef _WIN32
	unsigned long l = bNb ? 1 : 0;
	int n = ioctlsocket(s, FIONBIO, &l);
	if (n != 0)
	{
		int errcode;
		errcode = WSAGetLastError();
		ez_err_sys("ioctlsocket(FIONBIO) (%d) ", errcode);
		return en_false;
	}
#else
	int flags = 0;
	if ((flags = fcntl(s, F_GETFL, 0)) == -1)
	{
		ez_err_sys("fcntl(F_GETFL, O_NONBLOCK)");
		return en_false;
	}

	if (bNb)
	{
		if (fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			ez_err_sys("fcntl(F_SETFL, O_NONBLOCK)");
			return en_false;
		}
	}
	else
	{
		if (fcntl(s, F_SETFL, 0) == -1)
		{
			ez_err_sys("fcntl(F_SETFL, 0)");
			return en_false;
		}
	}
#endif

	return en_true;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#ifndef __trip
#define __trip printf("-W-%s(%d)\n", __FILE__, __LINE__);
#endif
#ifndef __fline
#define __fline printf("%s(%d)--", __FILE__, __LINE__);
#endif

int ez_writen(ez_socket_t fd, const void *vptr, size_t n)
{
	size_t		nleft;
	int		nwritten;
	char	*ptr;

	ptr = (char *)vptr;
	nleft = n;

	while (nleft > 0)
	{
		nwritten = send(fd, ptr, nleft, 0);

		if ( nwritten < 0)
		{
			int ierrno = 0;
			/*Begin: Add by yehao(10857) 2007-3-2*/
#ifdef WIN32
			ierrno = WSAGetLastError();
#else
			ierrno = errno;
#endif
			/*End: yehao(10857) 2007-3-2*/
			if (EWOULDBLOCK == ierrno)
			{
				nwritten = 0;		/* and call write() again */
				//__trip;
				continue;
			}
			else if (EINTR == ierrno)
			{
				//__trip;
				continue;
			}
			else
			{
				//perror ("send");
				return -10;
			}
		}
		else if (0 == nwritten)
		{
			//__trip;
			break;
		}
		else
		{
			nleft -= nwritten;
			ptr   += nwritten;
		}
	}

	return (ptr-(char *)vptr);
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int ez_writeton( ez_socket_t fd, const void *vptr, size_t n, const SA *saRemote, int struct_size)
{
	size_t nleft    = 0;
	int nwritten    = 0;
	const char *ptr = NULL;
	ptr = vptr;

	if(NULL == vptr || NULL == saRemote || NULL == ptr)
	{
		return -1;
	}

	nleft = n;
	while (nleft > 0)
	{
		if ( (nwritten = sendto(fd, (char *)ptr, nleft, 0, (SA *)saRemote, struct_size)) <= 0)
		{
			int ierrno = 0;
#ifdef WIN32
			ierrno = WSAGetLastError();
#else
			ierrno = errno;
#endif
			if (nwritten < 0 && EINTR == ierrno)
			{
				nwritten = 0;// and call write() again
			}
			else
			{
				perror ("send");
				return(-1);// error
			}
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
