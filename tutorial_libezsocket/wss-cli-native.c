/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * wss-cli-native.c - Native WSS (WebSocket over TLS) Client Example Program
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: wss-cli-native.c $
 *
 * Explain:
 *     WSS = WebSocket over TLS. WebSocket client example program using the
 *     ez_wss-client-native component (parallel to the plaintext
 *     ez_wsclient-native, which stays untouched).
 *     默认 trust 内置 CA 校验服务端证书（tls_verify_peer=1，与 wss-svr-native
 *     的内置 CA 互认配对）；--no-tls 走明文、--no-verify 仅加密不认证（降级档）。
 *
 * Update:
 *     2026-09-29 Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>

#include <ezutil/ez_system_api.h>

#define EZ_PRINT_LOG_LEVEL 0//EZ_PRINT_LOG_LEVEL_DEBUG
#include <ezutil/ez_def_devel_debug.h>

#include "ez_wss-client-native.h"

/* WebSocket协议和路径配置 */
#ifndef WSS_CLIENT_PROTOCOL
#define WSS_CLIENT_PROTOCOL "come.0"
#endif

#ifndef WSS_CLIENT_PATH
#define WSS_CLIENT_PATH "/come"
#endif

#ifndef WSS_CLIENT_PROMPT
#define WSS_CLIENT_PROMPT "wss-cli> "
#endif

/* 断线重连配置 */
#ifndef RECONNECT_INTERVAL_MS
#define RECONNECT_INTERVAL_MS (1*500)
#endif
#ifndef RECONNECT_MAX_RETRIES
#define RECONNECT_MAX_RETRIES 0
#endif
#ifndef RECONNECT_BACKOFF_ENABLE
#define RECONNECT_BACKOFF_ENABLE 1
#endif
#if RECONNECT_BACKOFF_ENABLE
#ifndef RECONNECT_BACKOFF_MIN_RETRIES
#define RECONNECT_BACKOFF_MIN_RETRIES 2
#endif
#ifndef RECONNECT_BACKOFF_HIGH_THRESHOLD
#define RECONNECT_BACKOFF_HIGH_THRESHOLD 5
#endif
#endif

struct console_handle {
	pthread_t console_thread;
	int console_running;
	int *interrupted;
	struct ez_wss_client_handle *ws_handle;
};

static void *console_thread_func(void *arg) {
	struct console_handle *console = (struct console_handle *)arg;
	char line[4096];
	int interactive = isatty(STDIN_FILENO);

	console->console_running = 1;
	if (interactive) {
		printf("Console ready. Commands:\n");
		printf("  status  - Show WebSocket status\n");
		printf("  help    - Show help information\n");
		printf("  quit    - Exit program\n");
		printf("  other   - Send to server\n");
	}

	if (interactive) {
		fputs(WSS_CLIENT_PROMPT, stdout);
		fflush(stdout);
	}

	while (console->console_running) {
		if (console->interrupted && *console->interrupted) {
			break;
		}

		fd_set readfds;
		struct timeval timeout;
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;

		int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			break;
		}

		if (console->interrupted && *console->interrupted) {
			break;
		}

		if (ret == 0 || !FD_ISSET(STDIN_FILENO, &readfds)) {
			continue;
		}

		if (!fgets(line, sizeof(line), stdin)) {
			if (feof(stdin)) {
				break;
			}
			continue;
		}

		size_t len = strlen(line);
		while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
			line[--len] = '\0';
		}
		if (!len) {
			if (interactive) {
				fputs(WSS_CLIENT_PROMPT, stdout);
				fflush(stdout);
			}
			continue;
		}

		if (!strcmp(line, "quit") || !strcmp(line, "exit")) {
			if (console->interrupted)
				*console->interrupted = 1;
			break;
		}

		if (!strcmp(line, "status")) {
			if (console->ws_handle) {
				enum ez_ws_state state = ez_wss_get_state(console->ws_handle);

				printf("\nWebSocket Status:\n");
				printf("  Connection: %s\n",
				       state == EZ_WS_STATE_CONNECTED ? "Connected" :
				       state == EZ_WS_STATE_CONNECTING ? "Connecting" :
				       state == EZ_WS_STATE_HANDSHAKING ? "Handshaking" :
				       "Disconnected");

#if defined(EZ_WS_CLIENT_ENABLE_STATS) && (EZ_WS_CLIENT_ENABLE_STATS == 1)
				struct ez_ws_client_stats stats;
				if (ez_wss_get_stats(console->ws_handle, &stats) == 0) {
					printf("\nStatistics:\n");
					printf("  TX: TEXT=%llu (%llu bytes), BINARY=%llu (%llu bytes), PING=%llu, PONG=%llu, CLOSE=%llu\n",
					       (unsigned long long)stats.tx_text_count,
					       (unsigned long long)stats.tx_text_bytes,
					       (unsigned long long)stats.tx_binary_count,
					       (unsigned long long)stats.tx_binary_bytes,
					       (unsigned long long)stats.tx_ping_count,
					       (unsigned long long)stats.tx_pong_count,
					       (unsigned long long)stats.tx_close_count);
					printf("  RX: TEXT=%llu (%llu bytes), BINARY=%llu (%llu bytes), PING=%llu, PONG=%llu, CLOSE=%llu\n",
					       (unsigned long long)stats.rx_text_count,
					       (unsigned long long)stats.rx_text_bytes,
					       (unsigned long long)stats.rx_binary_count,
					       (unsigned long long)stats.rx_binary_bytes,
					       (unsigned long long)stats.rx_ping_count,
					       (unsigned long long)stats.rx_pong_count,
					       (unsigned long long)stats.rx_close_count);
				}
#endif /* EZ_WS_CLIENT_ENABLE_STATS */
			} else {
				printf("WebSocket not initialized\n");
			}
			if (interactive) {
				fputs(WSS_CLIENT_PROMPT, stdout);
				fflush(stdout);
			}
			continue;
		}

		if (!strcmp(line, "help")) {
			printf("\nAvailable commands:\n");
			printf("  status       - Show WebSocket connection status\n");
			printf("  help         - Show this help message\n");
			printf("  quit/exit    - Exit program\n");
			printf("  other input  - Send as message to server\n");
			printf("\n");
			if (interactive) {
				fputs(WSS_CLIENT_PROMPT, stdout);
				fflush(stdout);
			}
			continue;
		}

		if (console->ws_handle) {
			int ret = ez_wss_send_text(console->ws_handle, line, len);
			if (ret != EZ_WS_OK) {
				printf("[console] Send failed: error code: %d\n", ret);
			}
		} else {
			printf("[console] WebSocket not initialized\n");
		}

		if (interactive) {
			fputs(WSS_CLIENT_PROMPT, stdout);
			fflush(stdout);
		}
	}

	console->console_running = 0;
	return NULL;
}

