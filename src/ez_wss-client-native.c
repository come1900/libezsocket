/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_wss-client-native.c - Native WSS (WebSocket over TLS) Client Implementation
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_wss-client-native.c $
 *
 * Explain:
 *     WSS = WebSocket over TLS. 与明文版 ez_wsclient-native 平行，原实现保持
 *     纯明文不动；本组件基于源码编译的 OpenSSL 静态库实现加密链路。
 *     TCP connect 成功后、发送 HTTP/WS 握手请求前先完成非阻塞 TLS 握手
 *     （SSL_connect，由 epoll 事件驱动 WANT_READ/WRITE）；TLS 握手完成后
 *     才发送握手请求，数据面走 SSL_read/SSL_write。
 *     由 ez_websocket.h 中的 EZ_WS_ENABLE_OPENSSLTLS 宏整文件包裹：
 *     宏被注释 → 本文件编译为空对象，不引入任何 OpenSSL 符号与依赖。
 *
 * Update:
 *     2026-09-29 Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#include "ez_websocket.h"         /* 定义 EZ_WS_ENABLE_OPENSSLTLS（注释宏即未定义） */
#include "ez_wss-client-native.h" /* 宏定义时才有内容 */

#if defined(EZ_WS_ENABLE_OPENSSLTLS) && (EZ_WS_ENABLE_OPENSSLTLS == 1)

#define HAVE_GETRANDOM // 低版本linux中，注释掉这两行
#include <sys/random.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <ezutil/base64.h>
#include <ezutil/ez_websocket_parser.h>
#include <ezutil/http_parser.h>
#include <ezutil/sha1.h>
#include <ezutil/str_opr.h>

#include <ezutil/ez_def_devel_debug.h>

/* ================== 链路保活配置（与明文版一致） ================== */
#define EZ_WS_CLIENT_PING_INTERVAL_MS      (10 * 1000)   /* 基础 ping 间隔（最小 1s），0 表示禁用客户端主动 ping */
// #define EZ_WS_CLIENT_PING_INTERVAL_MS      (0 * 1000)   /* 基础 ping 间隔（最小 1s），0 表示禁用客户端主动 ping */
#define EZ_WS_CLIENT_PING_JITTER_PERCENT   10            /* ping 间隔抖动百分比 (0-50)，推荐 10%，0 表示禁用抖动 */

#define EZ_WS_CLIENT_IDLE_TIMEOUT_MS       (120 * 1000)  /* 超过该时间无任何业务流量则判定失效 */

#define EZ_WS_CLIENT_PONG_TIMEOUT_MS       (EZ_WS_CLIENT_PING_INTERVAL_MS / 3)   /* 等待 pong 超时 = ping间隔/3 */
#define EZ_WS_CLIENT_HEARTBEAT_TICK_MS     (EZ_WS_CLIENT_PONG_TIMEOUT_MS / 2)    /* 心跳检测周期 = pong超时/2 */

#define EZ_WS_CLIENT_PING_JITTER_MS \
	((EZ_WS_CLIENT_PING_INTERVAL_MS * EZ_WS_CLIENT_PING_JITTER_PERCENT) / 100)

#define EZ_WS_CLIENT_PING_INTERVAL_MIN_MS \
	(EZ_WS_CLIENT_PING_INTERVAL_MS - EZ_WS_CLIENT_PING_JITTER_MS)

#define EZ_WS_CLIENT_PING_INTERVAL_MAX_MS \
	(EZ_WS_CLIENT_PING_INTERVAL_MS + EZ_WS_CLIENT_PING_JITTER_MS)

#if EZ_WS_CLIENT_PING_INTERVAL_MS > 0 && EZ_WS_CLIENT_PING_INTERVAL_MS < 1000
#error "PING_INTERVAL should be 0 (disabled) or at least 1000ms (1 second)"
#endif

#if EZ_WS_CLIENT_PING_JITTER_PERCENT < 0 || EZ_WS_CLIENT_PING_JITTER_PERCENT > 50
#error "PING_JITTER_PERCENT should be between 0 and 50 (0% to 50%)"
#endif

#if EZ_WS_CLIENT_IDLE_TIMEOUT_MS > 0 && EZ_WS_CLIENT_IDLE_TIMEOUT_MS <= (2 * EZ_WS_CLIENT_PING_INTERVAL_MAX_MS + EZ_WS_CLIENT_PONG_TIMEOUT_MS)
#error "IDLE_TIMEOUT should be 0 (disabled, not recommended) or large enough for at least 2 ping/pong rounds even with maximum jitter"
#endif

#if EZ_WS_CLIENT_IDLE_TIMEOUT_MS > 0 && EZ_WS_CLIENT_IDLE_TIMEOUT_MS < (2 * EZ_WS_CLIENT_PING_INTERVAL_MS)
#error "IDLE_TIMEOUT should be 0 (disabled, not recommended) or at least 2x PING_INTERVAL"
#endif

#ifndef RECONNECT_INTERVAL_MS
#define RECONNECT_INTERVAL_MS (1*500)
#endif

#ifndef RECONNECT_MAX_RETRIES
#define RECONNECT_MAX_RETRIES 0
#endif

#ifndef RECONNECT_BACKOFF_ENABLE
#define RECONNECT_BACKOFF_ENABLE 1
#endif

#define EZ_PRINT_LOG_DEBUG_ENABLED 0

#if RECONNECT_BACKOFF_ENABLE
#ifndef RECONNECT_BACKOFF_MIN_RETRIES
#define RECONNECT_BACKOFF_MIN_RETRIES 2
#endif

#ifndef RECONNECT_BACKOFF_HIGH_THRESHOLD
#define RECONNECT_BACKOFF_HIGH_THRESHOLD 5
#endif
#endif

/* ez_wss_client_handle_reset 标志位 */
#define EZ_WSS_RESET_HTTP       (1u << 0)
#define EZ_WSS_RESET_WS         (1u << 1)
#define EZ_WSS_RESET_HEARTBEAT  (1u << 2)

/* 安全释放内存宏 */
#define SAFE_FREE(ptr) do { \
	if (ptr) { \
		free(ptr); \
		ptr = NULL; \
	} \
} while (0)

/* WebSocket 帧数据收集器 */
struct ez_ws_frame_data {
	uint8_t *buffer;
	size_t buffer_size;
	size_t buffer_cap;
	int opcode;
	int is_binary;
};

/* 发送消息队列节点 */
struct send_queue_node {
	char *data;
	size_t len;
	int opcode;
	struct send_queue_node *next;
};

/* 发送缓冲区 */
struct send_buffer {
	uint8_t *frame_data;
	size_t frame_len;
	size_t sent_len;
	int is_active;
};

static int ez_wss_send_control_frame(struct ez_wss_client_handle *ws, int opcode,
				 const void *payload, size_t len);

static int ez_wss_create_handshake_request(struct ez_wss_client_handle *ws, char *buffer, size_t buffer_size);

static uint64_t
ez_wss_client_now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

/* 生成带抖动的 ping 间隔（毫秒） */
static uint32_t
ez_wss_client_get_ping_interval_with_jitter(void)
{
#if EZ_WS_CLIENT_PING_INTERVAL_MS == 0
	return 0;
#elif EZ_WS_CLIENT_PING_JITTER_PERCENT == 0
	return EZ_WS_CLIENT_PING_INTERVAL_MS;
#else
	uint32_t jitter_range = EZ_WS_CLIENT_PING_JITTER_MS * 2;
	uint32_t random_offset = (uint32_t)(rand() % (jitter_range + 1));
	return EZ_WS_CLIENT_PING_INTERVAL_MIN_MS + random_offset;
#endif
}

/* WebSocket客户端（wss）句柄（完整定义） */
struct ez_wss_client_handle {
	/* 配置 */
	struct ez_wss_client_config config;

	/* TLS */
	SSL_CTX *ssl_ctx;              /* 客户端 CTX（每句柄一个，仅校验与初始握手） */
	SSL *ssl;                      /* 当前连接 SSL 会话，NULL 表示纯明文连接 */
	int tls_enable;                /* 每实例/每条连接 TLS 开关 */
	int tls_verify_peer;           /* 校验服务端证书 */
	char *tls_ca_path;             /* 校验服务端所用 CA，可为空（空 → trust 内置 CA） */
	int tls_handshake_done;        /* TLS 握手已完成（HANDSHAKING 阶段子状态） */
	int write_blocked_on_read;     /* SSL_write 返回 WANT_READ：需先 SSL_read 才能续写 */

	/* 连接信息 */
	char *server_addr;
	unsigned short port;
	char *url_path;
	char *protocol;

	/* Socket */
	int sockfd;
	int epollfd;

	/* 定时器 */
	int reconnect_timerfd;
	int heartbeat_timerfd;

	/* 状态 */
	enum ez_ws_state state;
	int interrupted;

	/* 重连状态 */
	uint32_t retry_count;
	uint32_t current_retry_delay;

	/* 握手相关 */
	char *handshake_key;
	char handshake_response[4096];
	size_t handshake_response_len;

	/* HTTP 解析器 */
	http_parser http_parser;
	http_parser_settings http_parser_settings;
	int http_handshake_complete;
	char *http_header_field;
	char *http_header_value;
	char *http_sec_websocket_accept;

	/* WebSocket 解析器 */
	ez_websocket_parser ws_parser;
	ez_websocket_parser_settings ws_parser_settings;
	struct ez_ws_frame_data current_frame;

	/* 接收缓冲区 */
	uint8_t recv_buffer[65536];
	size_t recv_buffer_len;

	/* 发送队列 */
	struct send_queue_node *send_queue_head;
	struct send_queue_node *send_queue_tail;
	pthread_mutex_t send_queue_lock;
	size_t send_queue_size;
	size_t send_queue_max;

