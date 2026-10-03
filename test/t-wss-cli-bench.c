/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * t-wss-cli-bench.c - WSS 客户端性能测试（多链路、时延、带宽）
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: t-wss-cli-bench.c $
 *
 * Explain:
 *     WSS 客户端性能测试程序，配合 t-wss-svr-bench.c 使用。
 *     -n N 建立 N 条并发链路（默认 128）。
 *     带宽分三种方向：
 *       send  客户端 → 服务端吞吐（服务端需 sink 模式）
 *       recv  服务端 → 客户端吞吐（服务端需 stream 模式）
 *       both  双向同时收发（服务端需 echo 模式）
 *     时延 lat 通过 echo 回显 + 内嵌 8 字节序号匹配计算 RTT。
 *
 *     注意：每条链路使用单线程既驱动 ez_wss_service_exec 又负责发送。
 *     这是因为客户端发送路径（ez_wss_send_binary → SSL_write）与
 *     ez_wss_service_exec 内部的发送/冲刷共享 pending_send 与同一个 SSL，
 *     若在独立工作线程里高速并发发送，会在同一 SSL 上并发 SSL_write 导致
 *     连接被误判断开。单线程交替“服务+发送”可完全规避该竞争，测得稳定带宽。
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

/* 只保留 ERROR 级别日志，屏蔽库的 INFO 输出 */
#define EZ_PRINT_LOG_LEVEL 3
#include <ezutil/ez_def_devel_debug.h>

#include "ez_wss-client-native.h"

#ifndef BENCH_PROTOCOL
#define BENCH_PROTOCOL "come.0"
#endif
#ifndef BENCH_PATH
#define BENCH_PATH "/come"
#endif

/* ====================== 全局参数 ====================== */

static volatile int g_interrupted = 0;

static const char *g_host = "127.0.0.1";
static unsigned short g_port = 18460;
static int g_links = 128;           /* 并发链路数 */
static int g_mode = 3;              /* 0=lat 1=send 2=recv 3=both 4=verify（默认 both） */
static size_t g_payload = 4096;     /* 单帧载荷字节 */
static double g_duration = 5.0;     /* 测试时长（秒） */
static int g_lat_count = 50;        /* 时延探测次数 */
static long g_lat_wait_ms = 2000;   /* 时延等待最长（毫秒） */
static uint32_t g_connect_timeout_ms = 5000;
static int g_send_burst = 64;       /* 每次服务迭代之间批量发送帧数 */
static int g_no_tls = 0;
static int g_no_verify = 0;
static const char *g_ca_path = NULL;

/* ====================== 每链路上下文 ====================== */

struct link_ctx {
	int index;
	struct ez_wss_client_handle *ws;
	pthread_t thread;
	int thread_started;

	volatile int connected;

	/* 统计（原子累加） */
	volatile uint64_t tx_bytes;
	volatile uint64_t tx_msgs;
	volatile uint64_t rx_bytes;
	volatile uint64_t rx_msgs;

	/* 时延：内嵌 8 字节序号，收到 echo 后比对 */
	volatile uint64_t echo_seq;
	volatile uint64_t echo_seen;

	/* verify 模式：内嵌自增序号，回显后按序核对丢/重/坏 */
	volatile uint64_t verify_next;    /* 期望收到的下一序号 */
	volatile uint64_t rx_gap;         /* 丢帧数（序号跳变） */
	volatile uint64_t rx_dup;         /* 重号/乱序数（序号回退） */
	volatile uint64_t rx_bad;         /* 坏帧数（长度不符/无法解析） */
	uint8_t *v_buf;                   /* verify 发送缓冲（每链路一份，内嵌序号） */

	/* 测试起止（壁钟，用于带宽计算） */
	volatile uint64_t t_start_us;
	volatile uint64_t t_first_rx_us;
	volatile uint64_t t_end_us;

	/* 单线程循环结束标志 */
	volatile int done;
};

static struct link_ctx *g_links_arr = NULL;

static uint64_t now_us(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
}

static void sleep_us(long us)
{
	struct timespec ts;
	ts.tv_sec = us / 1000000L;
	ts.tv_nsec = (us % 1000000L) * 1000L;
	while (nanosleep(&ts, &ts) != 0 && errno == EINTR)
		;
}

/* ====================== 回调 ====================== */

