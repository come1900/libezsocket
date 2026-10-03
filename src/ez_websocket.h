/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_websocket.h - WebSocket Utility Functions Header
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_websocket.h 1 2011-12-27 20:00:00Z WHF $
 *
 * Explain:
 *     Common WebSocket utility functions for client and server.
 *     Provides socket operations and timer functions.
 *
 * Update:
 *     2011-12-27 20:00:00 WHF Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#ifndef EZ_WEBSOCKET_H
#define EZ_WEBSOCKET_H

/* wss（WebSocket over TLS）能力宏：默认启用，依赖 OpenSSL。
 * 注释本行即回到纯明文：wss 组件编译为空对象，库不依赖 OpenSSL。 */
#define EZ_WS_ENABLE_OPENSSLTLS 1

#include <fcntl.h>
#include <stdint.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 设置 socket 为非阻塞模式
 * @param fd socket 文件描述符
 * @return 0 表示成功，-1 表示失败
 */
int ez_websocket_set_nonblocking(int fd);

/**
 * 设置 socket 选项（SO_REUSEADDR 和 TCP_NODELAY）
 * @param sockfd socket 文件描述符
 * @return 0 表示成功，-1 表示失败
 */
int ez_websocket_set_socket_options(int sockfd);

/**
 * 创建定时器文件描述符
 * @param interval_ms 定时器间隔（毫秒）
 * @return 成功返回定时器文件描述符，失败返回 -1
 */
int ez_websocket_create_timerfd(uint32_t interval_ms);

/**
 * 更新定时器间隔
 * @param timerfd 定时器文件描述符
 * @param interval_ms 新的定时器间隔（毫秒）
 * @return 0 表示成功，-1 表示失败
 */
int ez_websocket_update_timerfd(int timerfd, uint32_t interval_ms);

/**
 * 创建监听 socket
 * @param ip 监听IP地址，NULL 或空串表示 INADDR_ANY（监听所有接口）
 * @param port 监听端口
 * @return 成功返回 socket 文件描述符，失败返回 -1
 */
int ez_websocket_create_listen_socket(const char *ip, int port);

#if defined(EZ_WS_ENABLE_OPENSSLTLS) && (EZ_WS_ENABLE_OPENSSLTLS == 1)
/**
 * 导出内置 CA 公钥证书（PEM）到调用方缓冲区，供 Python manager 等非 C 消费者
 * 落地为受信 CA 文件（如 ca.crt）。内置 CA 私钥永不导出。
 * @param buf 输出缓冲区；传 NULL 且 buf_size=0 时返回所需字节数（含末尾 '\0'）
 * @param buf_size 缓冲区容量（字节）
 * @return 成功返回写入的字节数（含末尾 '\0'）；buf 不足或失败返回 -1
 */
int ez_ws_export_builtin_ca_pem(char *buf, size_t buf_size);
#endif /* EZ_WS_ENABLE_OPENSSLTLS */

#ifdef __cplusplus
}
#endif

#endif /* EZ_WEBSOCKET_H */

