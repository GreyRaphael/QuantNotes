# Network

- [Network](#network)
  - [libhv](#libhv)
    - [websocket](#websocket)

`io_uring` can offer lower latency than `epoll` in many scenarios, particularly where reducing syscall overhead is crucial. 

## libhv

### websocket

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(ws_srv ws_srv.cpp)
add_executable(ws_cli ws_cli.cpp)
target_compile_definitions(ws_srv PRIVATE ENABLE_UDS)
target_compile_definitions(ws_cli PRIVATE ENABLE_UDS)

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