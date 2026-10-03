/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_wss-server-native.c - Native WSS (WebSocket over TLS) Server Implementation
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_wss-server-native.c $
 *
 * Explain:
 *     WSS = WebSocket over TLS. 与明文版 ez_wsserver-native 平行，原实现保持
 *     纯明文不动；本组件基于源码编译的 OpenSSL 静态库实现加密链路。
 *     复用 http_parser + ez_websocket_parser（回调式）做 HTTP/WS 帧解析；
 *     TLS 握手在任何 HTTP/WS 字节进入解析器之前完成（非阻塞，epoll 驱动）。
 *     由 ez_websocket.h 中的 EZ_WS_ENABLE_OPENSSLTLS 宏整文件包裹：
 *     宏被注释 → 本文件编译为空对象，不引入任何 OpenSSL 符号与依赖。
 *
 * Update:
 *     2026-09-29 Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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
#include "ez_wss-server-native.h" /* 宏定义时才有内容 */

#if defined(EZ_WS_ENABLE_OPENSSLTLS) && (EZ_WS_ENABLE_OPENSSLTLS == 1)

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <ezutil/base64.h>
#include <ezutil/ez_websocket_parser.h>
#include <ezutil/http_parser.h>
#include <ezutil/sha1.h>
#include <ezutil/ez_system_api.h>

/* HTTP 握手请求缓冲区大小（字节），与明文版一致 */
#ifndef EZ_WS_HANDSHAKE_REQUEST_BUFFER_SIZE
#define EZ_WS_HANDSHAKE_REQUEST_BUFFER_SIZE  1024
#endif

#ifndef EZ_WS_RECV_BUFFER_INITIAL_SIZE
#define EZ_WS_RECV_BUFFER_INITIAL_SIZE  256
#endif

#ifndef EZ_WS_RECV_BUFFER_MAX_SIZE
#define EZ_WS_RECV_BUFFER_MAX_SIZE  (1 * 1024 * 1024)
#endif

/* 内置 CA 证书有效期（秒），1 年 */
#define EZ_WSS_BUILTIN_CA_VALIDITY_SEC  (365L * 24L * 3600L)

/* 安全释放内存宏 */
#define SAFE_FREE(ptr) do { \
	if (ptr) { \
		free(ptr); \
		ptr = NULL; \
	} \
} while (0)

/* ================== 内置 CA（frp 风格：编译期内嵌、随库分发） ==================
 * 内置一把 EC P-256 的 CA 证书+私钥：当服务器未配置 tls_cert_path/tls_key_path 时，
 * 服务端用该 CA 在运行时签发一张叶子证书（CA:FALSE），客户端 tls_verify_peer=1 时
 * trust 同一把内置 CA，两端天然互认，零配置即加密+身份互认。
 * 内置 CA 私钥随库公开，属「防君子不防小人」的信任边界（详见设计文档）。
 * 证书/叶子用 OpenSSL 标准 API 全内存构造，不落盘。 */

static const char EZ_WSS_BUILTIN_CA_CERT_PEM[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIBujCCAWGgAwIBAgIUA+eeJOGSp6sL01TknxdLIZJRJP8wCgYIKoZIzj0EAwIw\n"
	"MzEaMBgGA1UEAwwRZXotd3NzLWJ1aWx0aW4tY2ExFTATBgNVBAoMDEVaTElCUyBU\n"
	"b3VjaDAeFw0yNjA5MjkwMTQ4NThaFw0zNjA5MjYwMTQ4NThaMDMxGjAYBgNVBAMM\n"
	"EWV6LXdzcy1idWlsdGluLWNhMRUwEwYDVQQKDAxFWkxJQlMgVG91Y2gwWTATBgcq\n"
	"hkjOPQIBBggqhkjOPQMBBwNCAAQ+Zy0et65J6xmjlunYjiBpXLMDQpJvazVxPpeQ\n"
	"1vF7oHFn+qjH7vqwpsemWgazDLKl4j3srVaJ+PH/flrlE2Wyo1MwUTAdBgNVHQ4E\n"
	"FgQU4+IoKz/nl84Ge78ZTZWGZTSyCP4wHwYDVR0jBBgwFoAU4+IoKz/nl84Ge78Z\n"
	"TZWGZTSyCP4wDwYDVR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNHADBEAiBU+95X\n"
	"47+5bC6L+2IZ9TUPE7EdVQnLfSklN/6olqGt8wIgeKk17lCAEnl1CWOkXNEg4hVR\n"
	"v4l7C9XpUyfV17Zlm+U=\n"
	"-----END CERTIFICATE-----\n";

static const char EZ_WSS_BUILTIN_CA_KEY_PEM[] =
	"-----BEGIN EC PRIVATE KEY-----\n"
	"MHcCAQEEIEdZKBBlcZcGtcyzRqbVZNp8GvSbU8Dn5FV9zhyn3I95oAoGCCqGSM49\n"
	"AwEHoUQDQgAEPmctHreuSesZo5bp2I4gaVyzA0KSb2s1cT6XkNbxe6BxZ/qox+76\n"
	"sKbHploGswyypeI97K1Wifjx/35a5RNlsg==\n"
	"-----END EC PRIVATE KEY-----\n";

/* 内置 CA 全局缓存（进程内首用懒加载一次） */
static pthread_once_t g_wss_builtin_ca_once = PTHREAD_ONCE_INIT;
static X509 *g_wss_builtin_ca_cert = NULL;
static EVP_PKEY *g_wss_builtin_ca_key = NULL;

/* 从内嵌 PEM 加载内置 CA 证书+私钥（仅首次调用，由 pthread_once 保证只执行一次） */
static void wss_builtin_ca_load(void)
{
	BIO *cert_bio = BIO_new_mem_buf((const void *)EZ_WSS_BUILTIN_CA_CERT_PEM, -1);
	BIO *key_bio = BIO_new_mem_buf((const void *)EZ_WSS_BUILTIN_CA_KEY_PEM, -1);
	if (!cert_bio || !key_bio)
		goto fail;

	g_wss_builtin_ca_cert = PEM_read_bio_X509(cert_bio, NULL, NULL, NULL);
	g_wss_builtin_ca_key = PEM_read_bio_PrivateKey(key_bio, NULL, NULL, NULL);
	if (!g_wss_builtin_ca_cert || !g_wss_builtin_ca_key)
		goto fail;

	BIO_free(cert_bio);
	BIO_free(key_bio);
	return;

fail:
	if (cert_bio)
		BIO_free(cert_bio);
	if (key_bio)
		BIO_free(key_bio);
	if (g_wss_builtin_ca_cert) {
		X509_free(g_wss_builtin_ca_cert);
		g_wss_builtin_ca_cert = NULL;
	}
	if (g_wss_builtin_ca_key) {
		EVP_PKEY_free(g_wss_builtin_ca_key);
		g_wss_builtin_ca_key = NULL;
	}
}

/* 获取内置 CA（线程安全），失败返回 -1 */
static int wss_builtin_ca_get(X509 **out_cert, EVP_PKEY **out_key)
{
	pthread_once(&g_wss_builtin_ca_once, wss_builtin_ca_load);
	if (!g_wss_builtin_ca_cert || !g_wss_builtin_ca_key)
		return -1;
	if (out_cert)
		*out_cert = g_wss_builtin_ca_cert;
	if (out_key)
		*out_key = g_wss_builtin_ca_key;
	return 0;
}

/* 生成 EC P-256 密钥对 */
static EVP_PKEY *wss_generate_ec_p256_key(void)
{
	EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
	EVP_PKEY *pkey = NULL;
	if (!pctx)
		return NULL;
	if (EVP_PKEY_keygen_init(pctx) <= 0)
		goto fail;
	if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1) <= 0)
		goto fail;
	if (EVP_PKEY_keygen(pctx, &pkey) <= 0)
		pkey = NULL;
fail:
	EVP_PKEY_CTX_free(pctx);
	return pkey;
}

/* 用内置 CA 运行时签发一张叶子证书（CA:FALSE），随机 serial、固定 1 年有效期 */
static X509 *wss_issue_leaf_cert(X509 *ca_cert, EVP_PKEY *ca_key, EVP_PKEY *leaf_key)
{
	X509 *leaf = X509_new();
	X509_NAME *subject = NULL;
	unsigned char serial_bytes[16];
	X509_EXTENSION *ext = NULL;
	X509V3_CTX v3ctx;

	if (!leaf)
		return NULL;

	/* 版本 3 */
	X509_set_version(leaf, 2);

	/* 随机 serial */
	if (RAND_bytes(serial_bytes, sizeof(serial_bytes)) != 1)
		goto fail;
	BIGNUM *bn = BN_bin2bn(serial_bytes, sizeof(serial_bytes), NULL);
	if (!bn)
		goto fail;
	ASN1_INTEGER *serial = ASN1_INTEGER_new();
	if (!serial) {
		BN_free(bn);
		goto fail;
	}
	BN_to_ASN1_INTEGER(bn, serial);
	BN_free(bn);
	X509_set_serialNumber(leaf, serial);
	ASN1_INTEGER_free(serial);

	/* 固定有效期（1 年），gmtime 构造 */
	X509_gmtime_adj(X509_getm_notBefore(leaf), 0);
	X509_gmtime_adj(X509_getm_notAfter(leaf), EZ_WSS_BUILTIN_CA_VALIDITY_SEC);

	/* 公钥 + subject/issuer（issuer = 内置 CA 的 subject） */
	X509_set_pubkey(leaf, leaf_key);
	subject = X509_get_subject_name(ca_cert);
	if (X509_set_issuer_name(leaf, subject) != 1)
		goto fail;
	subject = X509_NAME_new();
	if (!subject)
		goto fail;
	if (X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC,
	                               (const unsigned char *)"ez-wss-server", -1, -1, 0) != 1)
		goto fail;
	X509_set_subject_name(leaf, subject);
	X509_NAME_free(subject);

	/* basicConstraints: critical, CA:FALSE（叶子不能作为 CA） */
	X509V3_set_ctx(&v3ctx, ca_cert, ca_cert, NULL, NULL, 0);
	ext = X509V3_EXT_conf_nid(NULL, &v3ctx, NID_basic_constraints, "critical,CA:FALSE");
	if (!ext)
		goto fail;
	X509_add_ext(leaf, ext, -1);
	X509_EXTENSION_free(ext);

	/* 用内置 CA 私钥签名 */
	if (!X509_sign(leaf, ca_key, EVP_sha256()))
		goto fail;

	return leaf;

