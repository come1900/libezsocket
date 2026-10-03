/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * t-wss-svr-bench.c - WSS 服务端性能测试（多链路收/发）
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: t-wss-svr-bench.c $
 *
 * Explain:
 *     WSS 服务端性能测试程序，配合 t-wss-cli-bench.c 使用。
 *     三种工作模式：
 *       sink   （默认）丢弃收到的数据，不回发 —— 测纯接收吞吐
 *       echo   收到即回发 —— 用于时延（lat）与双向（both）带宽测试
 *       stream 定时向所有客户端推送数据 —— 用于客户端接收（recv）带宽测试
 *     stream 模式为每个客户端维护一个发送配额链表，由独立线程按配额推流，
 *     避免服务端发送队列无限增长。
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>

#include <ezutil/ez_system_api.h>

/* 只保留 ERROR 级别日志，屏蔽库的 INFO 输出，测试结果用 printf 打印 */
#define EZ_PRINT_LOG_LEVEL 3
#include <ezutil/ez_def_devel_debug.h>

#include "ez_wss-server-native.h"

#ifndef BENCH_PROTOCOL
#define BENCH_PROTOCOL "come.0"
#endif
#ifndef BENCH_PATH_PREFIX
#define BENCH_PATH_PREFIX "/come"
#endif

/* ====================== 全局参数 ====================== */

static volatile int g_interrupted = 0;

static int g_mode = 0;              /* 0=sink 1=echo 2=stream */
static int g_port = 18460;
static int g_no_tls = 0;
static const char *g_cert_path = NULL;
static const char *g_key_path = NULL;
static const char *g_ca_path = NULL;

/* stream 模式参数 */
static uint64_t g_stream_bytes = 16ull * 1024 * 1024;   /* 每个客户端总推送字节 */
static size_t   g_stream_chunk = 8192;                  /* 每次推送块大小 */
static long     g_stream_interval_us = 100;             /* 块间间隔（微秒） */

static struct ez_wss_server_handle *g_server = NULL;
static pthread_t g_svc_thread;

/* ====================== 时间工具 ====================== */

static void sleep_us(long us)
{
	struct timespec ts;
	ts.tv_sec = us / 1000000L;
	ts.tv_nsec = (us % 1000000L) * 1000L;
	while (nanosleep(&ts, &ts) != 0 && errno == EINTR)
		;
}

/* ====================== stream 配额管理 ====================== */

struct client_quota {
	int client_id;
	uint64_t remaining;       /* 剩余待推送字节 */
	struct client_quota *next;
};

static pthread_mutex_t g_quota_lock = PTHREAD_MUTEX_INITIALIZER;
static struct client_quota *g_quota_head = NULL;
static volatile int g_stream_thread_running = 0;
static pthread_t g_stream_thread;

static struct client_quota *quota_find_locked(int client_id)
{
	struct client_quota *q;
	for (q = g_quota_head; q; q = q->next) {
		if (q->client_id == client_id)
			return q;
	}
	return NULL;
}

static void quota_add(int client_id)
{
	pthread_mutex_lock(&g_quota_lock);
	if (!quota_find_locked(client_id)) {
		struct client_quota *q = malloc(sizeof(*q));
		if (q) {
			q->client_id = client_id;
			q->remaining = g_stream_bytes;
			q->next = g_quota_head;
			g_quota_head = q;
		}
	}
	pthread_mutex_unlock(&g_quota_lock);
}

static void quota_remove(int client_id)
{
	pthread_mutex_lock(&g_quota_lock);
	struct client_quota **pp = &g_quota_head;
	while (*pp) {
		struct client_quota *q = *pp;
		if (q->client_id == client_id) {
			*pp = q->next;
			free(q);
			break;
		}
		pp = &q->next;
	}
	pthread_mutex_unlock(&g_quota_lock);
}

/* ====================== 回调 ====================== */

static void on_connected(int client_id, const char *ip, int port, void *user_data)
{
	(void)user_data;
	printf("[svr] client #%d connected from %s:%d\n", client_id, ip ? ip : "?", port);
	if (g_mode == 2)
		quota_add(client_id);
}

static void on_disconnected(int client_id, void *user_data)
{
	(void)user_data;
	printf("[svr] client #%d disconnected\n", client_id);
	if (g_mode == 2)
		quota_remove(client_id);
}