	/* 发送缓冲区 */
	struct send_buffer pending_send;

	/* 回调 */
	struct ez_ws_callbacks callbacks;

	/* 线程 */
	pthread_t ws_thread;
	int ready;

	uint64_t last_rx_time_ms;
	uint64_t last_tx_time_ms;
	uint64_t last_activity_time_ms;
	uint64_t last_ping_time_ms;
	uint64_t connect_start_time_ms;  /* 连接开始时间（用于超时检测） */
	int awaiting_pong;
	uint32_t current_ping_interval_ms;  /* 当前使用的 ping 间隔（带抖动） */

#if defined(EZ_WS_CLIENT_ENABLE_STATS) && (EZ_WS_CLIENT_ENABLE_STATS == 1)
	/* 统计信息 */
	struct {
		uint64_t tx_text_count;      /* TEXT 帧数量 */
		uint64_t tx_text_bytes;      /* TEXT 帧字节数 */
		uint64_t tx_binary_count;    /* BINARY 帧数量 */
		uint64_t tx_binary_bytes;    /* BINARY 帧字节数 */
		uint64_t tx_ping_count;      /* PING 帧数量 */
		uint64_t tx_pong_count;      /* PONG 帧数量 */
		uint64_t tx_close_count;     /* CLOSE 帧数量 */

		uint64_t rx_text_count;      /* TEXT 帧数量 */
		uint64_t rx_text_bytes;      /* TEXT 帧字节数 */
		uint64_t rx_binary_count;    /* BINARY 帧数量 */
		uint64_t rx_binary_bytes;    /* BINARY 帧字节数 */
		uint64_t rx_ping_count;      /* PING 帧数量 */
		uint64_t rx_pong_count;      /* PONG 帧数量 */
		uint64_t rx_close_count;     /* CLOSE 帧数量 */
	} stats;
#endif /* EZ_WS_CLIENT_ENABLE_STATS */
};

/* ================== TLS 数据面 工具 ================== */

/* TLS 读：封装 SSL_read / recv，WANT_READ/WRITE 归一为 -1+EAGAIN */
static ssize_t wss_client_read(struct ez_wss_client_handle *ws, void *buf, size_t len)
{
	if (!ws->ssl)
		return recv(ws->sockfd, buf, len, 0);

	int n = SSL_read(ws->ssl, buf, (int)len);

	/* OpenSSL：SSL_write 返回 WANT_READ 后，只要执行过 SSL_read 即视为已推进，
	 * 允许续写（是否读到数据不限）。不在此判断返回值，避免漏解锁。 */
	ws->write_blocked_on_read = 0;

	if (n > 0)
		return n;

	int err = SSL_get_error(ws->ssl, n);
	if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
		errno = EAGAIN;
		return -1;
	}
	if (err == SSL_ERROR_ZERO_RETURN)
		return 0; /* 对端干净关闭 */

	return -1;
}

/* TLS 写：封装 SSL_write / send */
static ssize_t wss_client_write(struct ez_wss_client_handle *ws, const void *buf, size_t len)
{
	if (!ws->ssl)
		return send(ws->sockfd, buf, len, 0);

	int n = SSL_write(ws->ssl, buf, (int)len);
	if (n > 0) {
		ws->write_blocked_on_read = 0;
		return n;
	}

	int err = SSL_get_error(ws->ssl, n);
	if (err == SSL_ERROR_WANT_WRITE) {
		errno = EAGAIN;
		return -1;
	}
	if (err == SSL_ERROR_WANT_READ) {
		/* OpenSSL 要求：SSL_write 返回 WANT_READ 后必须先用 SSL_read 推进，
		 * 再续写，否则报 bad write retry。置标志，等 EPOLLIN 读取后再写。 */
		ws->write_blocked_on_read = 1;
		errno = EAGAIN;
		return -1;
	}
	if (err == SSL_ERROR_ZERO_RETURN)
		return 0;

	return -1;
}

/* 立即尝试把待发送缓冲写完（0 继续/完成，-1 连接错误） */
static int wss_client_flush_pending(struct ez_wss_client_handle *ws)
{
	if (!ws->pending_send.is_active)
		return 0;

	/* SSL_write 阻塞在读上：先不写，等 EPOLLIN 的 SSL_read 推进后再续写，
	 * 否则会触发 OpenSSL bad write retry */
	if (ws->write_blocked_on_read)
		return 0;

	/* 必须用与最初 SSL_write 完全相同的“缓冲区基址 + 全长”重试：
	 * OpenSSL 以 wpend_buf 校验缓冲区身份，偏移或长度不一致会触发
	 * bad write retry。未用 SSL_MODE_ENABLE_PARTIAL_WRITE 时，
	 * SSL_write 只会返回全长或 -1（WANT_*），故 sent_len 在完成前恒为 0，
	 * 这里始终以基址+全长调用即等价于“相同参数重试”。 */
	ssize_t sent = wss_client_write(ws,
	                                ws->pending_send.frame_data,
	                                ws->pending_send.frame_len);
	if (sent < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 0; /* 等待 EPOLLOUT / 等 EPOLLIN 推进 */
		return -1;
	}

	if (sent > 0) {
		ws->pending_send.sent_len += sent;
		ws->last_tx_time_ms = ez_wss_client_now_ms();
	}

	if (ws->pending_send.sent_len >= ws->pending_send.frame_len) {
		/* 发送完成 */
		free(ws->pending_send.frame_data);
		ws->pending_send.frame_data = NULL;
		ws->pending_send.is_active = 0;
		ws->pending_send.sent_len = 0;
		ws->pending_send.frame_len = 0;
	}

	return 0;
}

/* 创建客户端 SSL_CTX（仅校验与初始握手；tls_verify_peer=1 时 trust tls_ca_path 或内置 CA） */
static SSL_CTX *wss_client_create_ssl_ctx(const struct ez_wss_client_config *config)
{
	SSL_CTX *ctx;

	if (!OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS |
	                      OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL))
		return NULL;

	ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx)
		return NULL;

	SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

	if (config->tls_verify_peer) {
		SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
		if (config->tls_ca_path) {
			/* 显式指定受信 CA 文件 */
			if (SSL_CTX_load_verify_locations(ctx, config->tls_ca_path, NULL) <= 0) {
				fprintf(stderr, "[wss] Failed to load CA: %s\n", config->tls_ca_path);
				ERR_print_errors_fp(stderr);
				SSL_CTX_free(ctx);
				return NULL;
			}
		} else {
			/* 内置 CA 互认：trust 库内置的同一把 CA（与服务器内置 CA 同源） */
			char ca_pem[4096];
			int n = ez_ws_export_builtin_ca_pem(ca_pem, sizeof(ca_pem));
			if (n <= 0) {
				fprintf(stderr, "[wss] Failed to obtain built-in CA\n");
				SSL_CTX_free(ctx);
				return NULL;
			}

			BIO *b = BIO_new_mem_buf(ca_pem, n - 1);
			X509 *ca = NULL;
			if (b)
				ca = PEM_read_bio_X509(b, NULL, NULL, NULL);
			if (b)
				BIO_free(b);
			if (!ca) {
				fprintf(stderr, "[wss] Failed to parse built-in CA\n");
				SSL_CTX_free(ctx);
				return NULL;
			}
			if (X509_STORE_add_cert(SSL_CTX_get_cert_store(ctx), ca) != 1) {
				X509_free(ca);
				SSL_CTX_free(ctx);
				return NULL;
			}
			X509_free(ca);
		}
	} else {
		/* 降级档：仅加密、不认证 */
		SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	}

	return ctx;
}

/* TLS 握手阶段的电平触发事件掩码：want_write 决定是否挂 EPOLLOUT，
 * 不挂 EPOLLET（避免写就绪无跃迁时 EPOLLOUT 不再触发的握手死锁） */
static void wss_client_set_handshake_events(struct ez_wss_client_handle *ws, int want_write)
{
	struct epoll_event ev;

	ev.events = EPOLLIN | (want_write ? EPOLLOUT : 0);
	ev.data.fd = ws->sockfd;
	epoll_ctl(ws->epollfd, EPOLL_CTL_MOD, ws->sockfd, &ev);
}

/* 驱动 TLS 握手（非阻塞，epoll 事件驱动）：
 * 返回 1 握手完成、0 仍在进行（等待事件）、-1 错误 */
static int wss_client_tls_handshake(struct ez_wss_client_handle *ws)
{
	int r = SSL_connect(ws->ssl);

	if (r == 1) {
		ws->tls_handshake_done = 1;

		/* TLS 完成：切回 EPOLLET 数据面（与明文客户端一致） */
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
		ev.data.fd = ws->sockfd;
		epoll_ctl(ws->epollfd, EPOLL_CTL_MOD, ws->sockfd, &ev);

		/* TLS 完成：构造并投递 HTTP/WS 握手请求（走 pending_send，非阻塞发送） */
		char handshake[1024];
		int handshake_len = ez_wss_create_handshake_request(ws, handshake, sizeof(handshake));
		if (handshake_len <= 0)
			return -1;

		uint8_t *buf = malloc((size_t)handshake_len);
		if (!buf)
			return -1;
		memcpy(buf, handshake, (size_t)handshake_len);

		ws->pending_send.frame_data = NULL;
		ws->pending_send.frame_len = 0;
		ws->pending_send.sent_len = 0;
		ws->pending_send.is_active = 1;
		ws->pending_send.frame_data = buf;
		ws->pending_send.frame_len = (size_t)handshake_len;

		/* 立即尝试发送一次 */
		return wss_client_flush_pending(ws);
	}

	int err = SSL_get_error(ws->ssl, r);
	if (err == SSL_ERROR_WANT_READ) {
		wss_client_set_handshake_events(ws, 0);
		return 0;
	}
	if (err == SSL_ERROR_WANT_WRITE) {
		wss_client_set_handshake_events(ws, 1);
		return 0;
	}

	return -1;
}