struct console_handle *console_init(struct ez_wss_client_handle *ws_handle, int *interrupted_flag) {
	struct console_handle *console = calloc(1, sizeof(struct console_handle));
	if (!console)
		return NULL;

	console->interrupted = interrupted_flag;
	console->ws_handle = ws_handle;
	return console;
}

int console_start(struct console_handle *console) {
	if (!console)
		return -1;

	console->console_running = 1;
	if (pthread_create(&console->console_thread, NULL, console_thread_func, console) != 0) {
		console->console_running = 0;
		return -1;
	}

	return 0;
}

void console_stop(struct console_handle *console) {
	if (!console)
		return;

	console->console_running = 0;
	if (console->interrupted) {
		*console->interrupted = 1;
	}
	if (console->console_thread) {
		for (int i = 0; i < 3 && console->console_running; i++) {
			ez_usleep(100000);
		}
		if (console->console_thread) {
			pthread_join(console->console_thread, NULL);
		}
	}
}

void console_cleanup(struct console_handle *console) {
	if (!console)
		return;
	free(console);
}

static void example_on_receive(const void *data, size_t len, int is_binary, void *user_data) {
	EZ_PRINT_LOG_INFO("\n[Callback] Received %s data: %zu bytes\n",
	       is_binary ? "binary" : "text", len);

	if (!is_binary && len < 1024) {
		char buf[1024];
		memcpy(buf, data, len);
		buf[len] = '\0';
		EZ_PRINT_LOG_INFO("[Callback] Content: %s\n", buf);
	}
}

static void example_on_connected(void *user_data) {
	EZ_PRINT_LOG_INFO("\n[Callback] WebSocket connected\n");
}

static void example_on_disconnected(void *user_data) {
	EZ_PRINT_LOG_INFO("\n[Callback] WebSocket disconnected\n");
}

static void example_on_sent(const void *data, size_t len, void *user_data) {
	EZ_PRINT_LOG_INFO("[Callback] Data sent: %zu bytes\n", len);
}

static int g_interrupted = 0;
void sigint_handler(int sig) {
	g_interrupted = 1;
}

static void *ws_service_thread_func(void *arg) {
	struct ez_wss_client_handle *ws = (struct ez_wss_client_handle *)arg;

	while (!g_interrupted) {
		int ret = ez_wss_service_exec(ws, 0);
		if (ret < 0) {
			break;
		}
	}

	return NULL;
}

