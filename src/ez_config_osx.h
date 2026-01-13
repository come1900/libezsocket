/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_config_linux.h - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_config_linux.h 8293 2007-04-16 09:43:17Z wu_junjie $
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

#ifndef __EZ_CONFIG_LINUX_H
#define __EZ_CONFIG_LINUX_H

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- function support +-+-+-+-+-+-+-+-+-+-+-*/

#define ez_get_errno()  errno
#define ez_set_errno(x) errno = (x)

/* Define if you have the <syslog.h> header file. */
#define HAVE_SYSLOG_H 1

/* Define if you have the <netdb.h> header file. */
/* for getaddrinfo() struct addrinfo etc */
/* 已知的Win2003+vc7 / Linux2.4+gcc2.9 等已经有该库，设置为1即可*/
/* 如果没有，则使用此处实现 undef it*/
#define HAVE_ADDRINFO_STRUCT 1

#define AF_UNSPEC_SUPPORT 1

/* Define if the system supports IP Multicast */
#define MCAST 1


/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+ os include -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */
#if TIME_WITH_SYS_TIME
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>		/* timespec{} for pselect() */
#else
#if HAVE_SYS_TIME_H
#include	<sys/time.h>	/* includes <time.h> unsafely */
#else
#include	<time.h>		/* old system? */
#endif
#endif
#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<sys/uio.h>		/* for iovec{} and readv/writev */
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>		/* for Unix domain sockets */

#include	<stdio.h>
#include	<string.h>
#include	<errno.h>
#include	<unistd.h>
#include	<sys/socket.h>
#include	<sys/types.h>
#include	<netinet/if_ether.h>
#include	<net/if.h>
#include	<sys/ioctl.h>
#include	<unistd.h>

#ifdef	HAVE_SYS_SELECT_H
# include	<sys/select.h>	/* for convenience */
#endif

#ifdef	HAVE_SYS_SYSCTL_H
#ifdef	HAVE_SYS_PARAM_H
# include	<sys/param.h>	/* OpenBSD prereq for sysctl.h */
#endif
# include	<sys/sysctl.h>
#endif

#ifdef	HAVE_POLL_H
# include	<poll.h>		/* for convenience */
#endif

#ifdef	HAVE_SYS_EVENT_H
# include	<sys/event.h>	/* for kqueue */
#endif

#ifdef	HAVE_STRINGS_H
# include	<strings.h>		/* for convenience */
#endif

/* Three headers are normally needed for socket/file ioctl's:
 * <sys/ioctl.h>, <sys/filio.h>, and <sys/sockio.h>.
 */
#ifdef	HAVE_SYS_IOCTL_H
# include	<sys/ioctl.h>
#endif
#ifdef	HAVE_SYS_FILIO_H
# include	<sys/filio.h>
#endif
#ifdef	HAVE_SYS_SOCKIO_H
# include	<sys/sockio.h>
#endif

#ifdef	HAVE_PTHREAD_H
# include	<pthread.h>
#endif

#ifdef HAVE_NET_IF_DL_H
# include	<net/if_dl.h>
#endif

#ifdef HAVE_NETINET_SCTP_H
#include	<netinet/sctp.h>
#endif

#include <netinet/tcp.h>		/* TCP_NODELAY */

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#define MAXLINE 1024
#define BUFFSIZE 1024

#define __true 1
#define __false 0

typedef int ez_socket_t;

typedef socklen_t ez_socklen_t;
typedef unsigned short ez_port_t;
typedef int ez_ssize_t;
typedef unsigned short ez_u16_t;
typedef unsigned int ez_u32_t;
typedef long ez_pid_t;
typedef unsigned short ez_mode_t;
typedef unsigned int ez_sigset_t;

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
	#include <pthread.h>
	#define WINAPI
	#define EZ_STDPREFIX WINAPI
	typedef pthread_t ez_pthread_t;
	typedef pthread_attr_t * ez_pthread_attr_t;
	typedef void * ez_threadfunc_t;
	typedef void * ez_threadparam_t;
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#endif /* CONFIG_LINUX_H */
