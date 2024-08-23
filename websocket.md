# Websocket

- [Websocket](#websocket)
  - [python websocket](#python-websocket)
    - [py websocket server](#py-websocket-server)

## python websocket

> `pip install websockets`

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