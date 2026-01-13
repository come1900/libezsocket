 /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * uClinuxPatch.c - _explain_
 *
 * Copyright (C) 2005 DAHUA Technologies, All Rights Reserved.
 *
 * $Id: skel_uclinux.c 5901 2006-12-12 03:19:09Z wu_junjie $
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
#ifdef UCLINUX

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#include "skel_uclinux.h"

int gethostname(char *name, size_t len)
{
	strcpy(name, "localhost");

	return 0;
}

#endif


