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

#ifdef __cplusplus
}
#endif

#endif /* EZ_WEBSOCKET_H */

