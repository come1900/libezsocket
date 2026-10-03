/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * wss-svr-native.c - Native WSS (WebSocket over TLS) Server Example Program
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: wss-svr-native.c $
 *
 * Explain:
 *     WSS = WebSocket over TLS. WebSocket server example program using the
 *     ez_wss-server-native component (parallel to the plaintext
 *     ez_wsserver-native, which stays untouched).
 *     默认开箱即加密（tls_enable=1，未配证书走内置 CA 互认）；用 --no-tls
 *     关闭运行期 TLS（tls_enable=0），此时该监听退化为明文 ws，用于对照验证。
 *
 * Update:
 *     2026-09-29 Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include <ezutil/ez_system_api.h>
#include <ezutil/ez_def_devel_debug.h>
#include "ez_wss-server-native.h"

/* 控制台提示符 */
#ifndef WSS_SERVER_PROMPT
#define WSS_SERVER_PROMPT "wss-svr> "
#endif

/* WebSocket协议和路径配置 */
#ifndef WSS_SERVER_PROTOCOL
#define WSS_SERVER_PROTOCOL "come.0"
#endif

#ifndef WSS_SERVER_PATH_PREFIX
#define WSS_SERVER_PATH_PREFIX "/come"
#endif

/* ================== 链路保活配置（同明文版示例） ================== */
#define WSS_SERVER_PING_INTERVAL_MS     (30 * 1000)   /* 基础 ping 间隔，0 表示禁用 */
#define WSS_SERVER_PING_JITTER_PERCENT  10            /* ping 间隔抖动百分比 (0-50) */
#define WSS_SERVER_IDLE_TIMEOUT_MS      (180 * 1000)  /* 无业务流量断开时间，0 表示禁用 */

#define WSS_SERVER_PING_TIMEOUT_MS      (WSS_SERVER_PING_INTERVAL_MS / 3)
#define WSS_SERVER_TIMER_INTERVAL_MS    (WSS_SERVER_PING_TIMEOUT_MS / 3)

#define WSS_SERVER_PING_JITTER_MS \
	((WSS_SERVER_PING_INTERVAL_MS * WSS_SERVER_PING_JITTER_PERCENT) / 100)
#define WSS_SERVER_PING_INTERVAL_MAX_MS \
	(WSS_SERVER_PING_INTERVAL_MS + WSS_SERVER_PING_JITTER_MS)

#if WSS_SERVER_PING_INTERVAL_MS > 0 && WSS_SERVER_PING_INTERVAL_MS < 1000
#error "PING_INTERVAL should be 0 (disabled) or at least 1000ms (1 second)"
#endif
#if WSS_SERVER_PING_JITTER_PERCENT < 0 || WSS_SERVER_PING_JITTER_PERCENT > 50
#error "PING_JITTER_PERCENT should be between 0 and 50 (0% to 50%)"
#endif
#if WSS_SERVER_IDLE_TIMEOUT_MS > 0 && WSS_SERVER_IDLE_TIMEOUT_MS <= (2 * WSS_SERVER_PING_INTERVAL_MAX_MS + WSS_SERVER_PING_TIMEOUT_MS)
#error "IDLE_TIMEOUT should be 0 (disabled, not recommended) or large enough for at least 2 ping/pong rounds even with maximum jitter"
#endif
#if WSS_SERVER_IDLE_TIMEOUT_MS > 0 && WSS_SERVER_IDLE_TIMEOUT_MS < (2 * WSS_SERVER_PING_INTERVAL_MS)
#error "IDLE_TIMEOUT should be 0 (disabled, not recommended) or at least 2x PING_INTERVAL"
#endif

struct console_handle {
	pthread_t console_thread;
	int console_running;
	int *interrupted;
	struct ez_wss_server_handle *ws_handle;
};

static int interrupted = 0;
static pthread_t ws_thread;

static void* ws_server_thread_func(void *arg) {
	struct ez_wss_server_handle *handle = (struct ez_wss_server_handle *)arg;
	int n = 0;

	EZ_PRINT_LOG_INFO("WSS server thread started\n");

	while (n >= 0 && !interrupted) {
		n = ez_wss_server_service_exec(handle, 100);
	}

	EZ_PRINT_LOG_INFO("WSS server thread exiting\n");
	return NULL;
}

