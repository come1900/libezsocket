/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * touch-cli.c - WebSocket Client Example Program
 *
 * Copyright (C) 2011 ezlibs.com, All Rights Reserved.
 *
 * $Id: touch-cli.c 1 2011-12-27 20:00:00Z WHF $
 *
 * Explain:
 *     WebSocket client example program using ez_wsclient-native component.
 *
 * Update:
 *     2011-12-27 20:00:00 WHF Create
 *     2026-01-04 20:44:49 WHF Improve ping pong mechanism
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
#include <ezutil/ez_def_devel_debug.h>
#include "ez_wsclient-native.h"

/* WebSocket协议和路径配置 */
#ifndef EZ_WS_CLIENT_PROTOCOL
#define EZ_WS_CLIENT_PROTOCOL "come.0"
#endif

#ifndef EZ_WS_CLIENT_PATH
#define EZ_WS_CLIENT_PATH "/come"
#endif

/* 控制台提示符 */
#ifndef EZ_WS_CLIENT_PROMPT
#define EZ_WS_CLIENT_PROMPT "ws> "
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

/* 客户端信息结构 */
struct client_info {
	struct ez_ws_client_handle *ws_handle;
	pthread_t ws_service_thread;
	int client_id;  /* 客户端ID，从0开始 */
};

/* 单线程模式上下文结构 */
struct single_thread_context {
	struct client_info *clients;
	int client_count;
};

/* Console句柄 */
struct console_handle {
	pthread_t console_thread;
	int console_running;
	int *interrupted;
	
	/* Console到websocket的发送队列 */
	char **pending_lines;
	size_t pending_cap;
	size_t pending_count;
	pthread_mutex_t pending_lock;
	
	/* 多客户端支持 */
	struct client_info *clients;
	int client_count;  /* 客户端数量 */
	pthread_mutex_t clients_lock;  /* 保护客户端数组的锁 */
};


