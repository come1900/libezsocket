/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_config_linux.h - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_config_psos.h 1881 2006-09-22 01:39:05Z wanghf $
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

#ifndef __EZ_CONFIG_PSOS_H
#define __EZ_CONFIG_PSOS_H

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

struct  servent {
	char    * s_name;           /* official service name */
	char    * * s_aliases;  /* alias list */
#ifdef _WIN64
	char    * s_proto;          /* protocol to use */
	short   s_port;                 /* port # */
#else
	short   s_port;                 /* port # */
	char    * s_proto;          /* protocol to use */
#endif
};

struct hostent {                /* structure for gethostbyname */
	char           *h_name;     /* official name of host */
	char          **h_aliases;  /* alias list */
	int             h_addrtype; /* host address type */
	int             h_length;   /* length of address */
	char          **h_addr_list;/* list of addresses from name server */
#define	h_addr h_addr_list[0]   /* address, for backward compatiblity */
};

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+ os include -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#include	<psos.h>
#include	<pna.h>
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<fcntl.h>		/* for nonblocking */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#define MAXLINE 1024
#define BUFFSIZE 1024

#define __true 1
#define __false 0

typedef int ez_socket_t;

typedef int ez_socklen_t;
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
	#define WINAPI
	#define EZ_STDPREFIX WINAPI
	typedef void ez_pthread_t;
	typedef void * ez_pthread_attr_t;
	typedef void * ez_threadfunc_t;
	typedef void * ez_threadparam_t;
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#endif /* CONFIG_LINUX_H */