static void on_receive(const void *data, size_t len, int is_binary, void *user_data)
{
	(void)is_binary;
	struct link_ctx *lk = (struct link_ctx *)user_data;

	if (lk->t_first_rx_us == 0)
		lk->t_first_rx_us = now_us();

	lk->rx_bytes += len;
	lk->rx_msgs++;

	if (g_mode == 0 && len >= 8) {
		uint64_t seq;
		memcpy(&seq, data, sizeof(seq));
		if (seq == lk->echo_seq)
			lk->echo_seen = seq;
	}

	if (g_mode == 4 && len >= 8) {
		uint64_t seq;
		memcpy(&seq, data, sizeof(seq));
		if (len != g_payload) {
			lk->rx_bad++;
		} else if (lk->verify_next == 0) {
			/* 首个回显：以此为基准 */
			lk->verify_next = seq + 1;
		} else if (seq == lk->verify_next) {
			lk->verify_next++;
		} else if (seq > lk->verify_next) {
			/* 跳号：丢了 seq - verify_next 帧 */
			lk->rx_gap += (seq - lk->verify_next);
			lk->verify_next = seq + 1;
		} else {
			/* 回退：重复或乱序 */
			lk->rx_dup++;
		}
	}
}

static void on_connected(void *user_data)
{
	struct link_ctx *lk = (struct link_ctx *)user_data;
	lk->connected = 1;
}

static void on_disconnected(void *user_data)
{
	struct link_ctx *lk = (struct link_ctx *)user_data;
	lk->connected = 0;
}

/* ====================== 发送缓冲 ====================== */

static int g_buf_ready = 0;
static uint8_t *g_buf = NULL;

static void ensure_buf(void)
{
	if (g_buf_ready)
		return;
	g_buf = malloc(g_payload);
	if (!g_buf) {
		fprintf(stderr, "OOM\n");
		exit(1);
	}
	memset(g_buf, 0x5A, g_payload);
	g_buf_ready = 1;
}

/* 单次发送，处理队列满则短暂重试 */
static void send_payload(struct link_ctx *lk)
{
	int rc;
	int retry = 0;
	do {
		rc = ez_wss_send_binary(lk->ws, g_buf, g_payload);
		if (rc == EZ_WS_OK) {
			lk->tx_bytes += g_payload;
			lk->tx_msgs++;
			return;
		}
		if (rc == EZ_WS_ERR_QUEUE_FULL || rc == EZ_WS_ERR_CONNECTING || rc == EZ_WS_ERR_NOT_READY) {
			retry++;
			if (retry > 10000)
				return;
			sleep_us(200);
			continue;
		}
		return;   /* 其它错误直接放弃 */
	} while (1);
}

/* verify 模式发送：每链路缓冲，帧头嵌 8 字节自增序号，载荷由序号生成
 * （回显后按序号核对，同时验证内容是否被破坏） */
static void verify_send_payload(struct link_ctx *lk)
{
	uint64_t seq = lk->tx_msgs;
	uint8_t *buf = lk->v_buf;

	memcpy(buf, &seq, sizeof(seq));
	for (size_t i = 8; i < g_payload; i++)
		buf[i] = (uint8_t)((seq * 31 + i) & 0xFF);

	int rc;
	int retry = 0;
	do {
		rc = ez_wss_send_binary(lk->ws, buf, g_payload);
		if (rc == EZ_WS_OK) {
			lk->tx_bytes += g_payload;
			lk->tx_msgs++;
			return;
		}
		if (rc == EZ_WS_ERR_QUEUE_FULL || rc == EZ_WS_ERR_CONNECTING || rc == EZ_WS_ERR_NOT_READY) {
			retry++;
			if (retry > 10000)
				return;
			sleep_us(200);
			continue;
		}
		return;   /* 其它错误直接放弃 */
	} while (1);
}

/* ====================== 单线程链路循环 ====================== */

static void run_latency(struct link_ctx *lk)
{
	for (int i = 0; i < g_lat_count && !g_interrupted; i++) {
		uint64_t seq = (uint64_t)i + 1;
		uint8_t probe[16];
		memcpy(probe, &seq, sizeof(seq));

		lk->echo_seq = seq;
		lk->echo_seen = 0;

		uint64_t t0 = now_us();
		if (ez_wss_send_binary(lk->ws, probe, sizeof(probe)) != EZ_WS_OK)
			continue;

		uint64_t deadline = t0 + (uint64_t)g_lat_wait_ms * 1000ull;
		while (!lk->echo_seen && !g_interrupted && now_us() < deadline) {
			ez_wss_service_exec(lk->ws, 1);   /* 持续服务以接收回显 */
		}

		uint64_t t1 = now_us();
		if (lk->echo_seen) {
			printf("[link %3d] lat[%d] = %llu us\n", lk->index, i,
			       (unsigned long long)(t1 - t0));
			fflush(stdout);
		} else {
			printf("[link %3d] lat[%d] = timeout\n", lk->index, i);
			fflush(stdout);
		}
	}
}