/* Console线程函数 */
static void *console_thread_func(void *arg) {
	struct console_handle *console = (struct console_handle *)arg;
	char line[4096];
	int interactive = isatty(STDIN_FILENO);
	
	console->console_running = 1;
	if (interactive) {
		printf("Console ready. Commands:\n");
		printf("  clients       - Show all client connections list\n");
		printf("  send <id> <msg> - Send message via specific client\n");
		printf("  status        - Show WebSocket status\n");
		printf("  help          - Show help information\n");
		printf("  quit          - Exit program\n");
		printf("  other         - Broadcast to all clients\n");
	}
	
	/* 只在交互模式下才打印初始提示符 */
	if (interactive) {
		fputs(EZ_WS_CLIENT_PROMPT, stdout);
		fflush(stdout);
	}
	
	while (console->console_running) {
		/* 检查中断标志 */
		if (console->interrupted && *console->interrupted) {
			break;
		}
		
		/* 使用 select 来检查 stdin 是否有数据，同时可以定期检查中断标志 */
		fd_set readfds;
		struct timeval timeout;
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000; /* 100ms */
		
		int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			break;
		}
		
		/* 再次检查中断标志 */
		if (console->interrupted && *console->interrupted) {
			break;
		}
		
		/* 如果没有数据可读，继续循环（不打印提示符） */
		if (ret == 0 || !FD_ISSET(STDIN_FILENO, &readfds)) {
			continue;
		}
		
		/* 有数据可读，使用 fgets 读取 */
		if (!fgets(line, sizeof(line), stdin)) {
			/* EOF 或错误 */
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
			/* 空行，重新打印提示符 */
			if (interactive) {
				fputs(EZ_WS_CLIENT_PROMPT, stdout);
				fflush(stdout);
			}
			continue;
		}
		
		/* 处理quit/exit命令 */
		if (!strcmp(line, "quit") || !strcmp(line, "exit")) {
			if (console->interrupted)
				*console->interrupted = 1;
			break;
		}
		
		/* 处理clients命令 - 显示所有客户端连接列表 */
		if (!strcmp(line, "clients")) {
			if (console->clients && console->client_count > 0) {
				printf("\n=== Client Connections (%d) ===\n", console->client_count);
				pthread_mutex_lock(&console->clients_lock);
				for (int i = 0; i < console->client_count; i++) {
					enum ez_ws_state state = ez_ws_get_state(console->clients[i].ws_handle);
					
					printf("  [%d] Client #%d: %s", i + 1, i,
					       state == EZ_WS_STATE_CONNECTED ? "Connected" :
					       state == EZ_WS_STATE_CONNECTING ? "Connecting" :
					       state == EZ_WS_STATE_HANDSHAKING ? "Handshaking" :
					       "Disconnected");
					
#if defined(EZ_WS_CLIENT_ENABLE_STATS) && (EZ_WS_CLIENT_ENABLE_STATS == 1)
					/* 打印统计信息 */
					struct ez_ws_client_stats stats;
					if (ez_ws_get_stats(console->clients[i].ws_handle, &stats) == 0) {
						printf("\n      TX: TEXT=%llu (%llu bytes), BINARY=%llu (%llu bytes), PING=%llu, PONG=%llu, CLOSE=%llu",
						       (unsigned long long)stats.tx_text_count,
						       (unsigned long long)stats.tx_text_bytes,
						       (unsigned long long)stats.tx_binary_count,
						       (unsigned long long)stats.tx_binary_bytes,
						       (unsigned long long)stats.tx_ping_count,
						       (unsigned long long)stats.tx_pong_count,
						       (unsigned long long)stats.tx_close_count);
						printf("\n      RX: TEXT=%llu (%llu bytes), BINARY=%llu (%llu bytes), PING=%llu, PONG=%llu, CLOSE=%llu",
						       (unsigned long long)stats.rx_text_count,
						       (unsigned long long)stats.rx_text_bytes,
						       (unsigned long long)stats.rx_binary_count,
						       (unsigned long long)stats.rx_binary_bytes,
						       (unsigned long long)stats.rx_ping_count,
						       (unsigned long long)stats.rx_pong_count,
						       (unsigned long long)stats.rx_close_count);
					}
#endif /* EZ_WS_CLIENT_ENABLE_STATS */
					printf("\n");
				}
				pthread_mutex_unlock(&console->clients_lock);
				printf("\n");
			} else {
				printf("No clients initialized\n");
			}
			/* 处理完命令后，打印下一个提示符 */
			if (interactive) {
				fputs(EZ_WS_CLIENT_PROMPT, stdout);
				fflush(stdout);
			}
			continue;
		}
		
		/* 处理status命令 */
		if (!strcmp(line, "status")) {
			if (console->clients && console->client_count > 0) {
				printf("\nWebSocket Status (%d clients):\n", console->client_count);
				pthread_mutex_lock(&console->clients_lock);
				int connected_count = 0;
				int connecting_count = 0;
				int disconnected_count = 0;
				for (int i = 0; i < console->client_count; i++) {
					enum ez_ws_state state = ez_ws_get_state(console->clients[i].ws_handle);
					if (state == EZ_WS_STATE_CONNECTED) {
						connected_count++;
					} else if (state == EZ_WS_STATE_CONNECTING || state == EZ_WS_STATE_HANDSHAKING) {
						connecting_count++;
					} else {
						disconnected_count++;
					}
				}
				pthread_mutex_unlock(&console->clients_lock);
				printf("  Total clients: %d\n", console->client_count);
				printf("  Connected: %d\n", connected_count);
				printf("  Connecting/Handshaking: %d\n", connecting_count);
				printf("  Disconnected: %d\n", disconnected_count);
			} else {
				printf("WebSocket not initialized\n");
			}
			/* 处理完命令后，打印下一个提示符 */
			if (interactive) {
				fputs(EZ_WS_CLIENT_PROMPT, stdout);
				fflush(stdout);
			}
			continue;
		}
		
		/* 处理help命令 */
		if (!strcmp(line, "help")) {
			printf("\nAvailable commands:\n");
			printf("  clients          - Show all client connections list\n");
			printf("  send <id> <msg>  - Send message via specific client by ID\n");
			printf("  broadcast <msg>   - Broadcast message to all clients\n");
			printf("  status           - Show WebSocket connection status\n");
			printf("  help             - Show this help message\n");
			printf("  quit/exit        - Exit program\n");
			printf("  other input      - Broadcast message to all clients (default)\n");
			printf("\nExample:\n");
			printf("  send 0 Hello     - Send 'Hello' via client #0\n");
			printf("  broadcast Test   - Broadcast 'Test' to all clients\n");
			printf("\n");
			/* 处理完命令后，打印下一个提示符 */
			if (interactive) {
				fputs(EZ_WS_CLIENT_PROMPT, stdout);
				fflush(stdout);
			}
			continue;
		}
		
		/* 处理send命令 - 向指定客户端发送消息 */
		if (strncmp(line, "send ", 5) == 0) {
			if (!console->clients || console->client_count == 0) {
				printf("WebSocket not initialized\n");
				if (interactive) {
					fputs(EZ_WS_CLIENT_PROMPT, stdout);
					fflush(stdout);
				}
				continue;
			}
			
			/* 解析命令：send <client_id> <message> */
			char *cmd_ptr = line + 5;  /* 跳过 "send " */
			while (*cmd_ptr == ' ') cmd_ptr++;  /* 跳过空格 */
			
			if (!*cmd_ptr) {
				printf("Usage: send <client_id> <message>\n");
				if (interactive) {
					fputs(EZ_WS_CLIENT_PROMPT, stdout);
					fflush(stdout);
				}
				continue;
			}
			
			/* 解析客户端ID */
			char *endptr;
			long client_id = strtol(cmd_ptr, &endptr, 10);
			
			if (endptr == cmd_ptr || client_id < 0 || client_id >= console->client_count) {
				printf("Invalid client ID (valid range: 0-%d)\n", console->client_count - 1);
				if (interactive) {
					fputs(EZ_WS_CLIENT_PROMPT, stdout);
					fflush(stdout);
				}
				continue;
			}
			
			/* 跳过ID后的空格，找到消息内容 */
			cmd_ptr = endptr;
			while (*cmd_ptr == ' ') cmd_ptr++;
			
			if (!*cmd_ptr) {
				printf("Message cannot be empty\n");
				if (interactive) {
					fputs(EZ_WS_CLIENT_PROMPT, stdout);
					fflush(stdout);
				}
				continue;
			}
			
			/* 发送消息到指定客户端 */
			pthread_mutex_lock(&console->clients_lock);
			int ret = ez_ws_send_text(console->clients[client_id].ws_handle, cmd_ptr, strlen(cmd_ptr));
			pthread_mutex_unlock(&console->clients_lock);
			
			if (ret == EZ_WS_OK) {
				printf("Message sent via client #%d\n", (int)client_id);
			} else {
				printf("Send failed via client #%d, error code: %d\n", (int)client_id, ret);
			}
			
			if (interactive) {
				fputs(EZ_WS_CLIENT_PROMPT, stdout);
				fflush(stdout);
			}
			continue;
		}
		
		/* 处理broadcast命令 - 显式广播消息 */
		if (strncmp(line, "broadcast ", 10) == 0) {
			if (!console->clients || console->client_count == 0) {
				printf("WebSocket not initialized\n");
				if (interactive) {
					fputs(EZ_WS_CLIENT_PROMPT, stdout);
					fflush(stdout);
				}
				continue;
			}
			
			char *msg = line + 10;  /* 跳过 "broadcast " */
			while (*msg == ' ') msg++;  /* 跳过空格 */
			
			if (!*msg) {
				printf("Message cannot be empty\n");
				if (interactive) {
					fputs(EZ_WS_CLIENT_PROMPT, stdout);
					fflush(stdout);
				}
				continue;
			}
			
			size_t msg_len = strlen(msg);
			int success_count = 0;
			int fail_count = 0;
			pthread_mutex_lock(&console->clients_lock);
			for (int i = 0; i < console->client_count; i++) {
				int ret = ez_ws_send_text(console->clients[i].ws_handle, msg, msg_len);
				if (ret == EZ_WS_OK) {
					success_count++;
				} else {
					fail_count++;
				}
			}
			pthread_mutex_unlock(&console->clients_lock);
			
			if (fail_count > 0) {
				printf("Broadcast: %d succeeded, %d failed\n", success_count, fail_count);
			} else {
				printf("Broadcast to %d client(s) succeeded\n", success_count);
			}
			
			if (interactive) {
				fputs(EZ_WS_CLIENT_PROMPT, stdout);
				fflush(stdout);
			}
			continue;
		}
		
		/* 发送消息（默认广播到所有客户端） */
		if (console->clients && console->client_count > 0) {
			int success_count = 0;
			int fail_count = 0;
			pthread_mutex_lock(&console->clients_lock);
			for (int i = 0; i < console->client_count; i++) {
				int ret = ez_ws_send_text(console->clients[i].ws_handle, line, len);
				if (ret == EZ_WS_OK) {
					success_count++;
				} else {
					fail_count++;
				}
			}
			pthread_mutex_unlock(&console->clients_lock);
			if (fail_count > 0) {
				printf("[console] Broadcast: %d succeeded, %d failed\n", success_count, fail_count);
			}
		} else {
			printf("[console] WebSocket not initialized\n");
		}
		
		/* 处理完命令后，打印下一个提示符 */
		if (interactive) {
			fputs(EZ_WS_CLIENT_PROMPT, stdout);
			fflush(stdout);
		}
	}
	
	console->console_running = 0;
	return NULL;
}

