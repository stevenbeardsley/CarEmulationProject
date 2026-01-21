#include "CommandHttpServer.h"
#include "LogFile.h"
#include <thread>
#include <sstream>

CommandHttpServer::CommandHttpServer(net::io_context& ioc,
    std::atomic<bool>& runningFlag,
    unsigned short listenPort)
    : m_ioc(ioc)
    , m_acceptor(ioc, tcp::endpoint(tcp::v4(), listenPort))
    , m_running(runningFlag)
{
    LogFile::Info("CommandHttpServer listening on port " + std::to_string(listenPort));
}

void CommandHttpServer::SetCommandHandler(CommandHandler handler)
{
    m_commandHandler = std::move(handler);
}

void CommandHttpServer::Run()
{
    LogFile::Info("CommandHttpServer accepting connections...");
    while (m_running)
    {
        try { AcceptOne(); }
        catch (const std::exception& e)
        {
            LogFile::Error(std::string("HTTP accept loop error: ") + e.what());
        }
    }
    LogFile::Info("CommandHttpServer exiting.");
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
        LogFile::Error(std::string("HTTP session error: ") + e.what());
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

    // Expect JSON body like: {"messageId":"GEAR_UP","value":1}
    const std::string& body = req.body();
    const std::string messageId = ExtractJsonStringField(body, "messageId");
    if (messageId.empty())
        return badRequest("missing messageId");

    LogFile::Info("HTTP command received messageId=" + messageId + " body=" + body);

    if (m_commandHandler)
        m_commandHandler(messageId, body);

    http::response<http::string_body> res{ http::status::ok, req.version() };
    res.set(http::field::content_type, "application/json");
    res.body() = std::string("{\"ok\":true,\"messageId\":\"") + messageId + "\"}";
    res.content_length(res.body().size());
    res.keep_alive(false);
    return res;
}

// Tiny “good enough” JSON string-field extractor (no external deps).
// Assumes: "key":"VALUE" with double quotes.
std::string CommandHttpServer::ExtractJsonStringField(const std::string& json, const std::string& key)
{
    const std::string needle = "\"" + key + "\"";
    auto kpos = json.find(needle);
    if (kpos == std::string::npos) return {};

    auto colon = json.find(':', kpos + needle.size());
    if (colon == std::string::npos) return {};

    auto firstQuote = json.find('"', colon + 1);
    if (firstQuote == std::string::npos) return {};

    auto secondQuote = json.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) return {};

    return json.substr(firstQuote + 1, secondQuote - (firstQuote + 1));
}