static void on_receive(int client_id, const void *data, size_t len, int is_binary, void *user_data)
{
	(void)user_data;
	(void)is_binary;
	if (g_mode == 1) {   /* echo */
		ez_wss_server_send_binary(g_server, client_id, data, len);
	}
	/* sink 模式：直接丢弃 */
}

/* ====================== 服务线程 ====================== */

static void *svc_thread_func(void *arg)
{
	(void)arg;
	while (!g_interrupted) {
		if (ez_wss_server_service_exec(g_server, 100) < 0)
			break;
	}
	return NULL;
}

/* ====================== stream 推流线程 ====================== */

static void *stream_thread_func(void *arg)
{
	(void)arg;
	uint8_t *buf = malloc(g_stream_chunk);
	if (!buf)
		return NULL;
	memset(buf, 0xA5, g_stream_chunk);

	while (g_stream_thread_running && !g_interrupted) {
		int did_send = 0;

		pthread_mutex_lock(&g_quota_lock);
		struct client_quota *q = g_quota_head;
		while (q) {
			if (q->remaining > 0) {
				size_t send_len = g_stream_chunk;
				if (send_len > q->remaining)
					send_len = (size_t)q->remaining;
				int rc = ez_wss_server_send_binary(g_server, q->client_id, buf, send_len);
				if (rc == EZ_WS_SERVER_OK)
					q->remaining -= send_len;
				did_send = 1;
			}
			q = q->next;
		}
		pthread_mutex_unlock(&g_quota_lock);

		if (!did_send)
			sleep_us(g_stream_interval_us * 10);
		else if (g_stream_interval_us > 0)
			sleep_us(g_stream_interval_us);
	}

	free(buf);
	return NULL;
}

/* ====================== 状态打印 ====================== */

static void print_summary(void)
{
	int count = ez_wss_server_get_client_count(g_server);
	printf("\n================ WSS Server Bench Summary ================\n");
	printf("  Mode      : %s\n", g_mode == 0 ? "sink" : g_mode == 1 ? "echo" : "stream");
	printf("  Port      : %d (%s)\n", g_port, g_no_tls ? "ws" : "wss");
	printf("  Clients   : %d\n", count);

	if (g_mode == 2) {
		/* 汇总各客户端已推送字节 */
		uint64_t total = 0;
		pthread_mutex_lock(&g_quota_lock);
		struct client_quota *q = g_quota_head;
		while (q) {
			total += (g_stream_bytes - q->remaining);
			q = q->next;
		}
		pthread_mutex_unlock(&g_quota_lock);
		printf("  Pushed    : %llu bytes\n", (unsigned long long)total);
	}
	printf("===========================================================\n");
	fflush(stdout);
}

/* ====================== 控制台线程 ====================== */

static void *console_thread_func(void *arg)
{
	(void)arg;
	char line[128];
	while (!g_interrupted) {
		if (!fgets(line, sizeof(line), stdin))
			break;
		size_t len = strlen(line);
		while (len && (line[len-1]=='\n' || line[len-1]=='\r'))
			line[--len] = '\0';
		if (!strcmp(line, "quit") || !strcmp(line, "exit")) {
			g_interrupted = 1;
			break;
		}
		if (!strcmp(line, "status")) {
			print_summary();
		}
	}
	return NULL;
}

/* ====================== 主流程 ====================== */

static void sigint_handler(int sig)
{
	(void)sig;
	g_interrupted = 1;
}

static void usage(const char *prog)
{
	printf("Usage: %s [options]\n", prog);
	printf("  -p, --port PORT         Listen port (default: 18460)\n");
	printf("  -m, --mode MODE         sink|echo|stream (default: sink)\n");
	printf("      --no-tls            Plaintext ws (tls_enable=0)\n");
	printf("  -c, --cert FILE         TLS cert chain (optional)\n");
	printf("  -k, --key FILE          TLS private key (optional)\n");
	printf("  -a, --ca FILE           Client-auth CA (optional)\n");
	printf("      --stream-bytes N    Stream total bytes per client (default: 16777216)\n");
	printf("      --stream-chunk N    Stream chunk size (default: 8192)\n");
	printf("      --stream-interval-us N  Stream interval in us (default: 100)\n");
}