/* 初始化Console（单客户端版本，保留兼容性） */
struct console_handle *console_init(struct ez_ws_client_handle *ws_handle, int *interrupted_flag) {
	struct console_handle *console = calloc(1, sizeof(struct console_handle));
	if (!console)
		return NULL;
	
	console->interrupted = interrupted_flag;
	console->client_count = 1;
	console->clients = NULL;
	console->pending_cap = 64;
	console->pending_lines = calloc(console->pending_cap, sizeof(char *));
	if (!console->pending_lines) {
		free(console);
		return NULL;
	}
	
	pthread_mutex_init(&console->pending_lock, NULL);
	pthread_mutex_init(&console->clients_lock, NULL);
	return console;
}

/* 初始化Console（多客户端版本） */
struct console_handle *console_init_multi(struct client_info *clients, int client_count, int *interrupted_flag) {
	struct console_handle *console = calloc(1, sizeof(struct console_handle));
	if (!console)
		return NULL;
	
	console->interrupted = interrupted_flag;
	console->clients = clients;
	console->client_count = client_count;
	console->pending_cap = 64;
	console->pending_lines = calloc(console->pending_cap, sizeof(char *));
	if (!console->pending_lines) {
		free(console);
		return NULL;
	}
	
	pthread_mutex_init(&console->pending_lock, NULL);
	pthread_mutex_init(&console->clients_lock, NULL);
	return console;
}

