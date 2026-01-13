/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_config_win32.h - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_config_win32.h 17308 2008-05-30 01:10:18Z zhang_yaozhen $
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

#ifndef __EZ_CONFIG_WIN32
#define __EZ_CONFIG_WIN32

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+- function support +-+-+-+-+-+-+-+-+-+-+-+-*/

#define EZ_ERROR_FUNCTION 1

/* define if snprintf prototype is in <stdio.h> */
/* #undef HAVE_SNPRINTF_PROTO */

/* Define if you have the <syslog.h> header file. */
/* #undef HAVE_SYSLOG_H */

/* Define if you have the <netdb.h> header file. */
/* for getaddrinfo() struct addrinfo etc */
/* 已知的Win2003+vc7 / Linux2.4+gcc2.9 等新已经有该库，设置为1即可*/
/* 如果没有，则使用此处实现 undef it*/
#undef HAVE_ADDRINFO_STRUCT

#define HAVE_INET_PTON 1

/* Define if the system supports IPv4 */
#define IPV4 1

/* Define if the system supports IPv6 */
/* #undef IPV6 */

/* Define if the system supports IPv4 */
#define IPv4 1

/* Define if the system supports IPv6 */
/* #undef IPv6 */

/* Define if the system supports IP Multicast */
#define MCAST 1

/* Define if you have the `bzero' function. */
/* #undef HAVE_BZERO */

/* if support AF_UNSPEC. you define  */
/* #undef AF_UNSPEC_SUPPORT */

/*-+-+-+-+-+-+-+-+-+-+-+-+-+ os include -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#define AS_HAS_WINSOCK2 0

/* Always use WS2 when available */
#if !defined(AS_HAS_WINSOCK2)
 #define AS_HAS_WINSOCK2 1
#endif

/*#if defined (AS_HAS_WINSOCK2) && (AS_HAS_WINSOCK2 != 0)
	#include <winsock2.h>
#else
#endif*/

#ifndef _SIZE_T_DEFINED
typedef unsigned int   size_t;
#define _SIZE_T_DEFINED
#endif //_SIZE_T_DEFINED

typedef void *					LPVOID;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../Include/DVRDEF.H"

	/*
	* Basic system type definitions, taken from the BSD file sys/types.h.
	*/
	typedef unsigned char   u_char;
	typedef unsigned short  u_short;
	typedef unsigned int    u_int;
	typedef unsigned long   u_long;
	typedef unsigned __int64 u_int64;


	typedef int SOCKET;
	typedef int socklen_t;


	/*
	* Protocols
	*/
#define IPPROTO_IP              0               /* dummy for IP */
#define IPPROTO_HOPOPTS         0               /* IPv6 hop-by-hop options */
#define IPPROTO_ICMP            1               /* control message protocol */
#define IPPROTO_IGMP            2               /* internet group management protocol */
#define IPPROTO_GGP             3               /* gateway^2 (deprecated) */
#define IPPROTO_IPV4            4               /* IPv4 */
#define IPPROTO_TCP             6               /* tcp */
#define IPPROTO_PUP             12              /* pup */
#define IPPROTO_UDP             17              /* user datagram protocol */
#define IPPROTO_IDP             22              /* xns idp */
#define IPPROTO_IPV6            41              /* IPv6 */
#define IPPROTO_ROUTING         43              /* IPv6 routing header */
#define IPPROTO_FRAGMENT        44              /* IPv6 fragmentation header */
#define IPPROTO_ESP             50              /* IPsec ESP header */
#define IPPROTO_AH              51              /* IPsec AH */
#define IPPROTO_ICMPV6          58              /* ICMPv6 */
#define IPPROTO_NONE            59              /* IPv6 no next header */
#define IPPROTO_DSTOPTS         60              /* IPv6 destination options */
#define IPPROTO_ND              77              /* UNOFFICIAL net disk proto */
#define IPPROTO_ICLFXBM         78

#define IPPROTO_RAW             255             /* raw IP packet */
#define IPPROTO_MAX             256

	/*
	* Port/socket numbers: network standard functions
	*/
#define IPPORT_ECHO             7
#define IPPORT_DISCARD          9
#define IPPORT_SYSTAT           11
#define IPPORT_DAYTIME          13
#define IPPORT_NETSTAT          15
#define IPPORT_FTP              21
#define IPPORT_TELNET           23
#define IPPORT_SMTP             25
#define IPPORT_TIMESERVER       37
#define IPPORT_NAMESERVER       42
#define IPPORT_WHOIS            43
#define IPPORT_MTP              57

	/*
	* Port/socket numbers: host specific functions
	*/
#define IPPORT_TFTP             69
#define IPPORT_RJE              77
#define IPPORT_FINGER           79
#define IPPORT_TTYLINK          87
#define IPPORT_SUPDUP           95

	/*
	* UNIX TCP sockets
	*/
