/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * app_pub_def.h - _explain_
 *
 * Copyright (C) 2005 Technologies, All Rights Reserved.
 *
 * $Id: app_pub_def.h, v 1.0.0.1 2005-01-28 13:38:31 wjj Exp $
 *
 * _detail_explain_.
 * 
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#ifndef	_APP_PUB_DEF_H
#define	_APP_PUB_DEF_H

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#define	_D_LOCAL_NAME		"localhost"		/* TCP and UDP */
#define	_D_LOCAL_HOST		"127.0.0.1"		/* TCP and UDP */

#define	_D_SERV_IP		"10.6.3.81"		/* TCP and UDP */
#define	_D_SERV_PORT		34567			/* TCP and UDP */
#define	_D_SERV_PORT_STR	"34567"			/* TCP and UDP */

#define	_D_MCAST_IP			"239.255.42.42"	/* Multicast group */
#define	_D_MCAST_PORT		6666			/* Multicast port */
#define	_D_MCAST_PORT_STR	"6666"			/* Multicast port string */

#define _D_RUN_TIMES		1				/* */

#define _MAX_LINE_LEN		1024			/* */

#define _BUFFER_LEN_ONCE	2048
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#define SA struct sockaddr

#ifndef __trip
	#define __trip printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
#endif
#ifndef __fline
	#define __fline printf("%s(%d)--", __FILE__, __LINE__);
#endif

#define SHOW_VER printf("netowrk model suit - %s\n", argv[0]); printf("joy.woo 2003\n");

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#endif /*_APP_PUB_DEF_H*/
