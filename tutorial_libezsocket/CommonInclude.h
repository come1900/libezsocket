/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * CommonInclude.h - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: CommonInclude.h 0001 2012-4-11 9:32:40Z WuJunjie $
 *
 *  Explain:
 *     -Common define-
 *
 *  Update:
 *     2012-4-11 9:32:49 WuJunjie Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#ifndef _COMMONINCLUDE_H
#define _COMMONINCLUDE_H

#ifdef _MSC_VER
#pragma warning(disable:4786) // long name warning
//#pragma warning(disable: 4996)  // secure version deprecation warnings
//#pragma warning(disable: 4267)  // 64 bit signed/unsigned loss of data
#endif

#ifdef _DEBUG
	#define DEB_CODE(x) x
	#define DBG_CODE(x) x
#else
	#define DEB_CODE(x) x
	#define DBG_CODE(x)
#endif

#ifndef __trip
	#define __trip printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
#endif
#ifndef __fline
	#define __fline printf("%s(%d)--", __FILE__, __LINE__);
#endif

#define ARG_USED(x) (void)&x;

#endif // _COMMONINCLUDE_H