#define IPPORT_EXECSERVER       512
#define IPPORT_LOGINSERVER      513
#define IPPORT_CMDSERVER        514
#define IPPORT_EFSSERVER        520

	/*
	* UNIX UDP sockets
	*/
#define IPPORT_BIFFUDP          512
#define IPPORT_WHOSERVER        513
#define IPPORT_ROUTESERVER      520
	/* 520+1 also used */

	/*
	* Ports < IPPORT_RESERVED are reserved for
	* privileged processes (e.g. root).
	*/
#define IPPORT_RESERVED         1024



	/*
	* Definitions of bits in internet address integers.
	* On subnets, the decomposition of addresses to host and net parts
	* is done according to subnet mask, not the masks here.
	*/
#define IN_CLASSA(i)            (((long)(i) & 0x80000000) == 0)
#define IN_CLASSA_NET           0xff000000
#define IN_CLASSA_NSHIFT        24
#define IN_CLASSA_HOST          0x00ffffff
#define IN_CLASSA_MAX           128

#define IN_CLASSB(i)            (((long)(i) & 0xc0000000) == 0x80000000)
#define IN_CLASSB_NET           0xffff0000
#define IN_CLASSB_NSHIFT        16
#define IN_CLASSB_HOST          0x0000ffff
#define IN_CLASSB_MAX           65536

#define IN_CLASSC(i)            (((long)(i) & 0xe0000000) == 0xc0000000)
#define IN_CLASSC_NET           0xffffff00
#define IN_CLASSC_NSHIFT        8
#define IN_CLASSC_HOST          0x000000ff

#define IN_CLASSD(i)            (((long)(i) & 0xf0000000) == 0xe0000000)
#define IN_CLASSD_NET           0xf0000000       /* These ones aren't really */
#define IN_CLASSD_NSHIFT        28               /* net and host fields, but */
#define IN_CLASSD_HOST          0x0fffffff       /* routing needn't know.    */
#define IN_MULTICAST(i)         IN_CLASSD(i)

#define INADDR_ANY              (unsigned long)0x00000000
#define INADDR_LOOPBACK         0x7f000001
#define INADDR_BROADCAST        (unsigned long)0xffffffff
#define INADDR_NONE             0xffffffff

#define ADDR_ANY                INADDR_ANY

#define WSADESCRIPTION_LEN      256
#define WSASYS_STATUS_LEN       128


	/*
	* Definitions related to sockets: types, address families, options,
	* taken from the BSD file sys/socket.h.
	*/

	/*
	* This is used instead of -1, since the
	* SOCKET type is unsigned.
	*/
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

	/*
	* The  following  may  be used in place of the address family, socket type, or
	* protocol  in  a  call  to WSASocket to indicate that the corresponding value
	* should  be taken from the supplied WSAPROTOCOL_INFO structure instead of the
	* parameter itself.
	*/
#define FROM_PROTOCOL_INFO (-1)

	/*
	* Types
	*/
#define SOCK_STREAM     1               /* stream socket */
#define SOCK_DGRAM      2               /* datagram socket */
#define SOCK_RAW        3               /* raw-protocol interface */
#define SOCK_RDM        4               /* reliably-delivered message */
#define SOCK_SEQPACKET  5               /* sequenced packet stream */

	/*
	* Option flags per-socket.
	*/
#define SO_DEBUG        0x0001          /* turn on debugging info recording */
#define SO_ACCEPTCONN   0x0002          /* socket has had listen() */
#define SO_REUSEADDR    0x0004          /* allow local address reuse */
#define SO_KEEPALIVE    0x0008          /* keep connections alive */
#define SO_DONTROUTE    0x0010          /* just use interface addresses */
#define SO_BROADCAST    0x0020          /* permit sending of broadcast msgs */
#define SO_USELOOPBACK  0x0040          /* bypass hardware when possible */
#define SO_LINGER       0x0080          /* linger on close if data present */
#define SO_OOBINLINE    0x0100          /* leave received OOB data in line */

#define SO_DONTLINGER   (int)(~SO_LINGER)
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR)) /* disallow local address reuse */

	/*
	* Additional options.
	*/
#define SO_SNDBUF       0x1001          /* send buffer size */
#define SO_RCVBUF       0x1002          /* receive buffer size */
#define SO_SNDLOWAT     0x1003          /* send low-water mark */
#define SO_RCVLOWAT     0x1004          /* receive low-water mark */
#define SO_SNDTIMEO     0x1005          /* send timeout */
#define SO_RCVTIMEO     0x1006          /* receive timeout */
#define SO_ERROR        0x1007          /* get error status and clear */
#define SO_TYPE         0x1008          /* get socket type */

	/*
	* WinSock 2 extension -- new options
	*/
