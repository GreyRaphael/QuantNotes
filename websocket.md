# Websocket

- [Websocket](#websocket)
  - [python websocket](#python-websocket)
    - [python async basic](#python-async-basic)
    - [py websocket server](#py-websocket-server)
    - [py websocket client](#py-websocket-client)
    - [go websocket client](#go-websocket-client)

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

### go websocket client

```go
package main

import (
	"encoding/binary"
	"flag"
	"log"
	"net/url"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/gorilla/websocket"
)

func main() {
	// Command-line arguments for iteration control
	NUM := flag.Int("n", 100, "Number of messages to send")
	flag.Parse()

	interrupt := make(chan os.Signal, 1)
	signal.Notify(interrupt, os.Interrupt, syscall.SIGTERM)

	u := url.URL{Scheme: "ws", Host: "localhost:8888", Path: "/ws_echo"}
	log.Printf("connecting to %s", u.String())

	c, _, err := websocket.DefaultDialer.Dial(u.String(), nil)
	if err != nil {
		log.Fatal("dial:", err)
	}
	defer c.Close()

	done := make(chan struct{})

	// Reading Goroutine with controlled iterations
	go func() {
		defer close(done)
		var total int64
		for i := 0; i < *NUM; i++ {
			_, message, err := c.ReadMessage()
			if err != nil {
				log.Println("read:", err)
				return
			}
			end := time.Now().UnixNano()
			num := int64(binary.LittleEndian.Uint64(message))
			total += end - num
			log.Printf("recv: %d at %d, diff=%d", num, time.Now().UnixNano(), end-num)
		}
		log.Printf("round=%d, avg costs=%d\n", *NUM, total/(int64(*NUM)))
	}()

	// Writing Goroutine with controlled iterations
	go func() {
		for i := 0; i < *NUM; i++ {
			select {
			case <-done:
				return
			default:
				start := time.Now().UnixNano()
				byteArray := make([]byte, 8)
				binary.LittleEndian.PutUint64(byteArray, uint64(start))
				err := c.WriteMessage(websocket.TextMessage, byteArray)
				log.Printf("send: %d", start)
				if err != nil {
					log.Println("write:", err)
					return
				}
			case <-interrupt:
				log.Println("interrupt")

				// Cleanly close the connection by sending a close message and then
				// waiting (with timeout) for the server to close the connection.
				err := c.WriteMessage(websocket.CloseMessage, websocket.FormatCloseMessage(websocket.CloseNormalClosure, ""))
				if err != nil {
					log.Println("write close:", err)
					return
				}
				select {
				case <-done:
				case <-time.After(time.Second):
				}
				return
			}
		}
	}()

	<-done // Wait for read goroutine to exit.
}
```