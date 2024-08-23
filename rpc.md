# Remote Procedure Call

- [Remote Procedure Call](#remote-procedure-call)
  - [RPC by pycapnp](#rpc-by-pycapnp)
    - [simple invoke](#simple-invoke)
    - [callback invoke](#callback-invoke)
    - [sync and async order system](#sync-and-async-order-system)
  - [RPC by capnp in C++](#rpc-by-capnp-in-c)
    - [pingpong benchmark](#pingpong-benchmark)
  - [RPC by capnp in Rust](#rpc-by-capnp-in-rust)
  - [yaLanTingLibs rpc](#yalantinglibs-rpc)
    - [yaLanTingLibs pingpong rpc](#yalantinglibs-pingpong-rpc)
    - [yaLanTingLibs pingpong websocket](#yalantinglibs-pingpong-websocket)
      - [websocket mock rpc](#websocket-mock-rpc)


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

benchmark in Beelink
- round: 10000, costs 53489.530000 ns
- round: 100000, costs 47885.851000 ns
- round: 1000000, costs 47756.287300 ns


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

benchmark in Aliyun, for `*:8888` and `unix:/tmp/xxx`
- `*:8888`: round: 100000, costs 110561.675040 ns
- unix domain socket `unix:/tmp/xxx`: round: 100000, costs 68295.661150 ns
- abstract Unix domain socke `unix-abstract:yyy`: round: 100000, costs 68205.324150 ns

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

## RPC by capnp in Rust

benchmark in beelink:
- round=10000, avg costs=54498.02 ns
- round=100000, avg costs=47638.888 ns
- round=1000000, avg costs=47308.8719 ns

```bash
.
├── Cargo.toml
└── src
    ├── hello_world.capnp
    ├── hello_world_capnp.rs # generated
    ├── main.rs
    ├── client.rs
    └── server.rs
```

```toml
[package]
name = "proj_cap"
version = "0.1.0"
edition = "2021"

[dependencies]
capnp = "0.19"
capnp-rpc = "0.19"
futures = "0.3"
tokio = { version = "1", features = ["net", "rt", "macros", "rt-multi-thread"] }
tokio-util = { version = "0.7", features = ["compat"] }
```

> `capnp compile -orust hello_world.capnp`

```capnp
# hello_world.capnp
@0x9663f4dd604afa35;

interface HelloWorld {
    sayHello @0 (request: Int64) -> (reply: Int64);
}
```

```rs
// main.rs
pub mod hello_world_capnp;
pub mod client;
pub mod server;

// #[tokio::main(flavor = "current_thread")]
#[tokio::main(flavor = "multi_thread", worker_threads = 4)]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = ::std::env::args().collect();
    if args.len() >= 2 {
        match &args[1][..] {
            "client" => return client::main().await,
            "server" => return server::main().await,
            _ => (),
        }
    }

    println!("usage: {} [client | server] ADDRESS", args[0]);
    Ok(())
}
```

```rs
// client.rs
use crate::hello_world_capnp::hello_world;
use capnp_rpc::{rpc_twoparty_capnp, twoparty, RpcSystem};
use std::net::ToSocketAddrs;

use futures::AsyncReadExt;

pub async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = ::std::env::args().collect();
    if args.len() != 4 {
        println!("usage: {} client HOST:PORT ROUND", args[0]);
        return Ok(());
    }

    let addr = args[2]
        .to_socket_addrs()?
        .next()
        .expect("could not parse address");

    let num = args[3].parse::<i64>()?;

    tokio::task::LocalSet::new()
        .run_until(async move {
            let stream = tokio::net::TcpStream::connect(&addr).await?;
            stream.set_nodelay(true)?;
            let (reader, writer) =
                tokio_util::compat::TokioAsyncReadCompatExt::compat(stream).split();
            let rpc_network = Box::new(twoparty::VatNetwork::new(
                futures::io::BufReader::new(reader),
                futures::io::BufWriter::new(writer),
                rpc_twoparty_capnp::Side::Client,
                Default::default(),
            ));
            let mut rpc_system = RpcSystem::new(rpc_network, None);
            let hello_world: hello_world::Client =
                rpc_system.bootstrap(rpc_twoparty_capnp::Side::Server);

            tokio::task::spawn_local(rpc_system);

            let mut total: u128 = 0;

            for i in 0..num {
                let start=std::time::Instant::now();

                let mut request = hello_world.say_hello_request();
                request.get().set_request(i);
                let _reply = request.send().promise.await?;

                total += start.elapsed().as_nanos();
            }
            println!("round={}, avg costs={}", num, (total as f64) / (num as f64));

            Ok(())
        })
        .await
}
```

```rs
// server.rs
use capnp::capability::Promise;
use capnp_rpc::{rpc_twoparty_capnp, twoparty, RpcSystem};

use crate::hello_world_capnp::hello_world;

use futures::AsyncReadExt;
use std::net::ToSocketAddrs;

struct HelloWorldImpl;

impl hello_world::Server for HelloWorldImpl {
    fn say_hello(
        &mut self,
        params: hello_world::SayHelloParams,
        mut results: hello_world::SayHelloResults,
    ) -> Promise<(), ::capnp::Error> {
        let value = params.get().expect("not").get_request();
        results.get().set_reply(value);

        Promise::ok(())
    }
}

pub async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = ::std::env::args().collect();
    if args.len() != 3 {
        println!("usage: {} server ADDRESS[:PORT]", args[0]);
        return Ok(());
    }

    let addr = args[2]
        .to_socket_addrs()?
        .next()
        .expect("could not parse address");

    tokio::task::LocalSet::new()
        .run_until(async move {
            let listener = tokio::net::TcpListener::bind(&addr).await?;
            let hello_world_client: hello_world::Client = capnp_rpc::new_client(HelloWorldImpl);

            loop {
                let (stream, _) = listener.accept().await?;
                stream.set_nodelay(true)?;
                let (reader, writer) =
                    tokio_util::compat::TokioAsyncReadCompatExt::compat(stream).split();
                let network = twoparty::VatNetwork::new(
                    futures::io::BufReader::new(reader),
                    futures::io::BufWriter::new(writer),
                    rpc_twoparty_capnp::Side::Server,
                    Default::default(),
                );

                let rpc_system =
                    RpcSystem::new(Box::new(network), Some(hello_world_client.clone().client));

                tokio::task::spawn_local(rpc_system);
            }
        })
        .await
}
```

## yaLanTingLibs rpc

### yaLanTingLibs pingpong rpc

benchmark in Beelink, better than capnproto
> build with **Release**, then the log info disappeared
- server thread_nums= 1, rounds=1000000, costs=31345.727900 ns
- server thread_nums= 2, rounds=1000000, costs=29942.434900 ns 
- server thread_nums= 4, rounds=1000000, costs=30012.773400 ns
- server thread_nums= 8, rounds=1000000, costs=30086.524900 ns
- server thread_nums=16, rounds=1000000, costs=29847.370900 ns

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
    GIT_REPOSITORY https://github.com/alibaba/yalantinglibs.git
    GIT_TAG main
    GIT_SHALLOW 1
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
#include "rpc_service.hpp"

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        printf("usage: %s PORT THREAD_NUMS\n", argv[0]);
        return 1;
    }
    unsigned short port = std::stoi(argv[1]);
    unsigned thread_nums = std::stoi(argv[2]);

    coro_rpc::coro_rpc_server server{thread_nums, port};
    server.register_handler<bound>();  // register RPC function

    server.start();
}
```

### yaLanTingLibs pingpong websocket

benchmark in Beelink, better than capnproto
> build with **Release**, then the log info disappeared
- server thread_nums= 1, round=1000000, costs=31539.561100 ns
- server thread_nums= 2, round=1000000, costs=31522.231700 ns 
- server thread_nums= 4, round=1000000, costs=30851.884200 ns
- server thread_nums= 8, round=1000000, costs=30849.733100 ns
- server thread_nums=16, round=1000000, costs=31272.926000 ns

```bash
.
├── CMakeLists.txt
├── client.cpp
└── server.cpp
```

```cpp
// server.cpp
#include <format>
#include <iostream>
#include <string>
#include <ylt/coro_http/coro_http_server.hpp>

using namespace coro_http;

async_simple::coro::Lazy<void> handle_websocket(coro_http_request &req, coro_http_response &resp) {
    assert(req.get_content_type() == content_type::websocket);
    websocket_result result{};

    while (true) {
        result = co_await req.get_conn()->read_websocket();
        if (result.ec) {
            break;
        }

        if (result.type == ws_frame_type::WS_CLOSE_FRAME) {
            std::cout << "close frame\n";
            break;
        }

        if (result.type == ws_frame_type::WS_TEXT_FRAME || result.type == ws_frame_type::WS_BINARY_FRAME) {
            // std::cout << result.data << "\n";
        } else if (result.type == ws_frame_type::WS_PING_FRAME || result.type == ws_frame_type::WS_PONG_FRAME) {
            continue;
        } else {
            break;
        }

        auto ec = co_await req.get_conn()->write_websocket(result.data);
        if (ec) {
            break;
        }
        // // send twice
        // ec = co_await req.get_conn()->write_websocket(result.data);
        // if (ec) {
        //     break;
        // }
    }
}

int main(int argc, const char *argv[]) {
    if (argc != 3) {
        printf("usage: %s ADDRESS THREAD_NUMS\n", argv[0]);
        return 1;
    }
    auto thread_num = std::stol(argv[2]);
    coro_http_server server(/*thread_num*/ thread_num, /*address*/ argv[1], /*cpu_affinity*/ true);
    server.set_http_handler<GET>("/ws_echo", handle_websocket);
    auto r = server.sync_start();
    std::cout << std::format("start server error, msg={}\n", r.message());
}
```

```cpp
// client.cpp
#include <format>
#include <span>
#include <ylt/coro_http/coro_http_client.hpp>
#include <ylt/easylog.hpp>

using namespace coro_http;

async_simple::coro::Lazy<void> use_websocket(const char* address, size_t N) {
    coro_http_client client{};
    auto r = co_await client.connect(std::format("ws://{}/ws_echo", address));
    if (r.net_err) {
        ELOGFMT(ERROR, "net error {}", r.net_err.message());
        co_return;
    }

    long total = 0;
    for (size_t i = 0; i < N; ++i) {
        auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        ELOGFMT(INFO, "sending--->{}", start);
        auto view = std::span<char>(reinterpret_cast<char*>(&start), sizeof(long));
        co_await client.write_websocket(view, opcode::binary);
        auto resp = co_await client.read_websocket();
        auto result = reinterpret_cast<long const*>(resp.resp_body.data());
        total += std::chrono::high_resolution_clock::now().time_since_epoch().count() - *result;
        ELOGFMT(INFO, "receive===>{}", *result);
    }
    printf("round=%lu, costs=%f\n", N, total * 1.0 / N);
}

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        printf("usage: %s ADDRESS ROUNDS\n", argv[0]);
        return 1;
    }
    auto rounds = std::stol(argv[2]);
    async_simple::coro::syncAwait(use_websocket(argv[1], rounds));
}
```

#### websocket mock rpc

```cpp
// client.cpp
#include <format>
#include <future>
#include <span>
#include <ylt/coro_http/coro_http_client.hpp>
#include <ylt/easylog.hpp>

#include "async_simple/coro/SyncAwait.h"

using namespace coro_http;

class Client {
   public:
    Client(const char* address) : address_(address) {}

    async_simple::coro::Lazy<void> connect() {
        auto addr = std::format("ws://{}/ws_echo", address_);
        auto r = co_await client_.connect(addr);
        if (r.net_err) {
            ELOGFMT(ERROR, "net error {}", r.net_err.message());
            co_return;
        }
        ELOGFMT(INFO, "connected to {}", addr);
    }

    async_simple::coro::Lazy<bool> request(long value) {
        auto view = std::span<char>(reinterpret_cast<char*>(&value), sizeof(long));
        co_await client_.write_websocket(view, opcode::binary);
        co_return true;
    }

    async_simple::coro::Lazy<void> listen() {
        ELOGFMT(INFO, "listening");
        while (true) {
            auto resp = co_await client_.read_websocket();
            if (resp.net_err) {
                ELOGFMT(ERROR, "net error {}", resp.net_err.message());
                break;
            }
            auto result = reinterpret_cast<long const*>(resp.resp_body.data());
            onResponse(*result);
        }
    }

    void onResponse(long value) {
        ELOGFMT(INFO, "receive===>{}", value);
    }

   private:
    coro_http_client client_;
    const char* address_;
};

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        printf("usage: %s ADDRESS ROUNDS\n", argv[0]);
        return 1;
    }
    auto rounds = std::stol(argv[2]);
    Client client(argv[1]);

    async_simple::coro::syncAwait(client.connect());

    // Run listen in a separate background task
    auto listen_task = std::async(std::launch::async, [&client] {
        async_simple::coro::syncAwait(client.listen());
    });

    for (size_t i = 0; i < rounds; ++i) {
        auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        ELOGFMT(INFO, "sending--->{}", start);
        async_simple::coro::syncAwait(client.request(start));
    }

    // Wait for the listen task to complete (if needed)
    listen_task.wait();

    return 0;
}
```