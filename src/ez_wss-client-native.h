/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_wss-client-native.h - Native WSS (WebSocket over TLS) Client Header
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_wss-client-native.h $
 *
 * Explain:
 *     WSS = WebSocket over TLS. 与明文版 ez_wsclient-native 平行，原实现保持
 *     纯明文不动；本组件基于源码编译的 OpenSSL 静态库实现加密链路。
 *     由 ez_websocket.h 中的 EZ_WS_ENABLE_OPENSSLTLS 宏控制（默认启用，
 *     注释即关：本头与实现编译为空，不依赖 OpenSSL）。
 *
 * Update:
 *     2026-09-29 Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#ifndef EZ_WSS_CLIENT_NATIVE_H
#define EZ_WSS_CLIENT_NATIVE_H

#include <stdint.h>
#include <stddef.h>

#include "ez_websocket.h"
#include "ez_wsclient-native.h"   /* 复用回调/状态/错误码/统计结构 */

#if defined(EZ_WS_ENABLE_OPENSSLTLS) && (EZ_WS_ENABLE_OPENSSLTLS == 1)

#ifdef __cplusplus
extern "C" {
#endif

/* WebSocket客户端（wss）配置：镜像 ez_ws_client_config + TLS 字段 */
struct ez_wss_client_config {
	/* 连接配置 */
	const char *url_path;          /* WebSocket URL 路径 */
	const char *protocol;          /* WebSocket 子协议名称 */
	const char *server_addr;       /* 服务器地址 */
	unsigned short port;           /* 服务器端口 */
	uint32_t connect_timeout_ms;  /* 连接超时时间（毫秒），默认 1000 */

	/* 重连策略配置 */
	uint16_t reconnect_backoff_enable;        /* 是否启用退避策略 */
	uint16_t reconnect_max_retries;      /* 最大重试次数（0表示不限制） */
	uint16_t reconnect_backoff_min_retries;     /* 前 N 次重试不增加延迟 */
	uint16_t reconnect_backoff_high_threshold;  /* 达到或超过此次数后不再增加延迟 */
	uint16_t reconnect_interval_ms;      /* 重连间隔（毫秒） */

	/* TLS 配置 */
	int tls_enable;                /* 0/1，每实例/每条连接；默认 1（开箱即加密） */
	int tls_verify_peer;           /* 默认 1：校验服务端证书（内置 CA 互认）；跨信任域可配 0（仅加密） */
	const char *tls_ca_path;       /* 校验服务端所用 CA，可为空表示用内置/系统 CA */
};

/* WebSocket客户端（wss）句柄（不透明结构） */
struct ez_wss_client_handle;

/* 公共 API（签名与明文版 ez_ws_client_* 平行；回调/状态/统计结构复用明文版类型） */

/**
 * 创建WebSocket客户端（wss）句柄
 * @param config 客户端配置（可以为NULL，使用默认配置）
 * @param callbacks 回调函数结构（可以为NULL）
 * @return 成功返回句柄指针，失败返回NULL
 * @note 创建后不会立即连接，连接操作将在 ez_wss_service_exec 中异步执行，避免阻塞
 */
struct ez_wss_client_handle *ez_wss_client_handle_create(struct ez_wss_client_config *config,
                                                  struct ez_ws_callbacks *callbacks);

/**
 * 清理WebSocket客户端（wss）
 * @param ws 客户端句柄
 * @note 清理前会自动停止连接，无需额外调用停止函数
 */
void ez_wss_client_cleanup(struct ez_wss_client_handle *ws);

/**
 * WebSocket（wss）服务执行函数 - 执行一次事件循环迭代
 * 该函数应该在外部线程中循环调用
 * @param ws 客户端句柄
 * @param timeout_ms 超时时间（毫秒），0表示使用默认超时
 * @return 0表示继续运行，-1表示应该停止
 */
int ez_wss_service_exec(struct ez_wss_client_handle *ws, int timeout_ms);

/**
 * 发送文本消息
 * @param ws 客户端句柄
 * @param data 文本数据
 * @param len 数据长度，0表示自动计算（以\0结尾的字符串）
 * @return EZ_WS_OK表示成功，其他值表示错误
 */
int ez_wss_send_text(struct ez_wss_client_handle *ws, const char *data, size_t len);

/**
 * 发送二进制消息
 * @param ws 客户端句柄
 * @param data 二进制数据
 * @param len 数据长度
 * @return EZ_WS_OK表示成功，其他值表示错误
 */
int ez_wss_send_binary(struct ez_wss_client_handle *ws, const void *data, size_t len);

/**
 * 检查连接状态
 * @param ws 客户端句柄
 * @return 1表示已连接，0表示未连接
 */
int ez_wss_is_connected(struct ez_wss_client_handle *ws);

/**
 * 获取连接状态
 * @param ws 客户端句柄
 * @return 连接状态枚举值
 */
enum ez_ws_state ez_wss_get_state(struct ez_wss_client_handle *ws);

#if defined(EZ_WS_CLIENT_ENABLE_STATS) && (EZ_WS_CLIENT_ENABLE_STATS == 1)
/**
 * 获取统计信息
 * @param ws 客户端句柄
 * @param stats 输出统计信息结构（可以为NULL，仅返回是否成功）
 * @return 0表示成功，-1表示失败
 */
int ez_wss_get_stats(struct ez_wss_client_handle *ws, struct ez_ws_client_stats *stats);
#endif /* EZ_WS_CLIENT_ENABLE_STATS */

#ifdef __cplusplus
}
#endif

#endif /* EZ_WS_ENABLE_OPENSSLTLS */

#endif /* EZ_WSS_CLIENT_NATIVE_H */