int main(int argc, char **argv)
{
	struct ez_wss_server_config config;

	int opt;
	int option_index = 0;
	static struct option long_options[] = {
		{"port", required_argument, 0, 'p'},
		{"mode", required_argument, 0, 'm'},
		{"no-tls", no_argument, 0, 0},
		{"cert", required_argument, 0, 'c'},
		{"key", required_argument, 0, 'k'},
		{"ca", required_argument, 0, 'a'},
		{"stream-bytes", required_argument, 0, 0},
		{"stream-chunk", required_argument, 0, 0},
		{"stream-interval-us", required_argument, 0, 0},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "p:m:c:k:a:h", long_options, &option_index)) != -1) {
		switch (opt) {
		case 'p': g_port = atoi(optarg); break;
		case 'm':
			if (!strcmp(optarg, "sink")) g_mode = 0;
			else if (!strcmp(optarg, "echo")) g_mode = 1;
			else if (!strcmp(optarg, "stream")) g_mode = 2;
			else { fprintf(stderr, "bad mode: %s\n", optarg); return 1; }
			break;
		case 'c': g_cert_path = optarg; break;
		case 'k': g_key_path = optarg; break;
		case 'a': g_ca_path = optarg; break;
		case 'h': usage(argv[0]); return 0;
		case 0:
			if (!strcmp(long_options[option_index].name, "no-tls")) g_no_tls = 1;
			else if (!strcmp(long_options[option_index].name, "stream-bytes")) g_stream_bytes = strtoull(optarg, NULL, 10);
			else if (!strcmp(long_options[option_index].name, "stream-chunk")) g_stream_chunk = (size_t)strtoul(optarg, NULL, 10);
			else if (!strcmp(long_options[option_index].name, "stream-interval-us")) g_stream_interval_us = strtol(optarg, NULL, 10);
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	signal(SIGINT, sigint_handler);

	memset(&config, 0, sizeof(config));
	config.port = g_port;
	config.protocol = BENCH_PROTOCOL;
	config.path_prefix = BENCH_PATH_PREFIX;
	config.tls_enable = g_no_tls ? 0 : 1;
	config.tls_cert_path = g_cert_path;
	config.tls_key_path = g_key_path;
	config.tls_ca_path = g_ca_path;
	/* 保活全部禁用，保证带宽测试干净 */
	config.ping_interval_ms = 0;
	config.ping_timeout_ms = 0;
	config.idle_timeout_ms = 0;
	config.timer_interval_ms = 0;
	config.ping_jitter_percent = 0;

	struct ez_ws_server_callbacks cb;
	memset(&cb, 0, sizeof(cb));
	cb.on_receive = on_receive;
	cb.on_connected = on_connected;
	cb.on_disconnected = on_disconnected;
	cb.user_data = NULL;

	g_server = ez_wss_server_handle_create(&config, &cb);
	if (!g_server) {
		printf("[svr] init failed\n");
		return 1;
	}

	printf("[svr] WSS server listening on port %d (%s), mode=%s\n",
	       g_port, g_no_tls ? "ws" : "wss",
	       g_mode == 0 ? "sink" : g_mode == 1 ? "echo" : "stream");
	fflush(stdout);

	if (pthread_create(&g_svc_thread, NULL, svc_thread_func, NULL) != 0) {
		printf("[svr] failed to create service thread\n");
		ez_wss_server_cleanup(g_server);
		return 1;
	}

	if (g_mode == 2) {
		g_stream_thread_running = 1;
		if (pthread_create(&g_stream_thread, NULL, stream_thread_func, NULL) != 0) {
			g_stream_thread_running = 0;
			printf("[svr] failed to create stream thread\n");
		}
	}

	pthread_t console_thread;
	if (pthread_create(&console_thread, NULL, console_thread_func, NULL) != 0) {
		printf("[svr] no console (non-interactive)\n");
	}

	while (!g_interrupted)
		sleep(1);

	g_interrupted = 1;
	g_stream_thread_running = 0;

	if (g_stream_thread)
		pthread_join(g_stream_thread, NULL);
	pthread_join(g_svc_thread, NULL);

	print_summary();

	ez_wss_server_cleanup(g_server);
	printf("[svr] shutdown complete\n");
	return 0;
}
