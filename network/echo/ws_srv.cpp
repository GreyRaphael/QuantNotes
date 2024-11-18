#include <fmt/core.h>
#include <hv/WebSocketServer.h>

int main(int argc, char **argv) {
    if (argc < 4) {
        fmt::println("usage: {} HOST PORT IO_THD_NUM(>=0)", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);
    auto io_thread_num = atoi(argv[3]);

    WebSocketService ws;
    ws.onopen = [](const WebSocketChannelPtr &channel, const HttpRequestPtr &req) {
        fmt::println("onpen: GET {}", req->Path().c_str());
    };
    ws.onmessage = [](const WebSocketChannelPtr &channel, const std::string &msg) {
        channel->send(msg.data(), msg.size());
    };
    ws.onclose = [](const WebSocketChannelPtr &channel) {
        fmt::println("onclose");
    };

    hv::WebSocketServer srv{&ws};
    srv.setHost(host);
    srv.setPort(port);
    srv.setThreadNum(1 + io_thread_num);
    fmt::println("ws listening on {}:{}", host, port);
    srv.run();
}