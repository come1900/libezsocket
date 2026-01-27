# libezsocket

## Description

libezsocket is a lightweight C/C++ socket library that provides cross-platform network programming interfaces. It supports TCP/UDP socket operations, HTTP client/server functionality, WebSocket client/server, and various utility functions for network programming. The library features cross-platform support (Linux, Windows, macOS, embedded systems), easy-to-use APIs, and no external dependencies beyond standard system libraries.

## Quick Start

### Requirements

- C99 or higher compiler (GCC 4.8+ / Clang 3.3+)
- Make build tool
- pthread library (for multi-threading support)

### Installation

```bash
make
make install
```

The library files will be installed to `$(HOME)/libs/lib/`, and header files will be installed to `$(HOME)/libs/include/ezsocket/`.

## Usage

Include the corresponding header files in your project and link the library as needed, for example:

```c
#include <ezsocket/ez_socket.h>
#include <ezsocket/ez_websocket.h>
#include <ezutil/ez_url_parser.h>
...
```

Add link options when compiling: `-L$(HOME)/libs/lib -lezsocket-$(PLATFORM) -lezutil-$(PLATFORM) -lpthread`

3rd-libwebsockets/tutorials/touch-libwebsocket

## License

libezsocket is licensed under the LGPL v2.1 or later. You are free to use, modify, and distribute it under the terms of this license. See the LICENSE file for the complete license text.

