 /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * uClinuxPatch.c - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: skel_psos.c 1881 2006-09-24 01:39:05Z wanghf $
 *
 * _detail_explain_.
 *
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#ifdef PSOS

#include "skel_psos.h"

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	size_t		n;
	va_list		ap;

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);	/* Sigh, some vsprintf's return ptr, not length */
	n = strlen(buf);
	va_end(ap);

	return(n);
}

static char inet_ntoa_result[32];

char * inet_ntoa( struct   in_addr in )
{
	char addr_b[4];

	memcpy(addr_b, &in.s_addr, 4);//ingnore endian type

	sprintf("%d.%d.%d.%d", addr_b[0], addr_b[1], addr_b[2], addr_b[3]);

	return inet_ntoa_result;
}

struct servent * getservbyname(const char * name, const char * proto)
{
	return NULL;
};

#endif


