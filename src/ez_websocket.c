/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_websocket.c - WebSocket Utility Functions Implementation
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_websocket.c 1 2011-12-27 20:00:00Z WHF $
 *
 * Explain:
 *     Common WebSocket utility functions implementation.
 *     Provides socket operations and timer functions.
 *
 * Update:
 *     2011-12-27 20:00:00 WHF Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#include "ez_websocket.h"

/* 设置 socket 为非阻塞模式 */
int ez_websocket_set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* 设置 socket 选项 */
int ez_websocket_set_socket_options(int sockfd)
{
	int opt = 1;
	
	/* 允许地址重用 */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		return -1;
	
	/* 禁用 Nagle 算法（降低延迟） */
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0)
		return -1;
	
	return 0;
}

/* 创建定时器 */
int ez_websocket_create_timerfd(uint32_t interval_ms)
{
	int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (timerfd < 0)
		return -1;
	
	struct itimerspec timer_spec;
	timer_spec.it_value.tv_sec = interval_ms / 1000;
	timer_spec.it_value.tv_nsec = (interval_ms % 1000) * 1000000;
	timer_spec.it_interval.tv_sec = timer_spec.it_value.tv_sec;
	timer_spec.it_interval.tv_nsec = timer_spec.it_value.tv_nsec;
	
	if (timerfd_settime(timerfd, 0, &timer_spec, NULL) < 0) {
		close(timerfd);
		return -1;
	}
	
	return timerfd;
}

/* 更新定时器 */
int ez_websocket_update_timerfd(int timerfd, uint32_t interval_ms)
{
	struct itimerspec timer_spec;
	timer_spec.it_value.tv_sec = interval_ms / 1000;
	timer_spec.it_value.tv_nsec = (interval_ms % 1000) * 1000000;
	timer_spec.it_interval.tv_sec = timer_spec.it_value.tv_sec;
	timer_spec.it_interval.tv_nsec = timer_spec.it_value.tv_nsec;
	
	return timerfd_settime(timerfd, 0, &timer_spec, NULL);
}

/* 创建监听 socket */
int ez_websocket_create_listen_socket(const char *ip, int port)
{
	int sockfd;
	struct sockaddr_in addr;
	
	/* 创建 socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
		return -1;
	}
	
	/* 设置 socket 选项 */
	if (ez_websocket_set_socket_options(sockfd) < 0) {
		close(sockfd);
		return -1;
	}
	
	/* 设置为非阻塞 */
	if (ez_websocket_set_nonblocking(sockfd) < 0) {
		close(sockfd);
		return -1;
	}
	
	/* 绑定地址 */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	
	/* 如果 IP 为 NULL 或空串，使用 INADDR_ANY */
	if (!ip || ip[0] == '\0') {
		addr.sin_addr.s_addr = INADDR_ANY;
	} else {
		/* 解析 IP 地址 */
		if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
			fprintf(stderr, "Invalid IP address: %s\n", ip);
			close(sockfd);
			return -1;
		}
	}
	
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "Failed to bind to %s:%d: %s\n", 
		        (ip && ip[0] != '\0') ? ip : "0.0.0.0", port, strerror(errno));
		close(sockfd);
		return -1;
	}
	
	/* 开始监听 */
	if (listen(sockfd, SOMAXCONN) < 0) {
		fprintf(stderr, "Failed to listen: %s\n", strerror(errno));
		close(sockfd);
		return -1;
	}
	
	return sockfd;
}

