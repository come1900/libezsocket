# WSS 组件特点、功能与性能测试

## 概述

WSS（WebSocket over TLS）是对库中原有明文 WebSocket 组件的加密升级。它与明文版
`ez_wsclient-native` / `ez_wsserver-native` 平行，原明文实现保持不变；新增
`ez_wss-client-native` / `ez_wss-server-native` 两个组件，基于源码编译的静态
OpenSSL 3.4.1 实现 TLS 加密链路。

本文档记录 WSS 组件的设计特点、对外功能，以及配套性能测试程序与实测结果。

## 特点

### 开箱即加密

每个连接默认启用 TLS（`tls_enable` 默认 1）。未配置证书时，服务端与客户端走内置 CA
互认：库内固定内嵌一把 frp 风格的 EC P-256 根 CA（编译期 PEM 常量，`pthread_once`
加载），服务端运行时为每个连接签发叶子证书，客户端 trust 同一把内置 CA，实现零配置
的双向/单向认证。

### 与明文版完全平行

API 签名、回调、状态机、错误码、统计结构均复用明文版类型，迁移成本低。是否编译 TLS
由 `ez_websocket.h` 中的 `EZ_WS_ENABLE_OPENSSLTLS` 宏控制（默认开启；注释即关，编译
为空且不依赖 OpenSSL）。

### 静态链接、无外部依赖

通过 `WSS_TLS_LIBS` 静态链接源码编译的 libssl.a / libcrypto.a / libz.a。产物不依赖
系统 libssl/libcrypto（`ldd` 无相关动态依赖），适合嵌入部署。

### 事件驱动 + 电平触发握手

epoll 驱动非阻塞 TLS 握手。握手阶段使用电平触发 + 显式 EPOLLIN/EPOLLOUT 掩码
（WANT_READ → EPOLLIN，WANT_WRITE → EPOLLIN|EPOLLOUT），握手完成后切回 EPOLLET，
规避了 EPOLLET“可写但无跃迁导致 EPOLLOUT 不再触发”的握手死锁。

## 功能

### 服务端 API（ez_wss_server_*）

- `ez_wss_server_handle_create` / `ez_wss_server_cleanup`
- `ez_wss_server_service_exec`：事件循环迭代，在外部线程循环调用
- `ez_wss_server_send_text` / `ez_wss_server_send_binary`：单播或广播
- `ez_wss_server_get_client_count` / `ez_wss_server_foreach_client`：连接管理
- `ez_wss_server_close_client`：主动断开
- `ez_wss_server_get_client_stats`：每连接收发统计
- 回调：`on_receive` / `on_connected` / `on_disconnected`
- 配置：监听地址端口、子协议、路径前缀、保活参数、TLS 开关与证书

服务端发送为全异步：`send_to_client_internal` 将数据封装为帧后入队（`send_queue_lock`
保护），由 EPOLLOUT 驱动发送，跨线程调用安全；队列无上限（仅受内存约束）。

### 客户端 API（ez_wss_client_*）

- `ez_wss_client_handle_create` / `ez_wss_client_cleanup`
- `ez_wss_service_exec`：事件循环迭代
- `ez_wss_send_text` / `ez_wss_send_binary`
- `ez_wss_is_connected` / `ez_wss_get_state` / `ez_wss_get_stats`
- 回调：`on_receive` / `on_connected` / `on_disconnected` / `on_sent`
- 配置：服务器地址端口、URL 路径、子协议、连接超时、重连策略、TLS 开关与校验

客户端发送队列上限 1024 帧，队列满返回 `EZ_WS_ERR_QUEUE_FULL`。

### 配置项

- `tls_enable`：连接级 TLS 开关，默认 1
- `tls_verify_peer`：是否校验对端证书，默认 1（内置 CA 互认）
- `tls_ca_path` / `tls_cert_path` / `tls_key_path`：显式证书覆盖

## 性能测试程序

测试目录 `libezsocket/test/` 专为测试代码，不链接库归档，直接用 `src/` 源文件编译
（经 `Makefile.SrcLists` 的 `LIB_OBJS`）。所有 WSS 测试程序以 `t-wss` 开头：

- `t-wss-svr-bench.c`：服务端性能测试。三种模式 `sink`（丢弃，测纯接收）/
  `echo`（回显，测时延与双向）/ `stream`（按配额推流，测客户端接收）。
