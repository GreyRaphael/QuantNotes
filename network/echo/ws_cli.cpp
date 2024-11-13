#include <fmt/core.h>
#include <hv/WebSocketClient.h>

#include <span>

int main(int argc, char **argv) {
    if (argc < 4) {
        fmt::println("usage: {} HOST PORT NUM", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);
    auto addr = fmt::format("ws://{}:{}", host, port);
    auto INIT_N = atoi(argv[3]);
    auto N = INIT_N;
    long total = 0;

    std::atomic<bool> done(false);

    hv::WebSocketClient ws;
    ws.onopen = [&] {
        fmt::println("onopen");
        // first message must be in onopen
        auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto view = std::span<char>(reinterpret_cast<char *>(&start), sizeof(long));
        ws.send(view.data(), view.size());
    };
    ws.onclose = [&] {
        fmt::println("onclose");
        done.store(true);
    };
    ws.onmessage = [&](const std::string &msg) {
        auto result = reinterpret_cast<long const *>(msg.data());
        // fmt::println("cli recv {}", *result);
        total += std::chrono::high_resolution_clock::now().time_since_epoch().count() - *result;

        if (--N > 0) {
            auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            auto view = std::span<char>(reinterpret_cast<char *>(&start), sizeof(long));
            ws.send(view.data(), view.size());
        } else {
            fmt::println("round={}, avg costs={}ns", INIT_N, total * 1.0 / INIT_N);
            done.store(true);
        }
    };

    ws.open(addr.c_str());

    // Poll the atomic flag
    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}