static void usage(const char *prog)
{
	printf("Usage: %s [options]\n", prog);
	printf("  -h, --host HOST        Server hostname or IP (default: localhost)\n");
	printf("  -p, --port PORT        Server port (default: 54321)\n");
	printf("  -u, --url PATH         WebSocket URL path (default: /come)\n");
	printf("      --protocol PROTO   WebSocket subprotocol (default: come.0)\n");
	printf("      --no-tls           Plaintext ws (tls_enable=0)\n");
	printf("      --no-verify        tls_verify_peer=0: encrypt only, no cert verify (fallback)\n");
	printf("  -a, --ca FILE          CA file to verify server cert (default: built-in CA)\n");
}

int main(int argc, const char **argv) {
	struct ez_wss_client_handle *client_handle = NULL;
	struct console_handle *console = NULL;
	pthread_t ws_service_thread = 0;
	struct ez_ws_callbacks callbacks;
	struct ez_wss_client_config config = {0};
	int no_tls = 0;
	int no_verify = 0;
	const char *ca_path = NULL;

	config.server_addr = "localhost";
	config.port = 54321;
	config.url_path = WSS_CLIENT_PATH;
	config.protocol = WSS_CLIENT_PROTOCOL;
	config.connect_timeout_ms = 1*1000;
	config.reconnect_interval_ms = RECONNECT_INTERVAL_MS;
	config.reconnect_max_retries = RECONNECT_MAX_RETRIES;
	config.reconnect_backoff_enable = RECONNECT_BACKOFF_ENABLE;
	config.reconnect_backoff_min_retries = RECONNECT_BACKOFF_MIN_RETRIES;
	config.reconnect_backoff_high_threshold = RECONNECT_BACKOFF_HIGH_THRESHOLD;

	int opt;
	int option_index = 0;
	const char *optstring = "h:p:u:a:";
	struct option long_options[] = {
		{"host",     required_argument, 0, 'h'},
		{"port",     required_argument, 0, 'p'},
		{"url",      required_argument, 0, 'u'},
		{"ca",       required_argument, 0, 'a'},
		{"protocol", required_argument, 0, 0},
		{"no-tls",   no_argument, 0, 0},
		{"no-verify", no_argument, 0, 0},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, (char *const *)argv, optstring, long_options, &option_index)) != -1) {
		switch (opt) {
		case 'h':
			config.server_addr = optarg;
			break;
		case 'p':
			config.port = (unsigned short)atoi(optarg);
			break;
		case 'u':
			config.url_path = optarg;
			break;
		case 'a':
			ca_path = optarg;
			break;
		case 0:
			if (strcmp(long_options[option_index].name, "protocol") == 0) {
				config.protocol = optarg;
			} else if (strcmp(long_options[option_index].name, "no-tls") == 0) {
				no_tls = 1;
			} else if (strcmp(long_options[option_index].name, "no-verify") == 0) {
				no_verify = 1;
			}
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	/* TLS：默认开箱即加密 + trust 内置 CA；--no-tls → 明文；--no-verify → 仅加密 */
	config.tls_enable = no_tls ? 0 : 1;
	config.tls_verify_peer = (no_tls || no_verify) ? 0 : 1;
	config.tls_ca_path = ca_path;

	printf("Native WSS Client (WebSocket over TLS, using epoll/timerfd/socket)\n");
	printf("Connecting to %s:%hu%s (%s%s)\n", config.server_addr, config.port, config.url_path,
	       no_tls ? "ws" : "wss",
	       (!no_tls && no_verify) ? ", no-verify" : "");

	signal(SIGINT, sigint_handler);

	callbacks.on_receive = example_on_receive;
	callbacks.on_connected = example_on_connected;
	callbacks.on_disconnected = example_on_disconnected;
	callbacks.on_sent = example_on_sent;
	callbacks.user_data = NULL;

	client_handle = ez_wss_client_handle_create(&config, &callbacks);
	if (!client_handle) {
		printf("WebSocket client init failed\n");
		return 1;
	}

	console = console_init(client_handle, &g_interrupted);
	if (!console) {
		printf("Console init failed\n");
		ez_wss_client_cleanup(client_handle);
		return 1;
	}

	if (pthread_create(&ws_service_thread, NULL, ws_service_thread_func, client_handle) != 0) {
		printf("Failed to create WebSocket service thread\n");
		console_cleanup(console);
		ez_wss_client_cleanup(client_handle);
		return 1;
	}

	if (console_start(console) != 0) {
		printf("Failed to start console thread\n");
		if (ws_service_thread) {
			pthread_join(ws_service_thread, NULL);
		}
		console_cleanup(console);
		ez_wss_client_cleanup(client_handle);
		return 1;
	}

	printf("All threads started, entering main loop...\n");

	while (!g_interrupted) {
		sleep(1);
	}

	printf("Shutting down...\n");

	console_stop(console);

	if (ws_service_thread) {
		pthread_join(ws_service_thread, NULL);
	}

	console_cleanup(console);
	ez_wss_client_cleanup(client_handle);

	printf("Completed\n");
	return 0;
}
