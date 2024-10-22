# Network

- [Network](#network)
  - [libhv](#libhv)
    - [enable features in libhv](#enable-features-in-libhv)
    - [websocket example](#websocket-example)
    - [tcp example](#tcp-example)
      - [solve sticky packet](#solve-sticky-packet)

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
    // ws://ip:port/path or ip:port/path
    auto addr = fmt::format("{}:{}", host, port);

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

    // reconnect: 1,2,4,8,10,10,10...
    reconn_setting_t reconn;
    reconn.min_delay = 1000;
    reconn.max_delay = 10000;
    reconn.delay_policy = DEFAULT_RECONNECT_DELAY_POLICY;  // exponential
    ws.setReconnect(&reconn);

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

# add_definitions(-DENABLE_UDS) # global definition

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

TcpServer更多[实用接口](https://hewei.blog.csdn.net/article/details/113737580)
- `setThreadNum`：设置IO线程数
- `setMaxConnectionNum`：设置最大连接数
- `setLoadBalance`: 设置负载均衡策略（轮询、随机、最少连接数）
- `setUnpack`：设置拆包规则（固定包长、分界符、头部长度字段）
- `withTLS`：SSL/TLS加密通信
- `broadcast`: 广播

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

TcpClient更多实用接口
- `setConnectTimeout`：设置连接超时
- `setReconnect`：设置重连
- `setUnpack`: 设置拆包
- `withTLS`：SSL/TLS加密通信

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

    // reconnect: 1,2,4,8,10,10,10...
    reconn_setting_t reconn;
    reconn.min_delay = 1000;
    reconn.max_delay = 10000;
    reconn.delay_policy = DEFAULT_RECONNECT_DELAY_POLICY;  // exponential
    cli.setReconnect(&reconn);

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

#### solve sticky packet

```cpp
typedef enum {
    UNPACK_MODE_NONE        = 0,
    UNPACK_BY_FIXED_LENGTH  = 1,    // Not recommended
    UNPACK_BY_DELIMITER     = 2,    // Suitable for text protocol
    UNPACK_BY_LENGTH_FIELD  = 3,    // Suitable for binary protocol
} unpack_mode_e;
```

```cpp
struct Student {
    long age;
    double score;
};

// with padding, Message size is 24 bytes
struct Message {
    uint32_t head;
    Student body; // offset is 8
};
```

method1: solve sticky packet by fix-length

```cpp
unpack_setting_t setting{};
setting.mode = UNPACK_BY_FIXED_LENGTH;
setting.fixed_length = 24;
// server part
srv.setUnpack(&setting);
// client part
cli.setUnpack(&setting);
```

method2: solve sticky packet by length field

```cpp
// tcp_srv.cpp
#include <fmt/core.h>
#include <hv/Channel.h>
#include <hv/TcpServer.h>
#include <hv/hloop.h>
#include <hv/htime.h>

struct Request {
    int id;
    double raw_value;
};

struct Response {
    int id;
    double processed_value;
};

// Function to prepare the message with little-endian header
template <typename T>
auto prepare(T const& pod) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    constexpr uint32_t body_length = sizeof(T);
    // Ensure the body size fits in 4 bytes
    static_assert(body_length <= std::numeric_limits<uint32_t>::max(), "Type T is too large");
    std::array<char, 4 + body_length> result{};
    // Serialize the length in little-endian order
    std::memcpy(result.data(), &body_length, 4);
    // Copy the POD data after the header
    std::memcpy(result.data() + 4, &pod, body_length);
    return result;  // RVO likely applies here
}

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
        auto req = reinterpret_cast<Request*>((char*)buf->data() + 4);
        fmt::println("onMessage: req id={}, value={}", req->id, req->raw_value);
        Response rsp{req->id, req->raw_value * 10};
        auto packet = prepare(rsp);
        channel->write(packet.data(), packet.size());
    };
    srv.onWriteComplete = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        fmt::println("onWriteComplete: ok");
    };
    srv.setMaxConnectionNum(10);

    // solve sticky packet in srv.onMessage
    unpack_setting_t setting{};
    setting.mode = UNPACK_BY_LENGTH_FIELD;
    setting.package_max_length = DEFAULT_PACKAGE_MAX_LENGTH;
    setting.body_offset = 4;
    setting.length_field_offset = 0;
    setting.length_field_bytes = 4;
    setting.length_field_coding = ENCODE_BY_LITTEL_ENDIAN;
    srv.setUnpack(&setting);

    srv.start();

    while (getchar() != '\n');
    srv.stop();
}
```

```cpp
// tcp_cli.cpp
#include <fmt/core.h>
#include <hv/TcpClient.h>
#include <hv/htime.h>

#include <cstdint>
#include <cstring>
#include <string>

struct Request {
    int id;
    double raw_value;
};

struct Response {
    int id;
    double processed_value;
};

// Function to prepare the message with little-endian header
template <typename T>
auto prepare(T const& pod) noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    constexpr uint32_t body_length = sizeof(T);
    // Ensure the body size fits in 4 bytes
    static_assert(body_length <= std::numeric_limits<uint32_t>::max(), "Type T is too large");
    std::array<char, 4 + body_length> result{};
    // Serialize the length in little-endian order
    std::memcpy(result.data(), &body_length, 4);
    // Copy the POD data after the header
    std::memcpy(result.data() + 4, &pod, body_length);
    return result;  // RVO likely applies here
}

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
        auto rsp = reinterpret_cast<Response*>((char*)buf->data() + 4);
        fmt::println("onMessage: rsp id={},value={}", rsp->id, rsp->processed_value);
    };
    cli.onWriteComplete = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        fmt::println("onWriteComplete: done");
    };

    // solve sticky packet in cli.onMessage
    unpack_setting_t setting{};
    setting.mode = UNPACK_BY_LENGTH_FIELD;
    setting.package_max_length = DEFAULT_PACKAGE_MAX_LENGTH;
    setting.body_offset = 4;
    setting.length_field_offset = 0;
    setting.length_field_bytes = 4;
    setting.length_field_coding = ENCODE_BY_LITTEL_ENDIAN;
    cli.setUnpack(&setting);

    cli.start();

    for (auto i = 0; i < 10; ++i) {
        Request req{i * 100, i * 10.1};
        auto packet = prepare(req);
        cli.send(packet.data(), packet.size());
    }

    while (getchar() != '\n');
    cli.stop();
}
```

prepare dynamic length data

```cpp
// Function to prepare the message with little-endian header
auto prepare_dynamic(const void* ptr, uint32_t body_length) noexcept {
    std::vector<char> result(4 + body_length);
    // Serialize the length in little-endian order
    std::memcpy(result.data(), &body_length, 4);
    // Copy the POD data after the header
    std::memcpy(result.data() + 4, ptr, body_length);
    return result;  // RVO likely applies here
}
```