# Memory-Mapped File

- [Memory-Mapped File](#memory-mapped-file)
  - [Reading a Memory-Mapped File With mmap](#reading-a-memory-mapped-file-with-mmap)
  - [mmap and Shared Memory](#mmap-and-shared-memory)
    - [transfer data between processes by `SharedMemory`](#transfer-data-between-processes-by-sharedmemory)
    - [transfer file between processes by `SharedMemory`](#transfer-file-between-processes-by-sharedmemory)
    - [transfer file between processes by `mmap` in windows](#transfer-file-between-processes-by-mmap-in-windows)
    - [transfer file between processes by `mmap` in linux](#transfer-file-between-processes-by-mmap-in-linux)
  - [Transfer file between processes by boost.interprocess](#transfer-file-between-processes-by-boostinterprocess)


## Reading a Memory-Mapped File With mmap

> mmap is x15 faster than regular io

```py
import mmap
import time

def regular_io(filename):
    with open(filename, mode="r", encoding="utf8") as file_obj:
        text = file_obj.read()
        # print(text)

def mmap_io(filename):
    with open(filename, mode="r", encoding="utf8") as file_obj:
        # length=0. This is the length in bytes of the memory map.
        # 0 is a special value indicating that the system should create a memory map large enough to hold the entire file.
        with mmap.mmap(file_obj.fileno(), length=0, access=mmap.ACCESS_READ) as mmap_obj:
            text = mmap_obj.read()
            # print(text)

if __name__ == "__main__":
    t1 = time.perf_counter_ns()
    regular_io("2m_data.txt")
    t2 = time.perf_counter_ns()
    mmap_io("2m_data.txt")
    t3 = time.perf_counter_ns()
    print("regular_io costs ", (t2 - t1) / 1e9)
    print("mmap_io costs ", (t3 - t2) / 1e9)
    print(f"regular_io/mmap_io={(t2 - t1)/(t3 - t2):.2f}")  # ~15
```

## mmap and Shared Memory

simple example before python3.8, by `os.fork()`
> Until now, you’ve used memory-mapped files only for data on disk. However, you can also create anonymous memory maps that have no physical storage. This can be done by passing `-1` as the file descriptor

```py
import mmap
import os
import time

def sharing_with_mmap():
    BUF = mmap.mmap(-1, length=100, access=mmap.ACCESS_WRITE)

    pid = os.fork()
    if pid == 0:
        # Child process
        print("child process write")
        BUF[0:100] = b"a" * 100
    else:
        time.sleep(2)
        print(f"parent procee read {BUF[0:100]}")


if __name__ == "__main__":
    sharing_with_mmap()
```

shared memory by `shared_memory` module
> Notice only the name of the buffer is passed to the second process. Then the second process can retrieve that same block of memory using the unique name. This is a special feature of the shared_memory module that’s powered by mmap. 

```py
import time
from multiprocessing import Process
from multiprocessing import shared_memory

def modify(buf_name):
    print("process modify buf")
    time.sleep(3)
    shm = shared_memory.SharedMemory(buf_name)
    shm.buf[0:50] = b"b" * 50
    shm.close()

if __name__ == "__main__":
    shm = shared_memory.SharedMemory(create=True, size=100)
    print(f"random memory name={shm.name}")  # memory name=psm_f4d632c3F

    try:
        shm.buf[0:100] = b"a" * 100
        proc = Process(target=modify, args=(shm.name,))
        proc.start()
        proc.join()
        print(bytes(shm.buf[:100]))
    finally:
        shm.close()
        shm.unlink()
```

### transfer data between processes by `SharedMemory`

```bash
.
├── program1.py
└── program2.py
```

```py
# program1.py
from multiprocessing.shared_memory import SharedMemory
import struct
import time

shm = SharedMemory(name="MyMemory", size=1024, create=True)
# default shm.buf all is 0
shm.buf[0] = 128  # Byte data 0~255
# 4 bytes
shm.buf[4:8] = struct.pack("i", -1)  # int32 use struct
shm.buf[8:12] = struct.pack("I", 1)  # uint32 use struct
shm.buf[12:16] = struct.pack("f", 2.0)  # float32 use struct

try:
    while True:
        print(shm.buf[0], end=",")
        print(shm.buf[1], end=",")
        print(shm.buf[2], end=",")
        print(shm.buf[3], end=",")
        print(struct.unpack("i", shm.buf[4:8])[0], end=",")
        print(struct.unpack("I", shm.buf[8:12])[0], end=",")
        print(struct.unpack("f", shm.buf[12:16])[0])

        time.sleep(3)
except KeyboardInterrupt:
    shm.close()
    shm.unlink()
```

```py
# program2.py
from multiprocessing.shared_memory import SharedMemory
import struct

shm = SharedMemory(name="MyMemory", create=False)

# modify data
shm.buf[1] = 127
shm.buf[2] = 255
# shm.buf[3] = 256  # error
shm.buf[4:8] = struct.pack("i", -128)
shm.buf[8:12] = struct.pack("I", 6666)
shm.buf[12:16] = struct.pack("f", 1.414)

shm.close()
```

### transfer file between processes by `SharedMemory`

a complete example [FileTransfer](https://github.com/GreyRaphael/FileTransfer/blob/windows_simple/transfer.py)

```bash
.
├── data.zip
├── reader.py
└── writer.py
```

```py
# writer.py for windows & linux
from multiprocessing.shared_memory import SharedMemory
import time

with open("data.zip", "rb") as file:
    data = file.read()
    shm = SharedMemory(name="shm_test", create=True, size=len(data))
    shm.buf[:] = data

try:
    while True:
        print("data available")
        time.sleep(3)
finally:
    shm.close()
    shm.unlink() # destroied in producer
```

> if `del data` not exist, The error you’re encountering, `BufferError: cannot close exported pointers exist`, occurs because there are still references to the shared memory buffer when you attempt to close it. This can happen if the buffer is still being accessed or if there are lingering references to it. To resolve this issue, you can ensure that all references to the shared memory buffer are deleted before closing the shared memory.

```py
# reader.py, method1
from multiprocessing.shared_memory import SharedMemory

shm = SharedMemory("shm_test", create=False)
with open("result.zip", "wb") as file:
    data = shm.buf[:]
    file.write(data)

del data  # must del data,
# By explicitly deleting the data reference before closing the shared memory,
# you can avoid the `BufferError: cannot close exported pointers exist`
shm.close()
```

```py
# reader.py, method2 without using data variable
from multiprocessing.shared_memory import SharedMemory

shm = SharedMemory("shm_test", create=False)
with open("result.zip", "wb") as file:
    file.write(shm.buf[:])

shm.close()
```

```py
# reader.py for linux
from multiprocessing.shared_memory import SharedMemory
import multiprocessing.resource_tracker as rt

shm = SharedMemory("shm_test", create=False)
with open("result.zip", "wb") as file:
    file.write(shm.buf[:])

shm.close()
rt.unregister("/shm_test", "shared_memory")
```

### transfer file between processes by `mmap` in windows

The mmap library in Python doesn’t provide direct methods to retrieve metadata such as the size of the memory-mapped file. However, you can work around this limitation by using additional mechanisms to store and retrieve the size information.

```py
# writer.py
import mmap
import struct
import time

# Read the data from the file
with open("data.zip", "rb") as file:
    data = file.read()
    size = len(data)

# Create a memory-mapped file with a fixed-size header
header_size = struct.calcsize("Q")  # Size of an unsigned long long (8 bytes)
shm = mmap.mmap(-1, header_size + size, tagname="shm_test", access=mmap.ACCESS_WRITE)

# Write the size to the header
shm[:header_size] = struct.pack("Q", size)

# Write the data after the header
shm[header_size:] = data

try:
    while True:
        print("data available")
        time.sleep(3)
finally:
    shm.close()
```

```py
# reader.py
import mmap
import struct

# Define the size of the header
header_size = struct.calcsize("Q")  # Size of an unsigned long long (8 bytes)

# Open the memory-mapped file
shm = mmap.mmap(-1, header_size, tagname="shm_test", access=mmap.ACCESS_READ)

# Read the size from the header
size = struct.unpack("Q", shm[:header_size])[0]

# Resize the memory map to include the data
shm.close()
shm = mmap.mmap(-1, header_size + size, tagname="shm_test", access=mmap.ACCESS_READ)

# Read the data after the header
data = shm[header_size:]

# Write the data to a file
with open("result.zip", "wb") as file:
    file.write(data)

shm.close()
```

### transfer file between processes by `mmap` in linux

```py
# writer.py
# program1.py
import mmap
import struct
import time
import os

# Read the data from the file
with open("data.zip", "rb") as file:
    data = file.read()
    size = len(data)

# Create a memory-mapped file with a fixed-size header
header_size = struct.calcsize("Q")  # Size of an unsigned long long (8 bytes)
with open("/tmp/shm_test", "wb") as f:
    f.write(b"\x00" * (header_size + size))

with open("/tmp/shm_test", "r+b") as f:
    shm = mmap.mmap(f.fileno(), header_size + size, access=mmap.ACCESS_WRITE)

    # Write the size to the header
    shm[:header_size] = struct.pack("Q", size)

    # Write the data after the header
    shm[header_size:] = data

    try:
        while True:
            print("data available")
            time.sleep(3)
    finally:
        shm.close()
        os.remove("/tmp/shm_test")
```

```py
# reader.py
import mmap
import struct

# Define the size of the header
header_size = struct.calcsize("Q")  # Size of an unsigned long long (8 bytes)

with open("/tmp/shm_test", "r+b") as f:
    shm = mmap.mmap(f.fileno(), header_size, access=mmap.ACCESS_DEFAULT)

    # Read the size from the header
    size = struct.unpack("Q", shm[:header_size])[0]

    # Resize the memory map to include the data
    shm.resize(header_size + size)

    # Read the data after the header
    data = shm[header_size:]

    # Write the data to a file
    with open("result.zip", "wb") as file:
        file.write(data)

    shm.close()
```

## Transfer file between processes by boost.interprocess

` vcpkg install boost-interprocess`

```bash
.
├── CMakeLists.txt
├── recver.cpp
└── sender.cpp
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(sender sender.cpp)
add_executable(recver recver.cpp)

find_package(Boost REQUIRED COMPONENTS interprocess)
target_link_libraries(sender PRIVATE Boost::interprocess)
target_link_libraries(recver PRIVATE Boost::interprocess)
```

```cpp
// sender.cpp
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <cstdio>
#include <fstream>
#include <iostream>

using namespace boost::interprocess;

int main() {
    // Remove shared memory on construction and destruction
    struct shm_remove {
        shm_remove() { shared_memory_object::remove("shm_test"); }
        ~shm_remove() { shared_memory_object::remove("shm_test"); }
    } remover;

    // Open the file
    std::ifstream file("data.ipc", std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file\n";
        return 1;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    std::size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Create a shared memory object
    shared_memory_object shm(create_only, "shm_test", read_write);

    // Set size of the shared memory
    shm.truncate(fileSize);

    // Map the whole shared memory in this process
    mapped_region region(shm, read_write);

    // Get the address of the mapped region
    void* addr = region.get_address();

    // Read file content into shared memory
    file.read(static_cast<char*>(addr), fileSize);

    getchar();
}
```

```cpp
// recver.cpp
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <fstream>
#include <iostream>

using namespace boost::interprocess;

int main() {
    // Open the shared memory object
    shared_memory_object shm(open_only, "shm_test", read_only);

    // Map the whole shared memory in this process
    mapped_region region(shm, read_only);

    // Get the address of the mapped region
    void* addr = region.get_address();
    std::size_t size = region.get_size();

    // Open the output file
    std::ofstream file("data_out.bin", std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file\n";
        return 1;
    }

    // Write shared memory content to file
    file.write(static_cast<char*>(addr), size);

    return 0;
}
```