static void *link_thread_func(void *arg)
{
	struct link_ctx *lk = (struct link_ctx *)arg;

	/* 等待连接建立（连接在 service_exec 中异步推进） */
	uint64_t connect_deadline = now_us() + (uint64_t)g_connect_timeout_ms * 1000ull;
	while (!lk->connected && !g_interrupted && now_us() < connect_deadline) {
		ez_wss_service_exec(lk->ws, 1);
		sleep_us(500);
	}
	if (!lk->connected) {
		printf("[link %3d] connect timeout\n", lk->index);
		fflush(stdout);
		lk->done = 1;
		return NULL;
	}

	switch (g_mode) {
	case 0:   /* lat */
		run_latency(lk);
		break;
	case 1:   /* send */
	case 3:   /* both */
		/* 交替：服务一次 + 批量发送，单线程避免并发 SSL_write */
		while (!g_interrupted) {
			ez_wss_service_exec(lk->ws, 1);
			for (int i = 0; i < g_send_burst; i++) {
				if (g_interrupted)
					break;
				send_payload(lk);
			}
		}
		break;
	case 4:   /* verify：满速发送带序号帧 + echo 回显核对 */
		while (!g_interrupted) {
			ez_wss_service_exec(lk->ws, 1);
			for (int i = 0; i < g_send_burst; i++) {
				if (g_interrupted)
					break;
				verify_send_payload(lk);
			}
		}
		break;
	case 2:   /* recv：只收不发，由服务端 stream 推流 */
		while (!g_interrupted)
			ez_wss_service_exec(lk->ws, 1);
		break;
	default:
		break;
	}

	lk->done = 1;
	return NULL;
}

/* ====================== 启动/结束链路 ====================== */

static void start_link(struct link_ctx *lk)
{
	struct ez_wss_client_config cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.url_path = BENCH_PATH;
	cfg.protocol = BENCH_PROTOCOL;
	cfg.server_addr = g_host;
	cfg.port = g_port;
	cfg.connect_timeout_ms = g_connect_timeout_ms;
	cfg.reconnect_max_retries = 1;
	cfg.reconnect_backoff_enable = 0;
	cfg.reconnect_interval_ms = 1000;
	cfg.tls_enable = g_no_tls ? 0 : 1;
	cfg.tls_verify_peer = (g_no_tls || g_no_verify) ? 0 : 1;
	cfg.tls_ca_path = g_ca_path;

	if (g_mode == 4) {
		lk->v_buf = malloc(g_payload);
		if (!lk->v_buf) {
			fprintf(stderr, "[link %3d] OOM v_buf\n", lk->index);
			fflush(stdout);
			return;
		}
		lk->verify_next = 0;
		lk->rx_gap = 0;
		lk->rx_dup = 0;
		lk->rx_bad = 0;
	}

	struct ez_ws_callbacks cb;
	memset(&cb, 0, sizeof(cb));
	cb.on_receive = on_receive;
	cb.on_connected = on_connected;
	cb.on_disconnected = on_disconnected;
	cb.user_data = lk;

	lk->ws = ez_wss_client_handle_create(&cfg, &cb);
	if (!lk->ws) {
		printf("[link %3d] create failed\n", lk->index);
		fflush(stdout);
		return;
	}

	if (pthread_create(&lk->thread, NULL, link_thread_func, lk) == 0)
		lk->thread_started = 1;
}

static void end_link(struct link_ctx *lk)
{
	if (lk->thread_started) {
		pthread_join(lk->thread, NULL);
		lk->thread_started = 0;
	}
	if (lk->ws) {
		ez_wss_client_cleanup(lk->ws);
		lk->ws = NULL;
	}
	if (lk->v_buf) {
		free(lk->v_buf);
		lk->v_buf = NULL;
	}
}

/* ====================== 汇总报告 ====================== */

