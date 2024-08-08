# Remote Procedure Call

## RPC by pycapnp

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