/* 生成WebSocket握手密钥 */
static void generate_websocket_key(char *key, size_t key_size) {
	unsigned char random_bytes[16];
	FILE *urandom = fopen("/dev/urandom", "rb");
	if (urandom) {
		fread(random_bytes, 1, 16, urandom);
		fclose(urandom);
	} else {
		time_t t = time(NULL);
		memcpy(random_bytes, &t, sizeof(t));
		memcpy(random_bytes + sizeof(t), &t, 16 - sizeof(t));
	}

	char base64_key[32] = {0};
	ez_base64encode(base64_key, (const char *)random_bytes, 16);
	snprintf(key, key_size, "%s", base64_key);
}

/* 计算WebSocket Accept值 */
static int compute_accept(const char *key, char *accept) {
	char combined[256];
	snprintf(combined, sizeof(combined), "%s%s", key, EZ_WEBSOCKET_MAGIC_STRING);

	char hash2[SHA1HashSize] = {0};

	SHA1Context sha;
	SHA1Reset(&sha);
	SHA1Input(&sha, (const unsigned char *)combined, strlen(combined));
	SHA1Result(&sha, (unsigned char *)hash2);

	char base64_accept[32] = {0};
	ez_base64encode(base64_accept, (const char *)hash2, SHA1HashSize);
	snprintf(accept, 32, "%s", base64_accept);
	return 0;
}

/* 创建WebSocket握手请求 */
static int ez_wss_create_handshake_request(struct ez_wss_client_handle *ws, char *buffer, size_t buffer_size) {
	char key[32] = {0};
	generate_websocket_key(key, sizeof(key));

	SAFE_FREE(ws->handshake_key);
	ws->handshake_key = strdup(key);

	char host[256] = {0};
	snprintf(host, sizeof(host), "%s:%hu", ws->server_addr, ws->port);

	return snprintf(buffer, buffer_size,
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Key: %s\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"Sec-WebSocket-Protocol: %s\r\n"
		"\r\n",
		ws->url_path, host, key, ws->protocol ? ws->protocol : "");
}

/* HTTP 解析器回调：状态行 */
static int on_http_status(http_parser *parser, const char *at, size_t length) {
	(void)parser; (void)at; (void)length;
	return 0;
}

/* HTTP 解析器回调：头部字段 */
static int on_http_header_field(http_parser *parser, const char *at, size_t length) {
	struct ez_wss_client_handle *ws = (struct ez_wss_client_handle *)parser->data;

	if (ws->http_header_field)
		free(ws->http_header_field);
	ws->http_header_field = malloc(length + 1);
	if (!ws->http_header_field)
		return -1;
	memcpy(ws->http_header_field, at, length);
	ws->http_header_field[length] = '\0';

	return 0;
}

/* HTTP 解析器回调：头部值 */
static int on_http_header_value(http_parser *parser, const char *at, size_t length) {
	struct ez_wss_client_handle *ws = (struct ez_wss_client_handle *)parser->data;

	if (ws->http_header_value)
		free(ws->http_header_value);
	ws->http_header_value = malloc(length + 1);
	if (!ws->http_header_value)
		return -1;
	memcpy(ws->http_header_value, at, length);
	ws->http_header_value[length] = '\0';

	/* 检查是否是 Sec-WebSocket-Accept */
	if (ws->http_header_field &&
	    strcasecmp(ws->http_header_field, "Sec-WebSocket-Accept") == 0) {
		if (ws->http_sec_websocket_accept)
			free(ws->http_sec_websocket_accept);
		ws->http_sec_websocket_accept = strdup(ws->http_header_value);
	}

	return 0;
}

/* HTTP 解析器回调：头部完成 */
static int on_http_headers_complete(http_parser *parser) {
	struct ez_wss_client_handle *ws = (struct ez_wss_client_handle *)parser->data;

	/* 检查状态码 */
	if (parser->status_code != 101) {
		EZ_PRINT_LOG_ERROR("WebSocket handshake failed: invalid status code %d\n", parser->status_code);
		return -1;
	}

	/* 检查 Upgrade 和 Connection 标志 */
	if (!(parser->flags & F_UPGRADE) || !(parser->flags & F_CONNECTION_UPGRADE)) {
		EZ_PRINT_LOG_ERROR("WebSocket handshake failed: missing Upgrade or Connection: Upgrade header\n");
		return -1;
	}

	/* 验证 Sec-WebSocket-Accept */
	if (!ws->http_sec_websocket_accept) {
		EZ_PRINT_LOG_ERROR("WebSocket handshake failed: missing Sec-WebSocket-Accept header\n");
		return -1;
	}

	char expected_accept[64];
	compute_accept(ws->handshake_key, expected_accept);
	if (strcmp(ws->http_sec_websocket_accept, expected_accept) != 0) {
		EZ_PRINT_LOG_ERROR("WebSocket handshake failed: invalid Sec-WebSocket-Accept\n");
		EZ_PRINT_LOG_ERROR("  Expected: %s\n", expected_accept);
		EZ_PRINT_LOG_ERROR("  Received: %s\n", ws->http_sec_websocket_accept);
		return -1;
	}

	ws->http_handshake_complete = 1;
	return 0;
}

// 如果系统没有 sys/random.h，我们手动定义标志位和函数
#ifndef HAVE_GETRANDOM

#ifndef GRND_NONBLOCK
#define GRND_NONBLOCK 0x0001
#endif

#ifndef GRND_RANDOM
#define GRND_RANDOM 0x0002
#endif

static ssize_t getrandom(void *buf, size_t buflen, unsigned int flags) {
    int fd;
    ssize_t nread;
    (void)flags;

    fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        return -1;
    }

    size_t total_read = 0;
    while (total_read < buflen) {
        nread = read(fd, (char *)buf + total_read, buflen - total_read);
        if (nread <= 0) {
            if (errno == EINTR) {
                continue;
            }
            close(fd);
            return -1;
        }
        total_read += nread;
    }

    close(fd);
    return (ssize_t)buflen;
}

#endif // HAVE_GETRANDOM

/* 创建WebSocket帧（客户端必须 mask） */
static int create_ws_frame(const uint8_t *payload, size_t payload_len,
                          int opcode, int fin, uint8_t *frame, size_t *frame_len) {
	size_t pos = 0;

	frame[pos++] = (fin ? 0x80 : 0x00) | (opcode & 0x0F);

	if (payload_len < 126) {
		frame[pos++] = 0x80 | payload_len;
	} else if (payload_len < 65536) {
		frame[pos++] = 0x80 | 126;
		frame[pos++] = (payload_len >> 8) & 0xFF;
		frame[pos++] = payload_len & 0xFF;
	} else {
		frame[pos++] = 0x80 | 127;
		int i;
		for (i = 7; i >= 0; i--) {
			frame[pos++] = (payload_len >> (i * 8)) & 0xFF;
		}
	}

	uint8_t masking_key[4];

	ssize_t ret = getrandom(masking_key, sizeof(masking_key), GRND_NONBLOCK);
	if (ret != sizeof(masking_key)) {
		time_t t = time(NULL);
		pid_t pid = getpid();
		uint32_t combined = (uint32_t)(t ^ (pid << 16) ^ ((uint32_t)rand()));
		memcpy(masking_key, &combined, 4);
	}

	memcpy(frame + pos, masking_key, 4);
	pos += 4;

	size_t i;
	for (i = 0; i < payload_len; i++) {
		frame[pos + i] = payload[i] ^ masking_key[i % 4];
	}
	pos += payload_len;

	*frame_len = pos;
	return 0;
}

/* WebSocket 解析器回调：帧开始 */
static int on_ws_frame_begin(ez_websocket_parser *parser) {
	struct ez_wss_client_handle *ws = (struct ez_wss_client_handle *)parser->data;

	EZ_PRINT_LOG_DEBUG("on_ws_frame_begin: opcode=%d, fin=%d, has_mask=%d\n",
	          parser->opcode, parser->fin, parser->has_mask);

	SAFE_FREE(ws->current_frame.buffer);

	ws->current_frame.opcode = parser->opcode;
	ws->current_frame.is_binary = (parser->opcode == EZ_WS_OPCODE_BINARY);
	ws->current_frame.buffer_size = 0;
	ws->current_frame.buffer_cap = 65536;
	ws->current_frame.buffer = calloc(1, ws->current_frame.buffer_cap);
	if (!ws->current_frame.buffer) {
		return -1;
	}

	return 0;
}

/* WebSocket 解析器回调：帧负载 */
static int on_ws_frame_payload(ez_websocket_parser *parser, const char *at, size_t length) {
	struct ez_wss_client_handle *ws = (struct ez_wss_client_handle *)parser->data;

	if (EZ_PRINT_LOG_DEBUG_ENABLED)
	{
		char debug_buf[256];
		size_t show_len = length < sizeof(debug_buf) - 1 ? length : sizeof(debug_buf) - 1;
		memcpy(debug_buf, at, show_len);
		debug_buf[show_len] = '\0';
		EZ_PRINT_LOG_DEBUG("on_ws_frame_payload: opcode=%d, has_mask=%d, length=%zu, data=%s\n",
		          parser->opcode, parser->has_mask, length, debug_buf);
	}

	if (ws->current_frame.buffer_size + length > ws->current_frame.buffer_cap) {
		size_t new_cap = ws->current_frame.buffer_cap * 2;
		while (new_cap < ws->current_frame.buffer_size + length)
			new_cap *= 2;
		uint8_t *new_buf = realloc(ws->current_frame.buffer, new_cap);
		if (!new_buf)
			return -1;
		ws->current_frame.buffer = new_buf;
		ws->current_frame.buffer_cap = new_cap;
	}

	memcpy(ws->current_frame.buffer + ws->current_frame.buffer_size, at, length);
	ws->current_frame.buffer_size += length;

	return 0;
}

