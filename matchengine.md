# Match Engine

## MatchEngine in cpp

### limit price matcher

simple match engine of limit price order

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.5.0)
project(proj1 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
add_executable(proj1 main.cpp)

# this is heuristically generated, and may not be correct
find_package(absl CONFIG REQUIRED)
# note: 180 additional targets are not displayed.
target_link_libraries(proj1 PRIVATE absl::container_common)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(proj1 PRIVATE fmt::fmt)
```

```cpp
// main.cpp
#include <absl/container/btree_set.h>

#include <iostream>

struct Data {
    int64_t dt;
    double price;
    int32_t volume;
};

// Custom comparator for buy orders (highest price first)
struct BuyOrderCompare {
    bool operator()(const Data& lhs, const Data& rhs) const {
        if (lhs.price != rhs.price) {
            return lhs.price > rhs.price;  // Higher prices come first
        }
        return lhs.dt < rhs.dt;  // For equal prices, earlier timestamps come first
    }
};

// Custom comparator for sell orders (lowest price first)
struct SellOrderCompare {
    bool operator()(const Data& lhs, const Data& rhs) const {
        if (lhs.price != rhs.price) {
            return lhs.price < rhs.price;  // Lower prices come first
        }
        return lhs.dt < rhs.dt;  // For equal prices, earlier timestamps come first
    }
};

// Typedefs for ease of use
using BuyOrders = absl::btree_multiset<Data, BuyOrderCompare>;
using SellOrders = absl::btree_multiset<Data, SellOrderCompare>;

// Function to simulate order matching
void matchOrders(BuyOrders& buy_orders, SellOrders& sell_orders) {
    while (!buy_orders.empty() && !sell_orders.empty()) {
        const Data& highest_buy = *buy_orders.begin();
        const Data& lowest_sell = *sell_orders.begin();

        if (highest_buy.price >= lowest_sell.price) {
            // Match found
            int32_t matched_volume = std::min(highest_buy.volume, lowest_sell.volume);

            std::cout << "Match found!\n";
            std::cout << "Buy order: Price = " << highest_buy.price << ", Volume = " << matched_volume << '\n';
            std::cout << "Sell order: Price = " << lowest_sell.price << ", Volume = " << matched_volume << '\n';

            // Adjust or remove matched orders
            if (highest_buy.volume > matched_volume) {
                Data updated_buy = highest_buy;
                updated_buy.volume -= matched_volume;
                buy_orders.erase(buy_orders.begin());
                buy_orders.insert(updated_buy);
            } else {
                buy_orders.erase(buy_orders.begin());
            }

            if (lowest_sell.volume > matched_volume) {
                Data updated_sell = lowest_sell;
                updated_sell.volume -= matched_volume;
                sell_orders.erase(sell_orders.begin());
                sell_orders.insert(updated_sell);
            } else {
                sell_orders.erase(sell_orders.begin());
            }
        } else {
            // No match possible, break the loop
            break;
        }
    }
}

int main() {
    // Create the buy and sell order books
    BuyOrders buy_orders;
    SellOrders sell_orders;

    // Add some orders
    buy_orders.insert({1620000000, 100.5, 10});  // dt, price, volume
    buy_orders.insert({1620000100, 100.0, 5});
    sell_orders.insert({1620000200, 101.0, 20});
    sell_orders.insert({1620000300, 99.5, 15});

    // Print initial orders
    std::cout << "Initial Sell Orders:\n";
    for (auto it = sell_orders.rbegin(); it != sell_orders.rend(); ++it) {
        std::cout << "Price: " << it->price << ", Volume: " << it->volume << '\n';
    }

    std::cout << "Initial Buy Orders:\n";
    for (const auto& order : buy_orders) {
        std::cout << "Price: " << order.price << ", Volume: " << order.volume << '\n';
    }

    // Match orders
    matchOrders(buy_orders, sell_orders);

    // Print remaining orders
    std::cout << "\nRemaining Buy Orders:\n";
    for (const auto& order : buy_orders) {
        std::cout << "Price: " << order.price << ", Volume: " << order.volume << '\n';
    }

    std::cout << "\nRemaining Sell Orders:\n";
    for (const auto& order : sell_orders) {
        std::cout << "Price: " << order.price << ", Volume: " << order.volume << '\n';
    }

    return 0;
}
```