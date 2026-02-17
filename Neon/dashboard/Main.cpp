#include "CommandHttpServer.h"
#include "CommandMessage.h"
#include "DashboardDataSource.h"
#include "LogFile.h"
#include "Process.h"
#include "shared/Peers.h"
#include "shared/can/Bus.h"
#include "shared/can/Message.h"
#include "shared/can/Receiver.h"
#include "shared/config/Config.h"

#include <atomic>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

static std::atomic<bool> running(true);

// Used to break a blocking accept() when SIGINT/SIGTERM hits
static tcp::acceptor* g_acceptor = nullptr;

void signalHandler(int)
{
    LogFile::Info("Received stop signal, shutting down...");
    running = false;

    // Unblock the accept() call if it's currently waiting.
    if (g_acceptor)
    {
        boost::system::error_code ec;
        g_acceptor->close(ec);
    }
}

// NOTE: These handlers run on detached threads (one per client).
// They must not touch shared state unsafely.
void handleCarData(websocket::stream<tcp::socket> ws, dashboard::DashboardDataSource& dataSource)
{
    LogFile::Info("Client subscribed to /carData");
    try
    {
        while (running)
        {
            std::string currentData = dataSource.GetDataJson();
            ws.write(net::buffer(currentData));
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    catch (const std::exception& e)
    {
        LogFile::Error("Error in /carData connection: " + std::string(e.what()));
    }
}

void handleCarCommands(websocket::stream<tcp::socket> ws)
{
    LogFile::Info("Client subscribed to /carCommands");
    try
    {
        beast::flat_buffer buffer;
        while (running)
        {
            ws.read(buffer);
            std::string message = beast::buffers_to_string(buffer.data());
            LogFile::Info("Received command: " + message);
            buffer.consume(buffer.size());
        }
    }
    catch (const std::exception& e)
    {
        LogFile::Error("Error in /carCommands connection: " + std::string(e.what()));
    }
}

int main()
{
    try
    {
        // === Logging setup ===
        LogFile::Instance().setLogFile("dashboard.log");
        LogFile::Instance().setLevel(LogLevel::DEBUG);

        // === Setup signals ===
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);


        // =========================
        // Config file data 
        // =========================
        shared::config::Config config;
        config.LoadFromFile("config.json");
        const auto engineConfig = config.getEngineConfig();
        const auto transmissionConfig = config.getTransmissionConfig();


        // =========================
        // Shared dashboard data
        // =========================
        dashboard::UiData uiData{ 0, 0, 0, 0, true };
        dashboard::DashboardDataSource dataSource{ uiData };
        dataSource.SetMaxRpms(engineConfig.max_rpm);

        // =========================
        // Optional background process
        // =========================
        dashboard::Process process;
        std::thread dashboardProcessThread([&process]()
            {
                process.run();
            });

        // =========================
        // CAN bus + receiver threads
        // (MUST be created BEFORE the accept loop)
        // =========================
        shared::can::Bus canBus(
            0,      // ephemeral bind
            15000   // default peer destination
        );     
        
        if (!canBus.AddPeer("ecm", 15000))
        {
            LogFile::Error("Error: ecm was not added as a peer");
        }
        if (!canBus.AddPeer("tcm", 15000))
        {
            LogFile::Error("Error: tcm was not added as a peer");
        }

        shared::can::Receiver canRx(running, /*listenPort=*/15000);

        std::mutex inboxMutex;
        std::condition_variable inboxCv;
        std::queue<shared::can::Message> inbox;

        // Producer: receiver pushes into inbox
        canRx.SetHandler([&](shared::can::Message msg, const auto& sender)
            {
                (void)sender;
                {
                    std::lock_guard<std::mutex> lk(inboxMutex);
                    inbox.push(std::move(msg));
                }
                inboxCv.notify_one();
            });

        std::thread canRxThread([&]()
            {
                canRx.Run(); // blocking loop
            });

        // Consumer: pop inbox and update dashboard data
        std::thread canProcessThread([&]()
            {
                while (running)
                {
                    shared::can::Message msg = [&]()
                        {
                            std::unique_lock<std::mutex> lk(inboxMutex);
                            inboxCv.wait(lk, [&] { return !running || !inbox.empty(); });

                            if (!running && inbox.empty())
                            {
                                // Shutdown path: return a dummy; outer loop will exit.
                                return shared::can::Message(shared::can::MessageType::RPM, 0);
                            }

                            auto m0 = std::move(inbox.front());
                            inbox.pop();
                            return m0;
                        }();

                    if (!running)
                    {
                        break;
                    }

                    switch (msg.getMessageType())
                    {
                    case shared::can::MessageType::CurrentGear:
                        dataSource.SetGear(msg.getValue());
                        break;
                    case shared::can::MessageType::Speed:
                        dataSource.SetSpeed(msg.getValue());
                        break;
                    case shared::can::MessageType::RPM:
                        dataSource.SetRpm(msg.getValue());
                        break;
                    case shared::can::MessageType::EngineTemperature:
                        dataSource.SetEngineTemp(msg.getValue());
                        break;
                    }
                }
            });

        // =========================
        // Command server thread
        // Use its own io_context so it doesn't depend on websocket loop.
        // =========================
        net::io_context cmdIoc;

        dashboard::CommandHttpServer commandServer{ cmdIoc, running, 8081, canBus };
        commandServer.SetCommandHandler(
            [&](const std::string& messageId, const std::string& rawJson)
            {
                (void)messageId;
                (void)rawJson;
                LogFile::Info("Message Received");
                // TODO: parse command + publish CAN message(s)
            });

        std::thread commandThread([&]()
            {
                commandServer.Run(); // blocking loop (per your design)
            });

        LogFile::Info("HTTP Server up and running.");


        // =========================
        // WebSocket server (main thread)
        // =========================
        net::io_context wsIoc;
        tcp::acceptor acceptor(wsIoc, tcp::endpoint(tcp::v4(), 8080));
        g_acceptor = &acceptor;

        LogFile::Info("WebSocket server listening on port 8080");

        while (running)
        {
            try
            {
                tcp::socket socket(wsIoc);

                // Blocking accept; signal handler closes acceptor to break out.
                boost::system::error_code ec;
                acceptor.accept(socket, ec);

                if (!running) break;

                if (ec)
                {
                    // If we closed acceptor during shutdown, accept() will error.
                    if (!running) break;
                    LogFile::Error("Accept failed: " + ec.message());
                    continue;
                }

                beast::flat_buffer buffer;
                websocket::stream<tcp::socket> ws(std::move(socket));

                // Parse HTTP request first (before accepting WS)
                http::request<http::string_body> req;
                http::read(ws.next_layer(), buffer, req);

                const std::string target = std::string(req.target());
                LogFile::Info("Incoming WebSocket connection to: " + target);

                ws.accept(req);

                if (target == "/carData")
                {
                    std::thread(&handleCarData, std::move(ws), std::ref(dataSource)).detach();
                }
                else if (target == "/carCommands")
                {
                    std::thread(&handleCarCommands, std::move(ws)).detach();
                }
                else
                {
                    LogFile::Error("Unknown path: " + target);
                    ws.close(websocket::close_code::protocol_error);
                }
            }
            catch (const std::exception& e)
            {
                if (!running) break;
                LogFile::Error("Client connection error: " + std::string(e.what()));
            }
        }

        // =========================
        // Graceful shutdown
        // =========================
        LogFile::Info("Shutting down dashboard...");

        inboxCv.notify_all();

        if (commandThread.joinable()) commandThread.join();
        if (canRxThread.joinable()) canRxThread.join();
        if (canProcessThread.joinable()) canProcessThread.join();
        if (dashboardProcessThread.joinable()) dashboardProcessThread.join();

        LogFile::Info("All threads stopped cleanly.");
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal server error: " << e.what() << std::endl;
        LogFile::Error("Fatal error, server down");
        return 1;
    }
}