/* WebSocket 解析器回调：帧完成 */
static int on_ws_frame_complete(ez_websocket_parser *parser) {
	struct ez_wss_client_handle *ws = (struct ez_wss_client_handle *)parser->data;

	switch (ws->current_frame.opcode) {
	case EZ_WS_OPCODE_TEXT:
	case EZ_WS_OPCODE_BINARY:
		{
			EZ_PRINT_LOG_DEBUG("on_ws_frame_complete: opcode=%d, buffer_size=%zu\n",
			          ws->current_frame.opcode, ws->current_frame.buffer_size);
			size_t show = ws->current_frame.buffer_size > 512 ? 512 : ws->current_frame.buffer_size;
			char buf[513];
			if (show)
				memcpy(buf, ws->current_frame.buffer, show);
			buf[show] = '\0';
			EZ_PRINT_LOG_DEBUG("Buffer content before callback: %s\n", buf);
			EZ_PRINT_LOG_INFO("RECV[%zu]: %s%s\n", ws->current_frame.buffer_size, buf,
			         ws->current_frame.buffer_size > show ? "..." : "");
		}

#if defined(EZ_WS_CLIENT_ENABLE_STATS) && (EZ_WS_CLIENT_ENABLE_STATS == 1)
		if (ws->current_frame.opcode == EZ_WS_OPCODE_TEXT) {
			ws->stats.rx_text_count++;
			ws->stats.rx_text_bytes += ws->current_frame.buffer_size;
		} else if (ws->current_frame.opcode == EZ_WS_OPCODE_BINARY) {
			ws->stats.rx_binary_count++;
			ws->stats.rx_binary_bytes += ws->current_frame.buffer_size;
		}
#endif /* EZ_WS_CLIENT_ENABLE_STATS */

		if (ws->callbacks.on_receive && ws->current_frame.buffer_size > 0) {
			uint8_t *data_copy = malloc(ws->current_frame.buffer_size);
			if (data_copy) {
				memcpy(data_copy, ws->current_frame.buffer, ws->current_frame.buffer_size);
				ws->callbacks.on_receive(
					data_copy,
					ws->current_frame.buffer_size,
					ws->current_frame.is_binary,
					ws->callbacks.user_data
				);
				free(data_copy);
			} else {
				ws->callbacks.on_receive(
					ws->current_frame.buffer,
					ws->current_frame.buffer_size,
					ws->current_frame.is_binary,
					ws->callbacks.user_data
				);
			}
		}
		break;

	case EZ_WS_OPCODE_CLOSE:
		EZ_PRINT_LOG_INFO("=== Connection closed by server ===\n");
#if defined(EZ_WS_CLIENT_ENABLE_STATS) && (EZ_WS_CLIENT_ENABLE_STATS == 1)
		ws->stats.rx_close_count++;
#endif /* EZ_WS_CLIENT_ENABLE_STATS */
		break;

	case EZ_WS_OPCODE_PING:
#if defined(EZ_WS_CLIENT_ENABLE_STATS) && (EZ_WS_CLIENT_ENABLE_STATS == 1)
		ws->stats.rx_ping_count++;
#endif /* EZ_WS_CLIENT_ENABLE_STATS */
		EZ_PRINT_LOG_INFO("Ping received from server (payload: %zu bytes), sending Pong\n",
		         ws->current_frame.buffer_size);
		{
			if (ez_wss_send_control_frame(ws, EZ_WS_OPCODE_PONG,
						  ws->current_frame.buffer,
						  ws->current_frame.buffer_size) < 0) {
				EZ_PRINT_LOG_WARN("Failed to reply pong, closing connection soon if problem persists\n");
			} else {
				EZ_PRINT_LOG_INFO("Pong sent successfully\n");
			}
		}
		break;

	case EZ_WS_OPCODE_PONG:
#if defined(EZ_WS_CLIENT_ENABLE_STATS) && (EZ_WS_CLIENT_ENABLE_STATS == 1)
		ws->stats.rx_pong_count++;
#endif /* EZ_WS_CLIENT_ENABLE_STATS */
		ws->awaiting_pong = 0;
		{
			uint64_t now_ms = ez_wss_client_now_ms();
			uint64_t rtt_ms = now_ms - ws->last_ping_time_ms;
			EZ_PRINT_LOG_INFO("Pong received from server (RTT: %llu ms)\n",
			         (unsigned long long)rtt_ms);
			ws->last_rx_time_ms = now_ms;
			ws->last_activity_time_ms = now_ms;
		}
		break;

	default:
		break;
	}

	/* 清理帧数据 */
	SAFE_FREE(ws->current_frame.buffer);
	ws->current_frame.buffer_size = 0;

	return 0;
}

