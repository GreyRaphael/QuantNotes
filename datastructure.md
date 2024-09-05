# DataStructure

- [DataStructure](#datastructure)
  - [2d or 3d array based on single vector](#2d-or-3d-array-based-on-single-vector)

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