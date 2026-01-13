#include "ez_socket.h"

#include "./app_pub_def.h"

#include<stdio.h>

int main()
{
	int ret;
	ez_socket_t sockfd ;
		
	char recvline[HTTP_MAXLINE + 1];

	//********** You can change. Puy any values here *******
	char *hname = "sms.ezlibs.com";
	char *hport = "60088";
	char *page = "/Sms/Register";
	char *poststr = "{ \"AuthCode\" : \"VAuthCode\", \"AuthName\" : \"VAuthName\", \"LocalDateTime\" : \"VLocalDateTime\", \"LocalIp\" : \"VLocalIp\", \"Password\" : \"VPassword\", \"ProductID\" : \"VProductID\" }\n";
	//*******************************************************

	sockfd = ez_tcp_client(hname, hport);
	
	if (sockfd==INVALID_SOCKET)
	{
		printf("failed.\n");
		return -100;
	}

	// do not use fast mode
	ret = ez_http_post(sockfd, hname, page, poststr, recvline, HTTP_MAXLINE, 0);
	
	if (ret > 0)
	{
		printf("recv(%d):\n--------------------\n%s\n---------------------\n", ret, recvline);
	}
	else
	{
		printf("ez_http_post failed:%d\n", ret);
	}
	close (sockfd);
	
	return 0;
}
