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

//    ret = ez_nvp_add_account("11963609591");
//    printf("ez_nvp_add_account ret : %d.\n", ret);

    // ÃÌº”…Ë±∏
//    ret = ez_nvp_add_device("811BFC59081", "B5DD4F86");
//    printf("ez_nvp_add_device ret : %d.\n", ret);
    
    char strBody[2048];
    char strSession[2048];
    char *p_req = \
    "\
<REGISTER_REQ>\
    <ManufacturerNo>812</ManufacturerNo>\
    <Mode>DH-IPC-KW12</Mode>\
    <DevType>1</DevType>\
    <DevID>812A930E4BC</DevID>\
    <DevKey>FA0B4C24</DevKey>\
    <ChannelNum>1</ChannelNum>\
</REGISTER_REQ>\
";
	ret = ez_nvp_device_reg_2t(p_req, strSession, strBody);
    printf("ez_nvp_device_reg_2t ret : %d.\n", ret);
    if (ret == 0)
    {
    	
    	}

    return 0;
}



