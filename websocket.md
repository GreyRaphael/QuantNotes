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
    - [websocket by nng](#websocket-by-nng)

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

> benchmark in beelink: round=1000000, avg costs=92856

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

> benchmark in beelink: round=1000000, costs=66881

```cpp
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <chrono>
#include <cstdio>
#include <span>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// Coroutine for reading messages from the WebSocket
net::awaitable<void> read_websocket(websocket::stream<beast::tcp_stream>& ws, int count) {
    long total = 0;
    for (int i = 0; i < count; ++i) {
        beast::flat_buffer buffer;
        co_await ws.async_read(buffer, net::use_awaitable);
        auto result = reinterpret_cast<long const*>(buffer.data().data());
        auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        // printf("receive %lu at %lu, diff=%lu\n", *result, end, end - *result);
        total += end - *result;
    }
    printf("round=%d, costs=%lu\n", count, total / count);
}

// Coroutine for writing messages to the WebSocket
net::awaitable<void> write_websocket(websocket::stream<beast::tcp_stream>& ws, int count) {
    auto exec = co_await net::this_coro::executor;
    net::steady_timer timer{exec};
    for (int i = 0; i < count; ++i) {
        auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto view = std::span<unsigned char>(reinterpret_cast<unsigned char*>(&start), sizeof(long));
        // printf("send %lu\n", start);
        co_await ws.async_write(net::buffer(view.data(), view.size()), net::use_awaitable);
        timer.expires_after(std::chrono::microseconds(100));
        co_await timer.async_wait(net::use_awaitable);
    }
}

net::awaitable<void> rw_websocket(websocket::stream<beast::tcp_stream>& ws, int count) {
    long total = 0;
    for (int i = 0; i < count; ++i) {
        beast::flat_buffer buffer;
        auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto view = std::span<unsigned char>(reinterpret_cast<unsigned char*>(&start), sizeof(long));
        // printf("send %lu\n", start);
        co_await ws.async_write(net::buffer(view.data(), view.size()), net::use_awaitable);
        co_await ws.async_read(buffer, net::use_awaitable);
        auto result = reinterpret_cast<long const*>(buffer.data().data());
        auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        // printf("receive %lu at %lu, diff=%lu\n", *result, end, end - *result);
        total += end - *result;
    }
    printf("round=%d, costs=%lu\n", count, total / count);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("usage: %s rounds", argv[0]);
        return 1;
    }
    int rounds = std::stoi(argv[1]);

    net::io_context ioc;
    tcp::resolver resolver(ioc);
    auto endpoints = resolver.resolve("localhost", "8888");
    websocket::stream<beast::tcp_stream> ws(ioc);
    ws.binary(true);

    auto const& endpoint = *endpoints.begin();
    ws.next_layer().connect(endpoint);  // Synchronous connect, suitable for simplicity

    ws.handshake("localhost:8888", "/ws_echo");

    // Launch coroutines
    // net::co_spawn(ioc, read_websocket(ws, rounds), net::detached);
    // net::co_spawn(ioc, write_websocket(ws, rounds), net::detached);
    net::co_spawn(ioc, rw_websocket(ws, rounds), net::detached);

    ioc.run();
}
```

beast websocket client with class

```cpp
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <chrono>
#include <iostream>
#include <thread>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

class WebsocketClient {
   public:
    WebsocketClient(asio::io_context& ioc, const std::string& host, const std::string& port)
        : resolver_(ioc),
          ws_(ioc),
          host_(host) {
        ws_.binary(true);
        resolver_.async_resolve(host, port,
                                beast::bind_front_handler(&WebsocketClient::onResolve, this));
    }

    void sendMessage(const std::string& msg) {
        asio::post(ws_.get_executor(), beast::bind_front_handler(&WebsocketClient::doSend, this, msg));
    }

    void onMessage(const std::string& msg) {
        // Handle the received message (this will be invoked asynchronously)
        std::cout << "Received message: " << msg << std::endl;
    }

    void close() {
        ws_.async_close(websocket::close_code::normal,
                        beast::bind_front_handler(&WebsocketClient::onClose, this));
    }

   private:
    void onResolve(beast::error_code ec, asio::ip::tcp::resolver::results_type results) {
        if (ec) {
            std::cerr << "Resolve error: " << ec.message() << std::endl;
            return;
        }

        beast::get_lowest_layer(ws_).async_connect(results,
                                                   beast::bind_front_handler(&WebsocketClient::onConnect, this));
    }

    void onConnect(beast::error_code ec, asio::ip::tcp::resolver::results_type::endpoint_type) {
        if (ec) {
            std::cerr << "Connect error: " << ec.message() << std::endl;
            return;
        }

        ws_.async_handshake(host_, "/ws_echo",
                            beast::bind_front_handler(&WebsocketClient::onHandshake, this));
    }

    void onHandshake(beast::error_code ec) {
        if (ec) {
            std::cerr << "Handshake error: " << ec.message() << std::endl;
            return;
        }

        // Start reading messages asynchronously
        doRead();
    }

    void doSend(const std::string& msg) {
        if (ws_.is_open()) {
            ws_.async_write(asio::buffer(msg),
                            beast::bind_front_handler(&WebsocketClient::onWrite, this));
        } else {
            std::cerr << "WebSocket is closed, cannot send message." << std::endl;
        }
    }

    void onWrite(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cerr << "Write error: " << ec.message() << std::endl;
            return;
        }

        // Message successfully sent
        std::cout << "send: " << bytes_transferred << " bytes\n";
    }

    void doRead() {
        ws_.async_read(buffer_,
                       beast::bind_front_handler(&WebsocketClient::onRead, this));
    }

    void onRead(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec) {
            if (ec == websocket::error::closed) {
                std::cerr << "WebSocket connection closed by the server." << std::endl;
            } else {
                std::cerr << "Read error: " << ec.message() << std::endl;
            }
            return;
        }

        // Process the received message
        std::string message = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());  // Clear buffer
        onMessage(message);

        // Continue reading messages
        doRead();
    }

    void onClose(beast::error_code ec) {
        if (ec) {
            std::cerr << "Close error: " << ec.message() << std::endl;
        } else {
            std::cout << "WebSocket closed successfully." << std::endl;
        }
    }

   private:
    asio::ip::tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string host_;

    std::condition_variable cv_;
    std::mutex mutex_;
    bool handshake_done_;
};

int main() {
    asio::io_context ioc;
    WebsocketClient client(ioc, "localhost", "8888");

    std::thread t([&ioc]() { ioc.run(); });

    for (size_t i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        client.sendMessage("hello");
    }
    client.close();

    t.join();
}
```

### async_simple websocket client

> benchmark: round=1000000, costs=106204 ns

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

### websocket by nng

> benchmark in beelink
- server addr=ws://localhost:8888/ws_echo, round=1000000, costs=147968 ns
- server addr=tcp://localhost:9999, round=1000000, costs=104453 ns 
- server addr=ipc:///tmp/pingpong.ipc, round=1000000, costs=87915 ns

```bash
├── CMakeLists.txt
├── client.cpp
└── server.cpp
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(client client.cpp)
add_executable(server server.cpp)

# this is heuristically generated, and may not be correct
find_package(nng CONFIG REQUIRED)
target_link_libraries(client PRIVATE nng::nng)
target_link_libraries(server PRIVATE nng::nng)
```

```cpp
// client.cpp
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>

#include <chrono>
#include <cstdio>

int main(int argc, const char* argv[]) {
    if (argc < 3) {
        printf("usage: %s url rounds\n", argv[0]);
        return 1;
    }
    auto url = argv[1];
    auto N = std::stol(argv[2]);

    nng_socket sock{};

    nng_req0_open(&sock);
    nng_dial(sock, url, NULL, 0);

    long total = 0;
    for (size_t i = 0; i < N; ++i) {
        long* buf = nullptr;
        size_t sz;

        auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        nng_send(sock, &start, 8, 0);
        nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC);
        auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        total += end - *buf;
        nng_free(buf, sz);
    }

    printf("round=%lu, costs=%lu ns\n", N, total / N);

    nng_close(sock);
}
```

```cpp
// server.cpp
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>

#include <cstdio>

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        printf("usage: %s url", argv[0]);
        return 1;
    }
    auto url = argv[1];
    nng_socket sock{};

    nng_rep0_open(&sock);
    nng_listen(sock, url, NULL, 0);

    void *buf;
    size_t sz;
    while (true) {
        nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC);
        nng_send(sock, buf, sz, 0);
    }
    nng_free(buf, sz);

    nng_close(sock);
}
```