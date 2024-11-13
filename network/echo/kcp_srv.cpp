#include <fmt/core.h>
#include <hv/UdpServer.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        fmt::println("usage: {} HOST PORT", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);

    hv::UdpServer srv;
    if (auto bindfd = srv.createsocket(port, host); bindfd < 0) {
        return -20;
    }
    srv.onMessage = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        channel->write(buf->data(), buf->size());
    };

    srv.start();
    while (getchar() != '\n');
}