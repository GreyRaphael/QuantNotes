# Memory-Mapped File

- [Memory-Mapped File](#memory-mapped-file)
  - [Reading a Memory-Mapped File With mmap](#reading-a-memory-mapped-file-with-mmap)


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