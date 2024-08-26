# Websocket

- [Websocket](#websocket)
  - [python websocket](#python-websocket)
    - [python async basic](#python-async-basic)
    - [py websocket server](#py-websocket-server)
    - [py websocket client](#py-websocket-client)
  - [go websocket client](#go-websocket-client)
  - [cpp websocket](#cpp-websocket)
    - [beast websocket client](#beast-websocket-client)
    - [async\_simple websocket client](#async_simple-websocket-client)

## python websocket

> `pip install websockets`

[websockets documentation](https://websockets.readthedocs.io/en/stable/index.html)

### python async basic

- `await` must be in `async` function
- `asyncio.run` is used to run the asyncio event loop
- `await` will block the current coroutine until the task is completed, and then resume the coroutine
- If many coroutines are running, the event loop will automatically switch between them.

`asyncio.create_task` vs `asyncio.gather`
- Use `asyncio.create_task` when you want to start **a single coroutine** as a task that can run *independently*. 
- Use `asyncio.gather` when you need to handle **multiple coroutines concurrently** and **collect their results**.

```py
import asyncio

async def my_coroutine():
    print("Task started")
    await asyncio.sleep(1)
    return "Task completed"

async def main():
    task = my_coroutine() # not go to background
    # do something else ...
    result = await task
    print(result)

asyncio.run(main())
```

```py
import asyncio

async def my_coroutine():
    print("Task started")
    await asyncio.sleep(1)
    return "Task completed"

async def main():
    task = asyncio.create_task(my_coroutine()) # go to background
    # do something else ...
    result = await task
    print(result)

asyncio.run(main())
```

```py
import asyncio

async def my_coroutine():
    print("Task started")
    await asyncio.sleep(1)
    return "Task completed"

async def main():
    task = asyncio.gather(my_coroutine())  # go to background
    # do something else ...
    result = await task
    print(result)

asyncio.run(main())
```

```py
import asyncio

async def task1():
    await asyncio.sleep(1)
    return "Result of task 1"

async def task2():
    await asyncio.sleep(2)
    return "Result of task 2"

async def main():
    results = await asyncio.gather(task1(), task2())
    for result in results:
        print(result)

asyncio.run(main())
```

```py
import asyncio

async def task1():
    await asyncio.sleep(1)
    return "Result of task 1"

async def task2():
    await asyncio.sleep(2)
    return "Result of task 2"

async def main():
    tasks = [asyncio.create_task(task1()), asyncio.create_task(task2())]
    # do something else ...
    results = await asyncio.gather(*tasks)
    for result in results:
        print(result)

asyncio.run(main())
```

### py websocket server

example server1
- 多个客户端之间不影响，server可以并发处理多个客户端的消息
- 单个客户端发多个消息，server将前面的消息处理完，才能处理后面的消息

```py
import asyncio
from websockets.asyncio.server import serve
from random import randint

async def echo(websocket):
    print("invoke start")
    i = 0
    async for message in websocket: # websocket.recv() invoked in loop
        print("receive:", message)
        t = randint(1, 5)
        print(f"{message} go to sleep {t} s")
        await asyncio.sleep(t)
        response = f"output{i:02d}"
        await websocket.send(response)
        print("response:", response)
        i += 1
    print("invoke end")

async def main():
    await serve(echo, "localhost", 8888)
    await asyncio.get_running_loop().create_future()  # run forever


if __name__ == "__main__":
    asyncio.run(main())
```

examle server2
- 单个客户端发多个消息，server可以并发处理这些消息

```py
import asyncio
from websockets.asyncio.server import serve
from random import randint

async def handle_message(websocket, message, i):
    print("receive:", message)
    t = randint(1, 5)
    print(f"{message} go to sleep {t} s")
    await asyncio.sleep(t)
    response = f"output{i:02d}"
    await websocket.send(response)
    print("response:", response)


async def echo(websocket):
    print("invoke start")
    i = 0
    tasks = []
    async for message in websocket:
        task = asyncio.create_task(handle_message(websocket, message, i))
        tasks.append(task)
        i += 1
    await asyncio.gather(*tasks)
    print("invoke end")

async def main():
    await serve(echo, "localhost", 8888)
    await asyncio.get_running_loop().create_future()  # run forever


if __name__ == "__main__":
    asyncio.run(main())
```

### py websocket client

sync client

```py
from websockets.sync.client import connect

def hello():
    with connect("ws://localhost:8888/ws_echo") as websocket:
        for i in range(3):
            websocket.send(f"record-{i}")

        for i in range(3):
            message = websocket.recv()
            print(f"Received: {message}")


if __name__ == "__main__":
    hello()
```

async client

```py
import asyncio
from websockets.asyncio.client import connect


async def send_message(websocket, msg: str):
    await websocket.send(msg)


async def receive_messages(websocket):
    while True:
        message = await asyncio.wait_for(websocket.recv(), timeout=10)
        # message = await websocket.recv()  # default timeout 20s
        print(message)


async def hello():
    async with connect("ws://localhost:8888/ws_echo") as websocket:
        # Create a task for receiving messages
        receive_task = asyncio.create_task(receive_messages(websocket))

        for i in range(3):
            await send_message(websocket, f"input{i:02d}")
        print("finish all sending")

        # Wait for the receive task to complete
        await receive_task


async def hello_equivalent():
    # same as hello()
    websocket = await connect("ws://localhost:8888/ws_echo")
    # ......
    await websocket.close()


if __name__ == "__main__":
    asyncio.run(hello())
    asyncio.run(hello_equivalent())  # will never be invoked, because `await while True`
```

optimize async client

```py
import asyncio
from websockets.asyncio.client import connect

async def send_message(websocket):
    i = 0
    while True:
        await websocket.send(f"{i:02d}")
        await asyncio.sleep(2)
        i += 1

async def receive_messages(websocket):
    while True:
        message = await asyncio.wait_for(websocket.recv(), timeout=10)
        print(message)

# not working, just blocking
async def hello1():
    async with connect("ws://localhost:8888/ws_echo") as websocket:
        recv_coro = receive_messages(websocket)
        send_coro = send_message(websocket)

        # 以为while True await卡在了第一个recv_coro上
        await recv_coro
        await send_coro

# working
async def hello2():
    async with connect("ws://localhost:8888/ws_echo") as websocket:
        recv_task = asyncio.create_task(receive_messages(websocket))
        send_task = asyncio.create_task(send_message(websocket))

        # 并发执行
        await recv_task
        await send_task

# working
async def hello3():
    async with connect("ws://localhost:8888/ws_echo") as websocket:
        tasks = asyncio.gather(receive_messages(websocket), send_message(websocket))

        # 并发执行
        await tasks  # gather is good for results

if __name__ == "__main__":
    # asyncio.run(hello1()) # not working
    asyncio.run(hello2()) # good
    # asyncio.run(hello3()) # good
```

## go websocket client

```go
package main

import (
	"encoding/binary"
	"flag"
	"log"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

func main() {
	// Command-line arguments for iteration control
	NUM := flag.Int64("n", 10, "Number of messages to send")
	flag.Parse()

	c, _, err := websocket.DefaultDialer.Dial("ws://localhost:8888/ws_echo", nil)
	if err != nil {
		log.Fatal("dial:", err)
	}
	defer c.Close()

	// WaitGroup to sync completion of goroutines
	var wg sync.WaitGroup
	wg.Add(2)

	// Reading Goroutine
	go func() {
		defer wg.Done()
		var total int64
		for i := int64(0); i < *NUM; i++ {
			_, message, err := c.ReadMessage()
			if err != nil {
				log.Println("read:", err)
				return
			}
			end := time.Now().UnixNano()
			value := int64(binary.LittleEndian.Uint64(message))
			total += end - value
			log.Printf("recv: %d at %d, diff=%d", value, time.Now().UnixNano(), end-value)
		}
		log.Printf("round=%d, avg costs=%d\n", *NUM, total/(*NUM))
	}()

	// Writing Goroutine with controlled iterations
	go func() {
		defer wg.Done()
		for i := int64(0); i < *NUM; i++ {
			start := time.Now().UnixNano()
			byteArray := make([]byte, 8)
			binary.LittleEndian.PutUint64(byteArray, uint64(start))
			err := c.WriteMessage(websocket.BinaryMessage, byteArray)
			log.Printf("send: %d", start)
			if err != nil {
				log.Println("write:", err)
				return
			}
			time.Sleep(time.Millisecond) // simulate some delay
		}
	}()

	wg.Wait() // Wait for all goroutines to complete
}
```

## cpp websocket

### beast websocket client

```cpp
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <span>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// Coroutine for reading messages from the WebSocket
net::awaitable<void> read_websocket(websocket::stream<beast::tcp_stream>& ws, int count) {
    try {
        ws.binary(true);
        long total = 0;
        for (int i = 0; i < count; ++i) {
            beast::flat_buffer buffer;
            co_await ws.async_read(buffer, net::use_awaitable);
            auto result = reinterpret_cast<long const*>(buffer.data().data());
            auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            printf("receive %lu at %lu, diff=%lu\n", *result, end, end - *result);
            total += end - *result;
        }
        printf("round=%d, costs=%lu\n", count, total / count);
    } catch (std::exception& e) {
        std::cerr << "Read error: " << e.what() << std::endl;
    }
}

// Coroutine for writing messages to the WebSocket
net::awaitable<void> write_websocket(websocket::stream<beast::tcp_stream>& ws, int count) {
    try {
        ws.binary(true);
        auto exec = co_await net::this_coro::executor;
        net::steady_timer timer{exec};
        for (int i = 0; i < count; ++i) {
            auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            auto view = std::span<unsigned char>(reinterpret_cast<unsigned char*>(&start), sizeof(long));
            printf("send %lu\n", start);
            co_await ws.async_write(net::buffer(view.data(), view.size()), net::use_awaitable);
            timer.expires_after(std::chrono::microseconds(100));
            co_await timer.async_wait(net::use_awaitable);
        }
    } catch (std::exception& e) {
        std::cerr << "Write error: " << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <count> <message>\n";
            return 1;
        }

        int count = std::stoi(argv[1]);

        net::io_context ioc;
        tcp::resolver resolver(ioc);
        auto endpoints = resolver.resolve("localhost", "8888");
        websocket::stream<beast::tcp_stream> ws(ioc);

        // Properly connect using the tcp_stream
        auto const& endpoint = *endpoints.begin();
        ws.next_layer().connect(endpoint);  // Synchronous connect, suitable for simplicity

        ws.handshake("localhost:8888", "/ws_echo");

        // Launch coroutines
        net::co_spawn(ioc, read_websocket(ws, count), net::detached);
        net::co_spawn(ioc, write_websocket(ws, count), net::detached);

        ioc.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

### async_simple websocket client

```cpp
#include <async_simple/executors/SimpleExecutor.h>

#include <chrono>
#include <cstdio>
#include <format>
#include <span>
#include <ylt/coro_http/coro_http_client.hpp>
#include <ylt/easylog.hpp>

#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Sleep.h"
#include "cinatra/coro_http_client.hpp"

using namespace coro_http;

async_simple::coro::Lazy<void> send_loop(coro_http_client& client, long N) {
    for (size_t i = 0; i < N; ++i) {
        auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        ELOGFMT(WARN, "send {}", start);
        auto view = std::span<char>(reinterpret_cast<char*>(&start), sizeof(long));
        co_await client.write_websocket(view, opcode::binary);
        co_await async_simple::coro::sleep(std::chrono::microseconds(100));
    }
}

async_simple::coro::Lazy<void> recv_loop(coro_http_client& client, long N) {
    long total = 0;
    for (size_t i = 0; i < N; ++i) {
        auto resp = co_await client.read_websocket();
        auto result = reinterpret_cast<long const*>(resp.resp_body.data());
        auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        ELOGFMT(WARN, "recv {} at {}, diff={}", *result, end, end - *result);
        total += end - *result;
    }
    ELOGFMT(WARN, "round={}, costs={}", N, total / N);
}

async_simple::coro::Lazy<void> connect(coro_http_client& client, const char* address) {
    auto r = co_await client.connect(std::format("ws://{}/ws_echo", address));
    if (r.net_err) {
        ELOGFMT(ERROR, "net error {}", r.net_err.message());
        co_return;
    }
}

async_simple::coro::Lazy<void> sendrecv(coro_http_client& client, long N) {
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
    ELOGFMT(WARN, "round={}, costs={}", N, total / N);
}

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        printf("usage: %s ADDRESS ROUNDS\n", argv[0]);
        return 1;
    }
    auto rounds = std::stol(argv[2]);

    coro_http_client client;
    async_simple::coro::syncAwait(connect(client, argv[1]));
    // async_simple::coro::syncAwait(sendrecv(client, rounds));

    async_simple::executors::SimpleExecutor exec{8};
    send_loop(client, rounds).via(&exec).start([](auto&&) {});
    recv_loop(client, rounds).via(&exec).start([](auto&&) {});
    getchar();
}
```