#include <ezsocket/ez_socket.h>

#include "./app_pub_def.h"

#include<stdio.h>

#define DEFAULT_LOGIN 				 "hello"
#define DEFAULT_PASSWD 				 "hello"
#define DEFAULT_PUID 				 "helloworld"
#define DEFAULT_PUNAME 				 "Sunflower"
#define DEFAULT_PUIP 				 "192.168.1.62"
#define DEFAULT_PUMAC 				 "52:54:4c:ea:fc:7a"
#define DEFAULT_PUPORT				 "9001"
#define DEFAULT_HTTPPORT 			 "80"
#define DEFAULT_HOST 				 "http://www.faceaip.com"
#define DEFAULT_HOST_PORT			 "80"

int main(int argc, char *argv[])
{
	int ret;
	unsigned int used_times=0;
	char strPubIP[64] = {0};
	char serviceline[HTTP_MAXSUB+4096 + 1];

	char *pLogin				;
	char *pPasswd       ;
	char *pPuid         ;
	char *pPuip         ;
	char *pPuPort       ;
	char *pHttpPort     ;
	char *pPuname       ;
	char *pMac					;
	//	char *pHostPort			;
	//	char *pUpdateString ;
	//	char *pGetIpString  ;
	int iRet;
	int ii = 0;

	pLogin				= argc>1  ? argv[1 ] :(char *)DEFAULT_LOGIN 			  ;
	pPasswd       = argc>2  ? argv[2 ] :(char *)DEFAULT_PASSWD 			  ;
	pPuid         = argc>3  ? argv[3 ] :(char *)DEFAULT_PUID 				  ;
	pPuip         = argc>4  ? argv[4 ] :(char *)DEFAULT_PUIP 			 ;
	pPuPort       = argc>5  ? argv[5 ] :(char *)DEFAULT_PUPORT 				 ;
	pHttpPort     = argc>6  ? argv[6 ] :(char *)DEFAULT_HTTPPORT			 ;
	pPuname       = argc>7  ? argv[7 ] :(char *)DEFAULT_PUNAME 		 ;

	pMac		= argc>8  ? argv[8 ] :(char *)DEFAULT_PUMAC;

	while(1)
	{
		printf("Round %8d :-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-\n", ii++);
#if 1
		//printf("UpdateIP:-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-\n");
		iRet = ez_ivs_updateip(pLogin, pPasswd, pPuid, pPuip, pPuPort, pHttpPort, pPuname, pMac, "1", serviceline,
		                       sizeof(serviceline)-1/*HTTP_MAXSUB*/);
		printf("ez_ivs_updateip:[%d]\n\n", iRet);
#endif

		printf("%s", "get_ivs_service ");
		ret = ez_get_ivs_service(
		          "helloworld" ,
		          "1",
		          serviceline,
		          sizeof(serviceline)-1/*HTTP_MAXSUB*/);

		if (ret >= 0)
		{
			printf("get:%d\n----------------------------\n%s\n---------------------\n", ret, serviceline);
		}
		else
		{
			printf("failed.\n");
		}
		
		//////////////////////
		printf("%s", "2 -- get_ivs_service ");
		ret = ez_get_ivs_service(
		          "helloworld" ,
		          "1",
		          serviceline,
		          sizeof(serviceline)-1/*HTTP_MAXSUB*/);

		if (ret >= 0)
		{
			printf("get:%d\n----------------------------\n%s\n---------------------\n", ret, serviceline);
		}
		else
		{
			printf("failed.\n");
		}
		
		ez_sleep(0, 50);
	} // while

	return 0;
}
