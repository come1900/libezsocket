/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * def_rtp.h - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: def_rtp.h 5884 2020-04-09 01:14:14Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2020-04-09 01:14:14  Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#if !defined(_def_rtp_H)
#define _def_rtp_H

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#include <endian.h>

#ifdef __WIN32__

#define PACKED
#pragma pack(push,1)

#else
#define PACKED __attribute__ ((__packed__))
#endif

//---------------------- STATIC ASSERT ----------------------------------
//Source: http://www.pixelbeat.org/programming/gcc/static_assert.html
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
/* These can't be used after statements in c89. */
#ifdef __COUNTER__
#define STATIC_ASSERT(e,m) \
    ;enum { ASSERT_CONCAT(static_assert_, __COUNTER__) = 1/(!!(e)) }
#else
  /* This can't be used twice on the same line so ensure if using in headers
   * that the headers are not included twice (by wrapping in #ifndef...#endif)
   * Note it doesn't cause an issue when used on same line of separate modules
   * compiled with gcc -combine -fwhole-program.  */
#define STATIC_ASSERT(e,m) \
    ;enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#endif

typedef struct _RTPHeader
    {
      //first byte
    #if __BYTE_ORDER == __LITTLE_ENDIAN
      unsigned int         CC:4;        /* CC field */
      unsigned int         X:1;         /* X field */
      unsigned int         P:1;         /* padding flag */
      unsigned int         version:2;
      unsigned int         PT:7;     /* PT field */
      unsigned int         M:1;       /* M field */
    #elif __BYTE_ORDER == __BIG_ENDIAN
      unsigned int         version:2;
      unsigned int         P:1;         /* padding flag */
      unsigned int         X:1;         /* X field */
      unsigned int         CC:4;        /* CC field*/
      unsigned int         M:1;         /* M field */
      unsigned int         PT:7;       /* PT field */
    #else
      #error "__BYTE_ORDER should be big or little endian."
    #endif

      unsigned int              seq_num:16;      /* length of the recovery */
      unsigned int              TS;                   /* Timestamp */
      unsigned int              ssrc;
    } RTPHeader; //12 bytes

STATIC_ASSERT (sizeof (RTPHeader) == 12, "RTPHeader size doesn't seem to be cool.");

#ifdef __WIN32__
#pragma pack(pop)
#undef PACKED

#else
#undef PACKED
#endif

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/


#endif // !defined(_def_rtp_H)