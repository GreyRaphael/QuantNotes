# Remote Procedure Call

- [Remote Procedure Call](#remote-procedure-call)
  - [RPC by pycapnp](#rpc-by-pycapnp)
    - [simple invoke](#simple-invoke)
    - [callback invoke](#callback-invoke)
    - [sync and async order system](#sync-and-async-order-system)
  - [RPC by capnp in C++](#rpc-by-capnp-in-c)


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
- download [capnp tools](https://capnproto.org/capnp-tool.html), `capnp compile -oc++ .\trade.capnp`

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