- `t-wss-cli-bench.c`：客户端性能测试。`-n N` 并发链路（默认 128），四种模式
  `lat`（时延，内嵌 8 字节序号匹配回显）/ `send`（客户端发）/ `recv`（客户端收）/
  `both`（双向）。

明文版 `t-ws-svr-bench.c` / `t-ws-cli-bench.c` 与上述结构一致（API 为
`ez_ws_server_*` / `ez_ws_client_*`），用于与 TLS 版做同参对比。为便于同时起两套
服务端，二者默认端口错开：**明文 ws 默认 `18461`，TLS wss 默认 `18460`**（均可
`-p` 覆盖）。

每条客户端链路使用单线程交替驱动 `service_exec` 与发送，规避并发 `SSL_write` 竞争。

编译（`test/Makefile` 已修复 include 路径为 `lazaru/Makefile.Defines/`，并加入
OpenSSL 头与 `WSS_TLS_LIBS`）：

```
make -C libezsocket/test t-wss-svr-bench-linux t-wss-cli-bench-linux t-ws-svr-bench-linux t-ws-cli-bench-linux
```

运行示例：

```
./t-wss-svr-bench-linux -p 18460 -m echo
./t-wss-cli-bench-linux -h 127.0.0.1 -p 18460 -n 128 -m both --duration 5
./t-ws-svr-bench-linux -p 18461 -m sink      # 明文版可同时起，端口不冲突
./t-ws-cli-bench-linux -h 127.0.0.1 -p 18461 -n 16 -m send --duration 5
```

## 测试要点清单

- 单链路 send 吞吐（客户端→服务端，sink）
- 多链路 send 吞吐（16 / 128 链路，sink）
- 双向 both 吞吐（echo，16 链路）
- 时延 lat（echo 回显，1 链路）
- recv 接收（服务端 stream 推流）
- ws vs wss 同参对比（明文 vs TLS 开销）
- 数据正确性 verify（echo 回显 + 内嵌序号，校验丢/重/坏）
- 多链路连接稳定性（connected 保持）
- 服务端单线程回显瓶颈

## 实测结果

以下为 loopback（127.0.0.1）、TLS1.3、内置 CA、帧载荷 4096 字节、`send-burst 64`、
运行 5 秒的实测值。每条链路单线程交替驱动 `service_exec` 与发送。

### 单链路 send 吞吐（客户端→服务端，服务端 sink）

同一套 bench（`t-wss-*` 与明文 `t-ws-*` 参数完全一致），各跑多轮取中位数：

| 模式 | 吞吐（中位数） | 帧率 | 采样 |
| --- | --- | --- | --- |
| 明文 ws | ≈ 91.2 MB/s | ≈ 22.3k msg/s | 83.3 / 91.2 / 91.2 / 91.0 / 94.2 |
| TLS wss | ≈ 80.5 MB/s | ≈ 19.7k msg/s | 82.2 / 79.7 / 80.4 / 83.0 / 80.5 |

**TLS 相对明文的开销：约 10–12%**（(1 − 80.5/91.2) ≈ 11.7%；按均值算 ≈ 10%）。
该结论由“同源码同参数”的明文/加密两套 bench 直接对比得出，而非凭经验估算。
开销来源：TLS 记录加密/解密（AES-GCM）+ 非阻塞写状态机（WANT_READ/WANT_WRITE
续写）+ 每次 EPOLLIN 后的 `SSL_pending` 排空。

### 多链路 send 吞吐（服务端 sink）

| 链路数 | 明文 ws | TLS wss |
| --- | --- | --- |
| 16 | 173–233 MB/s | 259–318 MB/s |
| 128 | — | ≈ 286 MB/s（connected 128） |

多链路数值受 bench 单线程调度影响、轮间抖动较大，仅作量级参考。16 路以上 TLS
不劣于明文，主要因 TLS 路径每次 `SSL_write` 的摊销行为不同；单链路才是衡量
纯 TLS 开销的最干净基准。

### 时延（echo，1 链路）

典型 74–221 µs，偶发一次约 49 ms（多位于 WANT_READ 续写边界），无超时、无断连。

### 多链路稳定性

