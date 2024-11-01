# DataStructure

- [DataStructure](#datastructure)
  - [2d or 3d array based on single vector](#2d-or-3d-array-based-on-single-vector)
  - [fixed-size vector](#fixed-size-vector)

## 2d or 3d array based on single vector

dynamic 2d array

```cpp
#include <cmath>
#include <iostream>
#include <vector>

template <typename T>
class Dynamic2DArray {
   public:
    Dynamic2DArray(size_t rows, size_t cols) : rows_(rows), cols_(cols) {
        data_.reserve(rows * cols);
    }

    // Access element (i, j), inline is more efficient
    inline T& operator()(size_t i, size_t j) {
        return data_[i * cols_ + j];
    }

    // Const version for read-only access
    inline const T& operator()(size_t i, size_t j) const {
        return data_[i * cols_ + j];
    }

    void append(T x) {
        data_.emplace_back(x);
    }

    size_t rows() const { return std::ceil(1.0 * data_.size() / cols_); }
    size_t cols() const { return cols_; }

   private:
    size_t rows_;
    size_t cols_;
    std::vector<T> data_;
};

int main() {
    size_t rows = 3;
    size_t cols = 4;
    Dynamic2DArray<double> array(rows, cols);

    // Assign values
    for (size_t i = 0; i < 10; ++i) {
        array.append(i);
    }

    // Print values
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            std::cout << array(i, j) << " ";
        }
        std::cout << "\n";
    }

    std::cout << "shape: rows=" << array.rows() << ", cols=" << array.cols() << '\n';  // shape: rows=3, cols=4
}
```

## fixed-size vector

use fixed-size continuous memory on heap
- method1(recommended): `delete` size-changing methods like `push_back`, `emplace_back`, `resize`, and `clear` of `std::vector`
- method2: using `std::unique_ptr<T[]>` with `std::span<T>`
- method3: using `std::unique_ptr<T[]>` with size

method1: modified `std::vector`

```cpp
template <typename T>
class FixedSizeVector {
   public:
    FixedSizeVector(size_t size) : data(size) {}

    // Provide access to elements, boundary checking with .at()
    T& operator[](size_t index) { return data.at(index); }
    const T& operator[](size_t index) const { return data.at(index); }

    // Provide iterators if needed
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }

    size_t size() const { return data.size(); }

   private:
    std::vector<T> data;

    // Delete size-changing methods
    void push_back(const T&) = delete;
    void emplace_back(T&&) = delete;
    void resize(size_t) = delete;
    void clear() = delete;
};
```

method2: with `std::span`

```cpp
template <typename T>
struct MyStruct {
    // MyStruct(size_t Num) : buffer(new T[Num]), span(buffer.get(), Num) {}
    MyStruct(size_t Num) : buffer(std::make_unique<T[]>(Num)), span(buffer.get(), Num) {}

    // Use span for element access, no boundray checking
    T& operator[](size_t index) { return span[index]; }
    auto begin() { return span.begin(); }
    auto end() { return span.end(); }

   private:
    std::unique_ptr<T[]> buffer;
    std::span<T> span;
};
```

method3: no boundary checking

```cpp
template <typename T>
class FixedSizeContainer {
   public:
    FixedSizeContainer(size_t size) : size_(size), data_(new T[size]) {}

    // no bounded checking
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }

    size_t size() const { return size_; }

   private:
    size_t size_;
    std::unique_ptr<T[]> data_;
};
```