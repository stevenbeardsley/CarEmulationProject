#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <thread>
#include <atomic>
#include <string>

namespace net = boost::asio;
namespace websocket = boost::beast::websocket;
using tcp = net::ip::tcp;

class WebSocketReceiver {
public:
    WebSocketReceiver(tcp::acceptor& acceptor, std::atomic<bool>& running);
    ~WebSocketReceiver();

    void start();
    void stop();

private:
    void listen();
    void handleSession(tcp::socket socket);
    void processMessage(const std::string& msg);

    tcp::acceptor& acceptor_;
    std::atomic<bool>& running_;
    std::thread listenerThread_;
};