- 客户端 `send`（sink）：1 / 16 / 128 链路全程 5 秒稳定满速，`connected` 保持
  16 / 128，连接不再被误断。
- 客户端 `both`（echo，16 链路）：TX ≈ 255 MB/s，连接全程保持。
- 服务端仍为单线程事件循环，回显（echo）路径受其单线程吞吐限制；若要同时支撑
  大量连接的满速双向，需对服务端做多线程化或加强背压（属服务端演进方向，非本次
  客户端修复范畴）。

### 数据正确性（verify 模式）

新增 `-m verify` 模式：客户端每帧内嵌 8 字节自增序号 + 由序号生成的载荷，服务端
`echo` 原样回显，客户端按序号核对 **丢（gap）/ 重（dup）/ 坏（bad）**。

结果（1 链路，full-duplex，`--send-burst 1`，服务端 echo 能跟上的速率）：

| 模式 | 发送 | 回显 | lost | dup | bad | 结论 |
| --- | --- | --- | --- | --- | --- | --- |
| 明文 ws | 56849 | 56847 | 0 | 0 | 0 | 无丢失/重复/损坏 |
| TLS wss | 48957 | 48917 | 0 | 0 | 0 | 无丢失/重复/损坏 |

`sent - echoed` 的少量差值是测试窗口结束时仍在途、未完成往返的帧，非丢失。

结论：**在 echo 服务端能跟上的速率下，明文 ws 与 TLS wss 均验证为零丢包、零重复、
零损坏**。TLS 未引入额外数据错误。

注意：客户端满速（`--send-burst 64`）时，服务端单线程 echo 吞吐（约 200 msg/s）远
低于客户端发送速率，回显积压未排空，连接断开时被整体丢弃，故 `echoed << sent`；
但已回显的部分仍 `lost=0`。这是**服务端 echo 容量瓶颈**，并非传输丢包——传输本身
（TCP + WS 分帧，以及 TLS 的 SSL 层）在可持续速率下被证实可靠。

### 测试中发现并修复的客户端 TLS 缺陷

非阻塞 `SSL_write` 状态机有两类坑，均已在客户端修复：

1. **WANT_READ 续写（bad write retry #1）**
   - 现象：持续高速 `SSL_write` 约 1 秒后连接被误断（“Send error, closing
     connection”）。
   - 根因：`SSL_write` 返回 `SSL_ERROR_WANT_READ` 后直接重试写；OpenSSL 要求先执行
     `SSL_read` 推进 TLS 状态再续写，否则报 `tls_write_check_pending: bad write
     retry`。
   - 修复：新增 `write_blocked_on_read` 标志；WANT_READ 时置位、任意 `SSL_read`
     成功即清除；flush / send_internal / control_frame 三处调用点在置位时跳过写，
     等 EPOLLIN 读取推进后再续写。

2. **WANT_WRITE 时缓冲区身份（bad write retry #2，多链路必现）**
   - 现象：单链路稳定，但并发 16 路时偶发 16 次 `SSL_ERROR_SSL: bad write retry`
     后连接全部断链（DBG 统计：4406 次 flush fail，其中 16 次 err=1、随后 4390 次
     err=5 SYSCALL）。
   - 根因：`SSL_write` 返回 WANT_WRITE 后，OpenSSL 内部以 `wpend_buf` 记录传入缓冲
     区的身份与内部偏移（`wpend_off`）。原实现把帧“复制到新缓冲区并释放原帧”，
     重试时传入的是新指针，`wpend_buf != buf` → `bad write retry`。
   - 修复：WANT_READ/WANT_WRITE 时**保留原始缓冲区与全长，原样重试**（`pending_send`
     直接持有调用 `SSL_write` 的那个 `frame` 指针，不复制、不释放）；flush 重试时也
     以“基址 + 全长”调用，与最初调用完全一致。未启用
     `SSL_MODE_ENABLE_PARTIAL_WRITE` 时 `SSL_write` 只返回全长或 -1，故完成前
     `sent_len` 恒为 0，天然满足“相同参数重试”契约。
   - 效果：16 / 128 链路 send 全程稳定，不再断链。

   两处修复共同保证非阻塞 TLS 写状态机正确：先排 WANT_READ 的“读推进”，再保
   WANT_WRITE 的“缓冲区身份”，二者缺一不可。