static void ws_server_on_connected(int client_id, const char *ip, int port, void *user_data) {
	EZ_PRINT_LOG_INFO("Client #%d connected from %s:%d\n", client_id, ip ? ip : "unknown", port);
}

static void ws_server_on_disconnected(int client_id, void *user_data) {
	EZ_PRINT_LOG_INFO("Client #%d disconnected\n", client_id);
}

static void ws_server_on_receive(int client_id, const void *data, size_t len, int is_binary, void *user_data) {
	EZ_PRINT_LOG_INFO("RECEIVE CALLBACK: client_id=%d, len=%zu, is_binary=%d\n", client_id, len, is_binary);

	if (!data || !len) {
		EZ_PRINT_LOG_WARN("No data received\n");
		return;
	}

	if (is_binary) {
		EZ_PRINT_LOG_INFO("Received binary data from client #%d: %zu bytes\n", client_id, len);
	} else {
		size_t print_len = len > 256 ? 256 : len;
		char buf[257];
		memcpy(buf, data, print_len);
		buf[print_len] = '\0';

		for (size_t i = 0; i < print_len; i++) {
			if (buf[i] < 32 && buf[i] != '\n' && buf[i] != '\r' && buf[i] != '\t') {
				buf[i] = '.';
			}
		}

		EZ_PRINT_LOG_INFO("Received text from client #%d: '%s'%s\n", client_id, buf, len > 256 ? "..." : "");
	}
}

static uint64_t get_now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

struct list_client_context {
	int index;
	struct ez_wss_server_handle *server_handle;
};

static int print_client_callback(const struct ez_ws_client_info *info, void *user_data) {
	struct list_client_context *ctx = (struct list_client_context *)user_data;
	time_t now = time(NULL);
	int duration = (int)(now - info->connect_time);

	uint64_t idle_ms = 0;
	if (info->last_activity_ms > 0) {
		uint64_t now_ms = get_now_ms();
		if (now_ms >= info->last_activity_ms) {
			idle_ms = now_ms - info->last_activity_ms;
		}
	}

	EZ_PRINT_LOG_INFO("  [%d] ID=%d, IP=%s:%d, Connected=%ds ago, Idle=%llums\n",
	         ctx->index++, info->id, info->ip, info->port, duration,
	         (unsigned long long)idle_ms);

#if defined(EZ_WS_SERVER_ENABLE_STATS) && (EZ_WS_SERVER_ENABLE_STATS == 1)
	struct ez_ws_server_client_stats stats;
	if (ez_wss_server_get_client_stats(ctx->server_handle, info->id, &stats) == EZ_WS_SERVER_OK) {
		EZ_PRINT_LOG_INFO("\n      TX: TEXT=%llu (%llu bytes), BINARY=%llu (%llu bytes), PING=%llu, PONG=%llu, CLOSE=%llu",
		         (unsigned long long)stats.tx_text_count,
		         (unsigned long long)stats.tx_text_bytes,
		         (unsigned long long)stats.tx_binary_count,
		         (unsigned long long)stats.tx_binary_bytes,
		         (unsigned long long)stats.tx_ping_count,
		         (unsigned long long)stats.tx_pong_count,
		         (unsigned long long)stats.tx_close_count);
		EZ_PRINT_LOG_INFO("\n      RX: TEXT=%llu (%llu bytes), BINARY=%llu (%llu bytes), PING=%llu, PONG=%llu, CLOSE=%llu",
		         (unsigned long long)stats.rx_text_count,
		         (unsigned long long)stats.rx_text_bytes,
		         (unsigned long long)stats.rx_binary_count,
		         (unsigned long long)stats.rx_binary_bytes,
		         (unsigned long long)stats.rx_ping_count,
		         (unsigned long long)stats.rx_pong_count,
		         (unsigned long long)stats.rx_close_count);
	}
#endif /* EZ_WS_SERVER_ENABLE_STATS */

	EZ_PRINT_LOG_INFO("\n");
	return 0;
}