fail:
	if (subject)
		X509_NAME_free(subject);
	if (ext)
		X509_EXTENSION_free(ext);
	X509_free(leaf);
	return NULL;
}

/* 导出内置 CA 公钥证书（PEM）到调用方缓冲区（私钥永不导出） */
int ez_ws_export_builtin_ca_pem(char *buf, size_t buf_size)
{
	X509 *ca = NULL;
	BIO *bio = NULL;
	char *pem = NULL;
	long len;

	if (wss_builtin_ca_get(&ca, NULL) < 0)
		return -1;

	bio = BIO_new(BIO_s_mem());
	if (!bio)
		return -1;
	if (PEM_write_bio_X509(bio, ca) != 1) {
		BIO_free(bio);
		return -1;
	}

	len = BIO_get_mem_data(bio, &pem);
	if (len <= 0 || !buf || buf_size < (size_t)len + 1) {
		BIO_free(bio);
		return -1;
	}

	memcpy(buf, pem, (size_t)len);
	buf[len] = '\0';
	BIO_free(bio);
	return (int)len + 1;
}

/* ================== 基础工具 ================== */

/* 获取当前时间（毫秒） */
static uint64_t ez_wss_server_now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

/* 生成带抖动的 ping 间隔（毫秒） */
static uint32_t ez_wss_server_get_ping_interval_with_jitter(uint32_t base_interval_ms, uint32_t jitter_percent)
{
	if (base_interval_ms == 0)
		return 0;

	if (jitter_percent == 0)
		return base_interval_ms;

	uint32_t jitter_ms = (base_interval_ms * jitter_percent) / 100;
	uint32_t jitter_range = jitter_ms * 2;
	uint32_t random_offset = (uint32_t)(rand() % (jitter_range + 1));
	return base_interval_ms - jitter_ms + random_offset;
}

/* 客户端连接状态 */
enum client_state {
	CLIENT_STATE_TLS_HANDSHAKING,   /* TLS 握手中（tls_enable=1 时先于此阶段） */
	CLIENT_STATE_HTTP_HANDSHAKING,  /* HTTP/WebSocket 握手中 */
	CLIENT_STATE_CONNECTED,         /* 已连接 */
	CLIENT_STATE_CLOSING,           /* 关闭中 */
	CLIENT_STATE_CLOSED             /* 已关闭 */
};

/* 客户端连接信息 */
struct client_connection {
	int sockfd;                    /* socket 文件描述符 */
	SSL *ssl;                      /* TLS 会话，NULL 表示纯明文连接 */
	int tls_enable;                /* 本连接 TLS 开关（继承监听级） */
	int want_write;                /* TLS 层希望监听 EPOLLOUT（WANT_WRITE 驱动） */
	enum client_state state;       /* 连接状态 */

	/* 指向服务器句柄 */
	struct ez_wss_server_handle *server;

	/* 客户端信息（使用 ez_ws_client_info 结构体整合） */
	struct ez_ws_client_info client_info;

	/* HTTP 握手相关 */
	char handshake_request[EZ_WS_HANDSHAKE_REQUEST_BUFFER_SIZE];   /* HTTP 握手请求缓冲区 */
	size_t handshake_request_len;  /* 已接收的握手请求长度 */
	http_parser http_parser;
	http_parser_settings http_parser_settings;
	int http_handshake_complete;
	char *http_header_field;
	char *http_header_value;
	char *http_sec_websocket_key;
	char *http_sec_websocket_protocol;
	char *http_url_path;

	/* WebSocket 解析器 */
	ez_websocket_parser ws_parser;
	ez_websocket_parser_settings ws_parser_settings;

	/* 接收缓冲区（动态扩展） */
	uint8_t *recv_buffer;
	size_t recv_buffer_size;  /* 缓冲区容量 */
	size_t recv_buffer_len;   /* 已接收的数据长度 */

	/* 当前帧数据收集器 */
	struct {
		uint8_t *buffer;
		size_t buffer_size;
		size_t buffer_cap;
		int opcode;
		int is_binary;
	} current_frame;

	/* 发送队列 */
	struct send_queue_node {
		uint8_t *data;
		size_t len;
		struct send_queue_node *next;
	} *send_queue_head, *send_queue_tail;
	pthread_mutex_t send_queue_lock;
	size_t send_queue_size;

	/* 发送缓冲区（用于部分发送） */
	uint8_t *pending_send_data;
	size_t pending_send_len;
	size_t pending_send_sent;

	/* 保活相关 */
	uint64_t last_rx_time_ms;      /* 最后接收时间 */
	uint64_t last_tx_time_ms;      /* 最后发送时间 */
	uint64_t last_ping_time_ms;    /* 最后发送 ping 时间 */
	int awaiting_pong;             /* 是否等待 pong */
	uint32_t current_ping_interval_ms; /* 当前使用的 ping 间隔（带抖动） */

#if defined(EZ_WS_SERVER_ENABLE_STATS) && (EZ_WS_SERVER_ENABLE_STATS == 1)
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
#endif /* EZ_WS_SERVER_ENABLE_STATS */

	/* 链表指针 */
	struct client_connection *next;
};

/* WebSocket 服务端（wss）句柄（完整定义） */
struct ez_wss_server_handle {
	/* 配置 */
	struct ez_wss_server_config config;

	/* 回调函数 */
	struct ez_ws_server_callbacks callbacks;

	/* TLS（监听级，所有连接共享同一 SSL_CTX） */
	int tls_enable;                /* 监听级 TLS 开关（config.tls_enable 快照） */
	SSL_CTX *ssl_ctx;              /* 句柄级共享 SSL_CTX，NULL 表示纯明文监听 */

	/* TCP 服务器 */
	int listen_sockfd;             /* 监听 socket */
	int epollfd;                   /* epoll 文件描述符 */

	/* 定时器 */
	int timerfd;                   /* 定时器文件描述符 */

	/* 客户端连接列表 */
	struct client_connection *client_list;
	pthread_mutex_t client_list_lock;
	int client_count;

	/* 状态 */
	int ready;
	int interrupted;

	/* 中断标志指针（外部传入） */
	int *interrupted_ptr;
};

/* ================== 前向声明 ================== */
static int on_http_url(http_parser *parser, const char *at, size_t length);
static int on_http_header_field(http_parser *parser, const char *at, size_t length);
static int on_http_header_value(http_parser *parser, const char *at, size_t length);
static int on_http_headers_complete(http_parser *parser);

static int on_ws_frame_begin(ez_websocket_parser *parser);
static int on_ws_frame_payload(ez_websocket_parser *parser, const char *at, size_t length);
static int on_ws_frame_complete(ez_websocket_parser *parser);

static int process_send_queue(struct client_connection *client);
static int send_to_client_internal(struct client_connection *client, const uint8_t *data, size_t len, int opcode);
static void update_client_events(struct client_connection *client);

/* ================== TLS 数据面 工具 ================== */

/* TLS 读：封装 SSL_read / recv，WANT_READ/WRITE 归一为 -1+EAGAIN（复用明文 EAGAIN 分支）；
 * 返回 >0 字节、0 关闭、-1 错误（errno=EAGAIN 表示需等待） */
static ssize_t wss_read(struct client_connection *client, void *buf, size_t len)
{
	if (!client->ssl)
		return recv(client->sockfd, buf, len, 0);

	int n = SSL_read(client->ssl, buf, (int)len);
	if (n > 0)
		return n;

	int err = SSL_get_error(client->ssl, n);
	if (err == SSL_ERROR_WANT_READ) {
		client->want_write = 0;
		errno = EAGAIN;
		return -1;
	}
	if (err == SSL_ERROR_WANT_WRITE) {
		/* TLS 需要先向外发送字节（如 TLS1.2 重协商 / KeyUpdate），挂 EPOLLOUT */
		client->want_write = 1;
		errno = EAGAIN;
		return -1;
	}
	if (err == SSL_ERROR_ZERO_RETURN)
		return 0; /* 对端干净关闭 */

	return -1;
}

/* TLS 写：封装 SSL_write / send */
static ssize_t wss_write(struct client_connection *client, const void *buf, size_t len)
{
	if (!client->ssl)
		return send(client->sockfd, buf, len, 0);

	int n = SSL_write(client->ssl, buf, (int)len);
	if (n > 0)
		return n;

	int err = SSL_get_error(client->ssl, n);
	if (err == SSL_ERROR_WANT_WRITE) {
		client->want_write = 1;
		errno = EAGAIN;
		return -1;
	}
	if (err == SSL_ERROR_WANT_READ) {
		/* 写阻塞在读上（重协商），EPOLLIN 常驻即可，等待对端数据 */
		client->want_write = 0;
		errno = EAGAIN;
		return -1;
	}
	if (err == SSL_ERROR_ZERO_RETURN)
		return 0;

	return -1;
}

