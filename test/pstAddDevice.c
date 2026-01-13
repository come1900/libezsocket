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

    // ÃÌº”…Ë±∏
    ret = ez_nvp_add_device("811BFC59082", "B5DD4F86");
    printf("ez_nvp_add_device ret : %d.\n", ret);
    


    return 0;
}