static void *
console_thread_func(void *arg)
{
	struct console_handle *console = (struct console_handle *)arg;
	char line[4096];
	int interactive = isatty(STDIN_FILENO);

	console->console_running = 1;
	if (interactive) {
		EZ_PRINT_LOG_INFO("Server console ready. Commands:\n");
		EZ_PRINT_LOG_INFO("  clients       - Show connected clients list\n");
		EZ_PRINT_LOG_INFO("  send <id> <msg> - Send message to specific client\n");
		EZ_PRINT_LOG_INFO("  status        - Show server status\n");
		EZ_PRINT_LOG_INFO("  help          - Show help information\n");
		EZ_PRINT_LOG_INFO("  quit          - Exit server\n");
		EZ_PRINT_LOG_INFO("  other         - Broadcast to all clients\n");
	}

	while (console->console_running) {
		if (interactive) {
			fputs(WSS_SERVER_PROMPT, stdout);
			fflush(stdout);
		}
		if (!fgets(line, sizeof(line), stdin)) {
			ez_usleep(100000);
			continue;
		}

		size_t len = strlen(line);
		while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
			line[--len] = '\0';
		}
		if (!len)
			continue;

		if (!strcmp(line, "quit") || !strcmp(line, "exit")) {
			if (console->interrupted)
				*console->interrupted = 1;
			break;
		}

		if (!strcmp(line, "clients")) {
			if (console->ws_handle) {
				int count = ez_wss_server_get_client_count(console->ws_handle);
				EZ_PRINT_LOG_INFO("\n=== Connected Clients (%d) ===\n", count);
				if (count == 0) {
					EZ_PRINT_LOG_INFO("  No clients connected\n");
				} else {
					struct list_client_context ctx = { .index = 1, .server_handle = console->ws_handle };
					ez_wss_server_foreach_client(console->ws_handle, print_client_callback, &ctx);
				}
				EZ_PRINT_LOG_INFO("\n");
			} else {
				EZ_PRINT_LOG_WARN("WebSocket not initialized\n");
			}
			continue;
		}

		if (!strcmp(line, "status")) {
			if (console->ws_handle) {
				int count = ez_wss_server_get_client_count(console->ws_handle);
				int running = ez_wss_server_is_ready(console->ws_handle);
				EZ_PRINT_LOG_INFO("\nServer Status:\n");
				EZ_PRINT_LOG_INFO("  Port: listening\n");
				EZ_PRINT_LOG_INFO("  Connected clients: %d\n", count);
				EZ_PRINT_LOG_INFO("  Active: %s\n", running ? "running" : "stopped");
			} else {
				EZ_PRINT_LOG_WARN("WebSocket not initialized\n");
			}
			continue;
		}

		if (!strcmp(line, "help")) {
			EZ_PRINT_LOG_INFO("\nAvailable commands:\n");
			EZ_PRINT_LOG_INFO("  clients          - Show connected clients list\n");
			EZ_PRINT_LOG_INFO("  send <id> <msg>  - Send message to specific client by ID\n");
			EZ_PRINT_LOG_INFO("  status           - Show server status\n");
			EZ_PRINT_LOG_INFO("  help             - Show this help message\n");
			EZ_PRINT_LOG_INFO("  quit/exit        - Stop server and exit\n");
			EZ_PRINT_LOG_INFO("  other input      - Broadcast message to all connected clients\n");
			EZ_PRINT_LOG_INFO("\nExample:\n");
			EZ_PRINT_LOG_INFO("  send 1 Hello     - Send 'Hello' to client #1\n");
			EZ_PRINT_LOG_INFO("\n");
			continue;
		}

		if (strncmp(line, "send ", 5) == 0) {
			if (!console->ws_handle) {
				EZ_PRINT_LOG_WARN("WebSocket not initialized\n");
				continue;
			}

			char *cmd_ptr = line + 5;
			while (*cmd_ptr == ' ') cmd_ptr++;

			if (!*cmd_ptr) {
				EZ_PRINT_LOG_WARN("Usage: send <client_id> <message>\n");
				continue;
			}

			char *endptr;
			long client_id = strtol(cmd_ptr, &endptr, 10);

			if (endptr == cmd_ptr || client_id < 0) {
				EZ_PRINT_LOG_WARN("Invalid client ID\n");
				continue;
			}

			cmd_ptr = endptr;
			while (*cmd_ptr == ' ') cmd_ptr++;

			if (!*cmd_ptr) {
				EZ_PRINT_LOG_WARN("Message cannot be empty\n");
				continue;
			}

			int ret = ez_wss_server_send_text(console->ws_handle, (int)client_id, cmd_ptr, 0);

			if (ret == EZ_WS_SERVER_OK) {
				EZ_PRINT_LOG_INFO("Message sent to client #%d\n", (int)client_id);
			} else if (ret == EZ_WS_SERVER_ERR_CLIENT_NOT_FOUND) {
				EZ_PRINT_LOG_WARN("Client #%d not found\n", (int)client_id);
			} else {
				EZ_PRINT_LOG_ERROR("Send failed, error code: %d\n", ret);
			}
			continue;
		}

		if (console->ws_handle) {
			EZ_PRINT_LOG_INFO("Broadcasting message: '%s' (%zu bytes)\n", line, len);
			int ret = ez_wss_server_send_text(console->ws_handle, -1, line, len);
			if (ret == EZ_WS_SERVER_OK) {
				EZ_PRINT_LOG_INFO("Message broadcasted successfully\n");
			} else if (ret == EZ_WS_SERVER_ERR_QUEUE_FULL) {
				EZ_PRINT_LOG_WARN("Broadcast failed, queue full\n");
			} else if (ret == EZ_WS_SERVER_ERR_CLIENT_NOT_FOUND) {
				EZ_PRINT_LOG_WARN("Client not found\n");
			} else {
				EZ_PRINT_LOG_ERROR("Broadcast failed, error code: %d\n", ret);
			}
		} else {
			EZ_PRINT_LOG_WARN("WebSocket not initialized\n");
		}
	}

	console->console_running = 0;
	return NULL;
}

