# Inter-Process Communication

- [Inter-Process Communication](#inter-process-communication)
  - [nng or pynng](#nng-or-pynng)
    - [pynng supported transports](#pynng-supported-transports)
      - [pynng `inproc`](#pynng-inproc)
      - [pynng `ipc`, `tcp` and `ws`](#pynng-ipc-tcp-and-ws)
      - [pynng with `tls+tcp`](#pynng-with-tlstcp)
    - [pynng supported protocols](#pynng-supported-protocols)
      - [pynng with `Pair0`](#pynng-with-pair0)
      - [pynng with `Req0` and `Rsp0`](#pynng-with-req0-and-rsp0)
      - [pynng with `Pub0` and `Sub0`](#pynng-with-pub0-and-sub0)
      - [pynng with `Push0` and `Pull0`](#pynng-with-push0-and-pull0)
      - [pynng with `Surveyor0` and `Respondent0`](#pynng-with-surveyor0-and-respondent0)
      - [pynng with `Bus0`](#pynng-with-bus0)
    - [nng for cpp](#nng-for-cpp)
  - [`cpp-ipc` usage](#cpp-ipc-usage)

## nng or pynng

in nng(nanomsg next generation), The following protocols are available:
- `pair` - simple one-to-one communication. (`Pair0`, `Pair1`.), `Pair1` is deprecated.
- `request/response` - I ask, you answer. (`Req0`, `Rep0`)
- `pub/sub` - subscribers are notified of topics they are interested in. (`Pub0`, `Sub0`)
- `pipeline`, aka push/pull - load balancing. (`Push0`, `Pull0`)
- `survey` - query the state of multiple applications. (`Surveyor0`, `Respondent0`)
- `bus` - messages are sent to all connected sockets (`Bus0`)

The following transports are available, in the **performance descending order**
> `inproc > ipc > tcp > ws > tls+tcp > carrier pigeons`
- `inproc`: communication within a **single process**.
- `ipc`: communication across processes on a **single machine**.
- `tcp`: communication over **networks via tcp**.
- `ws`: communication over **networks with websockets**. (Probably only useful if one end is on a browser.)
- `tls+tcp`: Encrypted TLS communication **over networks**.
- `carrier pigeons`: (*useless*)communication via World War 1-style carrier pigeons. The latency is pretty high on this one.

### pynng supported transports

#### pynng `inproc`

pub/sub without topics

```py
import pynng
import threading
import time

address = "inproc://example"

def publisher():
    with pynng.Pub0(listen=address) as pub:
        counter = 0
        try:
            while True:
                message = f"Message {counter}"
                print(f"Publisher sending: {message}")
                pub.send(message.encode())
                time.sleep(1)  # send a message every second
                counter += 1
        except pynng.exceptions.Closed:
            print("Publisher closed.")

def subscriber():
    with pynng.Sub0(dial=address, topics=b"") as sub:  # empty topics to receive all messages
        try:
            while True:
                msg = sub.recv()  # receive messages synchronously
                print(f"Subscriber received: {msg.decode()}")
        except pynng.exceptions.Closed:
            print("Subscriber closed.")

if __name__ == "__main__":
    # Create and start threads for publisher and subscriber
    pub_thread = threading.Thread(target=publisher)
    sub_thread = threading.Thread(target=subscriber)

    pub_thread.start()
    sub_thread.start()

    # Wait for threads to complete (they won't in this infinite loop scenario)
    pub_thread.join()
    sub_thread.join()
```

pub/sub with topics

```py
import pynng
import threading
import time

address = "inproc://example"


def publisher():
    with pynng.Pub0(listen=address) as pub:
        counter = 0
        try:
            while True:
                message1 = f"topic1: Message {counter}" # message begin with topic1
                print(f"Publisher sending: {message1}")
                pub.send(message1.encode())
                message2 = f"topic2: Message {counter}" # message begin with topic2
                print(f"Publisher sending: {message2}")
                pub.send(message2.encode())
                time.sleep(1)  # send a message every second
                counter += 1
        except pynng.exceptions.Closed:
            print("Publisher closed.")


def subscriber(topic, thread_id):
    with pynng.Sub0(dial=address, topics=topic) as sub: # change here
        try:
            while True:
                msg = sub.recv()
                print(f"Subscriber-{thread_id} received: {msg.decode()}", flush=True)
        except pynng.exceptions.Closed:
            print("Subscriber closed.")


if __name__ == "__main__":
    pub_thread = threading.Thread(target=publisher)
    sub_thread1 = threading.Thread(target=subscriber, args=(b"topic1", 1)) # thread1 receive topic1
    sub_thread2 = threading.Thread(target=subscriber, args=(b"topic2", 2)) # thread2 receive topic2

    pub_thread.start()
    time.sleep(1)  # wait for publisher reader
    sub_thread1.start()
    sub_thread2.start()

    pub_thread.join()
    sub_thread1.join()
    sub_thread2.join()
```

pub/sub with asyncio

```py
import pynng
import asyncio

async def publisher():
    with pynng.Pub0(listen="inproc://example") as pub:
        counter = 0
        while True:
            message = f"Message {counter}"
            print(f"Publisher sending: {message}")
            pub.send(message.encode())
            await asyncio.sleep(1)  # send a message every second
            counter += 1

async def subscriber():
    with pynng.Sub0(dial="inproc://example", topics=b"") as sub:  # empty topics to receive all messages
        while True:
            msg = await sub.arecv()  # receive messages asynchronously
            print(f"Subscriber received: {msg.decode()}")

async def main():
    pub_task = asyncio.create_task(publisher())
    sub_task = asyncio.create_task(subscriber())
    await asyncio.gather(pub_task, sub_task)


if __name__ == "__main__":
    asyncio.run(main())
```

#### pynng `ipc`, `tcp` and `ws`

The `ipc` transport provides communication support between sockets within different processes on the same host. 
- For POSIX platforms, this is implemented using `UNIX domain sockets`. 
- For Windows, this is implemented using `Windows Named Pipes`. 
- Other platforms may have different implementation strategies.

```py
# publisher.py
import pynng
import time

# address = "ipc:///tmp/pubsub.ipc"
# address = "tcp://0.0.0.0:5555"
# address = "tcp4://0.0.0.0:4433" # using ipv4
address = "tcp6://[::1]:4433"  # using ipv6
# address = "ws://0.0.0.0:5555"


def publisher():
    with pynng.Pub0(listen=address) as pub:
        i = 0
        while True:
            message = f"Message {i}"
            print(f"SERVER: PUBLISHING DATE {message}")
            pub.send(message.encode())
            time.sleep(0.2)
            i += 1


if __name__ == "__main__":
    try:
        publisher()
    except KeyboardInterrupt:
        pass
```

```py
# subscriber.py
import pynng

# address = "ipc:///tmp/pubsub.ipc"
# address = "tcp://127.0.0.1:5555"  # <publisher_ip>:<publisher_port>
# address = "tcp4://127.0.0.1:4433"
address = "tcp6://[::1]:4433"
# address = "ws://127.0.0.1:5555"  # <publisher_ip>:<publisher_port>


def subscriber(name, max_msg=20):
    with pynng.Sub0(dial=address, topics=b"") as sub:
        while max_msg:
            msg = sub.recv()
            print(f"CLIENT ({name}): RECEIVED {msg.decode()}")
            max_msg -= 1


if __name__ == "__main__":
    try:
        subscriber("client0")
    except KeyboardInterrupt:
        pass
```

#### pynng with `tls+tcp`

```bash
# generate cert.pem and key.pem
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes

# then use mode Pair0 etc
```

[tls example](https://github.com/codypiersall/pynng/blob/master/test/test_tls.py)

### pynng supported protocols

#### pynng with `Pair0`

> `Pair0`, A socket for bidrectional, one-to-one communication, with a single partner.

```py
# sender.py
from pynng import Pair0

address = "tcp://127.0.0.1:13131"

with Pair0(listen=address, recv_timeout=3000) as s0:
    for i in range(20):
        msg_out = f"ping-{i}".encode()
        s0.send(msg_out)  # send bytes
        print(f">> {msg_out}")

        msg_in = s0.recv()
        print(f"<< {msg_in}")  # recv bytes
```

```py
# receiver.py
from pynng import Pair0
import time

address = "tcp://127.0.0.1:13131"

with Pair0(dial=address) as s1:
    for _ in range(10):
        msg_in = s1.recv()
        print(f"<< {msg_in}")

        value = int(msg_in[5:]) + 1000  # processing logic
        time.sleep(0.5)

        msg_out = f"xong-{value}".encode()
        s1.send(msg_out)
        print(f">> {msg_out}")
```

async version

```python
# sender.py
from trio import run
from pynng import Pair0

async def send_and_recv():
    address = "tcp://127.0.0.1:13131"
    # in real code you should also pass recv_timeout and/or send_timeout
    with Pair0(listen=address, recv_timeout=3000) as s0:
        for i in range(20):
            msg_out = f"ping-{i}".encode()
            await s0.asend(msg_out)
            print(">>", msg_out)

            print("<<", await s0.arecv())


run(send_and_recv)
```

```python
# receiver.py
from trio import run, sleep
from pynng import Pair0

async def send_and_recv():
    address = "tcp://127.0.0.1:13131"
    # in real code you should also pass recv_timeout and/or send_timeout
    with Pair0(dial=address) as s1:
        for _ in range(10):
            # await sleep(1)
            sleep(1)
            msg_in = await s1.arecv()
            print("<<", msg_in)

            value = int(msg_in[5:]) * 1000
            msg_out = f"xong-{value}".encode()
            await s1.asend(msg_out)


run(send_and_recv)

```

#### pynng with `Req0` and `Rsp0`

A `Req0` socket is paired with a `Rep0` socket and together they implement **normal request/response behavior**. the req socket `send()`s a request, the rep socket `recv()`s it, the rep socket `send()`s a response, and the req socket `recv()`s it.
> If a req socket attempts to do a `recv()` without first doing a `send()`, a `pynng.BadState` exception is raised.  
> If a rsp socket attempts to do a `send()` without first doing a `recv()`, a `pynng.BadState` exception is also raised.  

#### pynng with `Pub0` and `Sub0`

A `Pub0` socket calls `send()`, the data is published to all connected subscribers.
> Attempting to `recv()` with a `Pub0` socket will raise a `pynng.NotSupported` exception.

A `Sub0` also has one additional keyword argument: topics
> Attempting to `send()` with a `Sub0` socket will raise a `pynng.NotSupported` exception.

#### pynng with `Push0` and `Pull0`

> a `Push0` socket sends messages to one or more `Pull0` sockets in a round-robin fashion.

Comparison
- `Request/Response`: Focuses on direct communication between a client and a server for immediate responses.
- `Pub/Sub`: Focuses on broadcasting messages to multiple subscribers based on topics.
- `Push/Pull`: Focuses on distributing tasks efficiently among workers.

Each pattern has its strengths and is suited for different scenarios. For instance, push/pull is great for task distribution, pub/sub is excellent for real-time updates, and request/response is perfect for direct interactions.

#### pynng with `Surveyor0` and `Respondent0`

In this pattern, a `Surveyor0` sends a message, and gives all `Respondent0`s a chance to chime in. The amount of time a survey is valid is set by the attribute `survey_time`.

#### pynng with `Bus0`

A `Bus0` socket sends a message to all directly connected peers. This enables creating mesh networks. Note that messages are only sent to directly connected peers. 

In bus mode, all nodes on the bus can send and receive messages to and from each other. This creates a peer-to-peer communication model where each node can act as both a sender and a receiver.

This pattern is useful for scenarios where multiple nodes need to communicate with each other directly without a central broker.

### nng for cpp

`vcpkg install ngg`

```bash
├── CMakeLists.txt
├── publisher.cpp
├── subscriber1.cpp
└── subscriber2.cpp
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.24.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(pub publisher.cpp)
add_executable(sub1 subscriber1.cpp)
add_executable(sub2 subscriber2.cpp)

find_package(nng CONFIG REQUIRED)
target_link_libraries(pub PRIVATE nng::nng)
target_link_libraries(sub1 PRIVATE nng::nng)
target_link_libraries(sub2 PRIVATE nng::nng)
```

```cpp
// publisher.cpp
#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/supplemental/util/platform.h>  // for sleep

#include <cstdio>

struct Data {
    int id;
    int64_t volume;
    double amount;
    int prices[20];
};

void publish(char const* url) {
    nng_socket pub_sock{};
    nng_pub0_open(&pub_sock);
    nng_listen(pub_sock, url, NULL, 0);  // listener=NULL; flags=0 ignored

    Data quote{};

    size_t i = 0;
    while (true) {
        // mock quote
        quote.id = i;
        quote.volume = i * 100;
        quote.amount = i * 111.1;
        for (size_t j = 0; j < 10; ++j) {
            quote.prices[j] = i * 100.2 + j;
        }

        // send quote on stack
        nng_send(pub_sock, &quote, sizeof(Data), 0);  // flags=0, default for pub mode

        printf("send quote.id=%d\n", quote.id);
        nng_msleep(500);  // sleep 500 ms
        ++i;
    }
}

int main() {
    publish("ipc:///tmp/pubsub.ipc");
}
```

without `NNG_FLAG_ALLOC`

```cpp
// subscriber1.cpp
#include <nng/nng.h>
#include <nng/protocol/pubsub0/sub.h>

#include <cstdio>

struct Data {
    int id;
    int64_t volume;
    double amount;
    int prices[20];
};

void subscribe_not_alloc(char const* url) {
    nng_socket sub_sock;
    nng_sub0_open(&sub_sock);
    // subscribe all topics
    nng_socket_set(sub_sock, NNG_OPT_SUB_SUBSCRIBE, "", 0);
    // nng_dialer=NULL, flags=0 ignored
    nng_dial(sub_sock, url, NULL, 0);

    Data quote{};
    size_t sz = sizeof(Data);  // preknow size
    for (size_t i = 0; i < 10; ++i) {
        // 0, copy receive data(on heap) to quote on stack
        nng_recv(sub_sock, &quote, &sz, 0);
        printf("receive quote.id=%d\n", quote.id);
    }
    nng_close(sub_sock);
}

int main(int argc, char** argv) {
    subscribe_not_alloc("ipc:///tmp/pubsub.ipc");
}
```

with `NNG_FLAG_ALLOC`, higher performance

```cpp
// subscriber2.cpp
#include <nng/nng.h>
#include <nng/protocol/pubsub0/sub.h>

#include <cstdio>

struct Data {
    int id;
    int64_t volume;
    double amount;
    int prices[20];
};

void subscribe_alloc(char const* url) {
    nng_socket sub_sock;
    nng_sub0_open(&sub_sock);
    // subscribe all topics
    nng_socket_set(sub_sock, NNG_OPT_SUB_SUBSCRIBE, "", 0);
    // nng_dialer=NULL, flags=0 ignored
    nng_dial(sub_sock, url, NULL, 0);

    Data* quote_ptr = nullptr;
    size_t sz;  // size get by nng_recv
    for (size_t i = 0; i < 10; ++i) {
        // NNG_FLAG_ALLOC, move receive data(on heap) ownership to quote_ptr
        nng_recv(sub_sock, &quote_ptr, &sz, NNG_FLAG_ALLOC);
        printf("receive quote.id=%d\n", quote_ptr->id);

        // must release heap data, otherwise memory leak
        nng_free(quote_ptr, sz);
    }
    nng_close(sub_sock);
}

int main(int argc, char** argv) {
    subscribe_alloc("ipc:///tmp/pubsub.ipc");
}
```

## `cpp-ipc` usage

[cpp-ipc](https://github.com/mutouyun/cpp-ipc): A high-performance IPC library using shared memory on Linux/Windows.
> `vcpkg install cpp-ipc`

modes: `relat:single`, `relat:multi`, `trans:broadcast`, `trans:unicast`
- `using route = chan<relat::single, relat::multi, trans::broadcast>;`
- `using channel = chan<relat::multi, relat::multi, trans::broadcast>;`

```bash
├── CMakeLists.txt
├── consumer.cpp
└── producer.cpp
```

```bash
# how to run
./producer

# consumer1
./consumer
# consumer2
./consumer
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.24.0)
project(proj5 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(producer producer.cpp)
add_executable(consumer consumer.cpp)

find_package(cpp-ipc CONFIG REQUIRED)
target_link_libraries(producer PRIVATE cpp-ipc::ipc)
target_link_libraries(consumer PRIVATE cpp-ipc::ipc)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(consumer PRIVATE fmt::fmt)
```

```cpp
// producer.cpp
#include <chrono>
#include <iostream>
#include <thread>

#include "libipc/ipc.h"

struct Stock {
    int id;
    int volume;
    double amount;
    double prices[10];
};

void do_send(int num, int interval) {
    // single produer multiple consumer
    ipc::route ipc{"ipc", ipc::sender};
    Stock stock{};
    for (size_t i = 0; i < num; ++i) {
        stock.id = i * 100;
        stock.amount = i * 100.1;

        std::cout << "send size: " << sizeof(Stock) << "\n";
        ipc.send(&stock, sizeof(Stock));

        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
}

int main() {
    do_send(20, 1000);
}
```

```cpp
// consumer.cpp

#include <fmt/core.h>

#include "libipc/ipc.h"

struct Stock {
    int id;
    int volume;
    double amount;
    double prices[10];
};

void do_recv(int interval) {
    ipc::route ipc{"ipc", ipc::receiver};
    while (true) {
        auto buf = ipc.recv(interval);  // if timeout, buff_t is empty
        if (!buf.empty()) {
            auto stock = buf.get<Stock *>();
            fmt::println("buf size={}, id={}, amount={}", buf.size(), stock->id, stock->amount);
        }
        fmt::println("no data comes...");
    }
}

int main() {
    do_recv(500);
}
```