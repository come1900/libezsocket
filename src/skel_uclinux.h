/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * skel_uclinux.h - _explain_
 *
 * Copyright (C) 2005 Technologies, All Rights Reserved.
 *
 * $Id: skel_uclinux.h 5901 2006-12-12 03:19:09Z wu_junjie $
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
#ifndef _skel_uclinux_H
#define _skel_uclinux_H

#ifdef UCLINUX 
	#include <stdlib.h>
	#include <string.h>
	#include <ctype.h>
	#include <sys/socket.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/wait.h>


	#ifdef __cplusplus
	extern "C" {
	#endif

	int gethostname(char *name, size_t len);

	#ifdef __cplusplus
	}
	#endif

#define USE_STATIC_APP_CLASS 1

#endif

#endif // _skel_uclinux_H