static struct console_handle*
console_init(struct ez_wss_server_handle *ws_handle, int *interrupted_flag)
{
	struct console_handle *console;

	if (!ws_handle || !interrupted_flag)
		return NULL;

	console = (struct console_handle *)malloc(sizeof(struct console_handle));
	if (!console)
		return NULL;

	memset(console, 0, sizeof(*console));

	console->interrupted = interrupted_flag;
	console->console_running = 0;
	console->ws_handle = ws_handle;

	return console;
}

static int
console_start(struct console_handle *console)
{
	if (!console)
		return -1;

	console->console_running = 1;
	if (pthread_create(&console->console_thread, NULL, console_thread_func, console) != 0) {
		EZ_PRINT_LOG_ERROR("Failed to create console thread\n");
		console->console_running = 0;
		return -1;
	}

	return 0;
}

static void
console_stop(struct console_handle *console)
{
	if (!console)
		return;

	console->console_running = 0;
	pthread_join(console->console_thread, NULL);
}

static void
console_cleanup(struct console_handle *console)
{
	if (!console)
		return;
	free(console);
}

void sigint_handler(int sig)
{
	(void)sig;
	interrupted = 1;
}

static void usage(const char *prog)
{
	EZ_PRINT_LOG_INFO("Usage: %s [-p PORT] [--no-tls] [-c CERT] [-k KEY] [-a CA]\n", prog);
	EZ_PRINT_LOG_INFO("  -p, --port PORT    Listen port (default: 54321)\n");
	EZ_PRINT_LOG_INFO("      --no-tls       Run plaintext ws for this listener (tls_enable=0)\n");
	EZ_PRINT_LOG_INFO("  -c, --cert FILE    TLS certificate chain file (optional)\n");
	EZ_PRINT_LOG_INFO("  -k, --key FILE     TLS private key file (optional)\n");
	EZ_PRINT_LOG_INFO("  -a, --ca FILE      Optional client-auth CA (mutual TLS) (optional)\n");
	EZ_PRINT_LOG_INFO("  (没有 -c/-k 时默认走内置 CA 互认：运行时签发叶子证书)\n");
}

