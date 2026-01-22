#pragma once
#include "CommandMessage.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <atomic>
#include <functional>
#include <string>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

namespace dashboard
{

class CommandHttpServer
{
public:
    using CommandHandler = std::function<void(const std::string& messageId, const std::string& rawJson)>;

    CommandHttpServer(net::io_context& ioc,
        std::atomic<bool>& runningFlag,
        unsigned short listenPort);

    void SetCommandHandler(CommandHandler handler);
    void Run(); // blocking accept loop
    std::pair<Command, int> ParseSingleCommandJson(const std::string& json);

private:
    void AcceptOne();
    void HandleSession(tcp::socket socket);

    http::response<http::string_body>
        MakeResponse(const http::request<http::string_body>& req);

    net::io_context& m_ioc;
    tcp::acceptor m_acceptor;
    std::atomic<bool>& m_running;
    CommandHandler m_commandHandler;
};

}