/* 按当前状态重算该连接的 epoll 事件掩码：
 * EPOLLIN 常驻；EPOLLOUT 仅在有待发数据或 TLS 层 want_write 时挂上 */
static void update_client_events(struct client_connection *client)
{
	struct epoll_event ev;
	uint32_t e = EPOLLIN | EPOLLET;

	if (client->send_queue_head || client->pending_send_data || client->want_write)
		e |= EPOLLOUT;

	ev.events = e;
	ev.data.ptr = client;
	epoll_ctl(client->server->epollfd, EPOLL_CTL_MOD, client->sockfd, &ev);
}

/* TLS 握手阶段的电平触发事件掩码：want_write 决定是否挂 EPOLLOUT，
 * 不挂 EPOLLET（避免写就绪无跃迁时 EPOLLOUT 不再触发的握手死锁） */
static void wss_set_handshake_events(struct client_connection *client, int want_write)
{
	struct epoll_event ev;

	ev.events = EPOLLIN | (want_write ? EPOLLOUT : 0);
	ev.data.ptr = client;
	epoll_ctl(client->server->epollfd, EPOLL_CTL_MOD, client->sockfd, &ev);
}

/* 创建 wss 服务端 SSL_CTX（句柄级）：
 * 配置了正式证书链 → 走标准文件加载；两者皆空 → 内置 CA 互认（运行时签发叶子）。 */
static SSL_CTX *wss_server_create_ssl_ctx(const struct ez_wss_server_config *config)
{
	SSL_CTX *ctx;
	X509 *ca = NULL;
	EVP_PKEY *ca_key = NULL;
	X509 *leaf = NULL;
	EVP_PKEY *leaf_key = NULL;

	if (!OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS |
	                      OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL))
		return NULL;

	ctx = SSL_CTX_new(TLS_server_method());
	if (!ctx)
		return NULL;

	SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

	if (config->tls_cert_path && config->tls_key_path) {
		/* 正式证书 */
		if (SSL_CTX_use_certificate_chain_file(ctx, config->tls_cert_path) <= 0) {
			fprintf(stderr, "[wss] Failed to load certificate chain: %s\n", config->tls_cert_path);
			ERR_print_errors_fp(stderr);
			goto fail;
		}
		if (SSL_CTX_use_PrivateKey_file(ctx, config->tls_key_path, SSL_FILETYPE_PEM) <= 0) {
			fprintf(stderr, "[wss] Failed to load private key: %s\n", config->tls_key_path);
			ERR_print_errors_fp(stderr);
			goto fail;
		}
		if (SSL_CTX_check_private_key(ctx) <= 0) {
			fprintf(stderr, "[wss] Private key does not match certificate\n");
			goto fail;
		}
	} else {
		/* 内置 CA 互认：运行时签发叶子证书，并把内置 CA 作为 extra chain 随包下发 */
		if (wss_builtin_ca_get(&ca, &ca_key) < 0)
			goto fail;

		leaf_key = wss_generate_ec_p256_key();
		if (!leaf_key)
			goto fail;
		leaf = wss_issue_leaf_cert(ca, ca_key, leaf_key);
		if (!leaf)
			goto fail;

		if (SSL_CTX_use_certificate(ctx, leaf) <= 0) {
			fprintf(stderr, "[wss] Failed to use built-in leaf certificate\n");
			ERR_print_errors_fp(stderr);
			goto fail;
		}
		if (SSL_CTX_use_PrivateKey(ctx, leaf_key) <= 0) {
			fprintf(stderr, "[wss] Failed to use built-in leaf private key\n");
			ERR_print_errors_fp(stderr);
			goto fail;
		}
		/* 把内置 CA 证书附到发送链上，客户端可完成「叶子 ← 内置 CA」信任链回溯 */
		if (!SSL_CTX_add_extra_chain_cert(ctx, X509_dup(ca))) {
			fprintf(stderr, "[wss] Failed to add built-in CA to chain\n");
			goto fail;
		}

		X509_free(leaf);
		leaf = NULL;
		EVP_PKEY_free(leaf_key);
		leaf_key = NULL;
	}

	/* 可选：双向认证 client CA（校验客户端证书；配置后才启用 SSL_VERIFY_PEER） */
	if (config->tls_ca_path) {
		if (SSL_CTX_load_verify_locations(ctx, config->tls_ca_path, NULL) <= 0) {
			fprintf(stderr, "[wss] Failed to load client CA: %s\n", config->tls_ca_path);
			ERR_print_errors_fp(stderr);
			goto fail;
		}
		SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	} else {
		SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	}

	return ctx;

fail:
	if (leaf)
		X509_free(leaf);
	if (leaf_key)
		EVP_PKEY_free(leaf_key);
	SSL_CTX_free(ctx);
	return NULL;
}

/* ================== 接收/握手 ================== */

/* 接受新连接 */
static int accept_new_connection(struct ez_wss_server_handle *server)
{
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	int client_sockfd;

	/* 接受连接 */
	client_sockfd = accept(server->listen_sockfd, (struct sockaddr *)&client_addr, &addr_len);
	if (client_sockfd < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			fprintf(stderr, "Failed to accept connection: %s\n", strerror(errno));
		}
		return -1;
	}

	/* 设置 socket 选项 */
	if (ez_websocket_set_socket_options(client_sockfd) < 0) {
		close(client_sockfd);
		return -1;
	}

	/* 设置为非阻塞 */
	if (ez_websocket_set_nonblocking(client_sockfd) < 0) {
		close(client_sockfd);
		return -1;
	}

	/* 创建客户端连接结构 */
	struct client_connection *client = calloc(1, sizeof(struct client_connection));
	if (!client) {
		close(client_sockfd);
		return -1;
	}

	/* 初始化客户端连接 */
	client->sockfd = client_sockfd;
	client->tls_enable = server->tls_enable;
	client->state = client->tls_enable ?
	                CLIENT_STATE_TLS_HANDSHAKING : CLIENT_STATE_HTTP_HANDSHAKING;
	client->server = server;

	/* 初始化接收缓冲区（动态分配） */
	client->recv_buffer = malloc(EZ_WS_RECV_BUFFER_INITIAL_SIZE);
	if (!client->recv_buffer) {
		free(client);
		close(client_sockfd);
		return -1;
	}
	client->recv_buffer_size = EZ_WS_RECV_BUFFER_INITIAL_SIZE;
	client->recv_buffer_len = 0;

	/* 初始化客户端信息（calloc 已将所有字段初始化为0） */
	client->client_info.connect_time = time(NULL);

	/* 获取客户端地址 */
	inet_ntop(AF_INET, &client_addr.sin_addr, client->client_info.ip, sizeof(client->client_info.ip));
	client->client_info.port = ntohs(client_addr.sin_port);

	/* 使用 sockfd 作为客户端 ID（仅内部使用，不会溢出） */
	client->client_info.id = client_sockfd;

	/* 初始化发送队列锁 */
	pthread_mutex_init(&client->send_queue_lock, NULL);

	/* 初始化 HTTP 解析器 */
	http_parser_init(&client->http_parser, HTTP_REQUEST);
	client->http_parser.data = client;

	client->http_parser_settings.on_url = on_http_url;
	client->http_parser_settings.on_header_field = on_http_header_field;
	client->http_parser_settings.on_header_value = on_http_header_value;
	client->http_parser_settings.on_headers_complete = on_http_headers_complete;

	/* 初始化 WebSocket 解析器 */
	ez_websocket_parser_init(&client->ws_parser);
	ez_websocket_parser_settings_init(&client->ws_parser_settings);
	client->ws_parser.data = client;

	client->ws_parser_settings.on_frame_begin = on_ws_frame_begin;
	client->ws_parser_settings.on_frame_payload = on_ws_frame_payload;
	client->ws_parser_settings.on_frame_complete = on_ws_frame_complete;

	/* 初始化时间戳 */
	uint64_t now_ms = ez_wss_server_now_ms();
	client->last_rx_time_ms = now_ms;
	client->last_tx_time_ms = now_ms;
	client->client_info.last_activity_ms = now_ms;

	/* TLS：创建 SSL 会话（SSL_set_fd 后由 epoll 驱动非阻塞 SSL_accept） */
	if (client->tls_enable) {
		client->ssl = SSL_new(server->ssl_ctx);
		if (!client->ssl) {
			pthread_mutex_destroy(&client->send_queue_lock);
			SAFE_FREE(client->recv_buffer);
			free(client);
			close(client_sockfd);
			return -1;
		}
		SSL_set_fd(client->ssl, client_sockfd);
	}

	/* 添加到客户端列表 */
	pthread_mutex_lock(&server->client_list_lock);
	client->next = server->client_list;
	server->client_list = client;
	server->client_count++;
	pthread_mutex_unlock(&server->client_list_lock);

	/* 添加到 epoll：TLS 握手阶段用电平触发（WANT_READ/WRITE 显式切事件，避免
	 * EPOLLET 下「写就绪但无状态跃迁 → EPOLLOUT 不再触发」的握手死锁）；
	 * 明文路径沿用明文版的 EPOLLIN|EPOLLOUT|EPOLLET */
	struct epoll_event ev;
	ev.events = client->tls_enable ? (EPOLLIN | EPOLLOUT) : (EPOLLIN | EPOLLOUT | EPOLLET);
	ev.data.ptr = client;
	if (epoll_ctl(server->epollfd, EPOLL_CTL_ADD, client_sockfd, &ev) < 0) {
		fprintf(stderr, "Failed to add client to epoll: %s\n", strerror(errno));
		/* 清理客户端连接 */
		pthread_mutex_lock(&server->client_list_lock);
		if (server->client_list == client) {
			server->client_list = client->next;
		} else {
			struct client_connection *prev = server->client_list;
			while (prev && prev->next != client)
				prev = prev->next;
			if (prev)
				prev->next = client->next;
		}
		server->client_count--;
		pthread_mutex_unlock(&server->client_list_lock);
		if (client->ssl) {
			SSL_free(client->ssl);
			client->ssl = NULL;
		}
		pthread_mutex_destroy(&client->send_queue_lock);
		SAFE_FREE(client->recv_buffer);
		free(client);
		close(client_sockfd);
		return -1;
	}

	/* 纯明文路径：连接回调在 TCP 建立时触发（与明文版一致）；
	 * TLS 路径的连接回调在握手完成后触发（见 do_tls_handshake） */
	if (!client->tls_enable && server->callbacks.on_connected) {
		server->callbacks.on_connected(client->client_info.id, client->client_info.ip,
		                               client->client_info.port, server->callbacks.user_data);
	}

	return 0;
}

