# Network

- [Network](#network)
  - [socket](#socket)
  - [libhv](#libhv)
    - [enable features in libhv](#enable-features-in-libhv)
    - [websocket example](#websocket-example)
    - [tcp example](#tcp-example)
      - [solve sticky packet](#solve-sticky-packet)
    - [udp example](#udp-example)
  - [unix domain socket in python SOCK\_STREAM](#unix-domain-socket-in-python-sock_stream)

`io_uring` can offer lower latency than `epoll` in many scenarios, particularly where reducing syscall overhead is crucial. 

## socket

IBM [DOCS](https://www.ibm.com/docs/en/aix/7.1?topic=s-socket-subroutine)

`int socket (AddressFamily,  Type,  Protocol)`
- TCP
  - ipv4: `int socket (AF_INET, SOCK_STREAM, 0)`
  - ipv6: `int socket (AF_INET6, SOCK_STREAM, 0)`
- UDP
  - ipv4: `int socket (AF_INET, SOCK_DGRAM, 0)`
  - ipv6: `int socket (AF_INET6, SOCK_DGRAM, 0)`
- UDS: Unix Domain Socket
  - `int socket (AF_UNIX, SOCK_STREAM, 0)`
  - `int socket (AF_UNIX, SOCK_DGRAM, 0)`
- RDS: Reliable Datagram Sockets
  - `int socket (AF_BYPASS, SOCK_SEQPACKET,BYPASSPROTO_RDS);`
- SCTP: Stream Control Transmission Protocol
  - One-to-One Interface
    - `int socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP);`
    - `int socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP);`
  - One-to-Many Interface
    - `int socket (AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);`
    - `int socket (AF_INET6, SOCK_SEQPACKET, IPPROTO_SCTP);`

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

### udp example

UDP features
- there is no sticky packet problem

```cmake
cmake_minimum_required(VERSION 3.20)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(udp_srv udp_srv.cpp)
add_executable(udp_cli udp_cli.cpp)
target_compile_definitions(udp_srv PRIVATE ENABLE_UDS)
target_compile_definitions(udp_cli PRIVATE ENABLE_UDS)
target_compile_definitions(udp_srv PRIVATE WITH_KCP)
target_compile_definitions(udp_cli PRIVATE WITH_KCP)

find_package(libhv CONFIG REQUIRED)
target_link_libraries(udp_srv PRIVATE hv_static)
target_link_libraries(udp_cli PRIVATE hv_static)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(udp_srv PRIVATE fmt::fmt)
target_link_libraries(udp_cli PRIVATE fmt::fmt)
```

```bash
# usage1: without unix domain socket
udp_srv localhost 8888
udp_cli localhost 8888

# usage2: without unix domain socket
udp_srv /tmp/my.sock -1
udp_cli /tmp/my.sock -1
```

```cpp
// udp_srv.cpp
#include <fmt/core.h>
#include <hv/UdpServer.h>

struct Request {
    int id;
    double raw_value;
};

struct Response {
    int id;
    double processed_value;
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fmt::println("usage: {} HOST PORT", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);

    hv::UdpServer srv;
    if (auto bindfd = srv.createsocket(port, host); bindfd < 0) {
        return -20;
    }
    srv.onMessage = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        auto req = reinterpret_cast<Request*>(buf->data());
        fmt::println("onMessage: req id={}, value={}", req->id, req->raw_value);
        Response rsp{req->id, req->raw_value * 10};
        channel->write(&rsp, sizeof(Response));
    };
    srv.onWriteComplete = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        fmt::println("onWriteComplete: ok");
    };
    srv.start();

    while (getchar() != '\n');
}
```

```cpp
// udp_cli.cpp
#include <fmt/core.h>
#include <hv/UdpClient.h>

struct Request {
    int id;
    double raw_value;
};

struct Response {
    int id;
    double processed_value;
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fmt::println("usage: {} HOST PORT", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);

    hv::UdpClient cli;
    if (auto sockfd = cli.createsocket(port, host); sockfd < 0) {
        return -20;
    }
    cli.onMessage = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        auto rsp = reinterpret_cast<Response*>(buf->data());
        fmt::println("onMessage: rsp id={}, value={}", rsp->id, rsp->processed_value);
    };
    cli.onWriteComplete = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        fmt::println("onWriteComplete: {} bytes", buf->size());
    };
    cli.start();

    for (auto i = 0; i < 10; ++i) {
        Request req{i * 100, i * 10.1};
        cli.sendto(&req, sizeof(Request));
    }

    while (getchar() != '\n');
}
```

kcp server and client

```cpp
// kcp_srv.cpp
#include <fmt/core.h>
#include <hv/UdpServer.h>
#include <hv/hloop.h>

struct Request {
    int id;
    double raw_value;
};

struct Response {
    int id;
    double processed_value;
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fmt::println("usage: {} HOST PORT", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);

    hv::UdpServer srv;
    if (auto bindfd = srv.createsocket(port, host); bindfd < 0) {
        return -20;
    }
    srv.onMessage = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        auto req = reinterpret_cast<Request*>(buf->data());
        fmt::println("onMessage: req id={}, value={}", req->id, req->raw_value);
        Response rsp{req->id, req->raw_value * 10};
        channel->write(&rsp, sizeof(Response));
    };
    srv.onWriteComplete = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        fmt::println("onWriteComplete: ok");
    };
    kcp_setting_t setting;
    srv.setKcp(&setting);

    srv.start();

    while (getchar() != '\n');
}
```

```cpp
// kcp_cli.cpp
#include <fmt/core.h>
#include <hv/UdpClient.h>

struct Request {
    int id;
    double raw_value;
};

struct Response {
    int id;
    double processed_value;
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fmt::println("usage: {} HOST PORT", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);

    hv::UdpClient cli;
    if (auto sockfd = cli.createsocket(port, host); sockfd < 0) {
        return -20;
    }
    cli.onMessage = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        auto rsp = reinterpret_cast<Response*>(buf->data());
        fmt::println("onMessage: rsp id={}, value={}", rsp->id, rsp->processed_value);
    };
    cli.onWriteComplete = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        fmt::println("onWriteComplete: {} bytes", buf->size());
    };
    kcp_setting_t setting;
    cli.setKcp(&setting);
    cli.start();

    for (auto i = 0; i < 10; ++i) {
        Request req{i * 100, i * 10.1};
        cli.sendto(&req, sizeof(Request));
    }

    while (getchar() != '\n');
}
```

## unix domain socket in python SOCK_STREAM

```py
# server.py
import socket
import os

def unix_domain_server(socket_path='/tmp/unix_socket'):
    # Ensure the socket does not already exist
    try:
        os.unlink(socket_path)
    except FileNotFoundError:
        pass

    # Create a UNIX domain socket
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as server_sock:
        server_sock.bind(socket_path)
        server_sock.listen()
        print(f"UNIX domain socket server listening at {socket_path}")

        while True:
            conn, _ = server_sock.accept()
            with conn:
                print("Client connected")
                while True:
                    data = conn.recv(1024)
                    if not data:
                        break
                    print(f"Received: {data.decode()}")
                    conn.sendall(data)  # Echo back

if __name__ == "__main__":
    unix_domain_server()
```

```py
# client.py
import socket


def unix_domain_client(socket_path="/tmp/unix_socket"):
    messages = [b"Hello", b"World", b"This is a UNIX socket test"]

    # Create a UNIX domain socket
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client_sock:
        client_sock.connect(socket_path)
        print(f"Connected to UNIX socket at {socket_path}")

        for message in messages:
            print(f"Sending: {message.decode()}")
            client_sock.sendall(message)

            # Receive echo
            data = client_sock.recv(1024)
            print(f"Received echo: {data.decode()}")


if __name__ == "__main__":
    unix_domain_client()
```

Combining TCP and UNIX Domain Sockets
1. TCP Sockets
    - Protocol: TCP (Transmission Control Protocol) operates over IP networks.
    - Addressing: Uses IP addresses and port numbers (e.g., localhost:8080).
    - Use Case: Suitable for network communication between different machines or processes over a network.
    - Socket Family: `AF_INET` for IPv4 or `AF_INET6` for IPv6.
2. UNIX Domain Sockets
    - Protocol: Operate within the same host using the file system.
    - Addressing: Uses file system paths (e.g., /tmp/socket).
    - Use Case: Ideal for inter-process communication (IPC) on the same machine with lower latency and overhead compared to TCP.
    - Socket Family: `AF_UNIX`.

These two types of sockets are not interchangeable because:
- Different Socket Families: AF_INET vs. AF_UNIX are distinct and incompatible.
- Addressing Mechanisms: IP addresses/ports vs. file system paths.
- Protocol Differences: TCP vs. UNIX domain protocols.

```py
# server.py
import socket
import threading
import os

def handle_client(conn, address):
    with conn:
        print(f"Connected by {address}")
        while True:
            data = conn.recv(1024)
            if not data:
                break
            print(f"Received from {address}: {data.decode()}")
            conn.sendall(data)

def start_tcp_server(host='localhost', port=12345):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as tcp_sock:
        tcp_sock.bind((host, port))
        tcp_sock.listen()
        print(f"TCP server listening on {host}:{port}")
        while True:
            conn, addr = tcp_sock.accept()
            threading.Thread(target=handle_client, args=(conn, addr)).start()

def start_unix_server(socket_path='/tmp/unix_socket'):
    # Ensure the socket does not already exist
    try:
        os.unlink(socket_path)
    except FileNotFoundError:
        pass

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as unix_sock:
        unix_sock.bind(socket_path)
        unix_sock.listen()
        print(f"UNIX domain socket server listening at {socket_path}")
        while True:
            conn, _ = unix_sock.accept()
            threading.Thread(target=handle_client, args=(conn, socket_path)).start()

def main():
    tcp_thread = threading.Thread(target=start_tcp_server, daemon=True)
    unix_thread = threading.Thread(target=start_unix_server, daemon=True)
    tcp_thread.start()
    unix_thread.start()

    # Keep the main thread alive
    tcp_thread.join()
    unix_thread.join()

if __name__ == "__main__":
    main()
```

```py
# client.py
import socket
import struct
import argparse
import sys
import os

def send_message(sock, message):
    """
    Sends a length-prefixed message over the given socket.
    """
    msg = struct.pack('!I', len(message)) + message
    sock.sendall(msg)

def recv_fixed_length(sock, length):
    """
    Receives an exact number of bytes from the socket.
    """
    data = b''
    while len(data) < length:
        more = sock.recv(length - len(data))
        if not more:
            raise EOFError('Socket closed prematurely')
        data += more
    return data

def recv_message(sock):
    """
    Receives a length-prefixed message from the socket.
    """
    raw_len = recv_fixed_length(sock, 4)
    if not raw_len:
        return None
    msg_len = struct.unpack('!I', raw_len)[0]
    return recv_fixed_length(sock, msg_len)

def tcp_client(host, port, messages):
    """
    TCP client that connects to the specified host and port,
    sends messages, and receives echoes.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        try:
            sock.connect((host, port))
            print(f"Connected to TCP server at {host}:{port}")
        except Exception as e:
            print(f"Failed to connect to TCP server at {host}:{port}: {e}")
            sys.exit(1)

        for msg in messages:
            print(f"Sending: {msg.decode()}")
            send_message(sock, msg)
            try:
                response = recv_message(sock)
                if response:
                    print(f"Received echo: {response.decode()}")
                else:
                    print("No response received. Server may have closed the connection.")
                    break
            except EOFError:
                print("Connection closed by server.")
                break

def unix_domain_client(socket_path, messages):
    """
    UNIX domain socket client that connects to the specified socket path,
    sends messages, and receives echoes.
    """
    if not os.path.exists(socket_path):
        print(f"UNIX socket path '{socket_path}' does not exist.")
        sys.exit(1)

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
        try:
            sock.connect(socket_path)
            print(f"Connected to UNIX socket at {socket_path}")
        except Exception as e:
            print(f"Failed to connect to UNIX socket at {socket_path}: {e}")
            sys.exit(1)

        for msg in messages:
            print(f"Sending: {msg.decode()}")
            sock.sendall(msg)  # For UNIX sockets, message boundaries are preserved
            try:
                data = sock.recv(4096)
                if data:
                    print(f"Received echo: {data.decode()}")
                else:
                    print("No response received. Server may have closed the connection.")
                    break
            except Exception as e:
                print(f"Error receiving data: {e}")
                break

def parse_arguments():
    """
    Parses command-line arguments to determine connection type and parameters.
    """
    parser = argparse.ArgumentParser(description="Client supporting both TCP and UNIX domain sockets.")
    subparsers = parser.add_subparsers(dest='mode', required=True, help='Connection mode')

    # TCP mode
    tcp_parser = subparsers.add_parser('tcp', help='Use TCP to connect to the server')
    tcp_parser.add_argument('--host', type=str, default='localhost', help='TCP server host (default: localhost)')
    tcp_parser.add_argument('--port', type=int, default=12345, help='TCP server port (default: 12345)')

    # UNIX domain socket mode
    unix_parser = subparsers.add_parser('unix', help='Use UNIX domain socket to connect to the server')
    unix_parser.add_argument('--socket', type=str, default='/tmp/unix_socket', help='UNIX socket path (default: /tmp/unix_socket)')

    return parser.parse_args()

def main():
    args = parse_arguments()

    # Define messages to send
    messages = [b'Hello', b'World', b'This is a test message']

    if args.mode == 'tcp':
        tcp_client(args.host, args.port, messages)
    elif args.mode == 'unix':
        unix_domain_client(args.socket, messages)
    else:
        print("Invalid mode selected. Use 'tcp' or 'unix'.")
        sys.exit(1)

if __name__ == "__main__":
    main()
```