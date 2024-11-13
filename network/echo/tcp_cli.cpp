#include <fmt/core.h>
#include <hv/TcpClient.h>

#include <span>

int main(int argc, char** argv) {
    if (argc < 4) {
        fmt::println("usage: {} HOST PORT NUM", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);
    auto INIT_N = atoi(argv[3]);
    auto N = INIT_N;
    long total = 0;

    hv::TcpClient cli;
    if (auto connfd = cli.createsocket(port, host); connfd < 0) {
        return -20;
    }
    std::atomic<bool> done(false);

    cli.onConnection = [&](const hv::SocketChannelPtr& channel) {
        std::string peeraddr = channel->peeraddr();
        if (channel->isConnected()) {
            fmt::println("connected to {}! connfd={}", peeraddr, channel->fd());

            auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            auto view = std::span<char>(reinterpret_cast<char*>(&start), sizeof(long));
            cli.send(view.data(), view.size());
        } else {
            fmt::println("disconnected to {}! connfd={}", peeraddr, channel->fd());
            done.store(true);
        }
    };
    cli.onMessage = [&](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        auto result = reinterpret_cast<long const*>(buf->data());
        total += std::chrono::high_resolution_clock::now().time_since_epoch().count() - *result;

        if (--N > 0) {
            auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            auto view = std::span<char>(reinterpret_cast<char*>(&start), sizeof(long));
            cli.send(view.data(), view.size());
        } else {
            fmt::println("round={}, avg costs={}ns", INIT_N, total * 1.0 / INIT_N);
            done.store(true);
        }
    };

    unpack_setting_t setting{};
    setting.mode = UNPACK_BY_FIXED_LENGTH;
    setting.fixed_length = 8;
    cli.setUnpack(&setting);
    cli.start();

    // Poll the atomic flag
    while (!done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}