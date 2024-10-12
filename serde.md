# serialize and deserialize

- [serialize and deserialize](#serialize-and-deserialize)
  - [parse bytes manually](#parse-bytes-manually)
    - [cpp serialize/deserialize bytes](#cpp-serializedeserialize-bytes)
    - [Rust serialize/deserialize bytes](#rust-serializedeserialize-bytes)
    - [python serialize/deserialize bytes](#python-serializedeserialize-bytes)
    - [golang serialize/deserialize bytes](#golang-serializedeserialize-bytes)
  - [serde with `flatbuffers`](#serde-with-flatbuffers)
    - [serde for python](#serde-for-python)
    - [serde for golang](#serde-for-golang)
    - [serde for cpp](#serde-for-cpp)
    - [serde for rust](#serde-for-rust)
    - [serde between cpp and python with nng](#serde-between-cpp-and-python-with-nng)
    - [serde for cpp pratical](#serde-for-cpp-pratical)
  - [serde by yalantinglibs](#serde-by-yalantinglibs)
    - [struct\_pack absl container](#struct_pack-absl-container)


## parse bytes manually

### cpp serialize/deserialize bytes

```cpp
#include <cstdio>
#include <iostream>

struct Data {
    int id;
    long volume;
    double amount;
    int prices[20];
};

int main() {
    // bytes to struct
    {
        // auto byte_str = (void *)"e\x00\x00\x00\x00\x00\x00\x00t'\x00\x00\x00\x00\x00\x00\xcc\xcc\xcc\xcc\x8c\xea\xc5@\x88'\x00\x00\x89'\x00\x00\x8a'\x00\x00\x8b'\x00\x00\x8c'\x00\x00\x8d'\x00\x00\x8e'\x00\x00\x8f'\x00\x00\x90'\x00\x00\x91'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
        // same the above
        char byte_str[] = "\x65\x00\x00\x00\x00\x00\x00\x00\x74\x27\x00\x00\x00\x00\x00\x00\xcd\xcc\xcc\xcc\x8c\xea\xc5\x40\x88\x27\x00\x00\x89\x27\x00\x00\x8a\x27\x00\x00\x8b\x27\x00\x00\x8c\x27\x00\x00\x8d\x27\x00\x00\x8e\x27\x00\x00\x8f\x27\x00\x00\x90\x27\x00\x00\x91\x27\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
        auto ptr = reinterpret_cast<Data *>(byte_str);
        std::cout << ptr->id << '\n';
        std::cout << ptr->volume << '\n';
        std::cout << ptr->amount << '\n';
        for (auto &&e : ptr->prices) {
            std::cout << e << ',';
        }
        std::cout << '\n';
    }
    // struct to bytes
    {
        Data quote{
            101,
            10100,
            11221.1,
            10120,
            10121,
            10122,
            10123,
            10124,
            10125,
            10126,
            10127,
            10128,
            10129,
            0,
            0};
        auto ptr = reinterpret_cast<unsigned char *>(&quote);
        for (size_t i = 0; i < sizeof(Data); ++i) {
            printf("\\x%02x", ptr[i]);
        }
    }
}
```

### Rust serialize/deserialize bytes

```rs
// as rust auto memory alignment, but C don't
// so, must repr(C), because memory alignment;
#[repr(C)]
#[derive(Debug, Default)]
struct Data {
    id: i32,
    volume: i64,
    amount: f64,
    prices: [i32; 20],
}

fn main() {
    // bytes to struct
    {
        let byte_data: &[u8]=b"e\x00\x00\x00\x00\x00\x00\x00t'\x00\x00\x00\x00\x00\x00\xcc\xcc\xcc\xcc\x8c\xea\xc5@\x88'\x00\x00\x89'\x00\x00\x8a'\x00\x00\x8b'\x00\x00\x8c'\x00\x00\x8d'\x00\x00\x8e'\x00\x00\x8f'\x00\x00\x90'\x00\x00\x91'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
        println!("{:?}", byte_data);

        // method1: copies the data from the location pointed to by the pointer into a newly constructed Data object.
        let data1: Data = unsafe { std::ptr::read(byte_data.as_ptr() as *const _) };
        // method2: cast a byte pointer into a reference to Data. This doesn't actually move or copy the data
        let data2: &Data = unsafe { std::mem::transmute(byte_data.as_ptr()) };

        println!("{:?}\n{:?}", data1, data2);
        println!("-------------------");
    }
    // struct to bytes
    {
        let quote = Data {
            id: 101,
            volume: 10100,
            amount: 11221.099999999999,
            ..Default::default()
        };
        // Safe way to convert struct to bytes
        let data_bytes: &[u8] = unsafe {
            std::slice::from_raw_parts(
                (&quote as *const Data) as *const u8,
                std::mem::size_of::<Data>(),
            )
        };

        // Use data_bytes as needed
        println!("{:?}", data_bytes);
        let data1: Data = unsafe { std::ptr::read(data_bytes.as_ptr() as *const _) };
        println!("{:?}\n{:?}", quote, data1);
    }
}
```

### python serialize/deserialize bytes

```py
import struct

# struct Data {
#     int id;
#     int64_t volume;
#     double amount;
#     int prices[20];
# };

origin_bytes = b"e\x00\x00\x00\x00\x00\x00\x00t'\x00\x00\x00\x00\x00\x00\xcc\xcc\xcc\xcc\x8c\xea\xc5@\x88'\x00\x00\x89'\x00\x00\x8a'\x00\x00\x8b'\x00\x00\x8c'\x00\x00\x8d'\x00\x00\x8e'\x00\x00\x8f'\x00\x00\x90'\x00\x00\x91'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"


# bytes to tuple
data_fmt = "i q d 20i"
data_tuple = struct.unpack(data_fmt, origin_bytes)

# tuple to bytes
final_bytes = struct.pack(data_fmt, *data_tuple)
print(origin_bytes == final_bytes)  # True
```

### golang serialize/deserialize bytes

```go
package main

import (
	"fmt"
	"os"
	"unsafe"
)

type Data struct {
	ID     int32
	Volume int64
	Amount float64
	Prices [20]int32
}

func main() {
	// struct to bytes
	{
		data := Data{
			ID:     123,
			Volume: 456,
			Amount: 789.12,
			Prices: [20]int32{1, 2, 3}, // rest are zero-initialized
		}

		dataSize := unsafe.Sizeof(data)
		// Get the byte slice from struct
		dataSlice := (*[1 << 30]byte)(unsafe.Pointer(&data))[:dataSize:dataSize]
		fmt.Println("size is", dataSize)

		// Create or open the file
		file, err := os.Create("output_cstyle.bin")
		if err != nil {
			fmt.Println("Cannot create file:", err)
			return
		}
		defer file.Close()

		// Write the byte slice to the file
		if _, err := file.Write(dataSlice); err != nil {
			fmt.Println("Failed to write to file:", err)
			return
		}

		fmt.Println("Data written to file in C-style.")
	}
	// bytes to struct
	{
		// Simulate reading this from a file or similar source
		file, err := os.Open("output_cstyle.bin")
		if err != nil {
			fmt.Println("Error opening file:", err)
			return
		}
		defer file.Close()

		// Allocate space for the struct
		data := Data{}
		// Read data into the struct memory directly
		dataSize := unsafe.Sizeof(data)
		fmt.Println("size is", dataSize)
		dataPointer := (*[1 << 30]byte)(unsafe.Pointer(&data))[:dataSize:dataSize]
		if _, err := file.Read(dataPointer); err != nil {
			fmt.Println("Failed to read data:", err)
			return
		}

		fmt.Printf("Decoded Data: %+v\n", data)
	}

}
```

## serde with `flatbuffers`

### serde for python

how to serde with flatbuffers for python
- downlod [flatc](https://github.com/google/flatbuffers/releases)
- `pip install flatbuffers`
- write `datatypes.fbs`
- `./flatc datatypes.fbs --python`
- use python files which generated by `flatc`

```bash
.
├── main.py
├── datatypes.fbs # schema
├── flatc # executable
├── BarData.py # generated
└── TickData.py # generated
```

> `struct` in flatbuffers, similar to a `table`, only now none of the fields are optional (so no defaults either), and fields may not be added or be deprecated. Structs may only contain scalars or other structs. Use this for simple objects where you are very sure no changes will ever be made. Structs use less memory than tables and are even faster to access (they are always stored in-line in their parent object, and use no virtual table).

```fbs
// datatypes.fbs
struct TickData {
    symbol: [byte:6];
    volume: int64;
    amount: float64;
    prices: [float32:10];
}

struct BarData {
    symbol: [byte:6];
    open: float32;
    volume: int64;
    amount: float64;
}
```

```py
# main.py
import flatbuffers
from TickData import TickData, CreateTickData


def bytes2object(buf, offset):
    tick = TickData()
    tick.Init(buf, offset)

    print(bytes(tick.Symbol()))
    print(tick.Volume())
    print(tick.Amount())
    print(tick.Prices())


def object2bytes():
    mem_size = 100  # or 2 * TickData.SizeOf()
    builder = flatbuffers.Builder(mem_size)

    symbol = list(b"688538")
    prices = [i for i in range(10, 20)]
    # data prepend from end to front
    offset = CreateTickData(builder, symbol, 1000, 12345.56, prices)

    print(f"data head: {builder.head}/{mem_size}, offset: {offset} from head")
    return builder.Bytes[builder.head :]


if __name__ == "__main__":
    buf = object2bytes()
    print(buf)

    print(len(buf), TickData.SizeOf())
    bytes2object(buf, 0)
```

### serde for golang

```bash
./flatc datatypes.fbs --go
# error, not supported now
```

### serde for cpp

how to serde with flatbuffers for cpp
- downlod [flatc](https://github.com/google/flatbuffers/releases)
- `vcpkg install flatbuffers`
- write `datatypes.fbs`
- `./flatc datatypes.fbs --cpp`
- use header files which generated by `flatc`

```bash
├── CMakeLists.txt
├── main.cpp
├── flatc # executable
├── datatypes.fbs # schema
└── datatypes_generated.h # generated
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.24.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(proj1 main.cpp)

find_package(flatbuffers CONFIG REQUIRED)
target_link_libraries(proj1 PRIVATE flatbuffers::flatbuffers)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(proj1 PRIVATE fmt::fmt)
```

```cpp
// main.cpp
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <array>

#include "datatypes_generated.h"

TickData serialize() {
    // prepare data
    std::array<int8_t, 6> symbol{};
    fmt::format_to(symbol.data(), "{:06d}", 688538);

    std::array<float, 10> prices{};
    for (size_t i = 0; i < 10; ++i) {
        prices[i] = 10.1 * i;
    }

    return TickData{symbol, 1000, 123456.78, prices};
}

void deserialize(void* bytes_ptr) {
    auto ptr = reinterpret_cast<TickData*>(bytes_ptr);
    std::string symbol{reinterpret_cast<const char*>(ptr->symbol()->data()), ptr->symbol()->size()};
    auto vol = ptr->volume();
    auto amt = ptr->amount();
    auto prices = ptr->prices();
    fmt::println("symbol={},vol={},amt={}", symbol, vol, amt);
    for (auto&& e : *prices) {
        fmt::print("{},", e);
    }
    fmt::println("");
}

int main(int, char**) {
    auto tick = serialize();
    fmt::println("{}", tick.amount());
    deserialize(&tick);
}
```

### serde for rust

how to serde with flatbuffers for Rust
- downlod [flatc](https://github.com/google/flatbuffers/releases)
- `cargo add flatbuffers`
- write `datatypes.fbs`
- `./flatc datatypes.fbs --rust`
- use rust files which generated by `flatc`

```bash
.
├── Cargo.lock
├── Cargo.toml
├── datatypes.fbs
├── flatc
└── src
    ├── datatypes_generated.rs
    └── main.rs
```

```rs
extern crate flatbuffers;
mod datatypes_generated;
use datatypes_generated::TickData;

fn main() {
    // Example data
    // let i8_array: [i8; 6] = [65, 66, 67, 68, 69, 70]; // ABCDEF in ASCII
    let raw_bytes_string = b"688538";
    let i8_array: &[i8; 6] = unsafe { std::mem::transmute(raw_bytes_string) };
    let volume: i64 = 1000;
    let amount: f64 = 1234.56;
    let prices: [f32; 10] = [1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.10];

    // Create a new TickData instance
    let mut tick_data = TickData::new(i8_array, volume, amount, &prices);

    println!("{:?}", tick_data);

    // Modify fields
    let new_symbol: [i8; 6] = [70, 69, 68, 67, 66, 65]; // FEDCBA in ASCII
    tick_data.set_symbol(&new_symbol);
    tick_data.set_volume(2000);
    tick_data.set_amount(4321.65);
    let new_prices: [f32; 10] = [10.10, 9.9, 8.8, 7.7, 6.6, 5.5, 4.4, 3.3, 2.2, 1.1];
    tick_data.set_prices(&new_prices);

    // Access modified fields
    println!("New Symbol: {:?}", tick_data.symbol());
    println!("New Volume: {}", tick_data.volume());
    println!("New Amount: {}", tick_data.amount());
    println!("New Prices: {:?}", tick_data.prices());

    let bytes: &[u8; 64] = unsafe { std::mem::transmute(&tick_data) };
    println!("{:?}", bytes);

    // Deserialize the byte array into a TickData instance
    let tick_data: &TickData = unsafe { std::mem::transmute(bytes.as_ptr()) };
    println!("{:?}", tick_data);
}
```

### serde between cpp and python with nng

how to serde with flatbuffers for cpp and python with nng
- downlod [flatc](https://github.com/google/flatbuffers/releases)
- `vcpkg install nng fmt flatbuffers`
- `pip install pynng flatbuffers`
- `./flatc datatypes.fbs --cpp --python`


```bash
├── CMakeLists.txt
├── publisher.cpp
├── subscriber.py
├── datatypes.fbs # schema
├── flatc
├── datatypes_generated.h # generated
└── TickData.py # generated
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.24.0)
project(proj VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(pub publisher.cpp)

find_package(nng CONFIG REQUIRED)
target_link_libraries(pub PRIVATE nng::nng)

find_package(flatbuffers CONFIG REQUIRED)
target_link_libraries(pub PRIVATE flatbuffers::flatbuffers)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(pub PRIVATE fmt::fmt)
```

```cpp
// publisher.cpp
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/supplemental/util/platform.h>  // for sleep

#include <array>
#include <cstdint>
#include <bit>  // for std::bit_cast

#include "datatypes_generated.h"

void publish(char const* url) {
    nng_socket pub_sock{};
    nng_pub0_open(&pub_sock);
    nng_listen(pub_sock, url, NULL, 0);  // listener=NULL; flags=0 ignored

    std::array<int8_t, 6> symbols{};
    std::array<float, 10> prices{};

    int64_t i = 0;
    while (true) {
        // mock quote
        // sprintf(symbol.data(), "%06lu", i);
        fmt::format_to(symbols.data(), "{:06d}", i);
        for (size_t j = 0; j < 10; ++j) {
            prices[j] = 100.2 * i + j;
        }
        TickData quote{symbols, 1000 * i, 100.2 * i, prices};

        // send quote on stack
        nng_send(pub_sock, &quote, sizeof(TickData), 0);  // flags=0, default for pub mode

        // fmt::println(">> size={}, symbol={}, vol={}", sizeof(TickData), fmt::join(reinterpret_cast<std::array<char, 6>&>(symbols), ""), quote.volume());
        // fmt::println(">> size={}, symbol={}, vol={}", sizeof(TickData), fmt::join(*reinterpret_cast<std::array<char, 6> const*>(quote.symbol()), ""), quote.volume());
        fmt::println(">> size={}, symbol={}, vol={}", sizeof(TickData), fmt::join(*std::bit_cast<std::array<char, 6>*>(quote.symbol()), ""), quote.volume());
        nng_msleep(500);  // sleep 500 ms
        ++i;
    }
}

int main() {
    publish("ipc:///tmp/pubsub.ipc");
}
```

```py
# receiver.py
import pynng
from TickData import TickData

address = "ipc:///tmp/pubsub.ipc"


def subscribe(max_msg=10):
    with pynng.Sub0(dial=address, topics=b"") as sub:
        tick = TickData()
        for _ in range(max_msg):
            msg = sub.recv()
            tick.Init(msg, 0)
            print(f">> {tick.SizeOf()} bytes, ", bytes(tick.Symbol()), tick.Volume(), tick.Amount(), tick.Prices())


if __name__ == "__main__":
    try:
        subscribe()
    except KeyboardInterrupt:
        pass
```

### serde for cpp pratical

- `vcpkg install fmt flatbuffers`
- `flatc datatypes.fbs --cpp --cpp-std c++17`

```bash
.
├── CMakeLists.txt
├── datatypes.fbs
├── datatypes_generated.h # generated
└── main.cpp
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)
project(proj VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(proj1 main.cpp)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(proj1 PRIVATE fmt::fmt)

find_package(flatbuffers CONFIG REQUIRED)
target_link_libraries(proj1 PRIVATE flatbuffers::flatbuffers)
```

```cpp
// datatypes.fbs
namespace Messages;

// Enum to identify the type of message
enum MessageType : byte {
    NONE = 0,
    BarData = 1,
    TickData = 2
}

// Table representing BarData
table BarData {
    id: int;             // int32_t id
    symbol: string;      // char symbol[6] represented as a string
    price: float;        // float price
    volume: long;        // int64_t volume
    amount: double;      // double amount
}

// Table representing TickData
table TickData {
    id: int;                 // int32_t id
    symbol: string;          // char symbol[6] represented as a string
    open: double;            // double open
    high: double;            // double high
    volumes: [int];          // int volumes[10] represented as a vector of ints
}

// Union to hold either BarData or TickData
union Payload {
    BarData,
    TickData
}

// Root table that encapsulates any message
table Message {
    type: MessageType;       // Type identifier
    payload: Payload;        // Payload containing the actual data
}

// Specify Message as the root type
root_type Message;
```

- `GetCurrentBufferPointer` is before `build.Finish()`
- `GetBufferPointer` is after `build.Finish()`

```cpp
// main.cpp
#include <flatbuffers/flatbuffers.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include "datatypes_generated.h"

int main() {
    flatbuffers::FlatBufferBuilder builder;

    auto bar1 = Messages::CreateBarDataDirect(builder, 1, "APPLE", 12.3, 1000, 12300.0);
    auto msg1 = Messages::CreateMessage(builder, Messages::MessageType::BarData, Messages::Payload::BarData, bar1.Union());
    fmt::println("size={}, addr={}", builder.GetSize(), fmt::ptr(builder.GetCurrentBufferPointer()));

    auto bar2 = Messages::CreateBarDataDirect(builder, 2, "APPLE", 12.3, 1000, 12300.0);
    auto msg2 = Messages::CreateMessage(builder, Messages::MessageType::BarData, Messages::Payload::BarData, bar2.Union());
    fmt::println("size={}, addr={}", builder.GetSize(), fmt::ptr(builder.GetCurrentBufferPointer()));

    //  builder.Finish() can be invoked only once
    builder.Finish(msg1);  // print id 1
    // builder.Finish(msg2);  // print id 2
    auto bar_ptr = builder.GetBufferPointer();
    fmt::println("size={}, addr={}", builder.GetSize(), fmt::ptr(bar_ptr));
    auto msg = Messages::GetMessage(bar_ptr);
    fmt::println("id {}", msg->payload_as_BarData()->id());
}
```

practical example

```cpp
// main.cpp
void serialize_bar_data(flatbuffers::FlatBufferBuilder& builder) {
    auto bar = Messages::CreateBarDataDirect(builder, 1, "apple", 12.3, 1000, 2300.0);
    auto msg1 = Messages::CreateMessage(builder, Messages::MessageType::BarData, Messages::Payload::BarData, bar.Union());
    builder.Finish(msg1);
}

void serialize_tick_data(flatbuffers::FlatBufferBuilder& builder) {
    std::vector<int> vols{1, 2, 3, 45};
    auto tick = Messages::CreateTickDataDirect(builder, 2, "msft", 12.3, 12.4, &vols);
    auto msg2 = Messages::CreateMessage(builder, Messages::MessageType::TickData, Messages::Payload::TickData, tick.Union());
    builder.Finish(msg2);
}

void deserialize_messages(const uint8_t* buffer, size_t size) {
    auto msg_ptr = Messages::GetMessage(buffer);

    if (msg_ptr->type() == Messages::MessageType::BarData) {
        auto bar = msg_ptr->payload_as_BarData();
        fmt::println("Deserialized BarData, id={}, symbol={}, price={}, volume={}, amount={}", bar->id(), bar->symbol()->str(), bar->price(), bar->volume(), bar->amount());
    } else if (msg_ptr->type() == Messages::MessageType::TickData) {
        auto tick = msg_ptr->payload_as_TickData();
        fmt::print("Deserialized TickData, id={}, symbol={}, open={}, high={}, volumes=[", tick->id(), tick->symbol()->str(), tick->open(), tick->high());

        for (auto volume : *tick->volumes()) {
            fmt::print("{} ", volume);
        }
        fmt::println("]");
    }
}

int main() {
    // Serialize BarData
    flatbuffers::FlatBufferBuilder bar_builder;
    serialize_bar_data(bar_builder);
    std::cout << "Size after BarData: " << bar_builder.GetSize() << '\n';

    // Serialize TickData
    flatbuffers::FlatBufferBuilder tick_builder;
    serialize_tick_data(tick_builder);
    std::cout << "Size after TickData: " << tick_builder.GetSize() << '\n';

    // Deserialize BarData
    deserialize_messages(bar_builder.GetBufferPointer(), bar_builder.GetSize());

    // Deserialize TickData
    deserialize_messages(tick_builder.GetBufferPointer(), tick_builder.GetSize());
}
```

## serde by yalantinglibs

```bash
.
├── CMakeLists.txt
└── main.cpp
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.24.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(proj1 main.cpp)

include(FetchContent)
FetchContent_Declare(
    yalantinglibs
    GIT_REPOSITORY https://github.com/alibaba/yalantinglibs.git
    GIT_TAG main
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(yalantinglibs)

target_link_libraries(proj1 yalantinglibs::yalantinglibs)
```

```cpp
// main.cpp
#include <array>
#include <ylt/easylog.hpp>
#include <ylt/reflection/member_names.hpp>
#include <ylt/struct_pack.hpp>

struct Stock {
    std::array<char, 6> symbols;
    int open;
    std::array<int, 10> ask_prices;
};

int main(int, char **) {
    {
        // field names
        auto names = ylt::reflection::member_names<Stock>;
        for (int i = 0; auto &&name : names) {
            ELOGFMT(INFO, "name={},idx={}", name, i);
            ++i;
        }
    }
    {
        auto name_map = ylt::reflection::member_names_map<Stock>;
        for (auto &&[name, idx] : name_map) {
            std::string s{name.begin(), name.end()};
            ELOGFMT(INFO, "name={}, idx={}", s, idx);
        }
    }
    {
        // serialize & deserialize
        Stock tick{};
        auto vec_chars = struct_pack::serialize(tick);
        auto tick2 = struct_pack::deserialize<Stock>(vec_chars);
        ELOGFMT(INFO, "TICK2 {}", tick2->open);
    }
}
```

`struct_pack` vs `nlohmann` benchmark:
- struct_pack iterations=1000000, avg serialization time: 40 ns, avg deserialization time: 43 ns
- nlohmann iterations=1000000, avg serialization time: 5506 ns, avg deserialization time: 1447 ns

```cmake
cmake_minimum_required(VERSION 3.20.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(proj1 main.cpp)

include(FetchContent)
FetchContent_Declare(
    yalantinglibs
    GIT_REPOSITORY https://github.com/alibaba/yalantinglibs.git
    GIT_TAG main 
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(yalantinglibs)
target_link_libraries(proj1 PRIVATE yalantinglibs)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(proj1 PRIVATE fmt::fmt)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(proj1 PRIVATE nlohmann_json::nlohmann_json)
```

```cpp
// main.cpp
#include <fmt/core.h>

#include <chrono>
#include <nlohmann/json.hpp>
#include <ylt/struct_pack.hpp>

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

// Serialization: Convert ATickL2 to json
void to_json(nlohmann::json& j, const ATickL2& tick) {
    j = nlohmann::json{
        {"code", std::string(tick.code.data(), tick.code.size())},
        {"dt", tick.dt},
        {"preclose", tick.preclose},
        {"open", tick.open},
        {"last", tick.last},
        {"iopv", tick.iopv},
        {"high_limit", tick.high_limit},
        {"low_limit", tick.low_limit},
        {"num_trades", tick.num_trades},
        {"volume", tick.volume},
        {"tot_av", tick.tot_av},
        {"tot_bv", tick.tot_bv},
        {"amount", tick.amount},
        {"avg_ap", tick.avg_ap},
        {"avg_bp", tick.avg_bp},
        {"aps", tick.aps},
        {"bps", tick.bps},
        {"avs", tick.avs},
        {"bvs", tick.bvs},
        {"ans", tick.ans},
        {"bns", tick.bns}};
}

// Deserialization: Convert json to ATickL2
void from_json(const nlohmann::json& j, ATickL2& tick) {
    std::string code_str = j.at("code").get<std::string>();
    std::copy_n(code_str.begin(), std::min(code_str.size(), tick.code.size()), tick.code.begin());

    j.at("dt").get_to(tick.dt);
    j.at("preclose").get_to(tick.preclose);
    j.at("open").get_to(tick.open);
    j.at("last").get_to(tick.last);
    j.at("iopv").get_to(tick.iopv);
    j.at("high_limit").get_to(tick.high_limit);
    j.at("low_limit").get_to(tick.low_limit);
    j.at("num_trades").get_to(tick.num_trades);
    j.at("volume").get_to(tick.volume);
    j.at("tot_av").get_to(tick.tot_av);
    j.at("tot_bv").get_to(tick.tot_bv);
    j.at("amount").get_to(tick.amount);
    j.at("avg_ap").get_to(tick.avg_ap);
    j.at("avg_bp").get_to(tick.avg_bp);
    j.at("aps").get_to(tick.aps);
    j.at("bps").get_to(tick.bps);
    j.at("avs").get_to(tick.avs);
    j.at("bvs").get_to(tick.bvs);
    j.at("ans").get_to(tick.ans);
    j.at("bns").get_to(tick.bns);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fmt::println("usage: {} iterations", argv[0]);
        return 1;
    }
    ATickL2 tick = {
        .code = {'A', 'B', 'C', '\0'},
        .dt = 1234567890,
        .preclose = 100,
        .open = 101,
        .last = 102,
        .iopv = 103,
        .high_limit = 104,
        .low_limit = 99,
        .num_trades = 10,
        .volume = 1000,
        .tot_av = 10000,
        .tot_bv = 11000,
        .amount = 123456,
        .avg_ap = 105,
        .avg_bp = 106,
        .aps = {101, 102, 103, 104, 105, 106, 107, 108, 109, 110},
        .bps = {201, 202, 203, 204, 205, 206, 207, 208, 209, 210},
        .avs = {1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900},
        .bvs = {2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900},
        .ans = {300, 310, 320, 330, 340, 350, 360, 370, 380, 390},
        .bns = {400, 410, 420, 430, 440, 450, 460, 470, 480, 490}};
    auto iterations = atoi(argv[1]);
    {
        long serialize_time = 0;
        long deserialize_time = 0;
        for (size_t i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            auto buf = struct_pack::serialize(tick);
            auto mid = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            auto tick2 = struct_pack::deserialize<ATickL2>(buf);
            auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            serialize_time += mid - start;
            deserialize_time += end - mid;
        }
        fmt::println("struct_pack iterations={}, avg serialization time: {} ns, avg deserialization time: {} ns", iterations, serialize_time / iterations, deserialize_time / iterations);
    }
    {
        long serialize_time = 0;
        long deserialize_time = 0;
        for (size_t i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            nlohmann::json j = tick;
            auto mid = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            ATickL2 tick2;
            j.get_to(tick2);
            auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            serialize_time += mid - start;
            deserialize_time += end - mid;
        }
        fmt::println("nlohmann iterations={}, avg serialization time: {} ns, avg deserialization time: {} ns", iterations, serialize_time / iterations, deserialize_time / iterations);
    }
}
```

### struct_pack absl container

```cmake
cmake_minimum_required(VERSION 3.24.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(proj1 main.cpp)

include(FetchContent)
FetchContent_Declare(
    yalantinglibs
    GIT_REPOSITORY https://github.com/alibaba/yalantinglibs.git
    GIT_TAG main
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(yalantinglibs)
target_link_libraries(proj1 PRIVATE yalantinglibs::yalantinglibs)

# this is heuristically generated, and may not be correct
find_package(absl CONFIG REQUIRED)
# note: 180 additional targets are not displayed.
target_link_libraries(proj1 PRIVATE absl::container_common absl::flat_hash_map)
```

as absl obey the `template <typename Key, typename Value>` constraint

```cpp
// main.cpp
#include <absl/container/btree_set.h>
#include <absl/container/flat_hash_map.h>
#include <ylt/struct_pack.hpp>
#include <cstdio>

struct MyStruct {
    absl::btree_set<double> bset;
    absl::flat_hash_map<int, double> map;
};

int main(int argc, char const *argv[]) {
    MyStruct data{{8, 9, 10, 3, 1}, {{1, 11.1}, {2, 12.2}, {3, 13.3}}};

    // serialize
    auto buf = struct_pack::serialize(data);
    // deserialize
    auto data2 = struct_pack::deserialize<MyStruct>(buf);
    for (auto &&e : data2->bset) {
        printf("%f\t", e);
    }
    printf("\n");
    for (auto &&[k, v] : data2->map) {
        printf("key=%d, value=%f\n", k, v);
    }
}
```