#define SO_GROUP_ID       0x2001      /* ID of a socket group */
#define SO_GROUP_PRIORITY 0x2002      /* the relative priority within a group*/
#define SO_MAX_MSG_SIZE   0x2003      /* maximum message size */
#define SO_PROTOCOL_INFOA 0x2004      /* WSAPROTOCOL_INFOA structure */
#define SO_PROTOCOL_INFOW 0x2005      /* WSAPROTOCOL_INFOW structure */
#ifdef UNICODE
#define SO_PROTOCOL_INFO  SO_PROTOCOL_INFOW
#else
#define SO_PROTOCOL_INFO  SO_PROTOCOL_INFOA
#endif /* UNICODE */
#define PVD_CONFIG        0x3001       /* configuration info for service provider */
#define SO_CONDITIONAL_ACCEPT 0x3002   /* enable true conditional accept: */
	/*  connection is not ack-ed to the */
	/*  other side until conditional */
	/*  function returns CF_ACCEPT */
	/*
	* TCP options.
	*/
#define TCP_NODELAY     0x0001

	/*
	* Address families.
	*/
#define AF_UNSPEC       0               /* unspecified */
	/*
	* Although  AF_UNSPEC  is  defined for backwards compatibility, using
	* AF_UNSPEC for the "af" parameter when creating a socket is STRONGLY
	* DISCOURAGED.    The  interpretation  of  the  "protocol"  parameter
	* depends  on the actual address family chosen.  As environments grow
	* to  include  more  and  more  address families that use overlapping
	* protocol  values  there  is  more  and  more  chance of choosing an
	* undesired address family when AF_UNSPEC is used.
	*/
#define AF_UNIX         1               /* local to host (pipes, portals) */
#define AF_INET         2               /* internetwork: UDP, TCP, etc. */
#define AF_IMPLINK      3               /* arpanet imp addresses */
#define AF_PUP          4               /* pup protocols: e.g. BSP */
#define AF_CHAOS        5               /* mit CHAOS protocols */
#define AF_NS           6               /* XEROX NS protocols */
#define AF_IPX          AF_NS           /* IPX protocols: IPX, SPX, etc. */
#define AF_ISO          7               /* ISO protocols */
#define AF_OSI          AF_ISO          /* OSI is ISO */
#define AF_ECMA         8               /* european computer manufacturers */
#define AF_DATAKIT      9               /* datakit protocols */
#define AF_CCITT        10              /* CCITT protocols, X.25 etc */
#define AF_SNA          11              /* IBM SNA */
#define AF_DECnet       12              /* DECnet */
#define AF_DLI          13              /* Direct data link interface */
#define AF_LAT          14              /* LAT */
#define AF_HYLINK       15              /* NSC Hyperchannel */
#define AF_APPLETALK    16              /* AppleTalk */
#define AF_NETBIOS      17              /* NetBios-style addresses */
#define AF_VOICEVIEW    18              /* VoiceView */
#define AF_FIREFOX      19              /* Protocols from Firefox */
#define AF_UNKNOWN1     20              /* Somebody is using this! */
#define AF_BAN          21              /* Banyan */
#define AF_ATM          22              /* Native ATM Services */
#define AF_INET6        23              /* Internetwork Version 6 */
#define AF_CLUSTER      24              /* Microsoft Wolfpack */
#define AF_12844        25              /* IEEE 1284.4 WG AF */
#define AF_IRDA         26              /* IrDA */
#define AF_NETDES       28              /* Network Designers OSI & gateway
	enabled protocols */
#define AF_TCNPROCESS   29
#define AF_TCNMESSAGE   30
#define AF_ICLFXBM      31

#define AF_MAX          32



/*
* Level number for (get/set)sockopt() to apply to socket itself.
*/
#define SOL_SOCKET      0xffff          /* options for socket level */

/*
* Maximum queue length specifiable by listen.
*/
#define SOMAXCONN       0x7fffffff

#define MSG_OOB         0x1             /* process out-of-band data */
#define MSG_PEEK        0x2             /* peek at incoming message */
#define MSG_DONTROUTE   0x4             /* send without using routing tables */

#define MSG_PARTIAL     0x8000          /* partial send or recv for message xport */

/*
* WinSock 2 extension -- new flags for WSASend(), WSASendTo(), WSARecv() and
*                          WSARecvFrom()
*/
#define MSG_INTERRUPT   0x10            /* send/recv in the interrupt context */

#define MSG_MAXIOVLEN   16



/* options for fcntl */

#define O_NDELAY 				0x04        /* non-blocking */
#define FNDELAY					O_NDELAY    /* synonym */
#define F_GETFL					3           /* get flags */
#define F_SETFL					4           /* set flags */

/*
 * Commands for ioctlsocket(),  taken from the BSD file fcntl.h.
 *
 *
 * Ioctl's have the command encoded in the lower word,
 * and the size of any in or out parameters in the upper
 * word.  The high 2 bits of the upper word are used
 * to encode the in/out status of the parameter; for now
 * we restrict parameters to at most 128 bytes.
 */