/* 启动Console线程 */
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

/* 停止Console线程 */
void console_stop(struct console_handle *console) {
	if (!console)
		return;
	
	/* 设置停止标志 */
	console->console_running = 0;
	
	/* 如果设置了中断标志，也设置它 */
	if (console->interrupted) {
		*console->interrupted = 1;
	}
	
	/* 等待线程退出（最多等待 200ms，因为 select 超时是 100ms） */
	if (console->console_thread) {
		/* 使用 pthread_timedjoin_np 或简单的轮询等待 */
		for (int i = 0; i < 3 && console->console_running; i++) {
			ez_usleep(100000); /* 100ms */
		}
		
		/* 如果线程还在运行，强制 join（此时应该已经退出了） */
		if (console->console_thread) {
			pthread_join(console->console_thread, NULL);
		}
	}
}

/* 清理Console */
void console_cleanup(struct console_handle *console) {
	if (!console)
		return;
	
	pthread_mutex_destroy(&console->pending_lock);
	pthread_mutex_destroy(&console->clients_lock);
	
	if (console->pending_lines) {
		for (size_t i = 0; i < console->pending_cap; ++i) {
			free(console->pending_lines[i]);
		}
		free(console->pending_lines);
	}
	
	free(console);
}

/* 示例回调函数 - 支持多客户端 */
static void example_on_receive(const void *data, size_t len, int is_binary, void *user_data) {
	int client_id = user_data ? *(int *)user_data : -1;
	EZ_PRINT_LOG_INFO("\n[Callback] Client #%d received %s data: %zu bytes\n", 
	       client_id, is_binary ? "binary" : "text", len);
	
	if (!is_binary && len < 1024) {
		char buf[1024];
		memcpy(buf, data, len);
		buf[len] = '\0';
		EZ_PRINT_LOG_INFO("[Callback] Client #%d content: %s\n", client_id, buf);
	}
}

