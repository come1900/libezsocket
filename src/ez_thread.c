/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_thread.c - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_thread.c 5901 2006-12-12 03:19:09Z wu_junjie $
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

int ez_pthread_create(ez_pthread_t *	phThread,
                      ez_pthread_attr_t	thread_attr,
                      ez_threadfunc_t (WINAPI *pStartAddress)(ez_threadparam_t),
                      ez_threadparam_t	pParameter)
{
/*
#ifdef WIN32
	static unsigned int dwThreadID;

	*phThread = CreateThread(thread_attr,
	                         0,
	                         pStartAddress,
	                         pParameter,
	                         0,
	                         &dwThreadID);
#else
	pthread_create(phThread, thread_attr, pStartAddress, pParameter);
#endif
*/
	return 0;
}