#define IOCPARM_MASK    0x7f            /* parameters must be < 128 bytes */
#define IOC_VOID        0x20000000      /* no parameters */
#define IOC_OUT         0x40000000      /* copy out parameters */
#define IOC_IN          0x80000000      /* copy in parameters */
#define IOC_INOUT       (IOC_IN|IOC_OUT)
                                        /* 0x20000000 distinguishes new &
                                           old ioctl's */
#define _IO(x,y)        (IOC_VOID|((x)<<8)|(y))

#define _IOR(x,y,t)     (IOC_OUT|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))

#define _IOW(x,y,t)     (IOC_IN|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))

/* options for ioctl */

#define SIOCATMARK				7           /* check for out of bound data */
#define FIONREAD    _IOR('f', 127, u_long) /* get # bytes to read */
#define FIONBIO     _IOW('f', 126, u_long) /* set/clear non-blocking i/o */

/* structures */

struct sockaddr {               /* generic socket address */
    unsigned short  sa_family;  /* address family */
    char            sa_data[14];/* up to 14 bytes of direct address */
};
struct in_addr {                /* Internet address */
    unsigned long   s_addr;
};
struct sockaddr_in {            /* Internet socket address */
    short           sin_family; /* should be unsigned but this is BSD */
    unsigned short  sin_port;   /* network order !!! */
    struct in_addr  sin_addr;
    char            sin_zero[8];
};

struct iovec {                  /* address and length */
    char           *iov_base;   /* base */
    int             iov_len;    /* size */
};
struct msghdr {                 /* Message header for recvmsg and sendmsg. */
    char           *msg_name;   /* optional address */
    int             msg_namelen;/* size of address */
    struct iovec   *msg_iov;    /* scatter/gather array */
    int             msg_iovlen; /* # elements in msg_iov */
    char           *msg_accrights;  /* access rights sent/received */
    int             msg_accrightslen;
};

struct  hostent {
	char     * h_name;           /* official name of host */
	char     *  * h_aliases;  /* alias list */
	short   h_addrtype;             /* host address type */
	short   h_length;               /* length of address */
	char     *  * h_addr_list; /* list of addresses */
#define h_addr  h_addr_list[0]          /* address, for backward compat */
};

/*
* It is assumed here that a network number
* fits in 32 bits.
*/
struct  netent {
	char    * n_name;           /* official name of net */
	char    * * n_aliases;  /* alias list */
	short   n_addrtype;             /* net address type */
	u_long  n_net;                  /* network # */
};

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

struct  protoent {
	char    * p_name;           /* official protocol name */
	char    * * p_aliases;  /* alias list */
	short   p_proto;                /* protocol # */
};


/*
** The maximum number of socket descriptors that USNET can have is the same
**   as the number of possible connections, NCONNS.
*/
#ifndef FD_SETSIZE
#define FD_SETSIZE      64
#endif /* FD_SETSIZE */

typedef struct fd_set {
	u_int fd_count;               /* how many are SET? */
	SOCKET  fd_array[FD_SETSIZE];   /* an array of SOCKETs */
} fd_set;

#undef far
#undef near
#undef pascal

#define far
#define near

#define PASCAL      __stdcall

#ifndef FAR
#define FAR                 far
#endif

extern int PASCAL FAR __WSAFDIsSet(SOCKET, fd_set FAR *);

#define FD_CLR(fd, set) do { \
	u_int __i; \
	for (__i = 0; __i < ((fd_set FAR *)(set))->fd_count ; __i++) { \
	if (((fd_set FAR *)(set))->fd_array[__i] == fd) { \
	while (__i < ((fd_set FAR *)(set))->fd_count-1) { \
	((fd_set FAR *)(set))->fd_array[__i] = \
	((fd_set FAR *)(set))->fd_array[__i+1]; \
	__i++; \
	} \
	((fd_set FAR *)(set))->fd_count--; \
	break; \
	} \
	} \
} while(0)

#define FD_SET(fd, set) do { \
	u_int __i; \
	for (__i = 0; __i < ((fd_set FAR *)(set))->fd_count; __i++) { \
	if (((fd_set FAR *)(set))->fd_array[__i] == (fd)) { \
	break; \
	} \
	} \
	if (__i == ((fd_set FAR *)(set))->fd_count) { \
	if (((fd_set FAR *)(set))->fd_count < FD_SETSIZE) { \
	((fd_set FAR *)(set))->fd_array[__i] = (fd); \
	((fd_set FAR *)(set))->fd_count++; \
	} \
	} \
} while(0)

#define FD_ZERO(set) (((fd_set FAR *)(set))->fd_count=0)

#define FD_ISSET(fd, set) __WSAFDIsSet((SOCKET)(fd), (fd_set FAR *)(set))

/*
* Structure used in select() call, taken from the BSD file sys/time.h.
*/
struct timeval {
	long    tv_sec;         /* seconds */
	long    tv_usec;        /* and microseconds */
};



