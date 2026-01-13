/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_appdef.h - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_appdef.h 8293 2007-04-16 09:43:17Z wu_junjie $
 *
 *   app ip/port def
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

#ifndef	__EZ_APPDEF_H
#define	__EZ_APPDEF_H

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#if defined(DBG_LIB)
#define	_D_LOCAL_NAME		"localhost"		/* TCP and UDP */
#ifdef WIN32
#define	_D_LOCAL_IPADD		"10.6.2.38"		/* TCP and UDP */
#else
#define	_D_LOCAL_IPADD		"10.6.2.38"		/* TCP and UDP */
#endif

#define	_D_SERV_IP			"10.1.30.211"	/* TCP and UDP */
#define	_D_SERV_PORT		3017			/* TCP and UDP */
#define	_D_SERV_PORT_STR	"3017"			/* TCP and UDP */

#define	_D_MCAST_IP			"239.255.42.42"	/* Multicast group */
#define	_D_MCAST_PORT		6666			/* Multicast port */
#define	_D_MCAST_PORT_STR	"6666"			/* Multicast port string */
#endif // 
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#if defined(WIN32) || defined(PSOS)
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321
#define BYTE_ORDER LITTLE_ENDIAN 
#endif
#endif

#ifdef LINUX
#include <endian.h>
#endif

#define	IP_MAXPACKET	65535		/* maximum packet size */

/*
 * Internet address (old style... should be updated)
 */
struct ez_in_addr
{
	union
	{
		struct __c_ip
		{
			unsigned char s_b1,s_b2,s_b3,s_b4;
		}
		c_ip;
		struct __s_ip
		{
			unsigned short s_w1,s_w2;
		}
		s_ip;
		unsigned int S_addr;
	} EZ_S_un;
};
/*
 * Definitions for internet protocol version 4.
 * Per RFC 791, September 1981.
 */
#define	IPVERSION	4

/*
 * Structure of an internet header, naked of options.
 *
 * We declare ip_len and ip_off to be short, rather than u_short
 * pragmatically since otherwise unsigned comparisons can result
 * against negative integers quite easily, and fail in subtle ways.
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

struct ez_iphdr
{
#if BYTE_ORDER == LITTLE_ENDIAN 
	unsigned char	ip_hl:4;		/* header length */
	unsigned char	ip_v:4;		/* version */
#endif
#if BYTE_ORDER == BIG_ENDIAN
	unsigned char	ip_v:4;			/* version */
	unsigned char	ip_hl:4;		/* header length */
#endif
	unsigned char	ip_tos;			/* type of service */
	short	ip_len;			/* total length */
	unsigned short	ip_id;			/* identification */
	short	ip_off;			/* fragment offset field */
	unsigned char	ip_ttl;			/* time to live */
	unsigned char	ip_p;			/* protocol */
	unsigned short	ip_sum;			/* checksum */
	struct	ez_in_addr ip_src,ip_dst;	/* source and dest address */
};

/*
 * TCP header.
 * Per RFC 793, September, 1981.
 */
struct ez_tcphdr {
	unsigned short	th_sport;		/* source port */
	unsigned short	th_dport;		/* destination port */
	unsigned int	th_seq;			/* sequence number */
	unsigned int	th_ack;			/* acknowledgement number */
#if BYTE_ORDER == LITTLE_ENDIAN 
	unsigned char	th_r:4;			/* reserved */
	unsigned char	th_hl:4;		/* header length */
#endif
#if BYTE_ORDER == BIG_ENDIAN 
	unsigned char	th_hl:4;		/* header length */
	unsigned char	th_r:4;			/* reserved */
#endif
	unsigned char	th_flags;
	unsigned short	th_win;			/* window */
	unsigned short	th_sum;			/* checksum */
	unsigned short	th_urp;			/* urgent pointer */
};

/* 一般如此，具体请查看网卡驱动 */
#define __MTU__ 1500
/* Eth 14 + IP 20 + TCP 20 */
#define __TCP_IP_OFFSET__ 54
/* Ethernet header Len */
#define __ETH_HL__ 14

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#define __ez_pritnf printf
#define __ez_fline __ez_pritnf("%s(%d) -- ", __FILE__, __LINE__)
#define __ez_trip __ez_pritnf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#endif /*__EZ_APPDEF_H*/