/* 连接到服务器 */
static int ez_wss_connect(struct ez_wss_client_handle *ws) {
	if (ws->interrupted || !ws->ready) {
		return -1;
	}

	struct addrinfo hints, *result, *rp;
	char port_str[16];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	snprintf(port_str, sizeof(port_str), "%hu", ws->port);

	int ret = getaddrinfo(ws->server_addr, port_str, &hints, &result);
	if (ret != 0) {
		if (!ws->interrupted) {
			EZ_PRINT_LOG_ERROR("getaddrinfo failed: %s\n", gai_strerror(ret));
		}
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		if (ws->interrupted || !ws->ready) {
			freeaddrinfo(result);
			return -1;
		}

		ws->sockfd = socket(rp->ai_family,
		                    rp->ai_socktype,
		                    rp->ai_protocol);
		if (ws->sockfd < 0)
			continue;

		if (connect(ws->sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;

		close(ws->sockfd);
		ws->sockfd = -1;
	}

	freeaddrinfo(result);

	if (ws->sockfd < 0) {
		if (!ws->interrupted) {
			EZ_PRINT_LOG_WARN("Failed to connect to %s:%d\n", ws->server_addr, ws->port);
		}
		return -1;
	}

	/* 设置为非阻塞 */
	ez_websocket_set_nonblocking(ws->sockfd);

	/* 禁用 Nagle 算法（降低延迟） */
	int opt = 1;
	if (setsockopt(ws->sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0)
		return -1;

	/* 重置接收缓冲区和解析器 */
	ws->recv_buffer_len = 0;
	ez_websocket_parser_init(&ws->ws_parser);
	ws->ws_parser.data = ws;
	SAFE_FREE(ws->current_frame.buffer);
	ws->current_frame.buffer_size = 0;

	/* 添加到epoll。TLS 握手阶段用电平触发（不挂 EPOLLET），
	 * 避免写就绪无跃迁时 EPOLLOUT 不再触发的握手死锁；
	 * 握手完成后由 wss_client_tls_handshake 切回 EPOLLET 数据面。 */
	struct epoll_event ev;
	ev.events = ws->tls_enable ? (EPOLLIN | EPOLLOUT) : (EPOLLIN | EPOLLOUT | EPOLLET);
	ev.data.fd = ws->sockfd;
	if (epoll_ctl(ws->epollfd, EPOLL_CTL_ADD, ws->sockfd, &ev) < 0) {
		close(ws->sockfd);
		ws->sockfd = -1;
		return -1;
	}

	ws->state = EZ_WS_STATE_HANDSHAKING;

	/* 记录连接开始时间（用于超时检测） */
	ws->connect_start_time_ms = ez_wss_client_now_ms();

	if (ws->tls_enable) {
		/* TLS 路径：建 SSL 会话，握手由 epoll 驱动（先 SSL_connect，完成后再发 HTTP 握手） */
		ws->ssl = SSL_new(ws->ssl_ctx);
		if (!ws->ssl) {
			epoll_ctl(ws->epollfd, EPOLL_CTL_DEL, ws->sockfd, NULL);
			close(ws->sockfd);
			ws->sockfd = -1;
			ws->state = EZ_WS_STATE_DISCONNECTED;
			return -1;
		}
		SSL_set_fd(ws->ssl, ws->sockfd);
		ws->tls_handshake_done = 0;
	} else {
		/* 明文路径：直接发送握手请求（与明文版一致） */
		ws->tls_handshake_done = 1;
		char handshake[1024];
		int handshake_len = ez_wss_create_handshake_request(ws, handshake, sizeof(handshake));
		if (handshake_len < 0 || send(ws->sockfd, handshake, handshake_len, 0) < 0) {
			EZ_PRINT_LOG_ERROR("Failed to send handshake\n");
			epoll_ctl(ws->epollfd, EPOLL_CTL_DEL, ws->sockfd, NULL);
			close(ws->sockfd);
			ws->sockfd = -1;
			ws->state = EZ_WS_STATE_DISCONNECTED;
			return -1;
		}
	}

	EZ_PRINT_LOG_INFO("Connecting to %s:%d%s (%s)\n",
	                  ws->server_addr, ws->port, ws->url_path,
	                  ws->tls_enable ? "wss" : "ws");
	return 0;
}

/* 处理握手响应（TLS 与明文共用，wss_read 内部切换） */
static int ez_wss_handle_handshake(struct ez_wss_client_handle *ws) {
	if (ws->handshake_response_len >= sizeof(ws->handshake_response) - 1) {
		EZ_PRINT_LOG_ERROR("WebSocket handshake failed: response buffer overflow\n");
		return -1;
	}

	ssize_t n = wss_client_read(ws, ws->handshake_response + ws->handshake_response_len,
	                            sizeof(ws->handshake_response) - ws->handshake_response_len - 1);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 0;
		EZ_PRINT_LOG_ERROR("WebSocket handshake failed: recv error: %s\n", strerror(errno));
		return -1;
	}

	if (n == 0) {
		EZ_PRINT_LOG_ERROR("WebSocket handshake failed: connection closed by server\n");
		return -1;
	}

	ws->handshake_response_len += n;
	ws->handshake_response[ws->handshake_response_len] = '\0';

	(void)http_parser_execute(&ws->http_parser, &ws->http_parser_settings,
	                         ws->handshake_response,
	                         ws->handshake_response_len);

	if (HTTP_PARSER_ERRNO(&ws->http_parser) != HPE_OK) {
		EZ_PRINT_LOG_ERROR("WebSocket handshake failed: HTTP parse error: %s\n",
		          http_errno_description(HTTP_PARSER_ERRNO(&ws->http_parser)));
		return -1;
	}

	if (ws->http_handshake_complete) {
		ws->state = EZ_WS_STATE_CONNECTED;
		ws->retry_count = 0;
		ws->current_retry_delay = ws->config.reconnect_interval_ms;
		ws->awaiting_pong = 0;
#if EZ_WS_CLIENT_PING_INTERVAL_MS > 0
		ws->current_ping_interval_ms = ez_wss_client_get_ping_interval_with_jitter();
#else
		ws->current_ping_interval_ms = 0;
#endif
		{
			uint64_t now_ms = ez_wss_client_now_ms();
			ws->last_rx_time_ms = now_ms;
			ws->last_tx_time_ms = now_ms;
			ws->last_activity_time_ms = now_ms;
			ws->last_ping_time_ms = now_ms;
			ws->connect_start_time_ms = 0;
		}

		EZ_PRINT_LOG_INFO("=== Connection established ===\n");

		if (ws->callbacks.on_connected)
			ws->callbacks.on_connected(ws->callbacks.user_data);

		if (ws->heartbeat_timerfd >= 0) {
			ez_websocket_update_timerfd(ws->heartbeat_timerfd, EZ_WS_CLIENT_HEARTBEAT_TICK_MS);
		}

		ws->handshake_response_len = 0;

		return 1; /* 握手完成 */
	}

	return 0;
}

/* 处理WebSocket数据（TLS 与明文共用） */
static int ez_wss_handle_data(struct ez_wss_client_handle *ws) {
	ssize_t n = wss_client_read(ws, ws->recv_buffer + ws->recv_buffer_len,
	                            sizeof(ws->recv_buffer) - ws->recv_buffer_len);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 0;
		return -1;
	}
	if (n == 0)
		return -1; /* 连接关闭 */

	ws->recv_buffer_len += n;
	{
		uint64_t now_ms = ez_wss_client_now_ms();
		ws->last_rx_time_ms = now_ms;
		ws->last_activity_time_ms = now_ms;
	}

	size_t parsed = ez_websocket_parser_execute(&ws->ws_parser, &ws->ws_parser_settings,
	                                        (const char *)ws->recv_buffer,
	                                        ws->recv_buffer_len);

	if (ws->ws_parser.close_received) {
		return -1;
	}

	if (EZ_WEBSOCKET_PARSER_ERRNO(&ws->ws_parser) != EZ_WSE_OK) {
		EZ_PRINT_LOG_ERROR("WebSocket parse error: %s\n",
		          ez_websocket_errno_description(EZ_WEBSOCKET_PARSER_ERRNO(&ws->ws_parser)));
		return -1;
	}

	if (parsed > 0 && parsed <= ws->recv_buffer_len) {
		if (parsed < ws->recv_buffer_len) {
			memmove(ws->recv_buffer, ws->recv_buffer + parsed, ws->recv_buffer_len - parsed);
		}
		ws->recv_buffer_len -= parsed;
	}

	return 0;
}

/* 发送 WebSocket 帧的内部实现（TLS 走 SSL_write） */
static int ez_wss_send_internal(struct ez_wss_client_handle *ws, const void *data, size_t len, int opcode) {
	if (ws->state != EZ_WS_STATE_CONNECTED)
		return EZ_WS_ERR_NOT_CONNECTED;

	/* 如果已有待发送的数据，先尝试发送它 */
	if (ws->pending_send.is_active) {
		if (wss_client_flush_pending(ws) < 0) {
			return -1;
		}
		if (ws->pending_send.is_active) {
			return EZ_WS_ERR_QUEUE_FULL;
		}
	}

	/* 创建新的帧 */
	size_t frame_cap = len + EZ_WS_MAX_FRAME_OVERHEAD;
	uint8_t *frame = malloc(frame_cap);
	size_t frame_len;

	if (!frame)
		return EZ_WS_ERR_NO_MEMORY;

	if (create_ws_frame((const uint8_t *)data, len, opcode, 1, frame, &frame_len) < 0) {
		free(frame);
		return EZ_WS_ERR_NO_MEMORY;
	}

	/* SSL_write 阻塞在读上：直接写会触发 OpenSSL bad write retry，先挂起该帧，
	 * 等 EPOLLIN 读取推进 TLS 状态后再由 service 线程续写 */
	if (ws->write_blocked_on_read) {
		ws->pending_send.frame_data = frame;
		ws->pending_send.frame_len = frame_len;
		ws->pending_send.sent_len = 0;
		ws->pending_send.is_active = 1;
		return EZ_WS_ERR_QUEUE_FULL;
	}

	ssize_t sent = wss_client_write(ws, frame, frame_len);
	if (sent < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			/* WANT_READ / WANT_WRITE：必须保留传入 SSL_write 的原始缓冲区与长度，
			 * 重试时原样（相同指针 + 相同长度）再次调用。OpenSSL 内部用 wpend_buf
			 * 记录该缓冲区的身份与内部偏移（wpend_off），若此处复制/释放 buffer，
			 * 重试时 wpend_buf != 新指针 → 触发 bad write retry。绝不能 free(frame)。 */
			ws->pending_send.frame_data = frame;
			ws->pending_send.frame_len = frame_len;
			ws->pending_send.sent_len = 0;
			ws->pending_send.is_active = 1;
			return EZ_WS_ERR_QUEUE_FULL;
		}
		free(frame);
		return -1;
	}

	if (sent > 0) {
		uint64_t now_ms = ez_wss_client_now_ms();
		ws->last_tx_time_ms = now_ms;
		ws->last_activity_time_ms = now_ms;
	}

	if (sent < (ssize_t)frame_len) {
		ws->pending_send.frame_data = malloc(frame_len - sent);
		if (!ws->pending_send.frame_data) {
			free(frame);
			return EZ_WS_ERR_NO_MEMORY;
		}
		memcpy(ws->pending_send.frame_data, frame + sent, frame_len - sent);
		ws->pending_send.frame_len = frame_len - sent;
		ws->pending_send.sent_len = 0;
		ws->pending_send.is_active = 1;
		free(frame);
		return EZ_WS_ERR_QUEUE_FULL;
	}

	free(frame);

	if (ws->callbacks.on_sent)
		ws->callbacks.on_sent(data, len, ws->callbacks.user_data);

#if defined(EZ_WS_CLIENT_ENABLE_STATS) && (EZ_WS_CLIENT_ENABLE_STATS == 1)
	switch (opcode) {
	case EZ_WS_OPCODE_TEXT:
		ws->stats.tx_text_count++;
		ws->stats.tx_text_bytes += len;
		break;
	case EZ_WS_OPCODE_BINARY:
		ws->stats.tx_binary_count++;
		ws->stats.tx_binary_bytes += len;
		break;
	case EZ_WS_OPCODE_PING:
		ws->stats.tx_ping_count++;
		break;
	case EZ_WS_OPCODE_PONG:
		ws->stats.tx_pong_count++;
		break;
	case EZ_WS_OPCODE_CLOSE:
		ws->stats.tx_close_count++;
		break;
	default:
		break;
	}
#endif /* EZ_WS_CLIENT_ENABLE_STATS */

	{
		size_t show = len > 512 ? 512 : len;
		char buf[513];
		if (show)
			memcpy(buf, data, show);
		buf[show] = '\0';
		EZ_PRINT_LOG_INFO("SEND[%zu]: %s%s\n", len, buf, len > show ? "..." : "");
	}

	return EZ_WS_OK;
}

/* 发送控制帧（例如Ping） */
static int ez_wss_send_control_frame(struct ez_wss_client_handle *ws, int opcode,
				 const void *payload, size_t len)
{
	if (!ws || ws->state != EZ_WS_STATE_CONNECTED)
		return -1;
	if (len && !payload)
		return -1;

	size_t frame_cap = len + EZ_WS_MAX_FRAME_OVERHEAD;
	uint8_t stack_buf[EZ_WS_MAX_FRAME_OVERHEAD + 128];
	uint8_t *frame = stack_buf;

	if (frame_cap > sizeof(stack_buf)) {
		frame = malloc(frame_cap);
		if (!frame)
			return -1;
	}

	size_t frame_len = 0;
	if (create_ws_frame(payload ? (const uint8_t *)payload : NULL, len, opcode,
	                    1, frame, &frame_len) < 0) {
		if (frame != stack_buf)
			free(frame);
		return -1;
	}

	/* SSL_write 阻塞在读上：直接写会触发 bad write retry，控制帧可丢弃（心跳会重发） */
	if (ws->write_blocked_on_read) {
		if (frame != stack_buf)
			free(frame);
		return 0;
	}

	ssize_t sent = wss_client_write(ws, frame, frame_len);
	if (frame != stack_buf)
		free(frame);

	if (sent < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 0; /* 非阻塞：由 EPOLLOUT 继续（控制帧可丢弃，下一心跳会重发） */
		return -1;
	}
	if (sent != (ssize_t)frame_len)
		return 0; /* 部分发送：同样由后续 EPOLLOUT 兜底 */

	uint64_t now_ms = ez_wss_client_now_ms();
	ws->last_tx_time_ms = now_ms;
	ws->last_activity_time_ms = now_ms;

#if defined(EZ_WS_CLIENT_ENABLE_STATS) && (EZ_WS_CLIENT_ENABLE_STATS == 1)
	switch (opcode) {
	case EZ_WS_OPCODE_PING:
		ws->stats.tx_ping_count++;
		break;
	case EZ_WS_OPCODE_PONG:
		ws->stats.tx_pong_count++;
		break;
	case EZ_WS_OPCODE_CLOSE:
		ws->stats.tx_close_count++;
		break;
	default:
		break;
	}
#endif /* EZ_WS_CLIENT_ENABLE_STATS */

	return 0;
}