/*
** Multicast
*/
struct ip_mreq {
    struct in_addr imr_multiaddr;   /* IP class D multicast address */
    struct in_addr imr_interface;   /* IP address of local interface */
};
/*
* Options for use with [gs]etsockopt at the IP level.
*/

#if (_MSC_VER >= 1400)
#define IP_OPTIONS      1 /* set/get IP options */
#define IP_HDRINCL      2 /* header is included with data */
#define IP_TOS          3 /* IP type of service and preced*/
#define IP_TTL          4 /* IP time to live */
#define IP_MULTICAST_IF     9 /* set/get IP multicast i/f  */
#define IP_MULTICAST_TTL       10 /* set/get IP multicast ttl */
#define IP_MULTICAST_LOOP      11 /*set/get IP multicast loopback */
#define IP_ADD_MEMBERSHIP      12 /* add an IP group membership */
#define IP_DROP_MEMBERSHIP     13/* drop an IP group membership */
#define IP_DONTFRAGMENT     14 /* don't fragment IP datagrams */
#define IP_ADD_SOURCE_MEMBERSHIP  15 /* join IP group/source */
#define IP_DROP_SOURCE_MEMBERSHIP 16 /* leave IP group/source */
#define IP_BLOCK_SOURCE           17 /* block IP group/source */
#define IP_UNBLOCK_SOURCE         18 /* unblock IP group/source */
#define IP_PKTINFO                19 /* receive packet information for ipv4*/
#define IP_RECEIVE_BROADCAST      22 /* allow/block broadcast reception */
#elif (_MSC_VER >= 1300)

#define IP_OPTIONS      1 /* set/get IP options */
#define IP_HDRINCL      2 /* header is included with data */
#define IP_TOS          3 /* IP type of service and preced*/
#define IP_TTL          4 /* IP time to live */
#define IP_MULTICAST_IF     9 /* set/get IP multicast i/f  */
#define IP_MULTICAST_TTL       10 /* set/get IP multicast ttl */
#define IP_MULTICAST_LOOP      11 /*set/get IP multicast loopback */
#define IP_ADD_MEMBERSHIP      12 /* add an IP group membership */
#define IP_DROP_MEMBERSHIP     13/* drop an IP group membership */
#define IP_DONTFRAGMENT     14 /* don't fragment IP datagrams */
#define IP_ADD_SOURCE_MEMBERSHIP  15 /* join IP group/source */
#define IP_DROP_SOURCE_MEMBERSHIP 16 /* leave IP group/source */
#define IP_BLOCK_SOURCE           17 /* block IP group/source */
#define IP_UNBLOCK_SOURCE         18 /* unblock IP group/source */
#define IP_PKTINFO                19 /* receive packet information for ipv4*/
#define IP_RECEIVE_BROADCAST      22 /* allow/block broadcast reception */

#elif

#define IP_OPTIONS          1           /* set/get IP per-packet options    */
#define IP_MULTICAST_IF     2           /* set/get IP multicast interface   */
#define IP_MULTICAST_TTL    3           /* set/get IP multicast timetolive  */
#define IP_MULTICAST_LOOP   4           /* set/get IP multicast loopback    */
#define IP_ADD_MEMBERSHIP   5           /* add  an IP group membership      */
#define IP_DROP_MEMBERSHIP  6           /* drop an IP group membership      */
#define IP_TTL              7           /* set/get IP Time To Live          */
#define IP_TOS              8           /* set/get IP Type Of Service       */
#define IP_DONTFRAGMENT     9           /* set/get IP Don't Fragment flag   */

#endif

#define IP_DEFAULT_MULTICAST_TTL   1    /* normally limit m'casts to 1 hop  */
#define IP_DEFAULT_MULTICAST_LOOP  1    /* normally hear sends if a member  */
#define IP_MAX_MEMBERSHIPS         20   /* per socket; must fit in one mbuf */

/*
* WinSock error codes are also defined in winerror.h
* Hence the IFDEF.
*/
#ifndef WSABASEERR

/*
* All Windows Sockets error constants are biased by WSABASEERR from
* the "normal"
*/
#define WSABASEERR              10000

/*
* Windows Sockets definitions of regular Microsoft C error constants
*/
#define WSAEINTR                (WSABASEERR+4)
#define WSAEBADF                (WSABASEERR+9)
#define WSAEACCES               (WSABASEERR+13)
#define WSAEFAULT               (WSABASEERR+14)
#define WSAEINVAL               (WSABASEERR+22)
#define WSAEMFILE               (WSABASEERR+24)

