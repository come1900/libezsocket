/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * skel_uclinux.h - _explain_
 *
 * Copyright (C) 2005 Technologies, All Rights Reserved.
 *
 * $Id: skel_psos.h 1881 2006-09-24 01:39:05Z whf $
 *
 * _detail_explain_.
 * 
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#ifndef _skel_psos_h
#define _skel_psos_h

#ifdef PSOS 

	#ifdef __cplusplus
	extern "C" {
	#endif

	#include <string.h>
	#include <stdlib.h>
	#include <stdarg.h>		/* ANSI C header file */
	#include <pna.h>

	void myprintf(char * format, ...);
	int snprintf(char *buf, size_t size, const char *fmt, ...);
	char * inet_ntoa( struct   in_addr in );

	#ifdef __cplusplus
	}
	#endif

#define USE_STATIC_APP_CLASS 1

#endif

#endif // _skel_psos_h
