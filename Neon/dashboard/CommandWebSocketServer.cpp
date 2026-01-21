#include "CommandWebSocketServer.h"
#include <thread>

#include "LogFile.h"

CommandWebSocketServer::CommandWebSocketServer(
    net::io_context& ioc,
    std::atomic<bool>& runningFlag,
    unsigned short listenPort)
    : m_ioc(ioc)
    , m_acceptor(ioc, tcp::endpoint(tcp::v4(), listenPort))
    , m_running(runningFlag)
{
    LogFile::Info("CommandWebSocketServer listening on port " + std::to_string(listenPort));
}

void CommandWebSocketServer::SetCommandHandler(CommandHandler handler)
{
    m_commandHandler = std::move(handler);
}

void CommandWebSocketServer::Run()
{
    LogFile::Info("CommandWebSocketServer accepting connections...");

    while (m_running)
    {
        try { AcceptOne(); }
        catch (const std::exception& e)
        {
            LogFile::Error(std::string("Command accept loop error: ") + e.what());
        }
    }

    LogFile::Info("CommandWebSocketServer exiting.");
}

void CommandWebSocketServer::AcceptOne()
{
    tcp::socket socket(m_ioc);
    m_acceptor.accept(socket);

    beast::flat_buffer buffer;
    websocket::stream<tcp::socket> ws(std::move(socket));

    http::request<http::string_body> req;
    http::read(ws.next_layer(), buffer, req);

    const std::string target = req.target();
    LogFile::Info("Incoming WS connection to: " + target);

    ws.accept(req);

    if (target == "/carCommands")
    {
        std::thread(&CommandWebSocketServer::HandleCommands, this, std::move(ws)).detach();
    }
    else
    {
        LogFile::Error("Command server unknown path: " + target);
        ws.close(websocket::close_code::protocol_error);
    }
}

void CommandWebSocketServer::HandleCommands(websocket::stream<tcp::socket> ws)
{
    LogFile::Info("Client subscribed to /carCommands");

    try
    {
        beast::flat_buffer buffer;

        while (m_running)
        {
            ws.read(buffer);
            std::string msg = beast::buffers_to_string(buffer.data());
            buffer.consume(buffer.size());

            LogFile::Info("Received command: " + msg);

            if (m_commandHandler)
                m_commandHandler(msg);
        }
    }
    catch (const std::exception& e)
    {
        LogFile::Error(std::string("Error in /carCommands: ") + e.what());
    }
}
