#include <fmt/core.h>
#include <hv/UdpClient.h>

#include <span>

int main(int argc, char **argv) {
    if (argc < 4) {
        fmt::println("usage: {} HOST PORT NUM", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);
    auto INIT_N = atoi(argv[3]);
    auto N = INIT_N;
    long total = 0;

    hv::UdpClient cli;
    if (auto sockfd = cli.createsocket(port, host); sockfd < 0) {
        return -20;
    }
    std::atomic<bool> done(false);

    cli.onMessage = [&](const hv::SocketChannelPtr &channel, hv::Buffer *buf) {
        auto result = reinterpret_cast<long const *>(buf->data());
        // fmt::println("recv: {}", *result);
        total += std::chrono::high_resolution_clock::now().time_since_epoch().count() - *result;

        if (--N > 0) {
            auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            auto view = std::span<char>(reinterpret_cast<char *>(&start), sizeof(long));
            cli.sendto(view.data(), view.size());
        } else {
            fmt::println("round={}, avg costs={}ns", INIT_N, total * 1.0 / INIT_N);
            done.store(true);
        }
    };

    cli.start();

    auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto view = std::span<char>(reinterpret_cast<char *>(&start), sizeof(long));
    cli.sendto(view.data(), view.size());
    // Poll the atomic flag
    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}