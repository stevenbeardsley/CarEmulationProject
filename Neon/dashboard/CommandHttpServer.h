#ifndef DASHBOARD_COMMANDHTTPSERVER_H
#define DASHBOARD_COMMANDHTTPSERVER_H

#include "shared/can/MessageType.h"
#include "shared/can/Message.h"
#include "CommandMessage.h"
#include "shared/can/Bus.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <atomic>
#include <functional>
#include <string>
#include <cstdint>

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
        unsigned short listenPort,
        shared::can::Bus& bus);

    void SetCommandHandler(CommandHandler handler);
    void Run(); // blocking accept loop
    
    [[nodiscard]]
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
    shared::can::Bus& m_bus;
};

}
#endif 