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

        std::string sym_str;
        sym_str.reserve(symbols.size());
        for (auto&& c : *quote.symbol()) {
            sym_str.push_back(c);
        }

        fmt::println(">> size={}, symbol={}, vol={}", sizeof(TickData), sym_str, quote.volume());
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