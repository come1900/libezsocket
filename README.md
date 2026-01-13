# libezsocket - Network Socket Library

## Description

libezsocket is a comprehensive C network socket library that provides cross-platform socket programming abstractions for TCP, UDP, HTTP, and RTSP protocols. The library includes:

- **TCP/UDP Socket Operations**: High-level APIs for creating TCP/UDP servers and clients
- **HTTP Client**: GET and POST request functionality
- **RTSP Support**: RTSP URL validation and streaming support
- **Raw Socket**: IP packet sniffing capabilities
- **Network Utilities**: IP address parsing, hostname resolution, and network configuration

All implementations support cross-platform development (Linux, Windows, Cygwin) and provide a unified API for building network applications.

## Quick Start

### Requirements

- **Linux environment** with GCC compiler (C99 or higher)
- **libezutil** library (must be compiled and installed first)
  ```bash
  cd ../libezutil
  make && make install
  ```
- **pthread** library (standard system library)
- **Make** build tool

### Installation

```bash
cd libezsocket
make
make install
```

This will generate the static library `libezsocket-$(PLATFORM).a` and install it to:
- Library files: `$(HOME)/libs/lib/`
- Header files: `$(HOME)/libs/include/ezsocket/`

### Compiling Tutorial Examples

The library includes tutorial examples in the `tutorial_libezsocket` directory:

```bash
cd tutorial_libezsocket
make
```

**Note:** Compilation requires libezsocket and libezutil to be installed first (run `make install` in the parent directory).

## Usage

### Basic TCP Server

```c
#include "ezsocket/ez_socket.h"

// Create TCP server on port 8080
ez_socket_t server_fd = ez_tcp_server(8080, 10);  // backlog = 10
if (server_fd < 0) {
    // Error handling
}

// Accept client connection
struct sockaddr_in client_addr;
ez_socklen_t addr_len = sizeof(client_addr);
ez_socket_t client_fd = ez_accept(server_fd, (SA*)&client_addr, &addr_len);

// Read/write data
char buffer[1024];
int n = read(client_fd, buffer, sizeof(buffer));
ez_writen(client_fd, "Hello", 5);

// Close sockets
ez_close(client_fd);
ez_close(server_fd);
```

### Basic TCP Client

```c
#include "ezsocket/ez_socket.h"

// Connect to server with timeout
ez_socket_t sockfd = ez_tcp_connect("192.168.1.100", 8080, 5000000);  // 5 second timeout
if (sockfd < 0) {
    // Connection failed
}

// Send data
ez_writen(sockfd, "Hello Server", 12);

// Close socket
ez_close(sockfd);
```

### UDP Socket

```c
#include "ezsocket/ez_socket.h"

// Create UDP server
ez_socket_t udp_fd = ez_udp_server(8080);

// Send UDP packet
struct sockaddr_in dest_addr;
ez_set_address("192.168.1.100", "8080", &dest_addr, "udp");
ez_writeton(udp_fd, "Hello", 5, (SA*)&dest_addr, sizeof(dest_addr));

// Receive UDP packet
char buffer[1024];
struct sockaddr_in from_addr;
ez_socklen_t addr_len = sizeof(from_addr);
recvfrom(udp_fd, buffer, sizeof(buffer), 0, (SA*)&from_addr, &addr_len);
```

### HTTP GET Request

```c
#include "ezsocket/ez_socket.h"

// Connect to HTTP server
ez_socket_t sockfd = ez_tcp_connect("www.example.com", 80, 5000000);

// Send HTTP GET request
char recvline[4096];
ssize_t n = ez_http_get(sockfd, "www.example.com", "index.html", recvline, sizeof(recvline), 0);

if (n > 0) {
    printf("Response: %s\n", recvline);
}

ez_close(sockfd);
```

### HTTP POST Request

```c
#include "ezsocket/ez_socket.h"

// Connect to HTTP server
ez_socket_t sockfd = ez_tcp_connect("www.example.com", 80, 5000000);

// Send HTTP POST request
const char *post_data = "key1=value1&key2=value2";
char recvline[4096];
ssize_t n = ez_http_post(sockfd, "www.example.com", "api/endpoint", post_data, recvline, sizeof(recvline), 0);

if (n > 0) {
    printf("Response: %s\n", recvline);
}

ez_close(sockfd);
```

### RTSP URL Validation

```c
#include "ezsocket/ez_socket.h"

// Validate RTSP URL
int ret = ez_rtsp_valid("rtsp://192.168.1.100:554/stream", 0);
if (ret == 0) {
    printf("RTSP URL is valid\n");
} else {
    printf("RTSP URL validation failed: %d\n", ret);
}
```

### Linking in Your Project

When compiling your project, add the following flags:

```bash
-I$(HOME)/libs/include/ezsocket -I$(HOME)/libs/include/ezutil \
-L$(HOME)/libs/lib -lezsocket-$(PLATFORM) -lezutil-$(PLATFORM) -lpthread
```

Example Makefile:

```makefile
EZLIBS_BASEDIR_LIBS=$(HOME)/libs
CFLAGS += -I${EZLIBS_BASEDIR_LIBS}/include/ezsocket
CFLAGS += -I${EZLIBS_BASEDIR_LIBS}/include/ezutil
LIBS += -L${EZLIBS_BASEDIR_LIBS}/lib -lezsocket-$(PLATFORM) -lezutil-$(PLATFORM)
LIBS += -lpthread

myapp: myapp.o
	$(CC) -o $@ $^ $(LIBS)
```

## Architecture

### Core Components

- **ez_socket**: Basic socket operations (create, bind, listen, accept, connect)
- **ez_tcp**: TCP client/server helper functions
- **ez_udp**: UDP client/server helper functions
- **ez_http**: HTTP GET/POST request functions
- **ez_raw**: Raw socket and packet sniffing
- **ez_sockaddr**: Socket address manipulation utilities
- **ez_util**: Network utility functions

### Features

- **Cross-platform**: Supports Linux, Windows, and Cygwin
- **Timeout Support**: Connection and read operations with configurable timeouts
- **Non-blocking I/O**: Support for non-blocking socket operations
- **Error Handling**: Comprehensive error reporting and handling
- **IPv4/IPv6 Ready**: Socket address abstraction supports both protocols

## Tutorial Examples

The `tutorial_libezsocket` directory contains several example programs:

- **ez_telnet**: Telnet client example
- **tcp_svr_dumpdata**: TCP server that dumps received data
- **simple_rtsp_cli**: Simple RTSP client
- **rtsp_valid**: RTSP URL validation example
- **form_post_cli**: HTTP form POST client
- **get_pub_ip**: Get public IP address example
- **rtp_rcv_udp**: RTP UDP receiver example

To compile and run:

```bash
cd tutorial_libezsocket
make
./ez_telnet-$(PLATFORM) hostname port
./tcp_svr_dumpdata-$(PLATFORM) 8080
```

## License

MIT License

