#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <atomic>
#include <functional>
#include <string>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class CommandWebSocketServer
{
public:
    using CommandHandler = std::function<void(const std::string&)>;

    CommandWebSocketServer(
        net::io_context& ioc,
        std::atomic<bool>& runningFlag,
        unsigned short listenPort);

    void SetCommandHandler(CommandHandler handler);

    // Blocking accept loop (call in its own thread or main thread)
    void Run();

private:
    void AcceptOne();
    void HandleCommands(websocket::stream<tcp::socket> ws);

private:
    net::io_context& m_ioc;
    tcp::acceptor m_acceptor;
    std::atomic<bool>& m_running;
    CommandHandler m_commandHandler;
};