/* 驱动 TLS 握手（非阻塞，epoll 事件驱动）：
 * 返回 1 握手完成、0 仍在进行（等待事件）、-1 错误 */
static int do_tls_handshake(struct ez_wss_server_handle *server, struct client_connection *client)
{
	int r = SSL_accept(client->ssl);

	if (r == 1) {
		/* 握手完成：切回 EPOLLET，进入 HTTP/WS 握手阶段 */
		client->state = CLIENT_STATE_HTTP_HANDSHAKING;
		client->want_write = 0;
		update_client_events(client);

		if (server->callbacks.on_connected) {
			server->callbacks.on_connected(client->client_info.id, client->client_info.ip,
			                               client->client_info.port, server->callbacks.user_data);
		}
		return 1;
	}

	int err = SSL_get_error(client->ssl, r);
	if (err == SSL_ERROR_WANT_READ) {
		client->want_write = 0;
		wss_set_handshake_events(client, 0);
		return 0;
	}
	if (err == SSL_ERROR_WANT_WRITE) {
		client->want_write = 1;
		wss_set_handshake_events(client, 1);
		return 0;
	}

	/* 其他错误：握手失败，关闭连接 */
	return -1;
}

/* 查找客户端连接 */
static struct client_connection *find_client(struct ez_wss_server_handle *server, int client_id)
{
	struct client_connection *client;

	pthread_mutex_lock(&server->client_list_lock);
	client = server->client_list;
	while (client) {
		if (client->client_info.id == client_id)
			break;
		client = client->next;
	}
	pthread_mutex_unlock(&server->client_list_lock);

	return client;
}

/* 移除客户端连接（含 TLS 资源清理：先 SSL_shutdown 尽力关闭，再 SSL_free） */
static void remove_client(struct ez_wss_server_handle *server, struct client_connection *client)
{
	if (!server || !client)
		return;

	/* 保存 sockfd，防止后续访问 */
	int saved_sockfd = client->sockfd;
	SSL *ssl = client->ssl;

	/* 先从客户端列表中移除（防止 find_client 找到已断开的连接） */
	pthread_mutex_lock(&server->client_list_lock);
	if (server->client_list == client) {
		server->client_list = client->next;
	} else {
		struct client_connection *prev = server->client_list;
		while (prev && prev->next != client)
			prev = prev->next;
		if (prev)
			prev->next = client->next;
	}
	server->client_count--;
	pthread_mutex_unlock(&server->client_list_lock);

	/* 调用断开回调（在关闭 socket 之前通知上层） */
	if (server->callbacks.on_disconnected) {
		server->callbacks.on_disconnected(client->client_info.id, server->callbacks.user_data);
	}

	/* TLS：尽力完成关闭握手（非阻塞场景结果可忽略），再释放会话 */
	if (ssl) {
		SSL_shutdown(ssl);
		SSL_free(ssl);
		client->ssl = NULL;
	}

	/* 关闭 socket（close 会自动从 epoll 中移除） */
	close(saved_sockfd);

	/* 清理资源 */
	SAFE_FREE(client->http_header_field);
	SAFE_FREE(client->http_header_value);
	SAFE_FREE(client->http_sec_websocket_key);
	SAFE_FREE(client->http_sec_websocket_protocol);
	SAFE_FREE(client->http_url_path);
	SAFE_FREE(client->pending_send_data);
	SAFE_FREE(client->current_frame.buffer);
	SAFE_FREE(client->recv_buffer);

	/* 清理发送队列 */
	pthread_mutex_lock(&client->send_queue_lock);
	while (client->send_queue_head) {
		struct send_queue_node *node = client->send_queue_head;
		client->send_queue_head = node->next;
		SAFE_FREE(node->data);
		free(node);
	}
	client->send_queue_tail = NULL;
	client->send_queue_size = 0;
	pthread_mutex_unlock(&client->send_queue_lock);

	pthread_mutex_destroy(&client->send_queue_lock);

	/* 释放客户端结构 */
	free(client);
}

/* ================== 步骤 b: 使用 http_parser 做 HTTP 请求处理 ================== */

/* 计算 WebSocket Accept 值 */
static int compute_websocket_accept(const char *key, char *accept)
{
	char combined[256];
	snprintf(combined, sizeof(combined), "%s%s", key, EZ_WEBSOCKET_MAGIC_STRING);

	char hash[SHA1HashSize] = {0};
	SHA1Context sha;
	SHA1Reset(&sha);
	SHA1Input(&sha, (const unsigned char *)combined, strlen(combined));
	SHA1Result(&sha, (unsigned char *)hash);

	char base64_accept[32] = {0};
	ez_base64encode(base64_accept, (const char *)hash, SHA1HashSize);
	snprintf(accept, 32, "%s", base64_accept);
	return 0;
}

/* HTTP 解析器回调：URL */
static int on_http_url(http_parser *parser, const char *at, size_t length)
{
	struct client_connection *client = (struct client_connection *)parser->data;

	SAFE_FREE(client->http_url_path);
	client->http_url_path = malloc(length + 1);
	if (!client->http_url_path)
		return -1;
	memcpy(client->http_url_path, at, length);
	client->http_url_path[length] = '\0';

	return 0;
}

/* HTTP 解析器回调：头部字段 */
static int on_http_header_field(http_parser *parser, const char *at, size_t length)
{
	struct client_connection *client = (struct client_connection *)parser->data;

	SAFE_FREE(client->http_header_field);
	client->http_header_field = malloc(length + 1);
	if (!client->http_header_field)
		return -1;
	memcpy(client->http_header_field, at, length);
	client->http_header_field[length] = '\0';

	return 0;
}

/* HTTP 解析器回调：头部值 */
static int on_http_header_value(http_parser *parser, const char *at, size_t length)
{
	struct client_connection *client = (struct client_connection *)parser->data;

	SAFE_FREE(client->http_header_value);
	client->http_header_value = malloc(length + 1);
	if (!client->http_header_value)
		return -1;
	memcpy(client->http_header_value, at, length);
	client->http_header_value[length] = '\0';

	/* 检查重要的 WebSocket 头部 */
	if (client->http_header_field) {
		if (strcasecmp(client->http_header_field, "Sec-WebSocket-Key") == 0) {
			SAFE_FREE(client->http_sec_websocket_key);
			client->http_sec_websocket_key = strdup(client->http_header_value);
		} else if (strcasecmp(client->http_header_field, "Sec-WebSocket-Protocol") == 0) {
			SAFE_FREE(client->http_sec_websocket_protocol);
			client->http_sec_websocket_protocol = strdup(client->http_header_value);
		}
	}

	return 0;
}

/* HTTP 解析器回调：头部完成 */
static int on_http_headers_complete(http_parser *parser)
{
	struct client_connection *client = (struct client_connection *)parser->data;
	struct ez_wss_server_handle *server = client ? client->server : NULL;

	if (!client || !server)
		return -1;

	/* 检查方法必须是 GET */
	if (parser->method != 1) { /* HTTP_GET = 1 */
		return -1;
	}

	/* 检查 Upgrade 和 Connection 标志 */
	if (!(parser->flags & F_UPGRADE) || !(parser->flags & F_CONNECTION_UPGRADE)) {
		return -1;
	}

	/* 检查 Sec-WebSocket-Key */
	if (!client->http_sec_websocket_key) {
		return -1;
	}

	/* 检查路径前缀（如果配置了） */
	if (server->config.path_prefix) {
		if (!client->http_url_path ||
		    strncmp(client->http_url_path, server->config.path_prefix,
		            strlen(server->config.path_prefix)) != 0) {
			return -1;
		}
	}

	/* 检查协议（如果配置了） */
	if (server->config.protocol) {
		if (!client->http_sec_websocket_protocol) {
			return -1;
		}
		if (strcmp(client->http_sec_websocket_protocol, server->config.protocol) != 0) {
			return -1;
		}
	}

	/* 所有检查通过，标记握手完成 */
	client->http_handshake_complete = 1;
	return 0;
}

