# Websocket

- [Websocket](#websocket)
  - [python websocket](#python-websocket)
    - [python async basic](#python-async-basic)
    - [py websocket server](#py-websocket-server)
    - [py websocket client](#py-websocket-client)

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