static void example_on_connected(void *user_data) {
	int client_id = user_data ? *(int *)user_data : -1;
	EZ_PRINT_LOG_INFO("\n[Callback] Client #%d WebSocket connected\n", client_id);
}

static void example_on_disconnected(void *user_data) {
	int client_id = user_data ? *(int *)user_data : -1;
	EZ_PRINT_LOG_INFO("\n[Callback] Client #%d WebSocket disconnected\n", client_id);
}

static void example_on_sent(const void *data, size_t len, void *user_data) {
	int client_id = user_data ? *(int *)user_data : -1;
	EZ_PRINT_LOG_INFO("[Callback] Client #%d data sent: %zu bytes\n", client_id, len);
}

/* 信号处理 */
static int g_interrupted = 0;
void sigint_handler(int sig) {
	g_interrupted = 1;
}

/* WebSocket服务线程函数 - 多线程模式：每个客户端一个独立线程 */
static void *ws_service_thread_func(void *arg) {
	struct client_info *client = (struct client_info *)arg;
	int n = 0;
	
	EZ_PRINT_LOG_INFO("WebSocket service thread for client #%d started\n", client->client_id);
	
	while (n >= 0 && !g_interrupted) {
		/* 使用事件驱动：epoll_wait 阻塞等待事件，超时时间 100ms */
		/* 这样在没有事件时会阻塞，不会占用 CPU */
		n = ez_ws_service_exec(client->ws_handle, 100);
	}
	
	EZ_PRINT_LOG_INFO("WebSocket service thread for client #%d exiting\n", client->client_id);
	return NULL;
}

/* WebSocket服务线程函数 - 单线程模式：一个线程处理所有客户端 */
static void *ws_service_single_thread_func(void *arg) {
	/* 通过包装结构传递客户端数组和数量 */
	struct single_thread_context *ctx = (struct single_thread_context *)arg;
	
	EZ_PRINT_LOG_INFO("WebSocket single service thread started (handling %d clients)\n", ctx->client_count);
	
	while (!g_interrupted) {
		int processed_count = 0;
		
		/* 轮询所有客户端，使用短超时（10ms）以平衡响应性和CPU占用 */
		for (int i = 0; i < ctx->client_count; i++) {
			if (ctx->clients[i].ws_handle) {
				int ret = ez_ws_service_exec(ctx->clients[i].ws_handle, 10);
				if (ret > 0) {
					/* 有事件处理 */
					processed_count++;
				} else if (ret < 0) {
					/* 某个客户端出错，但不影响其他客户端 */
					/* 继续处理其他客户端 */
				}
			}
		}
		
		/* 如果没有任何客户端有活动，短暂休眠避免忙等待 */
		if (processed_count == 0) {
			ez_usleep(5000); /* 5ms */
		}
	}
	
	EZ_PRINT_LOG_INFO("WebSocket single service thread exiting\n");
	
	/* 释放上下文结构 */
	free(ctx);
	return NULL;
}