static void print_report(void)
{
	uint64_t total_tx = 0, total_rx = 0;
	uint64_t total_tx_msgs = 0, total_rx_msgs = 0;
	uint64_t total_gap = 0, total_dup = 0, total_bad = 0;
	int connected = 0;

	for (int i = 0; i < g_links; i++) {
		struct link_ctx *lk = &g_links_arr[i];
		total_tx += lk->tx_bytes;
		total_rx += lk->rx_bytes;
		total_tx_msgs += lk->tx_msgs;
		total_rx_msgs += lk->rx_msgs;
		total_gap += lk->rx_gap;
		total_dup += lk->rx_dup;
		total_bad += lk->rx_bad;
		if (lk->connected)
			connected++;
	}

	/* 带宽窗口：send/both 用 起→止；recv 用 首收→止（避免配额耗尽前被低估） */
	uint64_t t0 = UINT64_MAX, t1 = 0;
	for (int i = 0; i < g_links; i++) {
		struct link_ctx *lk = &g_links_arr[i];
		uint64_t s = lk->t_start_us, e = lk->t_end_us;
		if (g_mode == 2 && lk->t_first_rx_us && lk->t_first_rx_us < s)
			s = lk->t_first_rx_us;
		if (s && s < t0) t0 = s;
		if (e && e > t1) t1 = e;
	}
	double secs = 0;
	if (t1 > t0)
		secs = (double)(t1 - t0) / 1000000.0;

	printf("\n================ WSS Client Bench Report ================\n");
	printf("  Links     : %d (connected %d)\n", g_links, connected);
	printf("  Mode      : %s\n",
	       g_mode == 0 ? "latency" : g_mode == 1 ? "send (client->server)"
	       : g_mode == 2 ? "recv (server->client)"
	       : g_mode == 3 ? "both (full-duplex)" : "verify (echo+seq integrity)");
	printf("  Payload   : %zu bytes/frame\n", g_payload);
	printf("  Window    : %.3f s\n", secs);
	printf("  TX        : %llu bytes, %llu msgs  (%.2f MB/s, %.0f msg/s)\n",
	       (unsigned long long)total_tx, (unsigned long long)total_tx_msgs,
	       secs > 0 ? (double)total_tx / 1e6 / secs : 0,
	       secs > 0 ? (double)total_tx_msgs / secs : 0);
	printf("  RX        : %llu bytes, %llu msgs  (%.2f MB/s, %.0f msg/s)\n",
	       (unsigned long long)total_rx, (unsigned long long)total_rx_msgs,
	       secs > 0 ? (double)total_rx / 1e6 / secs : 0,
	       secs > 0 ? (double)total_rx_msgs / secs : 0);
	if (g_mode == 4) {
		printf("  INTEGRITY : sent=%llu echoed=%llu  lost=%llu  dup=%llu  bad=%llu\n",
		       (unsigned long long)total_tx_msgs,
		       (unsigned long long)total_rx_msgs,
		       (unsigned long long)total_gap,
		       (unsigned long long)total_dup,
		       (unsigned long long)total_bad);
		printf("  VERDICT   : %s\n",
		       (total_rx_msgs > 0 && total_gap == 0 && total_dup == 0 && total_bad == 0)
		       ? "OK - no loss/dup/corruption" : "FAIL - see counts above");
	}
	printf("==========================================================\n");
	fflush(stdout);
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
	printf("  -h, --host HOST        Server host (default: 127.0.0.1)\n");
	printf("  -p, --port PORT        Server port (default: 18460)\n");
	printf("  -n, --links N          Concurrent links (default: 128)\n");
	printf("  -m, --mode MODE        lat|send|recv|both|verify (default: both)\n");
	printf("      --payload N        Payload bytes/frame (default: 4096)\n");
	printf("      --duration SEC     Test duration (default: 5)\n");
	printf("      --lat-count N      Latency probe count (default: 50)\n");
	printf("      --lat-wait-ms N    Latency max wait ms (default: 2000)\n");
	printf("      --send-burst N     Frames sent per service iteration (default: 64)\n");
	printf("      --connect-timeout-ms N  (default: 5000)\n");
	printf("      --no-tls           Plaintext ws\n");
	printf("      --no-verify        Encrypt only, no cert verify\n");
	printf("  -a, --ca FILE          CA file (default: built-in)\n");
}

