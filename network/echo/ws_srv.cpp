#include <fmt/core.h>
#include <hv/WebSocketServer.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fmt::println("usage: {} HOST PORT", argv[0]);
        return 1;
    }
    auto host = argv[1];
    auto port = atoi(argv[2]);

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
    fmt::println("ws listening on {}:{}", host, port);
    srv.run();
}