# Memory-Mapped File

- [Memory-Mapped File](#memory-mapped-file)
  - [Reading a Memory-Mapped File With mmap](#reading-a-memory-mapped-file-with-mmap)
  - [mmap for Shared Memory](#mmap-for-shared-memory)


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

## mmap for Shared Memory

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