int main(int argc, char **argv)
{
	int opt;
	int option_index = 0;
	static struct option long_options[] = {
		{"host", required_argument, 0, 'h'},
		{"port", required_argument, 0, 'p'},
		{"links", required_argument, 0, 'n'},
		{"mode", required_argument, 0, 'm'},
		{"payload", required_argument, 0, 0},
		{"duration", required_argument, 0, 0},
		{"lat-count", required_argument, 0, 0},
		{"lat-wait-ms", required_argument, 0, 0},
		{"send-burst", required_argument, 0, 0},
		{"connect-timeout-ms", required_argument, 0, 0},
		{"no-tls", no_argument, 0, 0},
		{"no-verify", no_argument, 0, 0},
		{"ca", required_argument, 0, 'a'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "h:p:n:m:a:", long_options, &option_index)) != -1) {
		switch (opt) {
		case 'h': g_host = optarg; break;
		case 'p': g_port = (unsigned short)atoi(optarg); break;
		case 'n': g_links = atoi(optarg); break;
		case 'm':
			if (!strcmp(optarg, "lat")) g_mode = 0;
			else if (!strcmp(optarg, "send")) g_mode = 1;
			else if (!strcmp(optarg, "recv")) g_mode = 2;
			else if (!strcmp(optarg, "both")) g_mode = 3;
			else if (!strcmp(optarg, "verify")) g_mode = 4;
			else { fprintf(stderr, "bad mode: %s\n", optarg); return 1; }
			break;
		case 'a': g_ca_path = optarg; break;
		case 0:
			if (!strcmp(long_options[option_index].name, "payload")) g_payload = (size_t)strtoul(optarg, NULL, 10);
			else if (!strcmp(long_options[option_index].name, "duration")) g_duration = atof(optarg);
			else if (!strcmp(long_options[option_index].name, "lat-count")) g_lat_count = atoi(optarg);
			else if (!strcmp(long_options[option_index].name, "lat-wait-ms")) g_lat_wait_ms = strtol(optarg, NULL, 10);
			else if (!strcmp(long_options[option_index].name, "send-burst")) g_send_burst = atoi(optarg);
			else if (!strcmp(long_options[option_index].name, "connect-timeout-ms")) g_connect_timeout_ms = (uint32_t)strtoul(optarg, NULL, 10);
			else if (!strcmp(long_options[option_index].name, "no-tls")) g_no_tls = 1;
			else if (!strcmp(long_options[option_index].name, "no-verify")) g_no_verify = 1;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (g_links < 1) g_links = 1;
	ensure_buf();

	signal(SIGINT, sigint_handler);

	g_links_arr = calloc(g_links, sizeof(struct link_ctx));
	if (!g_links_arr) {
		fprintf(stderr, "OOM\n");
		return 1;
	}

	printf("WSS Client Bench: %d links -> %s:%u mode=%s\n",
	       g_links, g_host, (unsigned)g_port,
	       g_mode == 0 ? "lat" : g_mode == 1 ? "send"
	       : g_mode == 2 ? "recv" : g_mode == 3 ? "both" : "verify");
	fflush(stdout);

	uint64_t start = now_us();
	for (int i = 0; i < g_links; i++) {
		g_links_arr[i].index = i;
		start_link(&g_links_arr[i]);
	}

	/* 等待全部连接建立 */
	printf("Waiting for %d links to connect...\n", g_links);
	fflush(stdout);
	uint64_t wait_deadline = start + (uint64_t)g_connect_timeout_ms * 1000ull;
	while (!g_interrupted) {
		int n = 0;
		for (int i = 0; i < g_links; i++)
			if (g_links_arr[i].connected)
				n++;
		if (n == g_links)
			break;
		if (now_us() > wait_deadline)
			break;
		sleep_us(10000);
	}

	/* 记录测试窗口起点 */
	uint64_t t0 = now_us();
	for (int i = 0; i < g_links; i++)
		g_links_arr[i].t_start_us = t0;

	printf("Test started (%.1f s)...\n", g_duration);
	fflush(stdout);

	uint64_t run_deadline = now_us() + (uint64_t)(g_duration * 1000000.0);
	while (!g_interrupted && now_us() < run_deadline)
		sleep_us(20000);

	g_interrupted = 1;

	/* 记录窗口终点 */
	uint64_t t1 = now_us();
	for (int i = 0; i < g_links; i++) {
		g_links_arr[i].t_end_us = t1;
	}

	for (int i = 0; i < g_links; i++)
		end_link(&g_links_arr[i]);

	print_report();

	free(g_links_arr);
	if (g_buf) free(g_buf);
	printf("Done\n");
	return 0;
}
