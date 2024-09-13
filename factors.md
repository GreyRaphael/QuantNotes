# Factors

- [Factors](#factors)
  - [FactorFactory with self-registering](#factorfactory-with-self-registering)


## FactorFactory with self-registering

```cpp
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>
#include <unordered_map>

struct TickData {
    int tot_av;
    int tot_bv;
};

template <typename T>
struct IFactor {
    virtual ~IFactor() = default;

    double feature{};
    virtual std::string_view name() = 0;
    virtual double update_feature(T const& quote) = 0;
    void update(T const& quote) {
        feature = update_feature(quote);
    }
};

template <typename T>
struct FactorFactory {
    // using CreatorFunc = std::unique_ptr<IFactor<T>> (*)();
    using CreatorFunc = std::function<std::unique_ptr<IFactor<T>>()>;

    // Use a function that returns a reference to a static local variable
    static std::unordered_map<std::string_view, CreatorFunc>& creatorMap() {
        static std::unordered_map<std::string_view, CreatorFunc> instance;
        return instance;
    }

    FactorFactory() = delete;

    static bool regFactor(std::string_view name, CreatorFunc creator) {
        auto& map = creatorMap();
        if (map.find(name) == map.end()) {
            map.emplace(name, creator);
            return true;
        }
        return false;
    }

    static std::unique_ptr<IFactor<T>> create(std::string_view name) {
        auto& map = creatorMap();
        auto itr = map.find(name);
        if (itr != map.end()) {
            return itr->second();
        }
        return nullptr;
    }
};

struct Factor01 : IFactor<TickData> {
    std::string_view name() override { return "Factor01"; }
    double update_feature(TickData const& quote) override { return quote.tot_av; }

    static std::unique_ptr<IFactor<TickData>> create() {
        return std::make_unique<Factor01>();
    }

    // Register the factor without worrying about static initialization order
    static inline bool registered = FactorFactory<TickData>::regFactor("Factor01", Factor01::create);
};

struct Factor02 : IFactor<TickData> {
    std::string_view name() override { return "Factor02"; }
    double update_feature(TickData const& quote) override { return quote.tot_av + quote.tot_bv; }

    static std::unique_ptr<IFactor<TickData>> create() {
        return std::make_unique<Factor02>();
    }

    // Register the factor without worrying about static initialization order
    static inline bool registered = FactorFactory<TickData>::regFactor("Factor02", create);
};

int main(int argc, char const* argv[]) {
    auto tick = TickData{100, 200};
    auto obj1 = FactorFactory<TickData>::create("Factor01");
    auto obj2 = FactorFactory<TickData>::create("Factor02");
    std::cout << obj1->update_feature(tick) << '\n';
    std::cout << obj2->update_feature(tick) << '\n';
}
```

