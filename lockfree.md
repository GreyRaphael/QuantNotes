# Lock-free programming

- [Lock-free programming](#lock-free-programming)
  - [SPMC broadcast](#spmc-broadcast)

## SPMC broadcast

support SPSC and SPMC broadcast.

```cpp
#include <array>
#include <atomic>
#include <chrono>
#include <format>
#include <iostream>
#include <limits>
#include <optional>
#include <thread>
#include <vector>

template <typename T, size_t BufNum, size_t MaxReaderNum>
class LockFreeQueue {
    static_assert(BufNum >= 2, "BufNum must be at least 2");
    static_assert(MaxReaderNum >= 1, "MaxReaderNum must be at least 1");

    std::array<T, BufNum> buffer_{};
    std::atomic<size_t> writerIndex_{0};
    std::array<std::atomic<size_t>, MaxReaderNum> readerIndices_{};

    template <typename U>
    bool do_push(U&& u) {
        // Get the minimum reader index
        size_t minReaderIndex = std::numeric_limits<size_t>::max();  // SIZE_MAX
        for (size_t i = 0; i < MaxReaderNum; ++i) {
            size_t readerIndex = readerIndices_[i].load(std::memory_order_acquire);
            if (readerIndex < minReaderIndex) {
                minReaderIndex = readerIndex;
            }
        }

        size_t wIndex = writerIndex_.load(std::memory_order_relaxed);

        // Check if buffer is full
        if (wIndex - minReaderIndex >= BufNum) {
            return false;
        }

        // Write data and update writer index
        buffer_[wIndex % BufNum] = std::forward<U>(u);
        writerIndex_.store(wIndex + 1, std::memory_order_release);
        return true;
    }

   public:
    // Push method for lvalue references
    bool push(const T& t) { return do_push(t); }

    // Push method for rvalue references
    bool push(T&& t) { return do_push(std::move(t)); }

    // Pop method for consumers using their unique ID
    std::optional<T> pop(size_t consumerId) {
        size_t rIndex = readerIndices_[consumerId].load(std::memory_order_relaxed);
        size_t wIndex = writerIndex_.load(std::memory_order_acquire);

        // Check if there's data to read
        if (rIndex >= wIndex) {
            return std::nullopt;
        }

        // Read data and update reader index
        T value = buffer_[rIndex % BufNum];
        readerIndices_[consumerId].store(rIndex + 1, std::memory_order_release);
        return value;
    }

    // Check if the queue is empty for a specific consumer
    bool empty(size_t consumerId) {
        size_t rIndex = readerIndices_[consumerId].load(std::memory_order_relaxed);
        size_t wIndex = writerIndex_.load(std::memory_order_acquire);
        return rIndex >= wIndex;
    }

    // Check if the queue is full
    bool full() {
        size_t minReaderIndex = std::numeric_limits<size_t>::max();
        for (size_t i = 0; i < MaxReaderNum; ++i) {
            size_t readerIndex = readerIndices_[i].load(std::memory_order_acquire);
            if (readerIndex < minReaderIndex) {
                minReaderIndex = readerIndex;
            }
        }
        size_t wIndex = writerIndex_.load(std::memory_order_relaxed);
        return (wIndex - minReaderIndex) >= BufNum;
    }
};

int main() {
    constexpr int MAX_READERS = 3;
    constexpr int BUFFER_CAPACITY = 1024;
    LockFreeQueue<int, BUFFER_CAPACITY, MAX_READERS> queue;

    // Producer thread
    std::jthread producer([&queue] {
        for (int i = 0; i < 50; ++i) {
            while (!queue.push(i)) {
                // Wait if the queue is full
                std::cout << "full, cannot push\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    std::vector<std::jthread> consumers;
    for (auto consumerId = 0; consumerId < MAX_READERS; ++consumerId) {
        consumers.emplace_back([&queue, consumerId] {
            while (true) {
                std::optional<int> value;
                while (!(value = queue.pop(consumerId))) {
                    std::cout << std::format("empty, consumer{} cannot got\n", consumerId);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
                std::cout << std::format("consumer{} got {}\n", consumerId, *value);
            }
        });
    }
}
```