/* 调度重连 */
static void ez_wss_schedule_reconnect(struct ez_wss_client_handle *ws)
{
	if (!ws)
		return;
	if (ws->reconnect_timerfd >= 0 && !ws->interrupted) {
		ws->retry_count++;
		if (ws->config.reconnect_backoff_enable &&
		    ws->retry_count > ws->config.reconnect_backoff_min_retries &&
		    ws->retry_count <= ws->config.reconnect_backoff_high_threshold) {
			ws->current_retry_delay *= 2;
		}
		ez_websocket_update_timerfd(ws->reconnect_timerfd, ws->current_retry_delay);
	}
}

static void ez_wss_clear_send_buffer(struct ez_wss_client_handle *ws) {
	SAFE_FREE(ws->pending_send.frame_data);
	ws->pending_send.frame_len = 0;
	ws->pending_send.sent_len = 0;
	ws->pending_send.is_active = 0;
}

/* 释放当前连接的 TLS 会话（重连/清理时整链重建） */
static void ez_wss_reset_tls(struct ez_wss_client_handle *ws) {
	if (!ws)
		return;
	if (ws->ssl) {
		SSL_shutdown(ws->ssl); /* 尽力完成关闭握手，非阻塞场景结果忽略 */
		SSL_free(ws->ssl);
		ws->ssl = NULL;
	}
	ws->tls_handshake_done = 0;
}

static void ez_wss_client_handle_reset(struct ez_wss_client_handle *ws, unsigned int reset_flags) {
	if (!ws)
		return;

	if (ws->sockfd >= 0) {
		epoll_ctl(ws->epollfd, EPOLL_CTL_DEL, ws->sockfd, NULL);
		close(ws->sockfd);
		ws->sockfd = -1;
	}

	ez_wss_reset_tls(ws);

	ws->state = EZ_WS_STATE_DISCONNECTED;
	ez_wss_clear_send_buffer(ws);
	ws->awaiting_pong = 0;
	ws->last_rx_time_ms = 0;
	ws->last_tx_time_ms = 0;
	ws->last_activity_time_ms = 0;
	ws->last_ping_time_ms = 0;
	ws->connect_start_time_ms = 0;

	if (reset_flags & EZ_WSS_RESET_HTTP) {
		ws->handshake_response_len = 0;
		ws->http_handshake_complete = 0;

		http_parser_init(&ws->http_parser, HTTP_RESPONSE);
		ws->http_parser.data = ws;

		SAFE_FREE(ws->http_header_field);
		SAFE_FREE(ws->http_header_value);
		SAFE_FREE(ws->http_sec_websocket_accept);
	}

	if (reset_flags & EZ_WSS_RESET_WS) {
		ws->recv_buffer_len = 0;
		ez_websocket_parser_init(&ws->ws_parser);
		ws->ws_parser.data = ws;

		SAFE_FREE(ws->current_frame.buffer);
		ws->current_frame.buffer_size = 0;
		ws->current_frame.buffer_cap = 0;
	}

	if ((reset_flags & EZ_WSS_RESET_HEARTBEAT) && ws->heartbeat_timerfd >= 0) {
		struct itimerspec timer_spec = {0};
		timerfd_settime(ws->heartbeat_timerfd, 0, &timer_spec, NULL);
	}

	if (ws->callbacks.on_disconnected)
		ws->callbacks.on_disconnected(ws->callbacks.user_data);
}

/* 发送队列中的消息
 * 返回值：0 表示成功或队列为空，-1 表示连接错误（需要重连）
 */
static int ez_wss_process_send_queue(struct ez_wss_client_handle *ws) {
	pthread_mutex_lock(&ws->send_queue_lock);

	while (ws->send_queue_head) {
		struct send_queue_node *node = ws->send_queue_head;

		int ret = ez_wss_send_internal(ws, node->data, node->len, node->opcode);
		if (ret == EZ_WS_ERR_QUEUE_FULL) {
			pthread_mutex_unlock(&ws->send_queue_lock);
			return 0;
		} else if (ret != EZ_WS_OK) {
			pthread_mutex_unlock(&ws->send_queue_lock);
			return -1;
		}

		ws->send_queue_head = node->next;
		if (!ws->send_queue_head)
			ws->send_queue_tail = NULL;

		SAFE_FREE(node->data);
		SAFE_FREE(node);
		ws->send_queue_size--;
	}

	pthread_mutex_unlock(&ws->send_queue_lock);
	return 0;
}

