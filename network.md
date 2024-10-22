# Network

- [Network](#network)
  - [libhv](#libhv)
    - [enable features in libhv](#enable-features-in-libhv)
    - [websocket example](#websocket-example)
    - [tcp example](#tcp-example)

`io_uring` can offer lower latency than `epoll` in many scenarios, particularly where reducing syscall overhead is crucial. 

## libhv

### enable features in libhv

how to enable **Unix Domain Socket** and **KCP** in [libhv config](https://github.com/ithewei/libhv/blob/master/config.ini)?
- method1: change root `CMakeLists.txt` options in libhv repo, then build libhv from [source](https://github.com/ithewei/libhv)
- method2: change `vcpkg/ports/libhv/portfile.cmake`, then `vcpkg install libhv`
  
```cmake
# vcpkg/ports/libhv/portfile.cmake
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    DISABLE_PARALLEL_CONFIGURE
    OPTIONS
        -DBUILD_EXAMPLES=OFF
        -DENABLE_UDS=ON # ENABLE THIS
        -DWITH_KCP=ON # ENABLE THIS
        -DBUILD_UNITTEST=OFF
        -DBUILD_STATIC=${BUILD_STATIC}
        -DBUILD_SHARED=${BUILD_SHARED}
        ${FEATURE_OPTIONS}
)
```

### websocket example

> websocket not support Unix Domain Socket

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(ws_srv ws_srv.cpp)
add_executable(ws_cli ws_cli.cpp)

find_package(libhv CONFIG REQUIRED)
target_link_libraries(ws_srv PRIVATE hv_static)
target_link_libraries(ws_cli PRIVATE hv_static)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(ws_srv PRIVATE fmt::fmt)
target_link_libraries(ws_cli PRIVATE fmt::fmt)
```

```cpp
// ws_srv.cpp
#include <fmt/core.h>
#include <hv/WebSocketServer.h>
#include <hv/htime.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        fmt::println("usage: {} HOST PORT", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);

    WebSocketService ws;
    ws.onopen = [](const WebSocketChannelPtr& channel, const HttpRequestPtr& req) {
        fmt::println("onpen: GET {}", req->Path().c_str());
        channel->setKeepaliveTimeout(3000);  // require client ping smaller than 3s
    };
    ws.onmessage = [](const WebSocketChannelPtr& channel, const std::string& msg) {
        char dt_str[DATETIME_FMT_BUFLEN]{};
        auto dt = datetime_now();
        datetime_fmt(&dt, dt_str);
        fmt::println("onmessage: {} at {}", msg, dt_str);

        auto reply = fmt::format("{}:{}", dt_str, msg);
        channel->send(reply.data(), reply.size());
    };
    ws.onclose = [](const WebSocketChannelPtr& channel) {
        fmt::println("onclose");
    };

    hv::WebSocketServer srv{&ws};
    srv.setHost(host);
    srv.setPort(port);
    fmt::println("ws listening on {}:{}", host, port);
    srv.run();
}
````

```cpp
// ws_cli.cpp
#include <fmt/core.h>
#include <hv/WebSocketClient.h>
#include <hv/hplatform.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        fmt::println("usage: {} HOST PORT", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);

    hv::WebSocketClient ws;
    ws.setPingInterval(1000);  // ping 1s < 3s
    ws.onopen = [] {
        fmt::println("onopen");
    };
    ws.onclose = [] {
        fmt::println("onclose");
    };
    ws.onmessage = [](const std::string& msg) {
        fmt::println("onmessage: {}", msg);
    };

    auto addr = fmt::format("{}:{}", host, port);
    // ws://ip:port/path or ip:port/path
    ws.open(addr.c_str());
    for (size_t i = 0; i < 10; ++i) {
        auto msg = std::to_string(i);
        ws.send(msg);
        hv_msleep(200);
    }
    while (getchar() != '\n');
}
```

### tcp example

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(tcp_srv tcp_srv.cpp)
add_executable(tcp_cli tcp_cli.cpp)
# enable unix domain socket
target_compile_definitions(tcp_srv PRIVATE ENABLE_UDS)
target_compile_definitions(tcp_cli PRIVATE ENABLE_UDS)

find_package(libhv CONFIG REQUIRED)
target_link_libraries(tcp_srv PRIVATE hv_static)
target_link_libraries(tcp_cli PRIVATE hv_static)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(tcp_srv PRIVATE fmt::fmt)
target_link_libraries(tcp_cli PRIVATE fmt::fmt)
```

```bash
# usage1 with simple tcp
tcp_srv 127.0.0.1 8888
tcp_cli 127.0.0.1 8888

# usage1 with Unix Domain Socket
tcp_srv /tmp/my.sock -1
tcp_cli /tmp/my.sock -1
```

```cpp
// tcp_srv.cpp
#include <fmt/core.h>
#include <hv/Channel.h>
#include <hv/TcpServer.h>
#include <hv/hloop.h>
#include <hv/htime.h>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fmt::println("usage: {} HOST PORT", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);

    hv::TcpServer srv;
    if (auto listenfd = srv.createsocket(port, host); listenfd < 0) {
        return -20;
    }
    fmt::println("tcp listening {}:{}, listenfd={}", srv.host, srv.port, srv.listenfd);

    srv.onConnection = [](const hv::SocketChannelPtr& channel) {
        channel->setKeepaliveTimeout(3000);  // 3s
        auto peeraddr = channel->peeraddr();
        if (channel->isConnected()) {
            fmt::println("{} connected! connfd={}", peeraddr, channel->fd());
        } else {
            fmt::println("{} disconnected! connfd={}", peeraddr, channel->fd());
        }
    };
    srv.onMessage = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        fmt::println("onMessage: {}", (char*)buf->data());
        char dt_str[DATETIME_FMT_BUFLEN]{};
        auto dt = datetime_now();
        datetime_fmt(&dt, dt_str);

        auto reply = fmt::format("{}:{}", dt_str, (char*)buf->data());
        channel->write(reply.data(), reply.size());
    };
    srv.onWriteComplete = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        fmt::println("onWriteComplete: {}", (char*)buf->data());
    };
    srv.setMaxConnectionNum(10);
    srv.start();

    while (getchar() != '\n');
}
```

```cpp
// tcp_cli.cpp
#include <fmt/core.h>
#include <hv/TcpClient.h>
#include <hv/htime.h>

#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fmt::println("usage: {} HOST PORT", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);

    hv::TcpClient cli;
    if (auto connfd = cli.createsocket(port, host); connfd < 0) {
        return -20;
    }

    cli.onConnection = [](const hv::SocketChannelPtr& channel) {
        std::string peeraddr = channel->peeraddr();
        if (channel->isConnected()) {
            fmt::println("connected to {}! connfd={}", peeraddr, channel->fd());
        } else {
            fmt::println("disconnected to {}! connfd={}", peeraddr, channel->fd());
        }
    };
    cli.onMessage = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        fmt::println("onMessage: {}", (char*)buf->data());
    };
    cli.onWriteComplete = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        fmt::println("onWriteComplete: {}", (char*)buf->data());
    };
    cli.start();

    for (auto i = 0; i < 10; ++i) {
        auto msg = std::to_string(i);
        cli.send(msg);
        hv_msleep(200);
    }

    while (getchar() != '\n');
    cli.stop();
}
```

some problems:
- if `hs_msleep()` sleep time too small, `sticky packet` will happen
- there is no `sticky packet` problem in udp or websocket