/* 生成 WebSocket 握手响应 */
static int generate_websocket_handshake_response(struct client_connection *client,
                                                  struct ez_wss_server_handle *server,
                                                  char *response, size_t response_size)
{
	char accept_key[64] = {0};

	if (compute_websocket_accept(client->http_sec_websocket_key, accept_key) < 0) {
		return -1;
	}

	/* 路径和协议验证已在 on_http_headers_complete 中完成 */

	int len = snprintf(response, response_size,
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: %s\r\n",
		accept_key);

	if (len < 0 || len >= (int)response_size) {
		return -1;
	}

	if (server->config.protocol && client->http_sec_websocket_protocol) {
		int protocol_len = snprintf(response + len, response_size - len,
			"Sec-WebSocket-Protocol: %s\r\n",
			server->config.protocol);
		if (protocol_len < 0 || len + protocol_len >= (int)response_size) {
			return -1;
		}
		len += protocol_len;
	}

	int end_len = snprintf(response + len, response_size - len, "\r\n");
	if (end_len < 0 || len + end_len >= (int)response_size) {
		return -1;
	}
	len += end_len;

	if (len < (int)response_size) {
		response[len] = '\0';
	}

	return len;
}

/* 处理 HTTP 握手（只处理缓冲区中的数据，不接收） */
static int handle_handshake(struct ez_wss_server_handle *server, struct client_connection *client)
{
	if (client->handshake_request_len == 0)
		return 0;

	size_t parsed = http_parser_execute(&client->http_parser, &client->http_parser_settings,
	                                    client->handshake_request,
	                                    client->handshake_request_len);

	enum http_errno err = HTTP_PARSER_ERRNO(&client->http_parser);
	if (err != HPE_OK && err != HPE_PAUSED) {
		return -1;
	}

	if (parsed > 0 && parsed <= client->handshake_request_len) {
		if (parsed < client->handshake_request_len) {
			memmove(client->handshake_request,
			        client->handshake_request + parsed,
			        client->handshake_request_len - parsed);
		}
		client->handshake_request_len -= parsed;
	}

	if (client->http_handshake_complete) {
		char response[2048];
		int response_len = generate_websocket_handshake_response(client, server, response, sizeof(response));

		if (response_len < 0) {
			return -1;
		}

		/* 握手响应必须完整发送（TLS 为 SSL_write，可能 WANT_WRITE/WANT_READ） */
		ssize_t total_sent = 0;
		while (total_sent < response_len) {
			ssize_t sent = wss_write(client, response + total_sent, response_len - total_sent);
			if (sent < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					/* 发送缓冲区/SSL 阻塞，将剩余数据加入发送队列 */
					size_t remaining = response_len - total_sent;
					struct send_queue_node *node = malloc(sizeof(struct send_queue_node));
					if (!node)
						return -1;

					node->data = (uint8_t *)malloc(remaining);
					if (!node->data) {
						free(node);
						return -1;
					}

					memcpy(node->data, response + total_sent, remaining);
					node->len = remaining;
					node->next = NULL;

					pthread_mutex_lock(&client->send_queue_lock);
					if (client->send_queue_tail) {
						client->send_queue_tail->next = node;
					} else {
						client->send_queue_head = node;
					}
					client->send_queue_tail = node;
					client->send_queue_size++;
					pthread_mutex_unlock(&client->send_queue_lock);

					/* 确保监听 EPOLLOUT */
					update_client_events(client);

					total_sent = response_len; /* 其余交给队列 */
					break;
				}
				return -1; /* 发送错误 */
			}
			total_sent += sent;
			if (total_sent >= (ssize_t)response_len)
				break;
		}

		if (total_sent >= response_len) {
			uint64_t now_ms = ez_wss_server_now_ms();
			client->last_tx_time_ms = now_ms;
			client->client_info.last_activity_ms = now_ms;
		}

		/* 握手成功，切换到已连接状态 */
		client->state = CLIENT_STATE_CONNECTED;
		update_client_events(client);

		uint64_t now_ms = ez_wss_server_now_ms();
		client->client_info.last_activity_ms = now_ms;
		client->last_rx_time_ms = now_ms;
		client->last_tx_time_ms = now_ms;

		if (server->config.ping_interval_ms > 0) {
			client->current_ping_interval_ms = ez_wss_server_get_ping_interval_with_jitter(
				server->config.ping_interval_ms,
				server->config.ping_jitter_percent);
		}

		memset(client->handshake_request, 0, sizeof(client->handshake_request));
		client->handshake_request_len = 0;

		return 1; /* 握手完成 */
	}

	return 0; /* 继续等待更多数据 */
}

/* ================== 步骤 c: 使用 ez_websocket_parser 做 WebSocket 请求处理 ================== */

/* 创建 WebSocket 帧（服务端不需要 mask） */
static int create_ws_frame_server(const uint8_t *payload, size_t payload_len,
                                   int opcode, int fin, uint8_t *frame, size_t *frame_len)
{
	size_t pos = 0;

	frame[pos++] = (fin ? 0x80 : 0x00) | (opcode & 0x0F);

	if (payload_len < 126) {
		frame[pos++] = payload_len;
	} else if (payload_len < 65536) {
		frame[pos++] = 126;
		frame[pos++] = (payload_len >> 8) & 0xFF;
		frame[pos++] = payload_len & 0xFF;
	} else {
		frame[pos++] = 127;
		int i;
		for (i = 7; i >= 0; i--) {
			frame[pos++] = (payload_len >> (i * 8)) & 0xFF;
		}
	}

	if (payload && payload_len > 0) {
		memcpy(frame + pos, payload, payload_len);
		pos += payload_len;
	}

	*frame_len = pos;
	return 0;
}

/* WebSocket 解析器回调：帧开始 */
static int on_ws_frame_begin(ez_websocket_parser *parser)
{
	struct client_connection *client = (struct client_connection *)parser->data;

	SAFE_FREE(client->current_frame.buffer);

	client->current_frame.opcode = parser->opcode;
	client->current_frame.is_binary = (parser->opcode == EZ_WS_OPCODE_BINARY);
	client->current_frame.buffer_size = 0;
	client->current_frame.buffer_cap = 65536;
	client->current_frame.buffer = malloc(client->current_frame.buffer_cap);
	if (!client->current_frame.buffer) {
		client->current_frame.buffer_cap = 0;
		return -1;
	}

	return 0;
}

/* WebSocket 解析器回调：帧负载 */
static int on_ws_frame_payload(ez_websocket_parser *parser, const char *at, size_t length)
{
	struct client_connection *client = (struct client_connection *)parser->data;

	if (!client || !client->current_frame.buffer)
		return -1;

	if (client->current_frame.buffer_size + length > client->current_frame.buffer_cap) {
		size_t new_cap = client->current_frame.buffer_cap * 2;
		while (new_cap < client->current_frame.buffer_size + length)
			new_cap *= 2;

		uint8_t *new_buffer = realloc(client->current_frame.buffer, new_cap);
		if (!new_buffer)
			return -1;

		client->current_frame.buffer = new_buffer;
		client->current_frame.buffer_cap = new_cap;
	}

	memcpy(client->current_frame.buffer + client->current_frame.buffer_size, at, length);
	client->current_frame.buffer_size += length;

	return 0;
}

/* WebSocket 解析器回调：帧完成 */
static int on_ws_frame_complete(ez_websocket_parser *parser)
{
	struct client_connection *client = (struct client_connection *)parser->data;
	struct ez_wss_server_handle *server = client ? client->server : NULL;

	if (!server || !client)
		return -1;

	uint64_t now_ms = ez_wss_server_now_ms();
	client->last_rx_time_ms = now_ms;
	client->client_info.last_activity_ms = now_ms;

	switch (parser->opcode) {
	case EZ_WS_OPCODE_TEXT:
	case EZ_WS_OPCODE_BINARY: {
#if defined(EZ_WS_SERVER_ENABLE_STATS) && (EZ_WS_SERVER_ENABLE_STATS == 1)
		if (parser->opcode == EZ_WS_OPCODE_TEXT) {
			client->stats.rx_text_count++;
			client->stats.rx_text_bytes += client->current_frame.buffer_size;
		} else if (parser->opcode == EZ_WS_OPCODE_BINARY) {
			client->stats.rx_binary_count++;
			client->stats.rx_binary_bytes += client->current_frame.buffer_size;
		}
#endif /* EZ_WS_SERVER_ENABLE_STATS */
		if (server->callbacks.on_receive && client->current_frame.buffer) {
			server->callbacks.on_receive(client->client_info.id,
			                            client->current_frame.buffer,
			                            client->current_frame.buffer_size,
			                            client->current_frame.is_binary,
			                            server->callbacks.user_data);
		}
		SAFE_FREE(client->current_frame.buffer);
		client->current_frame.buffer_size = 0;
		client->current_frame.buffer_cap = 0;
		break;
	}
	case EZ_WS_OPCODE_PING:
#if defined(EZ_WS_SERVER_ENABLE_STATS) && (EZ_WS_SERVER_ENABLE_STATS == 1)
		client->stats.rx_ping_count++;
#endif /* EZ_WS_SERVER_ENABLE_STATS */
		/* 收到 ping，发送 pong（Pong 必须原样返回 Ping 的 payload；走队列保证 TLS 非阻塞发送正确） */
		send_to_client_internal(client, client->current_frame.buffer,
		                        client->current_frame.buffer_size, EZ_WS_OPCODE_PONG);
		SAFE_FREE(client->current_frame.buffer);
		client->current_frame.buffer_size = 0;
		client->current_frame.buffer_cap = 0;
		break;
	case EZ_WS_OPCODE_PONG:
#if defined(EZ_WS_SERVER_ENABLE_STATS) && (EZ_WS_SERVER_ENABLE_STATS == 1)
		client->stats.rx_pong_count++;
#endif /* EZ_WS_SERVER_ENABLE_STATS */
		/* 收到 pong，清除等待标志 */
		client->awaiting_pong = 0;
		break;
	case EZ_WS_OPCODE_CLOSE:
#if defined(EZ_WS_SERVER_ENABLE_STATS) && (EZ_WS_SERVER_ENABLE_STATS == 1)
		client->stats.rx_close_count++;
#endif /* EZ_WS_SERVER_ENABLE_STATS */
		parser->close_received = 1;
		client->state = CLIENT_STATE_CLOSING;
		break;
	default:
		break;
	}

	return 0;
}

