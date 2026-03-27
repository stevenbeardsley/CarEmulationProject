#ifndef DASHBOARD_COMMANDHTTPSERVER_H
#define DASHBOARD_COMMANDHTTPSERVER_H

#include "CommandMessage.h"
#include "shared/can/Bus.h"
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
        unsigned short listenPort,
        shared::can::Bus& bus);

    void setCommandHandler(CommandHandler handler);
    void run(); // blocking accept loop
    
    [[nodiscard]]
    static std::pair<Command, int> parseSingleCommandJson(const std::string& json);

private:
    void acceptOne();
    void handleSession(tcp::socket socket) const;

    http::response<http::string_body> makeResponse(const http::request<http::string_body>& req) const;

    net::io_context& m_ioc;
    tcp::acceptor m_acceptor;
    std::atomic<bool>& m_running;
    CommandHandler m_commandHandler;
    shared::can::Bus& m_bus;
};

}
#endif 