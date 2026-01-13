/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_socket.h - socket API
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_socket.h 16364 2008-04-25 03:59:51Z guan_shiyong $
 *
 * socket api, fit for the past and the future.
 * Platform:
 * 		linux+gcc / win32+vc6 / cygwin
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2005-04-02 17:19:24 WuJunjie  Create
 *		wjj 2005-04-02 15:16:48
 *			Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#ifndef	__EZ_SOCKET_H
#define	__EZ_SOCKET_H

#ifdef  __cplusplus
extern "C"
{
#endif

    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

    /* configuration options for current OS */
#include	"ez_config.h"

    /* debug and test parameter */
#include	"ez_appdef.h"

    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
    /* Following shortens all the typecasts of pointer arguments: */
//#define	SA	struct sockaddr
typedef struct sockaddr SA;

#ifdef WIN32
    static signed char __initialized = 0;
#endif

#ifndef	HAVE_SYSLOG_H
#define syslog(x, y)
#endif

    int		ez_snprintf(char *buf, size_t size, const char *fmt, ...);
#define	ez_bzero(ptr,n)		memset(ptr, 0, n)

#define	__ez_min(a,b)	((a) < (b) ? (a) : (b))
#define	__ez_max(a,b)	((a) > (b) ? (a) : (b))

    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

    ez_socket_t	 ez_tcp_server(const ez_port_t, const int);
    int ez_connect_nonblock(ez_socket_t fd, const SA *sa, ez_socklen_t salen, const long usec_timeout);
    ez_socket_t	 ez_tcp_connect(const char * hostip, const ez_port_t port, const long usec_timeout);
    ez_socket_t	 ez_tcp_client(const char *, const char *);
    ez_socket_t  ez_tcp_client_with_timeout(const char *hname, const char *sname, const long usec_timeout);

    ez_socket_t	 ez_udp_server(const ez_port_t);
    ez_socket_t	 ez_udp_connect(const char * hostip, const ez_port_t port);
    ez_socket_t	 ez_udp_client(const char * hostip, const ez_port_t port, SA *saptr, ez_socklen_t *lenp);
    ez_socket_t ez_udp_connect_host(char *hname, char *sname);

    int			 ez_mcast_set_loop(ez_socket_t, unsigned char);

    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+- raw socket api-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

    ez_socket_t	 ez_ip_snif_socket(const char *);

    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+basic socket api-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

    ez_socket_t	 ez_socket(int, int, int);
    int		 ez_bind(ez_socket_t, const SA *, ez_socklen_t);
    int		 ez_listen(ez_socket_t, int);
    ez_socket_t	 ez_accept(ez_socket_t, SA *, ez_socklen_t *);
    int		 ez_connect(ez_socket_t, const SA *, ez_socklen_t);
    int ez_read_with_timeout(int sockfd, void *buf, size_t count, struct timeval *ptime_out);

    int		 ez_socket_init(void);
    int		 ez_socket_cleanup(void);
    int		 ez_close(ez_socket_t fd);
    int		 ez_close_socket(ez_socket_t *fd);

    struct hostent * ez_gethostbyname(const char * name);
    int		 ez_getpeername(ez_socket_t, SA *, ez_socklen_t *);
    int		 ez_getsockname(ez_socket_t, SA *, ez_socklen_t *);
    int		 ez_gethostname(char *name, size_t len);

    int		 ez_getsockopt(ez_socket_t, int, int, void *, ez_socklen_t *);
    int		 ez_setsockopt(ez_socket_t, int, int, const void *, ez_socklen_t);
    int		 ez_shutdown(ez_socket_t, int);
    int		 ez_select(int, fd_set *, fd_set *, fd_set *, struct timeval *);

    en_bool	 ez_set_nodelay(ez_socket_t s);
    en_bool	 ez_set_nonblock(en_bool bNb, ez_socket_t s);

    /*-+-+-+-+-+-+-+-+-+-+-+-+-+- socket io   -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

    int ez_writen(ez_socket_t fd, const void *vptr, size_t n);
    int ez_writeton( ez_socket_t fd, const void *vptr, size_t n, const SA *saRemote, int struct_size);

    /*-+-+-+-+-+-+-+-+-+-+-+-+-+- socket addr -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

    int		 ez_inet_aton(const char *cp, struct in_addr *ap);
    int		 ez_inet_pton(int family, const char *strptr, void *addrptr);
    int		 ez_set_address(const char *hname,
                         const char *sname,
                         struct sockaddr_in *sap,
                         char *protocol );
    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#define HTTP_MAXLINE 4096
#define HTTP_MAXSUB  1024
#define DEFAULT_HTTP_PORT "80"
    // iFastMode 1, quick ret
    ssize_t ez_http_get(ez_socket_t sockfd, const char *host, const char *page, char *recvline, int recvbuflen, int iFastMode);
    ssize_t ez_http_post(ez_socket_t sockfd, const char *host, const char *page, const char *poststr, char *recvline, int recvbuflen, int iFastMode);

    en_bool	 ez_is_valid_ip4(const char *str);

    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
    //    int ez_pthread_create(ez_pthread_t *	phThread,
    //                          ez_pthread_attr_t	thread_attr,
    //                          ez_threadfunc_t (WINAPI *pStartAddress)(ez_threadparam_t),
    //                          ez_threadparam_t	pParameter);
    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+- net util api+-+-+-+-+-+-+-+-+-+-+-+-*/
    int get_pub_ip(char *pPubIP, size_t count);

    // ret 0 seccess, <0 failed
    int ez_ivs_updateip(
        const char *pLogin		 ,
        const char *pPasswd      ,
        const char *pPuid        ,
        const char *pPuip        ,
        const char *pPuPort      ,
        const char *pHttpPort    ,
        const char *pPuname      ,
        const char *pMac		 ,
        const char *pAttr		 ,
        const char *pServiceHost	   ,
        const char *pServicePort   , //
        const char *pServiceLoc	   ,
        char *recvline			 ,
        size_t count);


    // ret 0 seccess, <0 failed
    int ez_get_ivs_service(
        const char * const pPuid ,
        const char * const pService,
        char *recvline,
        size_t count);

    typedef enum __em_ez_rtsp_valid_t {
        em_ez_rtsp_valid_PlayUrlNull =    -10000  ,   //   1    -- 地址无法播放
        em_ez_rtsp_valid_PlayUrlEmpty            ,    //   1001 -- 播放地址为空
        em_ez_rtsp_valid_PlayUrlIPNotValid       ,    //   1002 -- 播放地址IP无效
        em_ez_rtsp_valid_PlayUrlPortNotValid     ,    //   1003 -- 播放地址端口无效
        em_ez_rtsp_valid_PlayUrlCanNotSupport    ,    //   1004 -- 不支持的播放地址
        em_ez_rtsp_valid_PlayUrlNetworkNotFit    ,    //   1005 -- 通讯不良（卡顿）
    }em_ez_rtsp_valid_t;
    // 0 - ok
    // < 0 NOT ok, em_ez_rtsp_valid_t
    int ez_rtsp_valid(const char *p_rtsp_url, int i_step);

    ssize_t ez_xml_post(ez_socket_t sockfd, const char *host, const char *page, const char *poststr, char *recvline, int recvbuflen, int iFastMode);
    // 0 - OK
    // >0 - error code
    // < 0, net error, -1
    int ez_nvp_add_account(const char *mobilePhone);
    // ok
    // >0 - error code
    // < 0, net error, -1
    int ez_nvp_add_device(const char *p_devid, const char *p_key);

    int ez_nvp_device_reg_2t(const char *p_reg_string, char *p_ret_session, char *p_ret_body);

    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
    #define ezDef_StreamType_Str_Streamming          "1"
    #define ezDef_StreamType_Str_IPC_hikvision       "2"
    #define ezDef_StreamType_Str_IPC_dahua           "3"
    #define ezDef_StreamType_Str_megaeye             "4"
    #define ezDef_StreamType_Str_tykd                "5"
    #define ezDef_StreamType_Str_hk8200              "6"
    #define ezDef_StreamType_Str_hk7600              "7" //海康7600平台
    #define ezDef_StreamType_Str_IPC_tdwy            "8" //天地伟业
    #define ezDef_StreamType_Str_spj                 "9"//shipinjia
    #define ezDef_StreamType_Str_JSvCloud            "10"
    #define ezDef_StreamType_Str_IPC_ryht            "11"//ruiying
    #define ezDef_StreamType_Str_IPC_xm              "12"//xiongmai
    #define ezDef_StreamType_Str_IPC_tp              "13"//tp-link
    #define ezDef_StreamType_Str_IPC_jiguang         "14"//集光
    #define ezDef_StreamType_Str_GB_HX               "15"//鸿信GB- restful
    #define ezDef_StreamType_Str_onvif               "16"
    #define ezDef_StreamType_Str_ppp                 "17"
    #define ezDef_StreamType_Str_mmt                 "18" // 无锡运维
    #define ezDef_StreamType_Str_XXXX_001            "21" // 视频压缩A  播放地址类似：rtsp://192.168.5.10/1
    #define ezDef_StreamType_Str_TUYA_001            "22" // 图鸭

    #define ezDef_StreamType_Streamming               1
    #define ezDef_StreamType_IPC_hikvision            2
    #define ezDef_StreamType_IPC_dahua                3
    #define ezDef_StreamType_megaeye                  4
    #define ezDef_StreamType_tykd                     5
    #define ezDef_StreamType_hk8200                   6
    #define ezDef_StreamType_hk7600                   7 //海康7600平台
    #define ezDef_StreamType_IPC_tdwy                 8 // 天地伟业
    #define ezDef_StreamType_spj                      9
    #define ezDef_StreamType_JSvCloud                 10
    #define ezDef_StreamType_IPC_ryht                 11//ruiying
    #define ezDef_StreamType_IPC_xm                   12//xiongmai
    #define ezDef_StreamType_IPC_tp                   13//tp-link
    #define ezDef_StreamType_IPC_jiguang              14//集光
    #define ezDef_StreamType_GB_HX                    15//鸿信GB- restful
    #define ezDef_StreamType_onvif                    16
    #define ezDef_StreamType_ppp                      17
    #define ezDef_StreamType_mmt                      18 // 无锡运维
    #define ezDef_StreamType_XXXX_001                 21
    #define ezDef_StreamType_TUYA_001                 22 // 图鸭压缩

    int ez_getStreamPlayUrl(
        const int iType      , /* 流类型，代号如下：1-流媒体  2-海康IPC 3-大华IPC . ezDef_StreamType_    */
        const char *pUrl     , /* 链路url，协议://ip:port，例如：rtsp://192.168.5.66:554                 */
        const char *pAccount , /* 账号、用户名                                                           */
        const char *pPasswd  , /* 密码                                                                   */
        const char *pChannel , /* 通道号 1-n                                                             */
        const char *pSubtype , /* 码流类型，主码流为1（即subtype=1），辅码流为2（即subtype=2），以此类推 */
        char       *pPlayUrl , /* 返回                                                                   */
        const size_t PlayUrlBufLen
        );
    
    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
    // iFastMode 1, 收到数据立刻返回， 不等链接断开
    int ez_http_post_to_host(const char *p_host, const char *p_port, const char *p_page, const char *p_post_string, char *recvline, int recvbuflen, int iFastMode);
    
    /*-+-+-+-+-+-+-+-+-+-+-+-+-+-+- net interface api+-+-+-+-+-+-+-+-+-+-+-+-*/

#if (defined(EZ_ERROR_FUNCTION) && (EZ_ERROR_FUNCTION != 0))

    void	 ez_err_dump(const char *, ...);
    void	 ez_err_msg(const char *, ...);
    void	 ez_err_quit(const char *, ...);
    void	 ez_err_ret(const char *, ...);
    void	 ez_err_sys(const char *, ...);
#else
	#ifdef PSOS
		#define ez_err_sys myprintf
	#else
		#define ez_err_sys printf
	#endif
    /*	#define ez_err_ret(format, a...) do{ printf(format, ## a); } while(0)
	#define ez_err_sys(format, a...) do{ printf(format, ## a); exit(1); } while(0)
	#define ez_err_dump(format, a...) do{ printf(format, ## a); exit(1); } while(0)
	#define ez_err_msg(format, a...) do{ printf(format, ## a); } while(0)
	#define ez_err_quit(format, a...) do{ printf(format, ## a); exit(1); } while(0)*/
#endif	/* EZ_ERROR_FUNCTION */

    //#define __trip __ez_pritnf("-W-%d::%s(%d)\n", (int)time(NULL), __FILE__, __LINE__);
    //#define __fline __ez_pritnf("%s(%d)--", __FILE__, __LINE__);

#ifdef  __cplusplus
}
#endif

#endif	/* ADSOCKET_H */

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- end -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