/* 处理 WebSocket 数据（只处理缓冲区中的数据，不接收） */
static int handle_websocket_data(struct ez_wss_server_handle *server, struct client_connection *client)
{
	if (client->recv_buffer_len == 0)
		return 0;

	uint64_t now_ms = ez_wss_server_now_ms();
	client->last_rx_time_ms = now_ms;
	client->client_info.last_activity_ms = now_ms;

	size_t parsed = ez_websocket_parser_execute(&client->ws_parser,
	                                           &client->ws_parser_settings,
	                                           (const char *)client->recv_buffer,
	                                           client->recv_buffer_len);

	if (client->ws_parser.close_received) {
		return -1;
	}

	enum ez_ws_errno ws_err = EZ_WEBSOCKET_PARSER_ERRNO(&client->ws_parser);
	if (ws_err != EZ_WSE_OK) {
		return -1;
	}

	if (parsed > 0 && parsed <= client->recv_buffer_len) {
		if (parsed < client->recv_buffer_len) {
			memmove(client->recv_buffer,
			        client->recv_buffer + parsed,
			        client->recv_buffer_len - parsed);
		}
		client->recv_buffer_len -= parsed;
	} else if (parsed == 0 && client->recv_buffer_len > 0) {
		if (client->recv_buffer_len >= client->recv_buffer_size) {
			if (client->recv_buffer_size >= EZ_WS_RECV_BUFFER_MAX_SIZE) {
				return -1;
			}

			size_t new_size = client->recv_buffer_size * 2;
			if (new_size > EZ_WS_RECV_BUFFER_MAX_SIZE) {
				new_size = EZ_WS_RECV_BUFFER_MAX_SIZE;
			}

			uint8_t *new_buffer = realloc(client->recv_buffer, new_size);
			if (!new_buffer)
				return -1;

			client->recv_buffer = new_buffer;
			client->recv_buffer_size = new_size;
		}
	}

	return 0;
}

/* ================== 步骤 d 和 e: 发送队列、保活机制、主事件循环 ================== */

/* 发送数据到客户端（内部函数，完全异步；TLS 走 SSL_write + 队列） */
static int send_to_client_internal(struct client_connection *client, const uint8_t *data, size_t len, int opcode)
{
	if (!client || client->state != CLIENT_STATE_CONNECTED)
		return -1;

#if defined(EZ_WS_SERVER_ENABLE_STATS) && (EZ_WS_SERVER_ENABLE_STATS == 1)
	switch (opcode) {
	case EZ_WS_OPCODE_TEXT:
		client->stats.tx_text_count++;
		client->stats.tx_text_bytes += len;
		break;
	case EZ_WS_OPCODE_BINARY:
		client->stats.tx_binary_count++;
		client->stats.tx_binary_bytes += len;
		break;
	case EZ_WS_OPCODE_PING:
		client->stats.tx_ping_count++;
		break;
	case EZ_WS_OPCODE_PONG:
		client->stats.tx_pong_count++;
		break;
	case EZ_WS_OPCODE_CLOSE:
		client->stats.tx_close_count++;
		break;
	default:
		break;
	}
#endif /* EZ_WS_SERVER_ENABLE_STATS */

	size_t frame_cap = len + EZ_WS_MAX_FRAME_OVERHEAD;
	uint8_t *frame = malloc(frame_cap);
	if (!frame)
		return -1;

	size_t frame_len;
	if (create_ws_frame_server(data, len, opcode, 1, frame, &frame_len) < 0) {
		free(frame);
		return -1;
	}

	/* 完全异步发送：直接将数据加入发送队列，由 EPOLLOUT 事件驱动发送 */
	struct send_queue_node *node = malloc(sizeof(struct send_queue_node));
	if (!node) {
		free(frame);
		return -1;
	}

	node->data = frame;
	node->len = frame_len;
	node->next = NULL;

	int need_epollout = 0;
	pthread_mutex_lock(&client->send_queue_lock);
	if (client->send_queue_tail) {
		client->send_queue_tail->next = node;
	} else {
		client->send_queue_head = node;
		need_epollout = 1;
	}
	client->send_queue_tail = node;
	client->send_queue_size++;
	pthread_mutex_unlock(&client->send_queue_lock);

	if (need_epollout && !client->pending_send_data) {
		update_client_events(client);
	}

	/* 尝试立即发送（如果 socket/SSL 可写且没有待发送数据） */
	if (!client->pending_send_data) {
		process_send_queue(client);
	}

	return 0;
}

/* 处理发送队列（TLS 走 SSL_write，WANT_WRITE/WANT_READ 归一为 EAGAIN 等待） */
static int process_send_queue(struct client_connection *client)
{
	if (!client || client->state != CLIENT_STATE_CONNECTED)
		return -1;

	/* 先处理 pending_send_data */
	if (client->pending_send_data) {
		ssize_t sent = wss_write(client,
		                         client->pending_send_data + client->pending_send_sent,
		                         client->pending_send_len - client->pending_send_sent);
		if (sent < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK)
				return -1;
			return 0; /* 继续等待 */
		}

		client->pending_send_sent += sent;
		if (client->pending_send_sent >= client->pending_send_len) {
			SAFE_FREE(client->pending_send_data);
			client->pending_send_len = 0;
			client->pending_send_sent = 0;
		} else {
			return 0; /* 继续等待 */
		}
	}

	/* 处理发送队列 */
	pthread_mutex_lock(&client->send_queue_lock);
	while (client->send_queue_head) {
		struct send_queue_node *node = client->send_queue_head;

		ssize_t sent = wss_write(client, node->data, node->len);
		if (sent < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			}
			client->send_queue_head = node->next;
			if (!client->send_queue_head)
				client->send_queue_tail = NULL;
			client->send_queue_size--;
			SAFE_FREE(node->data);
			free(node);
			pthread_mutex_unlock(&client->send_queue_lock);
			return -1;
		}

		uint64_t now_ms = ez_wss_server_now_ms();
		client->last_tx_time_ms = now_ms;
		client->client_info.last_activity_ms = now_ms;

		if (sent >= (ssize_t)node->len) {
			client->send_queue_head = node->next;
			if (!client->send_queue_head)
				client->send_queue_tail = NULL;
			client->send_queue_size--;
			SAFE_FREE(node->data);
			free(node);
		} else {
			client->pending_send_data = malloc(node->len - sent);
			if (!client->pending_send_data) {
				client->send_queue_head = node->next;
				if (!client->send_queue_head)
					client->send_queue_tail = NULL;
				client->send_queue_size--;
				SAFE_FREE(node->data);
				free(node);
				pthread_mutex_unlock(&client->send_queue_lock);
				return -1;
			}
			memcpy(client->pending_send_data, node->data + sent, node->len - sent);
			client->pending_send_len = node->len - sent;
			client->pending_send_sent = 0;

			client->send_queue_head = node->next;
			if (!client->send_queue_head)
				client->send_queue_tail = NULL;
			client->send_queue_size--;
			SAFE_FREE(node->data);
			free(node);
			break; /* 等待下次 EPOLLOUT */
		}
	}
	pthread_mutex_unlock(&client->send_queue_lock);

	/* 队列与待发数据均空时收敛 EPOLLOUT（TLS want_write 除外） */
	if (!client->send_queue_head && !client->pending_send_data) {
		update_client_events(client);
	}

	return 0;
}

/* 处理定时器事件（ping/pong 和超时检测） */
static void process_timer_events(struct ez_wss_server_handle *server)
{
	uint64_t now_ms = ez_wss_server_now_ms();

	pthread_mutex_lock(&server->client_list_lock);
	struct client_connection *client = server->client_list;
	struct client_connection *next;

	while (client) {
		next = client->next;

		if (client->state == CLIENT_STATE_CONNECTED) {
			/* 检查空闲超时 */
			if (server->config.idle_timeout_ms > 0) {
				if (client->client_info.last_activity_ms > 0 &&
				    now_ms - client->client_info.last_activity_ms > server->config.idle_timeout_ms) {
					pthread_mutex_unlock(&server->client_list_lock);
					remove_client(server, client);
					pthread_mutex_lock(&server->client_list_lock);
					client = next;
					continue;
				}
			}

			/* 检查 ping/pong */
			if (server->config.ping_interval_ms > 0) {
				if (client->last_ping_time_ms == 0 ||
				    (now_ms - client->last_ping_time_ms >= client->current_ping_interval_ms)) {
					/* 发送 ping（走队列，TLS 非阻塞发送） */
					send_to_client_internal(client, NULL, 0, EZ_WS_OPCODE_PING);

					client->last_ping_time_ms = now_ms;
					client->awaiting_pong = 1;

					client->current_ping_interval_ms = ez_wss_server_get_ping_interval_with_jitter(
						server->config.ping_interval_ms,
						server->config.ping_jitter_percent);
				}

				/* 检查 pong 超时 */
				if (server->config.ping_timeout_ms > 0 && client->awaiting_pong) {
					if (now_ms - client->last_ping_time_ms > server->config.ping_timeout_ms) {
						pthread_mutex_unlock(&server->client_list_lock);
						remove_client(server, client);
						pthread_mutex_lock(&server->client_list_lock);
						client = next;
						continue;
					}
				}
			}
		}

		client = next;
	}

	pthread_mutex_unlock(&server->client_list_lock);
}