/* WebSocket（wss）服务执行函数 - 执行一次事件循环迭代 */
int ez_wss_service_exec(struct ez_wss_client_handle *ws, int timeout_ms) {
	if (!ws)
		return -1;

	if (!ws->ready || ws->interrupted) {
		return -1;
	}

	/* 如果处于断开状态且未开始连接，尝试首次连接 */
	if (ws->state == EZ_WS_STATE_DISCONNECTED && ws->sockfd < 0 && ws->retry_count == 0) {
		if (ez_wss_connect(ws) < 0) {
			if (ws->reconnect_timerfd >= 0 && !ws->interrupted) {
				ws->retry_count++;
				ez_websocket_update_timerfd(ws->reconnect_timerfd, ws->current_retry_delay);
			}
		}
	}

	/* 检查连接超时（在握手阶段，含 TLS 握手） */
	if (ws->state == EZ_WS_STATE_HANDSHAKING && ws->connect_start_time_ms > 0) {
		uint64_t now_ms = ez_wss_client_now_ms();
		uint64_t elapsed_ms = now_ms - ws->connect_start_time_ms;

		if (elapsed_ms >= ws->config.connect_timeout_ms) {
			EZ_PRINT_LOG_WARN("Connection timeout after %llu ms, closing and reconnecting\n",
			         (unsigned long long)elapsed_ms);
			ez_wss_client_handle_reset(ws, EZ_WSS_RESET_HTTP);

			if (ws->reconnect_timerfd >= 0 && !ws->interrupted) {
				ws->retry_count++;
				if (ws->config.reconnect_backoff_enable &&
				    ws->retry_count > ws->config.reconnect_backoff_min_retries &&
				    ws->retry_count <= ws->config.reconnect_backoff_high_threshold) {
					ws->current_retry_delay *= 2;
				}
				ez_websocket_update_timerfd(ws->reconnect_timerfd, ws->current_retry_delay);
			}
		}
	}

	struct epoll_event events[10];

	int timeout = (timeout_ms > 0) ? timeout_ms : (ws->interrupted ? 10 : 100);
	int nfds = epoll_wait(ws->epollfd, events, 10, timeout);
	if (nfds < 0) {
		if (errno == EINTR)
			return 0;
		return -1;
	}

	int i;
	for (i = 0; i < nfds; i++) {
		if (events[i].data.fd == ws->sockfd) {
			if (events[i].events & EPOLLIN) {
				if (ws->state == EZ_WS_STATE_HANDSHAKING) {
					int handshake_result;
					/* TLS 路径：先驱动 SSL_connect（IN/OUT 皆可推进） */
					if (ws->tls_enable && !ws->tls_handshake_done) {
						int tr = wss_client_tls_handshake(ws);
						if (tr < 0) {
							ez_wss_client_handle_reset(ws, EZ_WSS_RESET_HTTP);
							ez_wss_schedule_reconnect(ws);
							continue;
						}
						if (tr == 0)
							continue; /* TLS 未完成，等下次事件 */
						/* TLS 完成：HTTP 握手请求已投递到 pending_send */
					}

					handshake_result = ez_wss_handle_handshake(ws);
					if (handshake_result < 0) {
						ez_wss_client_handle_reset(ws, EZ_WSS_RESET_HTTP);
						ez_wss_schedule_reconnect(ws);
					} else if (handshake_result == 0) {
						/* TLS：SSL 内部可能还有解密数据，排空 */
						while (ws->ssl && SSL_pending(ws->ssl) > 0) {
							int hr = ez_wss_handle_handshake(ws);
							if (hr < 0) {
								ez_wss_client_handle_reset(ws, EZ_WSS_RESET_HTTP);
								ez_wss_schedule_reconnect(ws);
								break;
							}
							if (hr > 0)
								break;
						}
					}
				} else if (ws->state == EZ_WS_STATE_CONNECTED) {
					if (ez_wss_handle_data(ws) < 0) {
						ez_wss_client_handle_reset(ws, EZ_WSS_RESET_WS | EZ_WSS_RESET_HEARTBEAT);
						ez_wss_schedule_reconnect(ws);
					} else {
						/* TLS：SSL 内部可能还有解密数据，排空（EPOLLET 下不会重复触发） */
						while (ws->ssl && SSL_pending(ws->ssl) > 0 &&
						       ws->state == EZ_WS_STATE_CONNECTED) {
							if (ez_wss_handle_data(ws) < 0) {
								ez_wss_client_handle_reset(ws, EZ_WSS_RESET_WS | EZ_WSS_RESET_HEARTBEAT);
								ez_wss_schedule_reconnect(ws);
								break;
							}
						}

						/* SSL_read 已推进 TLS 状态：若之前 SSL_write 阻塞在读上（WANT_READ），
						 * 现在立即续写，否则 EPOLLET 下 EPOLLOUT 不重新触发，pending_send 会
						 * 卡住，之后在错误时机重试写会触发 OpenSSL bad write retry */
						if (ws->pending_send.is_active && !ws->write_blocked_on_read) {
							if (wss_client_flush_pending(ws) < 0) {
								ez_wss_client_handle_reset(ws, EZ_WSS_RESET_WS | EZ_WSS_RESET_HEARTBEAT);
								ez_wss_schedule_reconnect(ws);
							}
						}
					}
				}
			}

			if (events[i].events & EPOLLOUT) {
				if (ws->state == EZ_WS_STATE_HANDSHAKING) {
					/* TLS 握手推进（客户端因需要写而触发 EPOLLOUT） */
					if (ws->tls_enable && !ws->tls_handshake_done) {
						int tr = wss_client_tls_handshake(ws);
						if (tr < 0) {
							ez_wss_client_handle_reset(ws, EZ_WSS_RESET_HTTP);
							ez_wss_schedule_reconnect(ws);
							continue;
						}
						if (tr == 0)
							continue;
						/* TLS 完成，HTTP 握手请求已投递到 pending_send，下面刷出 */
					}
					/* 刷出 HTTP 握手请求（TLS 完成后） */
					if (ws->pending_send.is_active) {
						if (wss_client_flush_pending(ws) < 0) {
							ez_wss_client_handle_reset(ws, EZ_WSS_RESET_HTTP);
							ez_wss_schedule_reconnect(ws);
						}
					}
				} else if (ws->state == EZ_WS_STATE_CONNECTED) {
					/* 先处理部分发送的缓冲区（TLS 走 SSL_write） */
					if (ws->pending_send.is_active) {
						if (wss_client_flush_pending(ws) < 0) {
							EZ_PRINT_LOG_ERROR("Send error, closing connection\n");
							ez_wss_client_handle_reset(ws, EZ_WSS_RESET_WS | EZ_WSS_RESET_HEARTBEAT);
							ez_wss_schedule_reconnect(ws);
							continue;
						}
					}

					if (!ws->pending_send.is_active) {
						if (ez_wss_process_send_queue(ws) < 0) {
							EZ_PRINT_LOG_ERROR("Send queue processing error, closing connection\n");
							ez_wss_client_handle_reset(ws, EZ_WSS_RESET_WS | EZ_WSS_RESET_HEARTBEAT);
							ez_wss_schedule_reconnect(ws);
						}
					}
				}
			}

			if (events[i].events & (EPOLLERR | EPOLLHUP)) {
				ez_wss_client_handle_reset(ws, EZ_WSS_RESET_HTTP | EZ_WSS_RESET_WS | EZ_WSS_RESET_HEARTBEAT);
				ez_wss_schedule_reconnect(ws);
			}
		} else if (events[i].data.fd == ws->reconnect_timerfd) {
			uint64_t expirations;
			read(ws->reconnect_timerfd, &expirations, sizeof(expirations));

			if (ws->interrupted || !ws->ready) {
				struct itimerspec timer_spec = {0};
				timerfd_settime(ws->reconnect_timerfd, 0, &timer_spec, NULL);
				continue;
			}

			if (ws->state == EZ_WS_STATE_DISCONNECTED) {
				if (ws->config.reconnect_max_retries == 0 || ws->retry_count < ws->config.reconnect_max_retries) {
					if (ez_wss_connect(ws) == 0) {
						struct itimerspec timer_spec = {0};
						timerfd_settime(ws->reconnect_timerfd, 0, &timer_spec, NULL);
					} else {
						ws->retry_count++;
						if (ws->config.reconnect_backoff_enable &&
						    ws->retry_count > ws->config.reconnect_backoff_min_retries &&
						    ws->retry_count <= ws->config.reconnect_backoff_high_threshold) {
							ws->current_retry_delay *= 2;
						}
						ez_websocket_update_timerfd(ws->reconnect_timerfd, ws->current_retry_delay);
					}
				} else {
					EZ_PRINT_LOG_WARN("Max retry count reached, stop reconnecting\n");
					ws->interrupted = 1;
				}
			}
		} else if (events[i].data.fd == ws->heartbeat_timerfd) {
			uint64_t expirations;
			read(ws->heartbeat_timerfd, &expirations, sizeof(expirations));

			if (ws->state != EZ_WS_STATE_CONNECTED)
				continue;

			if (ws->last_activity_time_ms == 0)
				continue;

			uint64_t now_ms = ez_wss_client_now_ms();

#if EZ_WS_CLIENT_PING_INTERVAL_MS > 0
			if (ws->awaiting_pong) {
				if (now_ms - ws->last_ping_time_ms >= EZ_WS_CLIENT_PONG_TIMEOUT_MS) {
					EZ_PRINT_LOG_WARN("Pong timeout, closing connection\n");
					ez_wss_client_handle_reset(ws, EZ_WSS_RESET_WS | EZ_WSS_RESET_HEARTBEAT);
					ez_wss_schedule_reconnect(ws);
				}
			} else {
				uint64_t idle_time_ms = now_ms - ws->last_activity_time_ms;

				if (idle_time_ms >= EZ_WS_CLIENT_IDLE_TIMEOUT_MS) {
					EZ_PRINT_LOG_WARN("Connection idle for %llu ms, closing\n",
					         (unsigned long long)idle_time_ms);
					ez_wss_client_handle_reset(ws, EZ_WSS_RESET_WS | EZ_WSS_RESET_HEARTBEAT);
					ez_wss_schedule_reconnect(ws);
				} else if (idle_time_ms >= ws->current_ping_interval_ms) {
					if (ez_wss_send_control_frame(ws, EZ_WS_OPCODE_PING, NULL, 0) == 0) {
						ws->awaiting_pong = 1;
						ws->last_ping_time_ms = now_ms;
						ws->current_ping_interval_ms = ez_wss_client_get_ping_interval_with_jitter();
						EZ_PRINT_LOG_INFO("Ping sent to server (next interval: %u ms)\n",
						         ws->current_ping_interval_ms);
					} else {
						EZ_PRINT_LOG_WARN("Failed to send ping, closing connection\n");
						ez_wss_client_handle_reset(ws, EZ_WSS_RESET_WS | EZ_WSS_RESET_HEARTBEAT);
						ez_wss_schedule_reconnect(ws);
					}
				}
			}
#else
			uint64_t idle_time_ms = now_ms - ws->last_activity_time_ms;

			if (idle_time_ms >= EZ_WS_CLIENT_IDLE_TIMEOUT_MS) {
				EZ_PRINT_LOG_WARN("Connection idle for %llu ms, closing\n",
				         (unsigned long long)idle_time_ms);
				ez_wss_client_handle_reset(ws, EZ_WSS_RESET_WS | EZ_WSS_RESET_HEARTBEAT);
				ez_wss_schedule_reconnect(ws);
			}
#endif
		}
	}

	return 0;
}

/* 公共API：发送文本 */
int ez_wss_send_text(struct ez_wss_client_handle *ws, const char *data, size_t len) {
	if (!ws || !data)
		return EZ_WS_ERR_INVALID_PARAM;

	if (!len)
		len = strlen(data);

	if (ws->state != EZ_WS_STATE_CONNECTED) {
		if (ws->send_queue_size < ws->send_queue_max) {
			struct send_queue_node *node = malloc(sizeof(struct send_queue_node));
			if (!node)
				return EZ_WS_ERR_NO_MEMORY;

			node->data = malloc(len + 1);
			if (!node->data) {
				free(node);
				return EZ_WS_ERR_NO_MEMORY;
			}

			memcpy(node->data, data, len);
			node->data[len] = '\0';
			node->len = len;
			node->opcode = EZ_WS_OPCODE_TEXT;
			node->next = NULL;

			pthread_mutex_lock(&ws->send_queue_lock);
			if (ws->send_queue_tail) {
				ws->send_queue_tail->next = node;
			} else {
				ws->send_queue_head = node;
			}
			ws->send_queue_tail = node;
			ws->send_queue_size++;
			pthread_mutex_unlock(&ws->send_queue_lock);

			return EZ_WS_OK;
		}
		return EZ_WS_ERR_NOT_CONNECTED;
	}

	return ez_wss_send_internal(ws, data, len, EZ_WS_OPCODE_TEXT);
}

/* 公共API：发送二进制 */
int ez_wss_send_binary(struct ez_wss_client_handle *ws, const void *data, size_t len) {
	if (!ws || !data || !len)
		return EZ_WS_ERR_INVALID_PARAM;

	if (ws->state != EZ_WS_STATE_CONNECTED)
		return EZ_WS_ERR_NOT_CONNECTED;

	return ez_wss_send_internal(ws, data, len, EZ_WS_OPCODE_BINARY);
}

/* 公共API：检查连接状态 */
int ez_wss_is_connected(struct ez_wss_client_handle *ws) {
	return (ws && ws->state == EZ_WS_STATE_CONNECTED) ? 1 : 0;
}