/*
* Windows Sockets definitions of regular Berkeley error constants
*/
#define WSAEWOULDBLOCK          (WSABASEERR+35)
#define WSAEINPROGRESS          (WSABASEERR+36)
#define WSAEALREADY             (WSABASEERR+37)
#define WSAENOTSOCK             (WSABASEERR+38)
#define WSAEDESTADDRREQ         (WSABASEERR+39)
#define WSAEMSGSIZE             (WSABASEERR+40)
#define WSAEPROTOTYPE           (WSABASEERR+41)
#define WSAENOPROTOOPT          (WSABASEERR+42)
#define WSAEPROTONOSUPPORT      (WSABASEERR+43)
#define WSAESOCKTNOSUPPORT      (WSABASEERR+44)
#define WSAEOPNOTSUPP           (WSABASEERR+45)
#define WSAEPFNOSUPPORT         (WSABASEERR+46)
#define WSAEAFNOSUPPORT         (WSABASEERR+47)
#define WSAEADDRINUSE           (WSABASEERR+48)
#define WSAEADDRNOTAVAIL        (WSABASEERR+49)
#define WSAENETDOWN             (WSABASEERR+50)
#define WSAENETUNREACH          (WSABASEERR+51)
#define WSAENETRESET            (WSABASEERR+52)
#define WSAECONNABORTED         (WSABASEERR+53)
#define WSAECONNRESET           (WSABASEERR+54)
#define WSAENOBUFS              (WSABASEERR+55)
#define WSAEISCONN              (WSABASEERR+56)
#define WSAENOTCONN             (WSABASEERR+57)
#define WSAESHUTDOWN            (WSABASEERR+58)
#define WSAETOOMANYREFS         (WSABASEERR+59)
#define WSAETIMEDOUT            (WSABASEERR+60)
#define WSAECONNREFUSED         (WSABASEERR+61)
#define WSAELOOP                (WSABASEERR+62)
#define WSAENAMETOOLONG         (WSABASEERR+63)
#define WSAEHOSTDOWN            (WSABASEERR+64)
#define WSAEHOSTUNREACH         (WSABASEERR+65)
#define WSAENOTEMPTY            (WSABASEERR+66)
#define WSAEPROCLIM             (WSABASEERR+67)
#define WSAEUSERS               (WSABASEERR+68)
#define WSAEDQUOT               (WSABASEERR+69)
#define WSAESTALE               (WSABASEERR+70)
#define WSAEREMOTE              (WSABASEERR+71)

/*
* Extended Windows Sockets error constant definitions
*/
#define WSASYSNOTREADY          (WSABASEERR+91)
#define WSAVERNOTSUPPORTED      (WSABASEERR+92)
#define WSANOTINITIALISED       (WSABASEERR+93)
#define WSAEDISCON              (WSABASEERR+101)
#define WSAENOMORE              (WSABASEERR+102)
#define WSAECANCELLED           (WSABASEERR+103)
#define WSAEINVALIDPROCTABLE    (WSABASEERR+104)
#define WSAEINVALIDPROVIDER     (WSABASEERR+105)
#define WSAEPROVIDERFAILEDINIT  (WSABASEERR+106)
#define WSASYSCALLFAILURE       (WSABASEERR+107)
#define WSASERVICE_NOT_FOUND    (WSABASEERR+108)
#define WSATYPE_NOT_FOUND       (WSABASEERR+109)
#define WSA_E_NO_MORE           (WSABASEERR+110)
#define WSA_E_CANCELLED         (WSABASEERR+111)
#define WSAEREFUSED             (WSABASEERR+112)

/*
* Error return codes from gethostbyname() and gethostbyaddr()
* (when using the resolver). Note that these errors are
* retrieved via WSAGetLastError() and must therefore follow
* the rules for avoiding clashes with error numbers from
* specific implementations or language run-time systems.
* For this reason the codes are based at WSABASEERR+1001.
* Note also that [WSA]NO_ADDRESS is defined only for
* compatibility purposes.
*/

/* Authoritative Answer: Host not found */
#define WSAHOST_NOT_FOUND       (WSABASEERR+1001)

/* Non-Authoritative: Host not found, or SERVERFAIL */
#define WSATRY_AGAIN            (WSABASEERR+1002)

/* Non-recoverable errors, FORMERR, REFUSED, NOTIMP */
#define WSANO_RECOVERY          (WSABASEERR+1003)

/* Valid name, no data record of requested type */
#define WSANO_DATA              (WSABASEERR+1004)