/* 主函数 */
int main(int argc, const char **argv) {
	struct console_handle *console = NULL;
	struct ez_ws_callbacks callbacks;
	struct ez_ws_client_config config = {0};

	/* 设置默认配置 */
	config.server_addr = "localhost";
	config.port = 54321;
	config.url_path = EZ_WS_CLIENT_PATH;
	config.protocol = EZ_WS_CLIENT_PROTOCOL;
	config.connect_timeout_ms = 1*1000;
	config.reconnect_interval_ms = RECONNECT_INTERVAL_MS;
	config.reconnect_max_retries = RECONNECT_MAX_RETRIES;
	config.reconnect_backoff_enable = RECONNECT_BACKOFF_ENABLE;
	config.reconnect_backoff_min_retries = RECONNECT_BACKOFF_MIN_RETRIES;
	config.reconnect_backoff_high_threshold = RECONNECT_BACKOFF_HIGH_THRESHOLD;
	
	/* 解析命令行参数 */
	int opt;
	int option_index = 0;
	int num_clients = 1;  /* 默认1个客户端 */
	int multi_thread_mode = 0;  /* 默认单线程模式 */
	const char *optstring = "h:p:u:s:n:m";
	struct option long_options[] = {
		{"host",     required_argument, 0, 'h'},
		{"port",     required_argument, 0, 'p'},
		{"url",      required_argument, 0, 'u'},
		{"server",   required_argument, 0, 's'},
		{"num-clients", required_argument, 0, 'n'},
		{"multi-thread", no_argument, 0, 'm'},
		{"protocol", required_argument, 0, 0},
		{0, 0, 0, 0}
	};
	
	while ((opt = getopt_long(argc, (char *const *)argv, optstring, long_options, &option_index)) != -1) {
		switch (opt) {
		case 'h':
		case 's':
			config.server_addr = optarg;
			break;
		case 'p':
			config.port = (unsigned short)atoi(optarg);
			break;
		case 'u':
			config.url_path = optarg;
			break;
		case 'n':
			num_clients = atoi(optarg);
			if (num_clients < 1) {
				printf("Error: Number of clients must be >= 1\n");
				return 1;
			}
			break;
		case 'm':
			multi_thread_mode = 1;
			break;
		case 0:
			/* 处理长选项 --protocol */
			if (strcmp(long_options[option_index].name, "protocol") == 0) {
				config.protocol = optarg;
			}
			break;
		default:
			printf("Usage: %s [-h|--host HOST] [-p|--port PORT] [-u|--url PATH] [-n|--num-clients N] [-m|--multi-thread] [--protocol PROTOCOL]\n", argv[0]);
			printf("  -h, --host HOST         Server hostname or IP address (default: localhost)\n");
			printf("  -s, --server HOST       Alias for --host\n");
			printf("  -p, --port PORT        Server port (default: 54321)\n");
			printf("  -u, --url PATH         WebSocket URL path (default: /come)\n");
			printf("  -n, --num-clients N    Number of client connections (default: 1)\n");
			printf("  -m, --multi-thread     Use multi-thread mode (default: single-thread)\n");
			printf("      --protocol PROTOCOL WebSocket subprotocol (default: come.0)\n");
			return 1;
		}
	}
	
	printf("Native WebSocket Multi-Client (using epoll/timerfd/socket)\n");
	printf("Mode: %s\n", multi_thread_mode ? "Multi-thread (one thread per client)" : "Single-thread (one thread for all clients)");
	printf("Connecting %d client(s) to %s:%hu%s\n", num_clients, config.server_addr, config.port, config.url_path);
	
	/* 注册信号处理 */
	signal(SIGINT, sigint_handler);
	
	/* 分配客户端数组 */
	struct client_info *clients = calloc(num_clients, sizeof(struct client_info));
	if (!clients) {
		printf("Failed to allocate memory for clients\n");
		return 1;
	}
	
	/* 初始化并启动多个WebSocket客户端 */
	for (int i = 0; i < num_clients; i++) {
		clients[i].client_id = i;
		
		/* 设置回调，传递客户端ID */
		int *client_id_ptr = &clients[i].client_id;
		callbacks.on_receive = example_on_receive;
		callbacks.on_connected = example_on_connected;
		callbacks.on_disconnected = example_on_disconnected;
		callbacks.on_sent = example_on_sent;
		callbacks.user_data = client_id_ptr;
		
		/* 创建客户端句柄 */
		clients[i].ws_handle = ez_ws_client_handle_create(&config, &callbacks);
		if (!clients[i].ws_handle) {
			printf("WebSocket client #%d init failed\n", i);
			/* 清理已创建的客户端 */
			for (int j = 0; j < i; j++) {
				ez_ws_client_cleanup(clients[j].ws_handle);
			}
			free(clients);
			return 1;
		}
		
		/* 多线程模式：为每个客户端创建独立的服务线程 */
		if (multi_thread_mode) {
			if (pthread_create(&clients[i].ws_service_thread, NULL, ws_service_thread_func, &clients[i]) != 0) {
				printf("Failed to create WebSocket service thread for client #%d\n", i);
				/* 清理已创建的客户端 */
				for (int j = 0; j < i; j++) {
					g_interrupted = 1;
					pthread_join(clients[j].ws_service_thread, NULL);
					ez_ws_client_cleanup(clients[j].ws_handle);
				}
				ez_ws_client_cleanup(clients[i].ws_handle);
				free(clients);
				return 1;
			}
		}
	}
	
	/* 单线程模式：创建一个线程处理所有客户端 */
	pthread_t single_service_thread = 0;
	if (!multi_thread_mode) {
		/* 创建包装结构传递客户端数组和数量 */
		struct single_thread_context *ctx = malloc(sizeof(struct single_thread_context));
		if (!ctx) {
			printf("Failed to allocate memory for single-thread context\n");
			/* 清理所有客户端 */
			for (int i = 0; i < num_clients; i++) {
				ez_ws_client_cleanup(clients[i].ws_handle);
			}
			free(clients);
			return 1;
		}
		ctx->clients = clients;
		ctx->client_count = num_clients;
		
		if (pthread_create(&single_service_thread, NULL, ws_service_single_thread_func, ctx) != 0) {
			printf("Failed to create single WebSocket service thread\n");
			/* 清理所有客户端 */
			for (int i = 0; i < num_clients; i++) {
				ez_ws_client_cleanup(clients[i].ws_handle);
			}
			free(ctx);
			free(clients);
			return 1;
		}
	}
	
	/* 初始化Console（传递客户端数组） */
	console = console_init_multi(clients, num_clients, &g_interrupted);
	if (!console) {
		printf("Console init failed\n");
		/* 清理所有客户端 */
		g_interrupted = 1;
		if (multi_thread_mode) {
			for (int i = 0; i < num_clients; i++) {
				pthread_join(clients[i].ws_service_thread, NULL);
				ez_ws_client_cleanup(clients[i].ws_handle);
			}
		} else {
			if (single_service_thread) {
				pthread_join(single_service_thread, NULL);
			}
			for (int i = 0; i < num_clients; i++) {
				ez_ws_client_cleanup(clients[i].ws_handle);
			}
		}
		free(clients);
		return 1;
	}
	
	/* 启动Console线程 */
	if (console_start(console) != 0) {
		printf("Failed to start console thread\n");
		/* 清理所有客户端 */
		g_interrupted = 1;
		if (multi_thread_mode) {
			for (int i = 0; i < num_clients; i++) {
				pthread_join(clients[i].ws_service_thread, NULL);
				ez_ws_client_cleanup(clients[i].ws_handle);
			}
		} else {
			if (single_service_thread) {
				pthread_join(single_service_thread, NULL);
			}
			for (int i = 0; i < num_clients; i++) {
				ez_ws_client_cleanup(clients[i].ws_handle);
			}
		}
		console_cleanup(console);
		free(clients);
		return 1;
	}
	
	printf("All threads started, entering main loop...\n");
	
	/* 主循环 */
	while (!g_interrupted) {
		sleep(1);
	}
	
	/* 清理资源 */
	printf("Shutting down...\n");
	
	console_stop(console);
	
	/* 等待所有WebSocket服务线程退出 */
	if (multi_thread_mode) {
		/* 多线程模式：等待每个客户端的线程 */
		for (int i = 0; i < num_clients; i++) {
			pthread_join(clients[i].ws_service_thread, NULL);
			ez_ws_client_cleanup(clients[i].ws_handle);
		}
	} else {
		/* 单线程模式：等待单个服务线程 */
		if (single_service_thread) {
			pthread_join(single_service_thread, NULL);
		}
		/* 清理所有客户端 */
		for (int i = 0; i < num_clients; i++) {
			ez_ws_client_cleanup(clients[i].ws_handle);
		}
	}
	
	console_cleanup(console);
	free(clients);
	
	printf("Completed\n");
	return 0;
}
