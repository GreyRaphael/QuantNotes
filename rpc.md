# Remote Procedure Call

- [Remote Procedure Call](#remote-procedure-call)
  - [RPC by pycapnp](#rpc-by-pycapnp)
    - [simple invoke](#simple-invoke)
    - [callback invoke](#callback-invoke)
    - [sync and async order system](#sync-and-async-order-system)
  - [RPC by capnp in C++](#rpc-by-capnp-in-c)
    - [pingpong benchmark](#pingpong-benchmark)
  - [yaLanTingLibs rpc](#yalantinglibs-rpc)
    - [yaLanTingLibs pingpong rpc](#yalantinglibs-pingpong-rpc)


## RPC by pycapnp

### simple invoke

```bash
├── example.capnp
├── server.py
└── client.py
```

```capnp
# example.capnp
@0x85150b117366d14b;

interface Calculator {
    myfunc @0 (x: Float64) -> (value: Float64);
}
```

```py
# server.py
import asyncio
import capnp
import example_capnp


class CalculatorImpl(example_capnp.Calculator.Server):
    async def myfunc(self, x, _context):
        return x * 100


async def new_connection(stream):
    await capnp.TwoPartyServer(stream, bootstrap=CalculatorImpl()).on_disconnect()


async def main():
    server = await capnp.AsyncIoStream.create_server(new_connection, "127.0.0.1", "8888")
    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(capnp.run(main()))
```

```py
# client.py
import asyncio
import capnp
import example_capnp


async def main(connection):
    client = capnp.TwoPartyClient(connection)
    calculator = client.bootstrap().cast_as(example_capnp.Calculator)
    promise = await calculator.myfunc(1.345)
    print(promise.value)


async def cmd_main():
    await main(await capnp.AsyncIoStream.create_connection(host="127.0.0.1", port="8888"))


if __name__ == "__main__":
    asyncio.run(capnp.run(cmd_main()))
```

### callback invoke

```bash
├── example.capnp # change this
├── server.py # same as the above
└── client.py # change this
```

```capnp
# example.capnp
@0x85150b117366d14b;

interface Calculator {
    myfunc @0 (x: Float64) -> (value: Float64);
}

interface Callback {
  onResult @0 (value: Float64) -> ();
}
```

```py
# client.py
import asyncio
import capnp
import example_capnp


class CallbackImpl(example_capnp.Callback.Server):
    async def onResult(self, value, **kwargs):
        print(f"Callback received value: {value}")


async def main(connection):
    client = capnp.TwoPartyClient(connection)
    calculator = client.bootstrap().cast_as(example_capnp.Calculator)
    callback = CallbackImpl()

    for i in range(10):
        promise = await calculator.myfunc(i * 1.1)
        await callback.onResult(promise.value)


async def cmd_main():
    await main(await capnp.AsyncIoStream.create_connection(host="127.0.0.1", port="8888"))


if __name__ == "__main__":
    asyncio.run(capnp.run(cmd_main()))
```

### sync and async order system

```bash
├── trade.capnp
├── server.py
└── client.py
```

```capnp
# trade.capnp
@0x85150b117366d14b;

interface TradeSpi {
  onOrder @0 (info: Text) -> ();
  onCancel @1 (info: Text) -> ();
}

interface TradeApi {
    reqOrder @0 (orderId:Int64, cb: TradeSpi) -> ();
    reqCancel @1 (orderId:Int64, cb: TradeSpi) -> ();
}
```

```py
# server.py
import capnp
import trade_capnp
import asyncio
import random


class TradeApiImpl(trade_capnp.TradeApi.Server):
    async def reqOrder(self, orderId, _context, **kwargs):
        # print(kwargs)
        callback = kwargs["cb"]
        print(f"server get orderid: {orderId}")
        for i in range(3):  # Random number of responses
            await asyncio.sleep(random.randint(1, 5) * 0.2)  # Random delay
            await callback.onOrder(info=f"inserted {i}x{orderId}={i*orderId}")  # Send a random result

    async def reqCancel(self, orderId, _context, **kwargs):
        print(kwargs)
        callback = kwargs["cb"]
        for i in range(3):  # Random number of responses
            await asyncio.sleep(random.randint(1, 5) * 0.2)  # Random delay
            print(f"server get cancel orderid: {i}")
            await callback.onCancel(info=f"canceled {i}x{orderId}={i*orderId}")  # Send a random result


async def new_connection(stream):
    await capnp.TwoPartyServer(stream, bootstrap=TradeApiImpl()).on_disconnect()


async def main():
    server = await capnp.AsyncIoStream.create_server(new_connection, "127.0.0.1", "8888")
    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(capnp.run(main()))
```

```py
# client.py
import asyncio
import capnp
import trade_capnp


class CallbackImpl(trade_capnp.TradeSpi.Server):
    async def onOrder(self, info, **kwargs):
        print(f"Callback of onOrder: {info}")

    async def onCancel(self, info, **kwargs):
        print(f"Callback of onCancel: {info}")


async def main(connection):
    client = capnp.TwoPartyClient(connection)
    trader = client.bootstrap().cast_as(trade_capnp.TradeApi)
    callback = CallbackImpl()

    # async Send 3 orders
    order_tasks = [trader.reqOrder(10000 * i, cb=callback) for i in range(3)]
    await asyncio.gather(*order_tasks)
    # sleep
    await asyncio.sleep(5)
    # async Cancel 3 orders
    cancel_tasks = [trader.reqCancel(10000 * i, cb=callback) for i in range(3)]
    await asyncio.gather(*cancel_tasks)

    # # sync Send 3 orders
    # for i in range(3):
    #     print(f"send orderid: {10000*i}")
    #     await trader.reqOrder(10000 * i, cb=callback)

    # sync Send 1 order
    # await trader.reqOrder(100235, cb=callback)
    # await trader.reqCancel(100235, cb=callback)


async def cmd_main():
    await main(await capnp.AsyncIoStream.create_connection(host="127.0.0.1", port="8888"))


if __name__ == "__main__":
    asyncio.run(capnp.run(cmd_main()))
```

## RPC by capnp in C++

- `vcpkg install capnproto`
- download [capnp tools](https://capnproto.org/capnp-tool.html), `capnp compile -oc++ trade.capnp`

```bash
.
├── CMakeLists.txt
├── client.cpp
├── server.cpp
├── trade.capnp
├── trade.capnp.c++ # generated
└── trade.capnp.h # generated
```

[]

```capnp
# trade.capnp
@0x85150b117366d14b;

interface TradeSpi {
  onOrder @0 (info: Text) -> ();
  onCancel @1 (info: Text) -> ();
}

interface TradeApi {
    reqOrder @0 (orderId:Int64, cb: TradeSpi) -> ();
    reqCancel @1 (orderId:Int64, cb: TradeSpi) -> ();
}
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.24.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(s2 server.cpp trade.capnp.c++)
add_executable(c2 client.cpp trade.capnp.c++)

# this is heuristically generated, and may not be correct
find_package(CapnProto CONFIG REQUIRED)
# note: 6 additional targets are not displayed.
target_link_libraries(s2 PRIVATE CapnProto::kj CapnProto::capnp CapnProto::capnp-rpc)
target_link_libraries(c2 PRIVATE CapnProto::kj CapnProto::capnp CapnProto::capnp-rpc)
```

```cpp
// client.py
#include <capnp/ez-rpc.h>

#include <iostream>

#include "trade.capnp.h"

class TradeSpiImpl : public TradeSpi::Server {
    ::kj::Promise<void> onOrder(OnOrderContext context) override {
        auto result = context.getParams().getInfo();
        std::cout << "onOrder: " << result.cStr() << '\n';
        return kj::READY_NOW;
    }

    ::kj::Promise<void> onCancel(OnCancelContext context) override {
        auto result = context.getParams().getInfo();
        std::cout << "onCancel: " << result.cStr() << '\n';
        return kj::READY_NOW;
    }
};

int main(int argc, char const *argv[]) {
    capnp::EzRpcClient client{"localhost:8888"};
    auto trader = client.getMain<TradeApi>();

    kj::Vector<kj::Promise<void>> promises;
    promises.reserve(40000);

    for (size_t i = 0; i < 20000; ++i) {
        auto req = trader.reqOrderRequest();
        req.setCb(kj::heap<TradeSpiImpl>());
        req.setOrderId(10000 + i);
        // req.send().wait(client.getWaitScope());
        promises.add(req.send().ignoreResult());
    }
    for (size_t i = 0; i < 20000; ++i) {
        auto req = trader.reqCancelRequest();
        req.setCb(kj::heap<TradeSpiImpl>());
        req.setOrderId(10000 + i);
        // req.send().wait(client.getWaitScope());
        promises.add(req.send().ignoreResult());
    }
    for (auto &&e : promises) {
        e.wait(client.getWaitScope());
    }
}
```

```cpp
// server.cpp
#include <capnp/ez-rpc.h>

#include <format>
#include <iostream>

#include "trade.capnp.h"

class TradeApiImpl : public TradeApi::Server {
   public:
    ::kj::Promise<void> reqOrder(ReqOrderContext context) override {
        auto orderId = context.getParams().getOrderId();
        auto callback = context.getParams().getCb();

        std::cout << "orderId=" << orderId << '\n';
        for (size_t i = 0; i < 3; ++i) {
            auto resp = callback.onOrderRequest();
            auto s = std::format("idx={}, oid={}", i, orderId);
            auto value = capnp::Text::Reader(s);
            resp.setInfo(value);
            resp.sendForPipeline();
        }
        return kj::READY_NOW;
    }

    ::kj::Promise<void> reqCancel(ReqCancelContext context) override {
        auto orderId = context.getParams().getOrderId();
        auto callback = context.getParams().getCb();

        std::cout << "cancel orderId=" << orderId << '\n';
        auto resp = callback.onCancelRequest();
        auto value = capnp::Text::Reader(std::format("cid={}", orderId));
        resp.setInfo(value);
        return resp.send().ignoreResult();
    }
};

int main(int argc, char const* argv[]) {
    capnp::EzRpcServer server{kj::heap<TradeApiImpl>(), "localhost:8888"};
    kj::NEVER_DONE.wait(server.getWaitScope());
}
```

### pingpong benchmark

```capnp
# pingpong.capnp
@0x98b1e5d1836a2beb;

interface PingPong {
  ping @0 (content :Int64) -> (reply :Int64);
}
```

> `capnp compile -oc++ pingpong.capnp`

```cmake
cmake_minimum_required(VERSION 3.12.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(server server.cpp pingpong.capnp.c++)
add_executable(client client.cpp pingpong.capnp.c++)

# this is heuristically generated, and may not be correct
find_package(CapnProto CONFIG REQUIRED)
# note: 6 additional targets are not displayed.
target_link_libraries(server PRIVATE CapnProto::kj CapnProto::capnp CapnProto::capnp-rpc)
target_link_libraries(client PRIVATE CapnProto::kj CapnProto::capnp CapnProto::capnp-rpc)
```

```cpp
// server.cpp
#include <capnp/ez-rpc.h>
#include <cstdio>
#include "pingpong.capnp.h"

class PingPongImpl final : public PingPong::Server {
   public:
    kj::Promise<void> ping(PingContext context) override {
        auto content = context.getParams().getContent();
        context.getResults().setReply(content);
        return kj::READY_NOW;
    }
};

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        printf("usage: %s ADDRESS[:PORT]\n", argv[0]);
        printf("Runs the server bound to the given address/port.\n");
        printf("ADDRESS may be '*' to bind to all local addresses.\n");
        printf(":PORT may be omitted to choose a port automatically.\n");
        printf("ADDRESS may be UNIX domain socket unix:/tmp/xxx or unix-abstract:xxx\n");
        return 1;
    }
    capnp::EzRpcServer server(kj::heap<PingPongImpl>(), argv[1]);
    auto& waitScope = server.getWaitScope();
    auto port = server.getPort().wait(waitScope);
    if (port == 0) {
        printf("Listening on Unix domain socket...\n");
    } else {
        printf("Listening on port %d...\n", port);
    }
    kj::NEVER_DONE.wait(waitScope);
}
```

```cpp
// client.cpp
#include <capnp/ez-rpc.h>
#include <chrono>
#include <cstdio>
#include <string>
#include "pingpong.capnp.h"

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        printf("usage: %s HOST:PORT ROUNDS\n", argv[0]);
        return 1;
    }

    capnp::EzRpcClient client(argv[1]);
    auto pingPong = client.getMain<PingPong>();
    auto& waitScope = client.getWaitScope();

    auto N = std::stol(argv[2]);
    size_t total = 0;
    for (size_t i = 0; i < N; ++i) {
        auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto request = pingPong.pingRequest();
        request.setContent(startTime);
        auto response = request.send().wait(waitScope);
        auto endTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto nanoseconds = endTime - response.getReply();
        total += nanoseconds;
    }

    printf("round: %lu, costs %f ns\n", N, total * 1.0 / N);
}
```

## yaLanTingLibs rpc

### yaLanTingLibs pingpong rpc

```bash
.
├── CMakeLists.txt
├── rpc_service.hpp
├── client.cpp
└── server.cpp
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.24.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(server server.cpp)
add_executable(client client.cpp)

include(FetchContent)

FetchContent_Declare(
    yalantinglibs
    GIT_REPOSITORY https://github.com/JYLeeLYJ/yalantinglibs.git
    GIT_TAG feat/fetch # optional ( default master / main )
    GIT_SHALLOW 1 # optional ( --depth=1 )
)

FetchContent_MakeAvailable(yalantinglibs)

target_link_libraries(server yalantinglibs::yalantinglibs)
target_link_libraries(client yalantinglibs::yalantinglibs)
```

```cpp
// rpc_service.hpp
#include <cstdint>

inline int64_t bound(int64_t value) {
    return value;
}
```

```cpp
// client.cpp
#include <chrono>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include "rpc_service.hpp"

async_simple::coro::Lazy<void> test_client(const char* addr, size_t rounds) {
    coro_rpc::coro_rpc_client client;
    co_await client.connect(addr);
    size_t total = 0;
    for (size_t i = 0; i < rounds; ++i) {
        auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto r = co_await client.call<bound>(startTime);  // 传参数调用rpc函数
        auto endTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        total += endTime - r.value();
    }
    printf("rounds=%lu, costs=%f ns\n", rounds, total * 1.0 / rounds);
}

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        printf("usage: %s HOST:PORT ROUNDS\n", argv[0]);
        return 1;
    }
    auto N = std::stol(argv[2]);
    syncAwait(test_client(argv[1], N));
}
```

```cpp
// server.cpp
#include <ylt/coro_rpc/coro_rpc_server.hpp>
#include <ylt/easylog.hpp>

#include "rpc_service.hpp"

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        printf("usage: %s PORT\n", argv[0]);
        return 1;
    }
    uint16_t port = std::stoi(argv[1]);

    easylog::set_console(false);  // turn off log
    coro_rpc::coro_rpc_server server{/*thread_num =*/16, /*port =*/port};
    server.register_handler<bound>();  // register RPC function

    printf("listening on port %d...\n", server.port());
    server.start();
}
```