/* 读取并处理 HTTP/WebSocket 握手数据（TLS 与明文共用；返回 0 继续等待、-1 需断开） */
static int read_handshake_data(struct ez_wss_server_handle *server, struct client_connection *client)
{
	for (;;) {
		if (client->handshake_request_len + 1 >= EZ_WS_HANDSHAKE_REQUEST_BUFFER_SIZE) {
			fprintf(stderr, "[ERROR] Handshake request buffer overflow: %zu bytes.\n",
			        client->handshake_request_len);
			return -1;
		}

		ssize_t n = wss_read(client,
		                     client->handshake_request + client->handshake_request_len,
		                     EZ_WS_HANDSHAKE_REQUEST_BUFFER_SIZE - client->handshake_request_len - 1);
		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				/* TLS：若 SSL 内部仍有解密数据，继续排空 */
				if (client->ssl && SSL_pending(client->ssl) > 0)
					continue;
				return 0;
			}
			return -1;
		}
		if (n == 0) {
			return -1; /* 连接关闭 */
		}

		client->handshake_request_len += n;
		client->handshake_request[client->handshake_request_len] = '\0';

		int ret = handle_handshake(server, client);
		if (ret < 0)
			return -1;
		if (ret > 0)
			return 0; /* 握手完成 */

		if (!client->ssl)
			return 0; /* 纯明文：等待下次 EPOLLIN */
		if (SSL_pending(client->ssl) <= 0)
			return 0;
	}
}

/* 处理 WebSocket 已连接数据：TLS 先排空 SSL 内部缓冲，再读 socket 直到 EAGAIN；
 * 返回 0 继续、-1 需断开 */
static int read_connected_data(struct ez_wss_server_handle *server, struct client_connection *client)
{
	int drain_guard = 0;

	for (;;) {
		if (client->recv_buffer_len >= client->recv_buffer_size) {
			if (client->recv_buffer_size >= EZ_WS_RECV_BUFFER_MAX_SIZE) {
				fprintf(stderr, "[ERROR] Recv buffer reached maximum size (%d bytes).\n",
				        EZ_WS_RECV_BUFFER_MAX_SIZE);
				return -1;
			}

			size_t new_size = client->recv_buffer_size * 2;
			if (new_size > EZ_WS_RECV_BUFFER_MAX_SIZE) {
				new_size = EZ_WS_RECV_BUFFER_MAX_SIZE;
			}

			uint8_t *new_buffer = realloc(client->recv_buffer, new_size);
			if (!new_buffer)
				return -1;

			client->recv_buffer = new_buffer;
			client->recv_buffer_size = new_size;
		}

		ssize_t n = wss_read(client,
		                     client->recv_buffer + client->recv_buffer_len,
		                     client->recv_buffer_size - client->recv_buffer_len);
		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				/* TLS：SSL 内部可能还有已解出但未取走的 record（EPOLLET 下不会
				 * 再有新边缘），必须继续排空，否则剩余数据会饿死在 SSL 内部。
				 * drain_guard 防御极端情况下 SSL 内部状态异常导致的死循环。 */
				if (client->ssl && SSL_pending(client->ssl) > 0 && drain_guard < 4096) {
					drain_guard++;
					continue;
				}
				break; /* 已真正读空，等待下次 EPOLLIN */
			}
			return -1;
		}
		drain_guard = 0;
		if (n == 0) {
			return -1; /* 连接关闭 */
		}

		client->recv_buffer_len += n;

		/* 循环处理，直到没有完整帧可解析 */
		while (client->recv_buffer_len > 0) {
			size_t old_buffer_len = client->recv_buffer_len;
			int ret = handle_websocket_data(server, client);
			if (ret < 0)
				return -1;
			if (client->recv_buffer_len == old_buffer_len)
				break;
		}

		if (client->state != CLIENT_STATE_CONNECTED)
			break;
	}

	return 0;
}

/* 主事件循环处理 */
static int process_events(struct ez_wss_server_handle *server, int timeout_ms)
{
	struct epoll_event events[64];

	if (timeout_ms == 0) {
		timeout_ms = 100;  /* 默认 100ms 超时，平衡响应性和 CPU 占用 */
	}

	int nfds = epoll_wait(server->epollfd, events, 64, timeout_ms);

	if (nfds < 0) {
		if (errno == EINTR)
			return 0;
		return -1;
	}

	int i;
	for (i = 0; i < nfds; i++) {
		if (events[i].data.fd == server->listen_sockfd) {
			/* 新连接 - 使用 EPOLLET 边缘触发模式，只需接受一次 */
			accept_new_connection(server);
		} else if (events[i].data.fd == server->timerfd) {
			/* 定时器事件 */
			uint64_t expirations;
			read(server->timerfd, &expirations, sizeof(expirations));
			process_timer_events(server);
		} else {
			/* 客户端连接事件 */
			struct client_connection *client = (struct client_connection *)events[i].data.ptr;

			if (events[i].events & (EPOLLERR | EPOLLHUP)) {
				/* 连接错误或关闭 */
				remove_client(server, client);
				continue;
			}

			/* TLS 握手阶段：IN/OUT 任一事件都驱动 SSL_accept（WANT_READ/WRITE 切换） */
			if (client->state == CLIENT_STATE_TLS_HANDSHAKING &&
			    (events[i].events & (EPOLLIN | EPOLLOUT))) {
				int ret = do_tls_handshake(server, client);
				if (ret < 0) {
					remove_client(server, client);
					continue;
				}
				if (ret == 0)
					continue; /* 握手未完成，等下次事件 */

				/* 握手完成：若 SSL 内部已有解密出的 HTTP 请求，立即处理 */
				if (!client->ssl || SSL_pending(client->ssl) <= 0) {
					if (!(events[i].events & EPOLLIN))
						continue;
				}
				/* 否则落到下面的 EPOLLIN 分支读取 HTTP 握手数据 */
			}

			if ((events[i].events & EPOLLOUT) && client->state == CLIENT_STATE_CONNECTED) {
				/* 可写，处理发送队列 */
				process_send_queue(client);
			}

			if (events[i].events & EPOLLIN) {
				if (client->state == CLIENT_STATE_HTTP_HANDSHAKING) {
					/* 接收并处理握手请求（TLS：SSL_pending 自动排空） */
					if (read_handshake_data(server, client) < 0) {
						remove_client(server, client);
						continue;
					}
				} else if (client->state == CLIENT_STATE_CONNECTED) {
					/* 接收 WebSocket 数据（EPOLLET：循环读直到 EAGAIN，防帧饿死） */
					if (read_connected_data(server, client) < 0) {
						remove_client(server, client);
						continue;
					}

					/* TLS 写阻塞在读（WANT_READ）恢复后，趁机补发队列数据 */
					if (client->ssl && (client->send_queue_head || client->pending_send_data)) {
						process_send_queue(client);
					}
				}
			}
		}
	}

	return 0;
}

/* ================== 公共 API 实现 ================== */

/* 创建 WebSocket 服务端（wss）句柄 */
struct ez_wss_server_handle *ez_wss_server_handle_create(struct ez_wss_server_config *config,
                                                        struct ez_ws_server_callbacks *callbacks)
{
	struct ez_wss_server_handle *server = calloc(1, sizeof(struct ez_wss_server_handle));
	if (!server)
		return NULL;

	/* 复制配置 */
	if (config) {
		memcpy(&server->config, config, sizeof(struct ez_wss_server_config));
	} else {
		memset(&server->config, 0, sizeof(struct ez_wss_server_config));
		server->config.port = 54321;
	}

	/* 检查必需配置 */
	if (!server->config.protocol) {
		fprintf(stderr, "Protocol must be specified\n");
		free(server);
		return NULL;
	}

	/* 复制回调函数 */
	if (callbacks) {
		memcpy(&server->callbacks, callbacks, sizeof(struct ez_ws_server_callbacks));
	} else {
		memset(&server->callbacks, 0, sizeof(struct ez_ws_server_callbacks));
	}

	/* 监听级 TLS 开关（默认 1 开箱即加密） */
	server->tls_enable = (server->config.tls_enable != 0) ? 1 : 0;

	/* 初始化客户端列表锁 */
	pthread_mutex_init(&server->client_list_lock, NULL);
	server->client_list = NULL;
	server->client_count = 0;

	/* TLS：创建句柄级共享 SSL_CTX（正式证书或内置 CA 互认） */
	if (server->tls_enable) {
		server->ssl_ctx = wss_server_create_ssl_ctx(&server->config);
		if (!server->ssl_ctx) {
			pthread_mutex_destroy(&server->client_list_lock);
			free(server);
			return NULL;
		}
	}

