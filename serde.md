# serialize and deserialize

- [serialize and deserialize](#serialize-and-deserialize)
  - [parse bytes manually](#parse-bytes-manually)
    - [cpp serialize/deserialize bytes](#cpp-serializedeserialize-bytes)
    - [Rust serialize/deserialize bytes](#rust-serializedeserialize-bytes)
    - [python serialize/deserialize bytes](#python-serializedeserialize-bytes)
    - [golang serialize/deserialize bytes](#golang-serializedeserialize-bytes)


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