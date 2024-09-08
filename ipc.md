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
      - [nng for pub0 and sub0](#nng-for-pub0-and-sub0)
      - [cpp nng for Req0 and Rep0](#cpp-nng-for-req0-and-rep0)
    - [nng for rust](#nng-for-rust)
  - [`cpp-ipc` usage](#cpp-ipc-usage)
  - [`ecal` for ipc](#ecal-for-ipc)

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

#### nng for pub0 and sub0

```bash
├── CMakeLists.txt
├── datatypes.h
├── publisher.cpp
└── subscriber.cpp
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.24.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(pub publisher.cpp)
add_executable(sub subscriber.cpp)

find_package(nng CONFIG REQUIRED)
target_link_libraries(pub PRIVATE nng::nng)
target_link_libraries(sub PRIVATE nng::nng)
```

```cpp
// datatypes.h
#pragma once
#include <array>
#include <cstdint>

struct ATickL2 {
    std::array<char, 16> code;
    int64_t dt;
    uint32_t preclose;
    uint32_t open;
    uint32_t last;
    uint32_t iopv;
    uint32_t high_limit;
    uint32_t low_limit;
    uint32_t num_trades;
    uint64_t volume;
    uint64_t tot_av;
    uint64_t tot_bv;
    uint64_t amount;
    uint32_t avg_ap;
    uint32_t avg_bp;
    std::array<uint32_t, 10> aps;
    std::array<uint32_t, 10> bps;
    std::array<uint32_t, 10> avs;
    std::array<uint32_t, 10> bvs;
    std::array<uint32_t, 10> ans;
    std::array<uint32_t, 10> bns;
};
```

```cpp
// publisher.cpp
#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/supplemental/util/platform.h>  // for sleep
#include <cstdio>
#include "datatypes.h"

void publish(char const* url) {
    nng_socket pub_sock{};
    nng_pub0_open(&pub_sock);
    nng_listen(pub_sock, url, NULL, 0);  // listener=NULL; flags=0 ignored

    ATickL2 quote{};

    size_t i = 0;
    while (true) {
        // mock quote
        quote.volume = i;

        // send quote on stack
        nng_send(pub_sock, &quote, sizeof(ATickL2), 0);  // flags=0, default for pub mode

        printf("send quote.id=%lu\n", quote.volume);
        nng_msleep(100);  // sleep 100 ms
        ++i;
    }
}

int main() {
    publish("ipc:///tmp/pubsub.ipc");
}
```

```cpp
// subscriber.cpp
#include <nng/nng.h>
#include <nng/protocol/pubsub0/sub.h>
#include <memory>
#include "datatypes.h"

// without NNG_FLAG_ALLOC
void subscribe_not_alloc(char const* url) {
    nng_socket sub_sock;
    nng_sub0_open(&sub_sock);
    // subscribe all topics
    nng_socket_set(sub_sock, NNG_OPT_SUB_SUBSCRIBE, "", 0);
    // nng_dialer=NULL, flags=0 ignored
    nng_dial(sub_sock, url, NULL, 0);

    ATickL2 quote{};
    size_t sz = sizeof(ATickL2);  // preknow size
    for (size_t i = 0; i < 10; ++i) {
        // 0, copy receive data(on heap) to quote on stack
        nng_recv(sub_sock, &quote, &sz, 0);
        printf("receive quote.volume=%lu\n", quote.volume);
    }
    nng_close(sub_sock);
}

// with NNG_FLAG_ALLOC, of higher performance
void subscribe_alloc(char const* url) {
    nng_socket sub_sock;
    nng_sub0_open(&sub_sock);
    // subscribe all topics
    nng_socket_set(sub_sock, NNG_OPT_SUB_SUBSCRIBE, "", 0);
    // nng_dialer=NULL, flags=0 ignored
    nng_dial(sub_sock, url, NULL, 0);

    for (size_t i = 0; i < 10; ++i) {
        auto quote_ptr = std::make_unique<ATickL2>();
        size_t sz;  // size get by nng_recv
        // NNG_FLAG_ALLOC, move receive data(on heap) ownership to quote_ptr
        nng_recv(sub_sock, &quote_ptr, &sz, NNG_FLAG_ALLOC);
        printf("receive quote.volume=%lu\n", quote_ptr->volume);
    }  // auto free quote_ptr by smart pointer
    nng_close(sub_sock);
}

int main(int argc, char** argv) {
    subscribe_not_alloc("ipc:///tmp/pubsub.ipc");
    printf("begin alloc case\n");
    subscribe_alloc("ipc:///tmp/pubsub.ipc");
}
```

#### cpp nng for Req0 and Rep0

```bash
.
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
#include <cstring>
#include <string>

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

#include <cstddef>
#include <cstdio>
#include <cstring>

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

### nng for rust

- `cargo new proj_nng`, then `cd proj_nng`
- `cargo add nng`, then edit main.rs

```rs
use nng::{
    options::{protocol::pubsub::Subscribe, Options},
    Protocol, Socket,
};

fn main() -> Result<(), nng::Error> {
    subscriber("ipc:///tmp/pubsub.ipc")
}

fn subscriber(url: &str) -> Result<(), nng::Error> {
    let s = Socket::new(Protocol::Sub0)?;
    s.dial(url)?;

    println!("SUBSCRIBER: SUBSCRIBING TO ALL TOPICS");
    let all_topics = vec![];
    s.set_opt::<Subscribe>(all_topics)?;

    loop {
        let msg = s.recv()?;
        let tick: &ATickL2 = unsafe { std::mem::transmute(msg.as_ptr()) };
        println!("receive quote.volume={}", tick.volume);
    }
}

#[repr(C)]
#[derive(Debug)]
struct ATickL2 {
    code: [i8; 16],
    dt: i64,
    preclose: u32,
    open: u32,
    last: u32,
    iopv: u32,
    high_limit: u32,
    low_limit: u32,
    num_trades: u32,
    volume: u64,
    tot_av: u64,
    tot_bv: u64,
    amount: u64,
    avg_ap: u32,
    avg_bp: u32,
    aps: [u32; 10],
    bps: [u32; 10],
    avs: [u32; 10],
    bvs: [u32; 10],
    ans: [u32; 10],
    bns: [u32; 10],
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

## `ecal` for ipc

[ecal](https://github.com/eclipse-ecal/ecal) is based on [iceoryx](https://github.com/eclipse-iceoryx/iceoryx), but `iceoryx` is not available on windows over shm
> `vcpkg install ecal`

simple pub/sub

```bash
.
├── CMakeLists.txt
├── pub.cpp
└── sub.cpp
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(pub pub.cpp)
add_executable(sub sub.cpp)

find_package(eCAL CONFIG REQUIRED)
target_link_libraries(pub PRIVATE eCAL::core)
target_link_libraries(sub PRIVATE eCAL::core)
```

```cpp
// pub.cpp
#include <ecal/ecal.h>
#include <cstdio>

struct Stock {
    int id;
    double price;
    int volume;
};

int main(int argc, char **argv) {
    // initialize eCAL API
    eCAL::Initialize(argc, argv, "binary_snd");

    // publisher for topic "blob"
    eCAL::CPublisher pub("blob");

    // send updates
    int i = 0;
    while (eCAL::Ok()) {
        auto tick = Stock{i, i * 1.1, i * 100};

        // send buffer
        pub.Send(&tick, sizeof(Stock));
        printf("publisher send i=%d\n", i++);

        // sleep 100 ms
        eCAL::Process::SleepMS(1000);
    }

    // finalize eCAL API
    eCAL::Finalize();
}
```

```cpp
// sub.cpp
#include <ecal/ecal.h>
#include <cstdio>

struct Stock {
    int id;
    double price;
    int volume;
};

// subscriber callback function
void OnReceive(const char* /*topic_name_*/, const struct eCAL::SReceiveCallbackData* data_) {
    if (data_->size < 1) return;
    auto tick = reinterpret_cast<Stock*>(data_->buf);
    printf("subscriber recv: id=%d, price=%f,volume=%d\n", tick->id, tick->price, tick->volume);
}

int main(int argc, char** argv) {
    // initialize eCAL API
    eCAL::Initialize(argc, argv, "binary_rec");

    // subscriber for topic "blob"
    eCAL::CSubscriber sub("blob");

    // assign callback
    sub.AddReceiveCallback(OnReceive);

    // idle main loop
    while (eCAL::Ok()) eCAL::Process::SleepMS(500);

    // finalize eCAL API
    eCAL::Finalize();
}
```