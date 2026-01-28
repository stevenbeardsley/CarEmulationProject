#include "LogFile.h"
#include "Process.h"
#include "CommandHttpServer.h"
#include "DashboardDataSource.h"
#include "CommandMessage.h"
#include "shared/Peers.h"
#include "shared/can/Receiver.h"
#include <queue>
#include "shared/can/Message.h"
#include "shared/can/Bus.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <atomic>
#include <csignal>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace net = boost::asio;

using tcp = net::ip::tcp;

std::atomic<bool> running(true);

void signalHandler(int signal)
{
    LogFile::Info("Received stop signal, shutting down...");
    running = false;
}

void handleCarData(websocket::stream<tcp::socket> ws, dashboard::DashboardDataSource& dataSource) {
    LogFile::Info("Client subscribed to /carData");
    try {
        while (running) {
            std::string currentData = dataSource.getData();
            ws.write(net::buffer(currentData));
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    catch (const std::exception& e) {
        LogFile::Error("Error in /carData connection: " + std::string(e.what()));
    }
}

void handleCarCommands(websocket::stream<tcp::socket> ws) {
    LogFile::Info("Client subscribed to /carCommands");
    try
    {
        beast::flat_buffer buffer;
        while (running)
       	{
            ws.read(buffer);
            std::string message = beast::buffers_to_string(buffer.data());
            LogFile::Info("Received command: " + message);
            buffer.consume(buffer.size()); // clear buffer
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

        net::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));
        dashboard::DashboardDataSource dataSource;

        // === Background process ===
        dashboard::Process process;
        std::thread processThread([&process]() { process.run(); });

        // Set up CAN bus
        auto canBus = shared::can::Bus(15000);
        canBus.AddPeer("ecm");
        canBus.AddPeer("tcm");

        // Command Server
        // Command HTTP Server (8081)
        dashboard::CommandHttpServer commandServer{ ioc, running, 8081, canBus };
        commandServer.SetCommandHandler(
            [&](const std::string& messageId, const std::string& rawJson)
            {
                LogFile::Info("Message Received");
            });

        // IMPORTANT: actually run the accept loop on a thread
        std::thread commandThread([&]() { commandServer.Run(); });
        LogFile::Info("HTTP Server up and running.");

        LogFile::Info("WebSocket server listening on port 8080");
        // === Data updater thread ===
        std::thread updater([&]()
            {
                auto iteration = 0;
                while (running)
                {
                    auto speed = 50 + (iteration % 10) * 10;
                    auto status = (iteration % 3 != 0);
                    dataSource.updateData(speed, status);
                    /*LogFile::Debug("Updated data[" + std::to_string(iteration) +
                        "]: speed=" + std::to_string(speed) +
                        ", status=" + std::to_string(status));
                    */
                    iteration++;
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
            });

        // === Main WebSocket accept loop ===
        while (running)
       	{
            try
	        {
                tcp::socket socket(ioc);
                acceptor.accept(socket);

                beast::flat_buffer buffer;
                websocket::stream<tcp::socket> ws(std::move(socket));

                // Parse HTTP request first (before accepting)
                http::request<http::string_body> req;
                http::read(ws.next_layer(), buffer, req);

                std::string target = req.target();
                LogFile::Info("Incoming WebSocket connection to: " + target);

                // Accept WebSocket handshake
                ws.accept(req);

                // Spawn thread to handle based on path
                if (target == "/carData") {
                    std::thread(&handleCarData, std::move(ws), std::ref(dataSource)).detach();
                }
                else if (target == "/carCommands") {
                    std::thread(&handleCarCommands, std::move(ws)).detach();
                }
                else {
                    LogFile::Error("Unknown path: " + target);
                    ws.close(websocket::close_code::protocol_error);
                }

            }
            catch (const std::exception& e) {
                LogFile::Error("Client connection error: " + std::string(e.what()));
            }
        }


        // Receiver to receive and store information 
        shared::can::Receiver canRx(running, /*listenPort=*/15000);
        std::queue<shared::can::Message> inbox;
        std::mutex m;
        std::condition_variable cv;
        // Receiver thread: enqueue + notify
        canRx.SetHandler([&](shared::can::Message msg, const auto& sender) {
            (void)sender;
            {
                std::lock_guard<std::mutex> lk(m);
                inbox.push(std::move(msg));
            }
            cv.notify_one();
            });

        std::thread rxThread([&]() {
            canRx.Run(); // blocking loop inside receiver
            });

        std::thread processThread([&]() {
            while (running) {
                shared::can::Message msg = [&]() {
                    std::unique_lock<std::mutex> lk(m);
                    cv.wait(lk, [&]() { return !running || !inbox.empty(); });

                    // shutdown path
                    if (!running && inbox.empty()) {
                        // return *something* unreachable; we'll break above
                        // but keep structure simple:
                        // (we'll handle break outside)
                    }

                    if (inbox.empty()) {
                        // running became false
                        return shared::can::Message(shared::can::MessageType::RPM, 0);
                    }

                    auto m0 = std::move(inbox.front());
                    inbox.pop();
                    return m0;
                    }();

                if (!running) break;

                switch (msg.getMessageType()) {
                case shared::can::MessageType::CurrentGear:
                    LogFile::Info("Current gear received.");
                    dataSource.updateData(speed, status);
                    break;
                default:
                    break;
                }
            }
            });

        // === Graceful shutdown ===
        LogFile::Info("Shutting down dashboard.");
        updater.join();
        processThread.join();
        LogFile::Info("All threads stopped cleanly.");

    }
    catch (const std::exception& e) {
        std::cerr << "Fatal server error: " << e.what() << std::endl;
	LogFile::Error("Fatal error, server down");
	return 1;
    }

    return 0;
}
