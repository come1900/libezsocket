/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * rtp_rcv_udp.c - _explain_
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: rtp_rcv_udp.c 5884 2019-12-12 04:23:22Z WuJunjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2019-12-12 04:23:22  Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include<stdio.h>

#include "str_opr.h"
#include "ez_socket.h"

#include "def_rtp.h"

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
	#define __trip  printf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
#endif
#ifndef __fline
	#define __fline printf("%s(%d)--", __FILE__, __LINE__);
#endif

#define ARG_USED(x) (void)&x;

#define UDP_MAXPKT         1500
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

//#define _Func_Save_PsStream 1

#define def_Server_Port "44444"

int main(int argc, char *argv[])
{
    char *pPort = argc>1?argv[1]:def_Server_Port;

    int iret = 9999;

    ez_port_t port_recv = atoi(pPort);

    ez_socket_t sock_fd_recv = INVALID_SOCKET;
    int cnt=0;

    if (argc<2)
    {
        printf("usage:%s [port=%s]\n", argv[0], def_Server_Port) ;
    }

    printf("on udp:%d\n", port_recv) ;

    //ez_socket_t sock_fd_send = INVALID_SOCKET;
    //char * hostip="127.0.0.1";
    //ez_port_t port;
    //SA *saptr;
    //ez_socklen_t *lenp;


#if defined (_Func_Save_PsStream)

    FILE *fp1=fopen("stream_gb.ps", "wb+");
    ARG_USED(fp1);
#endif //_Func_Save_PsStream

    do
    {
        sock_fd_recv = ez_udp_server(port_recv);

        if (sock_fd_recv==INVALID_SOCKET)
        {
            DBG(
                __fline;
                printf("ez_udp_server failed.\n");
            );
            iret = -100;
            break;
        }

        for(;;)
        {
            struct sockaddr_in   cliaddr;
            socklen_t len = sizeof (struct sockaddr_in);
            char                recvData[UDP_MAXPKT] = {0};

            /* waiting for receive data */
            int pktsize = recvfrom(sock_fd_recv, recvData, UDP_MAXPKT, 0, (struct sockaddr *)(&cliaddr), &len);

#if 0
            //printf("%ld - R:%d \n", time(NULL), n);
            struct sockaddr_in *p_peer_addr = (struct sockaddr_in *)(&cliaddr);

            printf("%ld - %s:%d, %4d Bytes\n",
                   time(NULL),
                   inet_ntoa(p_peer_addr->sin_addr),
                   ntohs(p_peer_addr->sin_port),
                   pktsize);
#endif //

            /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
            int parse_rtp=1;
            int parse_mpegts=1;
            ARG_USED(parse_mpegts);
            if (pktsize > 0)
            {
                //printf("Addr:%s\r\n",inet_ntoa(remoteAddr.sin_addr));
                //printf("packet size:%d\r\n",pktsize);
                //Parse RTP
                //
                if(parse_rtp!=0)
                {
                    char payload_str[16]={0};
                    RTPHeader rtp_header;
                    const int rtp_header_size=sizeof(RTPHeader);
                    
                    //printf("rtp_header_size:%d\n", rtp_header_size);

                    //RTP Header
                    memcpy((void *)&rtp_header,recvData,rtp_header_size);

                    //RFC3351
                    char payload=rtp_header.PT;
                    switch(payload)
                    {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                        case 8:
                        case 9:
                        case 10:
                        case 11:
                        case 12:
                        case 13:
                        case 14:
                        case 15:
                        case 16:
                        case 17:
                        case 18:
                        {
                            sprintf(payload_str,"Audio");
                            break;
                        }
                        case 31:
                        {
                            sprintf(payload_str,"H.261");
                            break;
                        }
                        case 32:
                        {
                            sprintf(payload_str,"MPV");
                            break;
                        }
                        case 33:
                        {
                            sprintf(payload_str,"MP2T");
                            break;
                        }
                        case 34:
                        {
                            sprintf(payload_str,"H.263");
                            break;
                        }
                        case 96:
                        {
                            sprintf(payload_str,"H.264/PS");
                            break;
                        }
                        default:
                        {
                            sprintf(payload_str,"other");
                            break;
                        }
                    }

                    unsigned int timestamp=ntohl(rtp_header.TS);
                    unsigned int seq_no=ntohs(rtp_header.seq_num);
                    static unsigned int _lg_timestamp = 0x7FFFFFFF;
                    ARG_USED(_lg_timestamp);

                    //if (_lg_timestamp!=timestamp)
                    {
                        fprintf(stdout,"[RTP Pkt] %5d| %5s| %10u| %5d| %5d| 0x%0x|\n",cnt, payload_str, timestamp, seq_no, pktsize, rtp_header.ssrc);
                        _lg_timestamp = timestamp;
                    }

                    //RTP Data
                    char *rtp_data=recvData+rtp_header_size;
                    int rtp_data_size=pktsize-rtp_header_size;
                    ARG_USED(rtp_data);
                    ARG_USED(rtp_data_size);
#if defined (_Func_Save_PsStream)

                    fwrite(rtp_data,rtp_data_size,1,fp1);
#endif
                    //Parse MPEGTS
#if 0

                    if(parse_mpegts!=0&&payload==33)
                    {
                        MPEGTS_FIXED_HEADER mpegts_header;
                        for(int i=0;i<rtp_data_size;i=i+188)
                        {
                            if(rtp_data[i]!=0x47)
                                break;
                            //MPEGTS Header
                            //memcpy((void *)&mpegts_header,rtp_data+i,sizeof(MPEGTS_FIXED_HEADER));
                            fprintf(stdout,"   [MPEGTS Pkt]\n");
                        }
                    }
#endif//

                }
                else
                {
                    fprintf(stdout,"[UDP Pkt] %5d| %5d|\n",cnt,pktsize);
                    //fwrite(recvData,pktsize,1,fp1);
                }

                cnt++;
            }
            /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

        }// for
    }
    while(0);

    if (sock_fd_recv!=INVALID_SOCKET)
    {
        close (sock_fd_recv);
    }

    return iret;
}
