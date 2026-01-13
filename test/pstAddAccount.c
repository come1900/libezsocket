#include "ez_socket.h"

#include "./app_pub_def.h"

#include<stdio.h>

#ifndef _DEBUG_THIS
    #define _DEBUG_THIS
#endif
#ifdef _DEBUG_THIS
	#define DEB(x) x
	#define DBG(x) x
#else
	#define DEB(x)
	#define DBG(x)
#endif

#ifndef __trip
	#define __trip printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
#endif
#ifndef __fline
	#define __fline printf("%s(%d)--", __FILE__, __LINE__);
#endif

//#define ARG_USED(x) (void)&x;

int main(int argc, char *argv[])
{
    int ret;
    //ez_socket_t sockfd ;

    ret = ez_nvp_add_account("18115886802");
    printf("ez_nvp_add_account ret : %d.\n", ret);

    return 0;
}



