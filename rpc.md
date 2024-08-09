# Remote Procedure Call

- [Remote Procedure Call](#remote-procedure-call)
  - [RPC by pycapnp](#rpc-by-pycapnp)
    - [simple invoke](#simple-invoke)
    - [callback invoke](#callback-invoke)


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