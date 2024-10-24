# log

- [log](#log)
  - [low latency log in cpp](#low-latency-log-in-cpp)

## low latency log in cpp

recommened low latency logger
- [fmtlog](https://github.com/MengRao/fmtlog), recommended
- [quill](https://github.com/odygrd/quill), you can visit complete benchmark of many kinds of log library

fmtlog usage:
1. Just copy fmtlog.h and fmtlog-inl.h to your project
2. Just `#include "fmtlog-inl.h"`

```cpp
// simple usage
#include "fmtlog-inl.h"

int main(int argc, char const *argv[]) {
    {
        fmtlog::setLogLevel(fmtlog::DBG);  // default is debug, you can delete this line
        logd("hello {}", 100);
        logi("hello {}", 100);
    }
    {
        fmtlog::setLogFile("test.log");
        logd("hey {}", 100);
        logi("hey {}", 100);
    }
}
```

```cpp
// logger in multi-threads, only one thread can poll
#include <chrono>
#include <thread>
#include <vector>
#include "fmtlog-inl.h"

int main(int argc, char const *argv[]) {
    std::vector<std::thread> threads;
    for (size_t j = 0; j < 5; ++j) {
        threads.emplace_back([j]() {
            for (size_t i = 0; i < 10; ++i) {
                // auto name = std::format("thread_{}", j);
                // fmtlog::setThreadName(name.c_str());
                logi("get {} in t{}", i, j);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }
    for (auto &&e : threads) {
        e.join();
    }

    // main thread poll
    fmtlog::poll();
}
```

```cpp
// logger in multi-threads, poll in background, recommended
#include <chrono>
#include <thread>
#include <vector>
#include "fmtlog-inl.h"

int main(int argc, char const *argv[]) {
    fmtlog::startPollingThread(500);  // background auto poll

    std::vector<std::jthread> threads;
    for (size_t j = 0; j < 5; ++j) {
        threads.emplace_back([j]() {
            for (size_t i = 0; i < 10; ++i) {
                logi("get {} in t{}", i, j);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }
}
```