int main(int argc, const char **argv)
{
	struct ez_wss_server_handle *server_handle = NULL;
	struct console_handle *console = NULL;
	struct ez_wss_server_config config;
	const char *cert_path = NULL;
	const char *key_path = NULL;
	const char *ca_path = NULL;
	int no_tls = 0;

	int port = 54321;
	for (int i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) && i + 1 < argc) {
			port = atoi(argv[i + 1]);
			i++;
		} else if (strcmp(argv[i], "--no-tls") == 0) {
			no_tls = 1;
		} else if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cert") == 0) && i + 1 < argc) {
			cert_path = argv[i + 1];
			i++;
		} else if ((strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--key") == 0) && i + 1 < argc) {
			key_path = argv[i + 1];
			i++;
		} else if ((strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--ca") == 0) && i + 1 < argc) {
			ca_path = argv[i + 1];
			i++;
		} else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			usage(argv[0]);
			return 0;
		} else {
			EZ_PRINT_LOG_WARN("Unknown option: %s\n", argv[i]);
			usage(argv[0]);
			return 1;
		}
	}

	EZ_PRINT_LOG_INFO("Native WSS Server (WebSocket over TLS, using epoll/timerfd/socket)\n");
	EZ_PRINT_LOG_INFO("Server listening on port %d (%s)\n", port, no_tls ? "plaintext ws" : "wss/TLS");

	memset(&config, 0, sizeof(config));
	config.port = port;
	config.protocol = WSS_SERVER_PROTOCOL;
	config.path_prefix = WSS_SERVER_PATH_PREFIX;
	config.options = 0;

	/* TLS：默认开箱即加密；--no-tls 置 tls_enable=0（监听级，退化为明文） */
	config.tls_enable = no_tls ? 0 : 1;
	config.tls_cert_path = cert_path;
	config.tls_key_path = key_path;
	config.tls_ca_path = ca_path;

	config.idle_timeout_ms = WSS_SERVER_IDLE_TIMEOUT_MS;

#if WSS_SERVER_PING_INTERVAL_MS > 0
	config.timer_interval_ms = WSS_SERVER_TIMER_INTERVAL_MS;
	config.ping_interval_ms = WSS_SERVER_PING_INTERVAL_MS;
	config.ping_timeout_ms = WSS_SERVER_PING_TIMEOUT_MS;
	config.ping_jitter_percent = WSS_SERVER_PING_JITTER_PERCENT;
#else
	config.timer_interval_ms = 0;
	config.ping_interval_ms = 0;
	config.ping_timeout_ms = 0;
	config.ping_jitter_percent = 0;
#endif

	signal(SIGINT, sigint_handler);

	struct ez_ws_server_callbacks ws_callbacks = {
		.on_receive = ws_server_on_receive,
		.on_connected = ws_server_on_connected,
		.on_disconnected = ws_server_on_disconnected,
		.user_data = NULL
	};

	server_handle = ez_wss_server_handle_create(&config, &ws_callbacks);
	if (!server_handle) {
		EZ_PRINT_LOG_ERROR("WSS server init failed\n");
		return 1;
	}

	console = console_init(server_handle, &interrupted);
	if (!console) {
		EZ_PRINT_LOG_ERROR("Console init failed\n");
		ez_wss_server_cleanup(server_handle);
		return 1;
	}

	if (pthread_create(&ws_thread, NULL, ws_server_thread_func, server_handle) != 0) {
		EZ_PRINT_LOG_ERROR("Failed to create WebSocket thread\n");
		console_cleanup(console);
		ez_wss_server_cleanup(server_handle);
		return 1;
	}

	if (console_start(console) != 0) {
		EZ_PRINT_LOG_ERROR("Failed to start console thread\n");
		interrupted = 1;
		pthread_join(ws_thread, NULL);
		console_cleanup(console);
		ez_wss_server_cleanup(server_handle);
		return 1;
	}

	EZ_PRINT_LOG_INFO("All threads started, entering main loop...\n");

	while (!interrupted) {
		sleep(1);
	}

	EZ_PRINT_LOG_INFO("Shutting down server...\n");

	console_stop(console);

	interrupted = 1;
	pthread_join(ws_thread, NULL);

	console_cleanup(console);
	ez_wss_server_cleanup(server_handle);

	EZ_PRINT_LOG_INFO("Server shutdown completed\n");

	return 0;
}