/*
* Define QOS related error return codes
*
*/
#define  WSA_QOS_RECEIVERS               (WSABASEERR + 1005)
/* at least one Reserve has arrived */
#define  WSA_QOS_SENDERS                 (WSABASEERR + 1006)
/* at least one Path has arrived */
#define  WSA_QOS_NO_SENDERS              (WSABASEERR + 1007)
/* there are no senders */
#define  WSA_QOS_NO_RECEIVERS            (WSABASEERR + 1008)
/* there are no receivers */
#define  WSA_QOS_REQUEST_CONFIRMED       (WSABASEERR + 1009)
/* Reserve has been confirmed */
#define  WSA_QOS_ADMISSION_FAILURE       (WSABASEERR + 1010)
/* error due to lack of resources */
#define  WSA_QOS_POLICY_FAILURE          (WSABASEERR + 1011)
/* rejected for administrative reasons - bad credentials */
#define  WSA_QOS_BAD_STYLE               (WSABASEERR + 1012)
/* unknown or conflicting style */
#define  WSA_QOS_BAD_OBJECT              (WSABASEERR + 1013)
/* problem with some part of the filterspec or providerspecific
* buffer in general */
#define  WSA_QOS_TRAFFIC_CTRL_ERROR      (WSABASEERR + 1014)
/* problem with some part of the flowspec */
#define  WSA_QOS_GENERIC_ERROR           (WSABASEERR + 1015)
/* general error */
#define  WSA_QOS_ESERVICETYPE            (WSABASEERR + 1016)
/* invalid service type in flowspec */
#define  WSA_QOS_EFLOWSPEC               (WSABASEERR + 1017)
/* invalid flowspec */
#define  WSA_QOS_EPROVSPECBUF            (WSABASEERR + 1018)
/* invalid provider specific buffer */
#define  WSA_QOS_EFILTERSTYLE            (WSABASEERR + 1019)
/* invalid filter style */
#define  WSA_QOS_EFILTERTYPE             (WSABASEERR + 1020)
/* invalid filter type */
#define  WSA_QOS_EFILTERCOUNT            (WSABASEERR + 1021)
/* incorrect number of filters */
#define  WSA_QOS_EOBJLENGTH              (WSABASEERR + 1022)
/* invalid object length */
#define  WSA_QOS_EFLOWCOUNT              (WSABASEERR + 1023)
/* incorrect number of flows */
#define  WSA_QOS_EUNKOWNPSOBJ            (WSABASEERR + 1024)
/* unknown object in provider specific buffer */
#define  WSA_QOS_EPOLICYOBJ              (WSABASEERR + 1025)
/* invalid policy object in provider specific buffer */
#define  WSA_QOS_EFLOWDESC               (WSABASEERR + 1026)
/* invalid flow descriptor in the list */
#define  WSA_QOS_EPSFLOWSPEC             (WSABASEERR + 1027)
/* inconsistent flow spec in provider specific buffer */
#define  WSA_QOS_EPSFILTERSPEC           (WSABASEERR + 1028)
/* invalid filter spec in provider specific buffer */
#define  WSA_QOS_ESDMODEOBJ              (WSABASEERR + 1029)
/* invalid shape discard mode object in provider specific buffer */
#define  WSA_QOS_ESHAPERATEOBJ           (WSABASEERR + 1030)
/* invalid shaping rate object in provider specific buffer */
#define  WSA_QOS_RESERVED_PETYPE         (WSABASEERR + 1031)
/* reserved policy element in provider specific buffer */

/*
* WinSock error codes are also defined in winerror.h
* Hence the IFDEF.
*/
#endif /* ifdef WSABASEERR */

#define EWOULDBLOCK WSAEWOULDBLOCK


#define WSADESCRIPTION_LEN      256
#define WSASYS_STATUS_LEN       128

typedef struct WSAData {
	unsigned short                    wVersion;
	unsigned short                    wHighVersion;
#ifdef _WIN64
	unsigned short          iMaxSockets;
	unsigned short          iMaxUdpDg;
	char *              lpVendorInfo;
	char                    szDescription[WSADESCRIPTION_LEN+1];
	char                    szSystemStatus[WSASYS_STATUS_LEN+1];
#else
	char                    szDescription[WSADESCRIPTION_LEN+1];
	char                    szSystemStatus[WSASYS_STATUS_LEN+1];
	unsigned short          iMaxSockets;
	unsigned short          iMaxUdpDg;
	char *              lpVendorInfo;
#endif
} WSADATA;

typedef WSADATA *LPWSADATA;

int __stdcall WSAStartup(
						   WORD wVersionRequired,
						   LPWSADATA lpWSAData);

int __stdcall WSACleanup(void);

void __stdcall WSASetLastError( int iError);

int __stdcall WSAGetLastError(void);

BOOL __stdcall WSAIsBlocking(void);

int __stdcall WSAUnhookBlockingHook(void);

typedef int (* FARPROC)();
FARPROC WSASetBlockingHook( FARPROC lpBlockFunc);

int __stdcall WSACancelBlockingCall(void);


int __stdcall WSACancelAsyncRequest( HANDLE hAsyncTaskHandle);


int __stdcall WSARecvEx (
						   SOCKET s,
						   char * buf,
						   int len,
						    int *flags);

/* function prototypes */

