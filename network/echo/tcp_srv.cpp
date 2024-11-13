#include <fmt/core.h>
#include <hv/Channel.h>
#include <hv/TcpServer.h>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fmt::println("usage: {} HOST PORT", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);

    hv::TcpServer srv;
    if (auto listenfd = srv.createsocket(port, host); listenfd < 0) {
        return -20;
    }
    fmt::println("tcp listening {}:{}, listenfd={}", srv.host, srv.port, srv.listenfd);

    srv.onConnection = [](const hv::SocketChannelPtr& channel) {
        auto peeraddr = channel->peeraddr();
        if (channel->isConnected()) {
            fmt::println("{} connected! connfd={}", peeraddr, channel->fd());
        } else {
            fmt::println("{} disconnected! connfd={}", peeraddr, channel->fd());
        }
    };
    srv.onMessage = [](const hv::SocketChannelPtr& channel, hv::Buffer* buf) {
        channel->write(buf->data(), buf->size());
    };

    unpack_setting_t setting{};
    setting.mode = UNPACK_BY_FIXED_LENGTH;
    setting.fixed_length = 8;
    srv.setUnpack(&setting);

    srv.start();
    while (getchar() != '\n');
}