#if defined(EZ_WS_CLIENT_ENABLE_STATS) && (EZ_WS_CLIENT_ENABLE_STATS == 1)
/* 公共API：获取统计信息 */
int ez_wss_get_stats(struct ez_wss_client_handle *ws, struct ez_ws_client_stats *stats) {
	if (!ws)
		return -1;

	if (stats) {
		stats->tx_text_count = ws->stats.tx_text_count;
		stats->tx_text_bytes = ws->stats.tx_text_bytes;
		stats->tx_binary_count = ws->stats.tx_binary_count;
		stats->tx_binary_bytes = ws->stats.tx_binary_bytes;
		stats->tx_ping_count = ws->stats.tx_ping_count;
		stats->tx_pong_count = ws->stats.tx_pong_count;
		stats->tx_close_count = ws->stats.tx_close_count;

		stats->rx_text_count = ws->stats.rx_text_count;
		stats->rx_text_bytes = ws->stats.rx_text_bytes;
		stats->rx_binary_count = ws->stats.rx_binary_count;
		stats->rx_binary_bytes = ws->stats.rx_binary_bytes;
		stats->rx_ping_count = ws->stats.rx_ping_count;
		stats->rx_pong_count = ws->stats.rx_pong_count;
		stats->rx_close_count = ws->stats.rx_close_count;
	}

	return 0;
}
#endif /* EZ_WS_CLIENT_ENABLE_STATS */

/* 公共API：获取连接状态 */
enum ez_ws_state ez_wss_get_state(struct ez_wss_client_handle *ws) {
	if (!ws)
		return EZ_WS_STATE_DISCONNECTED;
	return ws->state;
}

/* 初始化WebSocket客户端（wss） */
struct ez_wss_client_handle *ez_wss_client_handle_create(struct ez_wss_client_config *config,
                                        struct ez_ws_callbacks *callbacks) {
	struct ez_wss_client_handle *ws = calloc(1, sizeof(struct ez_wss_client_handle));
	if (!ws)
		return NULL;

	/* 复制配置（如果提供） */
	if (config) {
		ws->config = *config;
	} else {
		ws->config.server_addr = "localhost";
		ws->config.port = 54321;
		ws->config.url_path = "/";
		ws->config.protocol = "fws_a-burning";
		ws->config.reconnect_interval_ms = RECONNECT_INTERVAL_MS;
		ws->config.reconnect_max_retries = RECONNECT_MAX_RETRIES;
		ws->config.reconnect_backoff_enable = RECONNECT_BACKOFF_ENABLE;
		ws->config.reconnect_backoff_min_retries = RECONNECT_BACKOFF_MIN_RETRIES;
		ws->config.reconnect_backoff_high_threshold = RECONNECT_BACKOFF_HIGH_THRESHOLD;
	}

	if (ws->config.connect_timeout_ms == 0) {
		ws->config.connect_timeout_ms = (1 * 1000);
	}

	/* TLS 开关（默认 1 开箱即加密）；每实例，作用于每条连接 */
	ws->tls_enable = (ws->config.tls_enable != 0) ? 1 : 0;
	ws->tls_verify_peer = (ws->config.tls_verify_peer != 0) ? 1 : 0;
	if (ws->config.tls_ca_path)
		ws->tls_ca_path = strdup(ws->config.tls_ca_path);

	ws->server_addr = strdup(ws->config.server_addr);
	ws->port = ws->config.port;
	ws->url_path = strdup(ws->config.url_path ? ws->config.url_path : "/");
	ws->protocol = ws->config.protocol ? strdup(ws->config.protocol) : NULL;
	ws->sockfd = -1;
	ws->reconnect_timerfd = -1;
	ws->heartbeat_timerfd = -1;
	ws->state = EZ_WS_STATE_DISCONNECTED;
	ws->current_retry_delay = ws->config.reconnect_interval_ms;
	ws->send_queue_max = 1024;
	ws->handshake_response_len = 0;
	ws->http_handshake_complete = 0;
	ws->http_header_field = NULL;
	ws->http_header_value = NULL;
	ws->http_sec_websocket_accept = NULL;
	ws->current_frame.buffer = NULL;
	ws->current_frame.buffer_size = 0;

	ws->pending_send.frame_data = NULL;
	ws->pending_send.frame_len = 0;
	ws->pending_send.sent_len = 0;
	ws->pending_send.is_active = 0;

	/* TLS：创建客户端 SSL_CTX（仅校验与初始握手；verify_peer=1 默认 trust 内置 CA） */
	if (ws->tls_enable) {
		ws->ssl_ctx = wss_client_create_ssl_ctx(&ws->config);
		if (!ws->ssl_ctx) {
			free(ws->server_addr);
			free(ws->url_path);
			free(ws->protocol);
			SAFE_FREE(ws->tls_ca_path);
			free(ws);
			return NULL;
		}
	}

	if (callbacks)
		ws->callbacks = *callbacks;

	pthread_mutex_init(&ws->send_queue_lock, NULL);

	/* 初始化 HTTP 解析器 */
	http_parser_init(&ws->http_parser, HTTP_RESPONSE);
	ws->http_parser.data = ws;
	http_parser_settings_init(&ws->http_parser_settings);
	ws->http_parser_settings.on_status = on_http_status;
	ws->http_parser_settings.on_header_field = on_http_header_field;
	ws->http_parser_settings.on_header_value = on_http_header_value;
	ws->http_parser_settings.on_headers_complete = on_http_headers_complete;

	/* 初始化 WebSocket 解析器 */
	ez_websocket_parser_init(&ws->ws_parser);
	ws->ws_parser.data = ws;
	ez_websocket_parser_settings_init(&ws->ws_parser_settings);
	ws->ws_parser_settings.on_frame_begin = on_ws_frame_begin;
	ws->ws_parser_settings.on_frame_payload = on_ws_frame_payload;
	ws->ws_parser_settings.on_frame_complete = on_ws_frame_complete;

	/* 创建epoll */
	ws->epollfd = epoll_create1(EPOLL_CLOEXEC);
	if (ws->epollfd < 0) {
		if (ws->ssl_ctx)
			SSL_CTX_free(ws->ssl_ctx);
		pthread_mutex_destroy(&ws->send_queue_lock);
		free(ws->server_addr);
		free(ws->url_path);
		free(ws->protocol);
		SAFE_FREE(ws->tls_ca_path);
		free(ws);
		return NULL;
	}

	/* 创建重连定时器（初始时不启动） */
	ws->reconnect_timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (ws->reconnect_timerfd >= 0) {
		struct itimerspec timer_spec = {0};
		if (timerfd_settime(ws->reconnect_timerfd, 0, &timer_spec, NULL) < 0) {
			close(ws->reconnect_timerfd);
			ws->reconnect_timerfd = -1;
		} else {
			struct epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.fd = ws->reconnect_timerfd;
			epoll_ctl(ws->epollfd, EPOLL_CTL_ADD, ws->reconnect_timerfd, &ev);
		}
	}

	/* 创建心跳定时器 */
	ws->heartbeat_timerfd = ez_websocket_create_timerfd(EZ_WS_CLIENT_HEARTBEAT_TICK_MS);
	if (ws->heartbeat_timerfd >= 0) {
		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = ws->heartbeat_timerfd;
		epoll_ctl(ws->epollfd, EPOLL_CTL_ADD, ws->heartbeat_timerfd, &ev);

		struct itimerspec timer_spec = {0};
		timerfd_settime(ws->heartbeat_timerfd, 0, &timer_spec, NULL);
	}

	/* 重置 HTTP 解析器（准备新的握手） */
	http_parser_init(&ws->http_parser, HTTP_RESPONSE);
	ws->http_parser.data = ws;
	ws->http_handshake_complete = 0;
	SAFE_FREE(ws->http_header_field);
	SAFE_FREE(ws->http_header_value);
	SAFE_FREE(ws->http_sec_websocket_accept);
	ws->handshake_response_len = 0;

	/* 设置运行标志 */
	ws->ready = 1;
	ws->interrupted = 0;

	return ws;
}

/* 清理WebSocket客户端（wss），含 SSL/SSL_CTX 释放 */
void ez_wss_client_cleanup(struct ez_wss_client_handle *ws) {
	if (!ws)
		return;

	if (ws->reconnect_timerfd >= 0) {
		struct itimerspec timer_spec = {0};
		timerfd_settime(ws->reconnect_timerfd, 0, &timer_spec, NULL);
	}
	if (ws->heartbeat_timerfd >= 0) {
		struct itimerspec timer_spec = {0};
		timerfd_settime(ws->heartbeat_timerfd, 0, &timer_spec, NULL);
	}

	ws->interrupted = 1;
	ws->ready = 0;

	if (ws->sockfd >= 0) {
		epoll_ctl(ws->epollfd, EPOLL_CTL_DEL, ws->sockfd, NULL);
		shutdown(ws->sockfd, SHUT_RDWR);
		close(ws->sockfd);
		ws->sockfd = -1;
	}

	/* 释放 TLS 会话与上下文 */
	ez_wss_reset_tls(ws);
	if (ws->ssl_ctx) {
		SSL_CTX_free(ws->ssl_ctx);
		ws->ssl_ctx = NULL;
	}

	if (ws->epollfd >= 0)
		close(ws->epollfd);
	if (ws->reconnect_timerfd >= 0)
		close(ws->reconnect_timerfd);
	if (ws->heartbeat_timerfd >= 0)
		close(ws->heartbeat_timerfd);

	SAFE_FREE(ws->tls_ca_path);
	SAFE_FREE(ws->server_addr);
	SAFE_FREE(ws->url_path);
	SAFE_FREE(ws->protocol);
	SAFE_FREE(ws->handshake_key);

	SAFE_FREE(ws->http_header_field);
	SAFE_FREE(ws->http_header_value);
	SAFE_FREE(ws->http_sec_websocket_accept);

	SAFE_FREE(ws->current_frame.buffer);

	pthread_mutex_lock(&ws->send_queue_lock);
	while (ws->send_queue_head) {
		struct send_queue_node *node = ws->send_queue_head;
		ws->send_queue_head = node->next;
		free(node->data);
		free(node);
	}
	pthread_mutex_unlock(&ws->send_queue_lock);
	pthread_mutex_destroy(&ws->send_queue_lock);

	ez_wss_clear_send_buffer(ws);

	free(ws);
}

#endif /* EZ_WS_ENABLE_OPENSSLTLS */
