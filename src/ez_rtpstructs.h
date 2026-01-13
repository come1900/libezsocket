/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_rtpstructs.h - E:\joy\Megaeye\Src\Cha\PPSocket\rtp\ez_rtpstructs.h
 *
 * Copyright (C) 2007 DH Technologies, All Rights Reserved.
 *
 * $Id: ez_rtpstructs.h 1000 2007-05-05 11:43:41z wujj $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2007-05-05 11:43:41 WuJunjie 10221 Create
 * */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/


#ifndef _EZ_RTPSTRUCTS_H
#define _EZ_RTPSTRUCTS_H

typedef struct __ez_rtpheader_t
{
#ifdef _EZ_BIG_ENDIAN
	unsigned char version:2;
	unsigned char padding:1;
	unsigned char extension:1;
	unsigned char csrccount:4;

	unsigned char marker:1;
	unsigned char payloadtype:7;
#else // little endian
	unsigned char csrccount:4;
	unsigned char extension:1;
	unsigned char padding:1;
	unsigned char version:2;
	
	unsigned char payloadtype:7;
	unsigned char marker:1;
#endif // _EZ_BIG_ENDIAN
	
	unsigned short sequencenumber;
	unsigned int timestamp;
	unsigned int ssrc;
}EZ_RTPHEADER_T;
#define EZ_RTPHEADER_T_SIZE (sizeof(EZ_RTPHEADER_T))

typedef struct __ez_rtpheader_hxht_t
{
#ifdef _EZ_BIG_ENDIAN
	unsigned char version:2;
	unsigned char padding:1;
	unsigned char extension:1;
	unsigned char csrccount:4;

	unsigned char marker:1;
	unsigned char payloadtype:7;
#else // little endian
	unsigned char csrccount:4;
	unsigned char extension:1;
	unsigned char padding:1;
	unsigned char version:2;

	unsigned char payloadtype:7;
	unsigned char marker:1;
#endif // _EZ_BIG_ENDIAN

	unsigned short sequencenumber;
	unsigned int timestamp;
#if 1
	unsigned char frametype;
	unsigned char channel;
	unsigned short session;
#else // little endian
	unsigned short session;
	unsigned char channel;
	unsigned char frametype;
#endif
}EZ_RTPHEADER_HXHT_T;

#define DH_PT_4HXHT 0x04
#define DH_RTP_VER_4HXH 2

#define EZ_RTPHEADER_HXHT_T_SIZE sizeof(EZ_RTPHEADER_HXHT_T)
#define ZERO_EZ_RTPHEADER_HXHT_T(X) memset((X), 0, EZ_RTPHEADER_HXHT_T_SIZE);			\
							(X)->version = DH_RTP_VER_4HXH;	\
							(X)->payloadtype = DH_PT_4HXHT;

typedef struct __ez_rtpextensionheader
{
	unsigned short extid;
	unsigned short length;
}EZ_RTPEXTENSIONHEADER;

typedef struct __ez_rtpsourceidentifier
{
	unsigned int ssrc;
}EZ_RTPSOURCEIDENTIFIER;

typedef struct __ez_rtcpcommonheader
{
#ifdef _EZ_BIG_ENDIAN
	unsigned char version:2;
	unsigned char padding:1;
	unsigned char count:5;
#else // little endian
	unsigned char count:5;
	unsigned char padding:1;
	unsigned char version:2;
#endif // _EZ_BIG_ENDIAN

	unsigned char packettype;
	unsigned short length;
}EZ_RTCPCOMMONHEADER;

typedef struct __ez_rtcpsenderreport
{
	unsigned int ntptime_msw;
	unsigned int ntptime_lsw;
	unsigned int rtptimestamp;
	unsigned int packetcount;
	unsigned int octetcount;
}EZ_RTCPSENDERREPORT;

typedef struct __ez_rtcpreceiverreport
{
	unsigned int ssrc; // Identifies about which SSRC's data this report is...
	unsigned char fractionlost;
	unsigned char packetslost[3];
	unsigned int exthighseqnr;
	unsigned int jitter;
	unsigned int lsr;
	unsigned int dlsr;
}EZ_RTCPRECEIVERREPORT;

typedef struct __ez_rtcpsdesheader
{
	unsigned char sdesid;
	unsigned char length;
}EZ_RTCPSDESHEADER;

#endif // _RTPSTRUCTS_H