	/* 创建监听 socket */
	server->listen_sockfd = ez_websocket_create_listen_socket(server->config.ip, server->config.port);
	if (server->listen_sockfd < 0) {
		if (server->ssl_ctx)
			SSL_CTX_free(server->ssl_ctx);
		pthread_mutex_destroy(&server->client_list_lock);
		free(server);
		return NULL;
	}

	/* 创建 epoll */
	server->epollfd = epoll_create1(EPOLL_CLOEXEC);
	if (server->epollfd < 0) {
		close(server->listen_sockfd);
		if (server->ssl_ctx)
			SSL_CTX_free(server->ssl_ctx);
		pthread_mutex_destroy(&server->client_list_lock);
		free(server);
		return NULL;
	}

	/* 将监听 socket 添加到 epoll */
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = server->listen_sockfd;
	if (epoll_ctl(server->epollfd, EPOLL_CTL_ADD, server->listen_sockfd, &ev) < 0) {
		close(server->epollfd);
		close(server->listen_sockfd);
		if (server->ssl_ctx)
			SSL_CTX_free(server->ssl_ctx);
		pthread_mutex_destroy(&server->client_list_lock);
		free(server);
		return NULL;
	}

	/* 创建定时器（如果配置了） */
	if (server->config.timer_interval_ms > 0) {
		server->timerfd = ez_websocket_create_timerfd(server->config.timer_interval_ms);
		if (server->timerfd >= 0) {
			ev.events = EPOLLIN;
			ev.data.fd = server->timerfd;
			epoll_ctl(server->epollfd, EPOLL_CTL_ADD, server->timerfd, &ev);
		}
	} else {
		server->timerfd = -1;
	}

	server->ready = 1;

	return server;
}

/* 清理 WebSocket 服务端（wss），含 SSL_CTX 释放 */
void ez_wss_server_cleanup(struct ez_wss_server_handle *server)
{
	if (!server)
		return;

	server->ready = 0;
	server->interrupted = 1;
	if (server->interrupted_ptr)
		*server->interrupted_ptr = 1;

	/* 关闭所有客户端连接（remove_client 内会 SSL_shutdown/SSL_free） */
	pthread_mutex_lock(&server->client_list_lock);
	while (server->client_list) {
		struct client_connection *client = server->client_list;
		server->client_list = client->next;
		pthread_mutex_unlock(&server->client_list_lock);
		remove_client(server, client);
		pthread_mutex_lock(&server->client_list_lock);
	}
	pthread_mutex_unlock(&server->client_list_lock);

	/* 关闭定时器 */
	if (server->timerfd >= 0) {
		close(server->timerfd);
		server->timerfd = -1;
	}

	/* 关闭 epoll */
	if (server->epollfd >= 0) {
		close(server->epollfd);
		server->epollfd = -1;
	}

	/* 关闭监听 socket */
	if (server->listen_sockfd >= 0) {
		close(server->listen_sockfd);
		server->listen_sockfd = -1;
	}

	/* 释放 TLS 上下文 */
	if (server->ssl_ctx) {
		SSL_CTX_free(server->ssl_ctx);
		server->ssl_ctx = NULL;
	}

	/* 销毁锁 */
	pthread_mutex_destroy(&server->client_list_lock);

	/* 释放服务器结构 */
	free(server);
}

/* WebSocket 服务执行函数 */
int ez_wss_server_service_exec(struct ez_wss_server_handle *ws, int timeout_ms)
{
	if (!ws || !ws->ready)
		return -1;

	if (ws->interrupted || (ws->interrupted_ptr && *ws->interrupted_ptr))
		return -1;

	if (process_events(ws, timeout_ms) < 0)
		return -1;

	return 0;
}

/* 发送文本消息 */
int ez_wss_server_send_text(struct ez_wss_server_handle *ws, int client_id, const char *data, size_t len)
{
	if (!ws || !data)
		return EZ_WS_SERVER_ERR_INVALID_PARAM;

	if (len == 0)
		len = strlen(data);

	if (client_id == EZ_WS_SERVER_BROADCAST_ALL) {
		pthread_mutex_lock(&ws->client_list_lock);
		struct client_connection *client = ws->client_list;
		while (client) {
			if (client->state == CLIENT_STATE_CONNECTED) {
				send_to_client_internal(client, (const uint8_t *)data, len, EZ_WS_OPCODE_TEXT);
			}
			client = client->next;
		}
		pthread_mutex_unlock(&ws->client_list_lock);
		return EZ_WS_SERVER_OK;
	}

	struct client_connection *client = find_client(ws, client_id);
	if (!client)
		return EZ_WS_SERVER_ERR_CLIENT_NOT_FOUND;

	if (send_to_client_internal(client, (const uint8_t *)data, len, EZ_WS_OPCODE_TEXT) < 0)
		return EZ_WS_SERVER_ERR_QUEUE_FULL;

	return EZ_WS_SERVER_OK;
}

/* 发送二进制消息 */
int ez_wss_server_send_binary(struct ez_wss_server_handle *ws, int client_id, const void *data, size_t len)
{
	if (!ws || !data || len == 0)
		return EZ_WS_SERVER_ERR_INVALID_PARAM;

	if (client_id == EZ_WS_SERVER_BROADCAST_ALL) {
		pthread_mutex_lock(&ws->client_list_lock);
		struct client_connection *client = ws->client_list;
		while (client) {
			if (client->state == CLIENT_STATE_CONNECTED) {
				send_to_client_internal(client, (const uint8_t *)data, len, EZ_WS_OPCODE_BINARY);
			}
			client = client->next;
		}
		pthread_mutex_unlock(&ws->client_list_lock);
		return EZ_WS_SERVER_OK;
	}

	struct client_connection *client = find_client(ws, client_id);
	if (!client)
		return EZ_WS_SERVER_ERR_CLIENT_NOT_FOUND;

	if (send_to_client_internal(client, (const uint8_t *)data, len, EZ_WS_OPCODE_BINARY) < 0)
		return EZ_WS_SERVER_ERR_QUEUE_FULL;

	return EZ_WS_SERVER_OK;
}

/* 获取客户端数量 */
int ez_wss_server_get_client_count(struct ez_wss_server_handle *ws)
{
	if (!ws)
		return -1;

	int count;
	pthread_mutex_lock(&ws->client_list_lock);
	count = ws->client_count;
	pthread_mutex_unlock(&ws->client_list_lock);

	return count;
}

/* 检查服务是否运行 */
int ez_wss_server_is_ready(struct ez_wss_server_handle *ws)
{
	if (!ws)
		return 0;

	return ws->ready ? 1 : 0;
}

/* 关闭指定客户端连接 */
int ez_wss_server_close_client(struct ez_wss_server_handle *ws, int client_id)
{
	if (!ws)
		return EZ_WS_SERVER_ERR_INVALID_PARAM;

	struct client_connection *client = find_client(ws, client_id);
	if (!client)
		return EZ_WS_SERVER_ERR_CLIENT_NOT_FOUND;

	remove_client(ws, client);

	return EZ_WS_SERVER_OK;
}

/* 遍历客户端 */
int ez_wss_server_foreach_client(struct ez_wss_server_handle *ws,
                                 ez_ws_server_foreach_client_cb callback,
                                 void *user_data)
{
	if (!ws || !callback)
		return EZ_WS_SERVER_ERR_INVALID_PARAM;

	pthread_mutex_lock(&ws->client_list_lock);
	struct client_connection *client = ws->client_list;

	while (client) {
		struct ez_ws_client_info info;
		memcpy(&info, &client->client_info, sizeof(info));

		pthread_mutex_unlock(&ws->client_list_lock);
		int ret = callback(&info, user_data);
		pthread_mutex_lock(&ws->client_list_lock);

		if (ret != 0) {
			break;
		}

		client = client->next;
	}

	pthread_mutex_unlock(&ws->client_list_lock);

	return EZ_WS_SERVER_OK;
}

#if defined(EZ_WS_SERVER_ENABLE_STATS) && (EZ_WS_SERVER_ENABLE_STATS == 1)
/* 获取指定客户端的统计信息 */
int ez_wss_server_get_client_stats(struct ez_wss_server_handle *ws, int client_id,
                                   struct ez_ws_server_client_stats *stats)
{
	if (!ws || !stats)
		return EZ_WS_SERVER_ERR_INVALID_PARAM;

	struct client_connection *client = find_client(ws, client_id);
	if (!client)
		return EZ_WS_SERVER_ERR_CLIENT_NOT_FOUND;

	stats->tx_text_count = client->stats.tx_text_count;
	stats->tx_text_bytes = client->stats.tx_text_bytes;
	stats->tx_binary_count = client->stats.tx_binary_count;
	stats->tx_binary_bytes = client->stats.tx_binary_bytes;
	stats->tx_ping_count = client->stats.tx_ping_count;
	stats->tx_pong_count = client->stats.tx_pong_count;
	stats->tx_close_count = client->stats.tx_close_count;

	stats->rx_text_count = client->stats.rx_text_count;
	stats->rx_text_bytes = client->stats.rx_text_bytes;
	stats->rx_binary_count = client->stats.rx_binary_count;
	stats->rx_binary_bytes = client->stats.rx_binary_bytes;
	stats->rx_ping_count = client->stats.rx_ping_count;
	stats->rx_pong_count = client->stats.rx_pong_count;
	stats->rx_close_count = client->stats.rx_close_count;

	return EZ_WS_SERVER_OK;
}
#endif /* EZ_WS_SERVER_ENABLE_STATS */

#endif /* EZ_WS_ENABLE_OPENSSLTLS */
