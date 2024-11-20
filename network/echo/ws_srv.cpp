#include <fmt/core.h>
#include <hv/WebSocketServer.h>

int main(int argc, char **argv) {
    if (argc < 5) {
        fmt::println("usage: {} HOST PORT MODE(0|1) WORKERS(>=0)", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);
    auto mode = atoi(argv[3]);
    auto workers = atoi(argv[4]);  // worker number

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

    if (mode == 0) {
        srv.setThreadNum(1 + workers);
    } else if (mode == 1) {
        srv.setProcessNum(1 + workers);
    }

    fmt::println("ws listening on {}:{}", host, port);
    srv.run();
}