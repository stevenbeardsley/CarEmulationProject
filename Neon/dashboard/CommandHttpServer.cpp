#include "CommandHttpServer.h"
#include "LogFile.h"

#include "shared/can/MessageCategory.h"
#include "shared/can/messages/GearChange.h"
#include "shared/can/messages/Throttle.h"


namespace dashboard
{

CommandHttpServer::CommandHttpServer(net::io_context& ioc,
    std::atomic<bool>& runningFlag,
    unsigned short listenPort,
    shared::can::Bus& bus)
    : m_ioc(ioc)
    , m_acceptor(ioc, tcp::endpoint(tcp::v4(), listenPort))
    , m_running(runningFlag)
    , m_bus(bus)
{
    LogFile::info("CommandHttpServer listening on port " + std::to_string(listenPort));
}

void CommandHttpServer::SetCommandHandler(CommandHandler handler)
{
    m_commandHandler = std::move(handler);
}

void CommandHttpServer::Run()
{
    LogFile::info("CommandHttpServer accepting connections...");
    while (m_running)
    {
        try { AcceptOne(); }
        catch (const std::exception& e)
        {
            LogFile::error(std::string("HTTP accept loop error: ") + e.what());
        }
    }
    LogFile::info("CommandHttpServer exiting.");
}

void CommandHttpServer::AcceptOne()
{
    tcp::socket socket(m_ioc);
    m_acceptor.accept(socket);

    // One thread per connection (simple + fine for your use-case)
    std::thread(&CommandHttpServer::HandleSession, this, std::move(socket)).detach();
}

void CommandHttpServer::HandleSession(tcp::socket socket)
{
    try
    {
        beast::flat_buffer buffer;

        // Read exactly one HTTP request then respond (keep it simple)
        http::request<http::string_body> req;
        http::read(socket, buffer, req);

        auto res = MakeResponse(req);
        http::write(socket, res);

        // Close socket politely (since we’re not doing keep-alive)
        beast::error_code ec;
        socket.shutdown(tcp::socket::shutdown_send, ec);
    }
    catch (const std::exception& e)
    {
        LogFile::error(std::string("HTTP session error: ") + e.what());
    }
}

http::response<http::string_body>
CommandHttpServer::MakeResponse(const http::request<http::string_body>& req)
{
    auto badRequest = [&](std::string msg)
        {
            http::response<http::string_body> res{ http::status::bad_request, req.version() };
            res.set(http::field::content_type, "application/json");
            res.body() = std::string("{\"ok\":false,\"error\":\"") + msg + "\"}";
            res.content_length(res.body().size());
            res.keep_alive(false);
            return res;
        };

    auto notFound = [&]()
        {
            http::response<http::string_body> res{ http::status::not_found, req.version() };
            res.set(http::field::content_type, "application/json");
            res.body() = "{\"ok\":false,\"error\":\"not found\"}";
            res.content_length(res.body().size());
            res.keep_alive(false);
            return res;
        };

    // Only support POST /command
    if (req.method() != http::verb::post)
        return badRequest("use POST");

    if (req.target() != "/command")
        return notFound();

    const std::string& body = req.body();
    LogFile::info("Message body: " + body);
    
    // Extract JSON values 
    auto [command, value] = ParseSingleCommandJson(body);
    switch (command)
    {
        case Command::GearUp:
        {
            LogFile::info("Gear up request received.");
            shared::can::message::GearChange msg
            {
                shared::can::MessageCategory::Control,
                shared::can::headers::Control::GearUpRequest
            };
            m_bus.send(msg);
            break;
        }
        case Command::GearDown:
        {
            LogFile::info("Gear down request received.");
            shared::can::message::GearChange msg
            {
                shared::can::MessageCategory::Control,
                shared::can::headers::Control::GearDownRequest
            };
            m_bus.send(msg);
            break;
        }
        case Command::Throttle:
        {
            LogFile::info("Throttle change received.");
            shared::can::message::Throttle msg
            {
                shared::can::MessageCategory::Control,
                shared::can::headers::Control::ThrottleRequest,
                static_cast<std::uint32_t>(value)
            };
            m_bus.send(msg);
            break;
        }
        default:
        {
            LogFile::info("Unknown command type received.");
            break;
        }
    }
    

    http::response<http::string_body> res{ http::status::ok, req.version() };
    res.set(http::field::content_type, "application/json");
    res.body() = std::string("{\"ok\":true,\"command\":\"") +" TODO" +  "\"}";
    res.content_length(res.body().size());
    res.keep_alive(false);

    return res;
}

std::pair<Command, int> CommandHttpServer::ParseSingleCommandJson(const std::string& json)
{
    // Expected format: {"key": value}

    const std::size_t keyBegin = json.find('"') + 1;
    const std::size_t keyEnd = json.find('"', keyBegin);

    const std::size_t colonPos = json.find(':', keyEnd);
    std::size_t valPos = colonPos + 1;

    // Skip spaces after colon
    while (valPos < json.size() && json[valPos] == ' ')
        ++valPos;

    bool negative = false;
    if (json[valPos] == '-')
    {
        negative = true;
        ++valPos;
    }

    int value = 0;
    while (valPos < json.size() && std::isdigit(json[valPos]))
    {
        value = value * 10 + (json[valPos] - '0');
        ++valPos;
    }

    if (negative)
        value = -value;

    return {
        toCommand(json.substr(keyBegin, keyEnd - keyBegin)),
        value
    };
}


}