int __stdcall socket(int domain, int type, int protocol);
int __stdcall shutdown(SOCKET s, int how);
int __stdcall closesocket(int conno);
int __stdcall select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval * timeout);
int __stdcall accept(SOCKET s, struct sockaddr * name, int *namelen);
int __stdcall bind(SOCKET s, const struct sockaddr * name, int namelen);
int __stdcall connect(SOCKET s, const struct sockaddr * name, int namelen);
int __stdcall getsockname(SOCKET s, struct sockaddr * name, int *namelen);
int __stdcall getpeername(SOCKET s, struct sockaddr * peer, int *addrlen);
int	__stdcall gethostname(char *name, size_t len);
int __stdcall getsockopt(SOCKET s, int level, int optname, char *optval, int *optlen);
int __stdcall setsockopt(SOCKET s, int level, int optname, const char *optval, int optlen);
int __stdcall listen(SOCKET s, int backlog);
int __stdcall recv(SOCKET s, char *buf, int len, int flags);

int __stdcall recvfrom(SOCKET s, char *buf, int len, int flags, struct sockaddr * from, int *fromlen);
int __stdcall recvmsg(SOCKET s, struct msghdr * msg, int flags);
int __stdcall send(SOCKET s, const char *buf, int len, int flags);

int __stdcall sendto(SOCKET s, char *buf, int len, int flags, struct sockaddr * to, int tolen);
int __stdcall sendmsg(SOCKET s, struct msghdr * msg, int flags);
struct hostent * __stdcall gethostbyname_r(char *hnp, struct hostent * result,
           char *buffer, int buflen, int *h_errnop);
struct hostent * __stdcall gethostbyname(const char *hnp);
struct hostent *__stdcall gethostbyaddr_r(char *addr, int len, int type, struct hostent * result, char *buffer, int buflen, int *h_errnop);
struct hostent *__stdcall gethostbyaddr(char *addr, int len, int type);

struct servent * __stdcall getservbyport(int port, const char * proto);

struct servent * __stdcall getservbyname(const char * name, const char * proto);

struct protoent * __stdcall getprotobynumber(int proto);

struct protoent * __stdcall getprotobyname(const char * name);

int __stdcall fcntlsocket(int fildes, int cmd, int arg);
int __stdcall ioctlsocket (SOCKET s, long cmd, u_long *argp);
void __stdcall sleepsocket(int sec);
unsigned long  __stdcall  inet_addr(char *dotted);
char * __stdcall __stdcall inet_ntoa(struct in_addr addr);
int __stdcall closesocket(SOCKET s);

unsigned long __stdcall ntohl (unsigned long netlong);
unsigned short __stdcall ntohs (unsigned short netshort);
unsigned long __stdcall htonl (unsigned long hostlong);
unsigned short __stdcall htons (unsigned short hostshort);

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+ lib +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

	#pragma comment(lib, "ws2_32.lib")
	#pragma comment(lib, "mswsock.lib")
	#pragma comment(lib, "wsock32.lib")

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
typedef int ez_socket_t;
typedef int ez_socklen_t;
typedef unsigned short ez_port_t;
typedef int ez_ssize_t;
typedef unsigned short ez_u16_t;
typedef unsigned int ez_u32_t;
typedef long ez_pid_t;
typedef unsigned short ez_mode_t;
typedef unsigned int ez_sigset_t;

typedef char* caddr_t;

#define ez_get_errno()  WSAGetLastError()
#define ez_set_errno(x) WSASetLastError((x))
#    define EAFNOSUPPORT            WSAEAFNOSUPPORT
#	 define ECONNRESET				WSAECONNRESET

/*-+-+-+-+-+-+-+-+-+-+- signal +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#if !defined (SIGALRM)
#  define SIGALRM 0
#endif /* SIGALRM */

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#  if ! defined (EINTR)
#    define EINTR        4        /* interrupted system call     */
#  endif /* EINTR */

#  if ! defined (ENOTCONN)
#define ENOTCONN                WSAENOTCONN
#  endif /* ENOTCONN */

#  if ! defined (ETIME)
#    define ETIME        62        /* interrupted system call     */
#  endif /* ETIME */

#if !defined (ETIMEDOUT) && defined (ETIME)
#  define ETIMEDOUT ETIME
#endif /* ETIMEDOUT */

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#define 	 LOG_INFO 1
#define 	 LOG_ERR 1

#define 	 MAXLINE 1024
#define 	 BUFFSIZE 1024

#define 	 __true 1
#define 	 __false 0

static int 	 __socket_initialized	= __false;
typedef struct _SECURITY_ATTRIBUTES {
	DWORD nLength;
	LPVOID lpSecurityDescriptor;
	BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#define WINAPI
#define EZ_STDPREFIX WINAPI
typedef HANDLE					ez_pthread_t;
typedef LPSECURITY_ATTRIBUTES	ez_pthread_attr_t;
typedef DWORD					ez_threadfunc_t;
typedef LPVOID					ez_threadparam_t;	
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

//通用函数定义
#define snprintf _snprintf
#endif /* __EZ_CONFIG_WIN32 */
