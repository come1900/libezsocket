/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_wss-server-native.h - Native WSS (WebSocket over TLS) Server Header
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_wss-server-native.h $
 *
 * Explain:
 *     WSS = WebSocket over TLS. 与明文版 ez_wsserver-native 平行，原实现保持
 *     纯明文不动；本组件基于源码编译的 OpenSSL 静态库实现加密链路。
 *     由 ez_websocket.h 中的 EZ_WS_ENABLE_OPENSSLTLS 宏控制（默认启用，
 *     注释即关：本头与实现编译为空，不依赖 OpenSSL）。
 *
 * Update:
 *     2026-09-29 Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#ifndef EZ_WSS_SERVER_NATIVE_H
#define EZ_WSS_SERVER_NATIVE_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#include "ez_websocket.h"
#include "ez_wsserver-native.h"   /* 复用回调/错误码/客户端信息/统计结构 */

#if defined(EZ_WS_ENABLE_OPENSSLTLS) && (EZ_WS_ENABLE_OPENSSLTLS == 1)

#ifdef __cplusplus
extern "C" {
#endif

/* WebSocket服务端（wss）配置：镜像 ez_ws_server_config + TLS 字段 */
struct ez_wss_server_config {
	/* 连接配置 */
	const char *ip;                /* 监听IP地址，NULL 或空串表示 INADDR_ANY（监听所有接口） */
	int port;                      /* 监听端口 */
	const char *protocol;          /* WebSocket 子协议名称（必须指定，不能为 NULL） */
	const char *path_prefix;       /* URL 路径前缀（如 "/come"），NULL 表示不检查路径 */

	/* 保活配置（所有配置由应用层控制，库不提供默认值） */
	uint32_t ping_interval_ms;     /* 心跳间隔（毫秒），0 表示禁用服务端主动 ping */
	uint32_t ping_timeout_ms;      /* 等待 pong 最大时长（毫秒），0 表示禁用 pong 超时检测 */
	uint32_t idle_timeout_ms;      /* 无业务流量断开时间（毫秒），0 表示禁用空闲超时 */
	uint32_t timer_interval_ms;    /* 定时器检测周期（毫秒），0 表示禁用定时器 */
	uint32_t ping_jitter_percent;  /* ping 间隔抖动百分比（0-50），0 表示禁用抖动 */

	/* 选项 */
	int options;                   /* 选项标志（保留，当前未使用） */

	/* TLS 配置 */
	int tls_enable;                /* 0/1，监听级、新连接继承；默认 1（未配置证书时走内置 CA 互认） */
	const char *tls_cert_path;     /* 证书链，可为空（空 → 内置 CA 签发叶子证书） */
	const char *tls_key_path;      /* 私钥，可为空 */
	const char *tls_ca_path;       /* 可选：双向认证 client CA，可为空 */
};

/* WebSocket服务端（wss）句柄（不透明结构） */
struct ez_wss_server_handle;

/* 公共 API（签名与明文版 ez_ws_server_* 平行；回调/统计结构复用明文版类型） */

/**
 * 创建WebSocket服务端（wss）句柄
 * @param config 服务端配置（可以为NULL，使用默认配置）
 * @param callbacks 回调函数结构（可以为NULL）
 * @return 成功返回句柄指针，失败返回NULL
 */
struct ez_wss_server_handle *ez_wss_server_handle_create(struct ez_wss_server_config *config,
                                                          struct ez_ws_server_callbacks *callbacks);

/**
 * 清理WebSocket服务端（wss）
 * @param ws 服务端句柄
 * @note 清理前会自动停止服务，无需额外调用停止函数
 */
void ez_wss_server_cleanup(struct ez_wss_server_handle *ws);

/**
 * WebSocket服务端（wss）执行函数 - 执行一次事件循环迭代
 * 该函数应该在外部线程中循环调用
 * @param ws 服务端句柄
 * @param timeout_ms 超时时间（毫秒），此参数被忽略（内部使用智能调度）
 * @return 0表示继续运行，-1表示应该停止
 */
int ez_wss_server_service_exec(struct ez_wss_server_handle *ws, int timeout_ms);

/**
 * 发送文本消息到指定客户端
 * @param ws 服务端句柄
 * @param client_id 客户端ID，EZ_WS_SERVER_BROADCAST_ALL 表示广播到所有客户端
 * @param data 文本数据
 * @param len 数据长度，0表示自动计算（以\0结尾的字符串）
 * @return EZ_WS_SERVER_OK表示成功，其他值表示错误
 */
int ez_wss_server_send_text(struct ez_wss_server_handle *ws, int client_id, const char *data, size_t len);

/**
 * 发送二进制消息到指定客户端
 * @param ws 服务端句柄
 * @param client_id 客户端ID，EZ_WS_SERVER_BROADCAST_ALL 表示广播到所有客户端
 * @param data 二进制数据
 * @param len 数据长度
 * @return EZ_WS_SERVER_OK表示成功，其他值表示错误
 */
int ez_wss_server_send_binary(struct ez_wss_server_handle *ws, int client_id, const void *data, size_t len);

/**
 * 获取已连接的客户端数量
 * @param ws 服务端句柄
 * @return 客户端数量，-1表示错误
 */
int ez_wss_server_get_client_count(struct ez_wss_server_handle *ws);

/**
 * 检查服务是否正在运行
 * @param ws 服务端句柄
 * @return 1表示已准备好，0表示未准备好
 */
int ez_wss_server_is_ready(struct ez_wss_server_handle *ws);

/**
 * 关闭指定客户端连接
 * @param ws 服务端句柄
 * @param client_id 客户端ID
 * @return EZ_WS_SERVER_OK表示成功，其他值表示错误
 */
int ez_wss_server_close_client(struct ez_wss_server_handle *ws, int client_id);

/**
 * 遍历所有已连接的客户端
 * @param ws 服务端句柄
 * @param callback 遍历回调函数
 * @param user_data 用户自定义数据
 * @return EZ_WS_SERVER_OK表示成功，其他值表示错误
 */
int ez_wss_server_foreach_client(struct ez_wss_server_handle *ws,
                                  ez_ws_server_foreach_client_cb callback,
                                  void *user_data);

#if defined(EZ_WS_SERVER_ENABLE_STATS) && (EZ_WS_SERVER_ENABLE_STATS == 1)
/**
 * 获取指定客户端的统计信息
 * @param ws 服务端句柄
 * @param client_id 客户端ID
 * @param stats 输出统计信息结构（可以为NULL，仅返回是否成功）
 * @return EZ_WS_SERVER_OK表示成功，其他值表示错误
 */
int ez_wss_server_get_client_stats(struct ez_wss_server_handle *ws, int client_id,
                                   struct ez_ws_server_client_stats *stats);
#endif /* EZ_WS_SERVER_ENABLE_STATS */

#ifdef __cplusplus
}
#endif

#endif /* EZ_WS_ENABLE_OPENSSLTLS */

#endif /* EZ_WSS_SERVER_NATIVE_H */
