/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_sockaddr.c - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_sockaddr.c 5901 2006-12-12 03:19:09Z wu_junjie $
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

/**
 *  - 
 * @ : 
 * @ :
 * return:
 *	1 success
 * Desc
 */
int
ez_inet_aton(const char *cp, struct in_addr *ap)
{
	int dots = 0;
	register unsigned long acc = 0, addr = 0;

	do
	{
		register char cc = *cp;

		switch (cc)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			{
				acc = acc * 10 + (cc - '0');
				break;
			}

			case '.':
			{
				if (++dots > 3)
				{
					return 0;
				}
				/* Fall through */
			}

			case '\0':
			{
				if (acc > 255)
				{
					return 0;
				}
				addr = addr << 8 | acc;
				acc = 0;
				break;
			}

			default:
			{
				return 0;
			}
		}
	}
	while (*cp++) ;

	/* Normalize the address */
	if (dots < 3)
	{
		addr <<= 8 * (3 - dots) ;
	}

	/* Store it if requested */
	if (ap)
	{
		ap->s_addr = htonl(addr);
	}

	return 1;
}

/**
 *  - 
 * @ : 
 * @ :
 * return:
 *	1 success
 * Desc
 */
int
ez_inet_pton(int family, const char *strptr, void *addrptr)
{
	if (family == AF_INET)
	{
		struct in_addr  in_val;

		if (ez_inet_aton(strptr, &in_val))
		//if (inet_aton(strptr, &in_val))
		{
			memcpy(addrptr, &in_val, sizeof(struct in_addr));
			return (1);
		}
		return(0);
	}

	ez_set_errno(EAFNOSUPPORT);

	return (-1);
}

/** 
 * set_address - fill in a sockaddr_in structure 
 */
int ez_set_address(const char *hname, const char *sname,
	struct sockaddr_in *sap, char *protocol )
{
	struct servent *sp;
	struct hostent *hp;
	char *endptr;
	ez_port_t port;

	ez_bzero( sap, sizeof( *sap ) );
	sap->sin_family = AF_INET;
	if ( hname != NULL )
	{
		if ( !ez_inet_aton( hname, &sap->sin_addr ) )
		{
			hp = gethostbyname( hname );
			if ( hp == NULL )
			{
				ez_err_sys("unknown host: %s\n", hname );
				return -1;
			}

			sap->sin_addr = *( struct in_addr * )hp->h_addr;
		}
	}
	else
	{
		sap->sin_addr.s_addr = htonl( INADDR_ANY );
	}

	port = (ez_port_t)strtol( sname, &endptr, 0 );
	if ( *endptr == '\0' )
	{
		sap->sin_port = htons( port );
	}
	else
	{
		sp = getservbyname( sname, protocol );
		if ( sp == NULL )
		{
			ez_err_sys("unknown service: %s\n", sname );
			return -2;
		}

		sap->sin_port = sp->s_port;
	}

	return 0;
}

en_bool	 ez_is_valid_ip4(const char *str)
{
    //struct sockaddr_in sa;
    struct in_addr in_addr_ip;
    int result = inet_pton(AF_INET, str, &(/*sa.sin_addr*/in_addr_ip));
    return (result != 0)? en_true:en_false;
} // ez_is_valid_ip4
