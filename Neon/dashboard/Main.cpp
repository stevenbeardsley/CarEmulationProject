#include "CommandHttpServer.h"
#include "DashboardDataSource.h"
#include "LogFile.h"
#include "Process.h"
#include "shared/can/Bus.h"
#include "shared/can/messages/ErrorMessage.h"
#include "shared/can/messages/StatusMessage.h"
#include "shared/can/headers/Status.h"
#include "shared/can/Receiver.h"
#include "shared/config/Config.h"
#include "shared/bit_parser/BitReader.h"

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

namespace
{
	
std::atomic running(true);
tcp::acceptor* g_acceptor = nullptr;

void signalHandler(int)
{
    LogFile::info("Received stop signal, shutting down...");
    running = false;
    if (g_acceptor)
    {
        boost::system::error_code ec;
        g_acceptor->close(ec);
    }
}

void handleCarData(websocket::stream<tcp::socket> ws, dashboard::DashboardDataSource& dataSource)
{
    LogFile::info("Client subscribed to /carData");
    try
    {
        while (running)
        {
            std::string currentData = dataSource.getDataJson();
            ws.write(net::buffer(currentData));
            dataSource.clearErrors();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    catch (const std::exception& e)
    {
        LogFile::error("Error in /carData connection: " + std::string(e.what()));
    }
}

void handleCarCommands(websocket::stream<tcp::socket> ws)
{
    LogFile::info("Client subscribed to /carCommands");
    try
    {
        beast::flat_buffer buffer;
        while (running)
        {
            ws.read(buffer);
            std::string message = beast::buffers_to_string(buffer.data());
            LogFile::info("Received command: " + message);
            buffer.consume(buffer.size());
        }
    }
    catch (const std::exception& e)
    {
        LogFile::error("Error in /carCommands connection: " + std::string(e.what()));
    }
}
}

int main()
{
    try
    {
        // Logging & Signals 
        LogFile::instance().setLogFile("dashboard.log");
        LogFile::instance().setLevel(LogLevel::DEBUG);
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        // Config 
        shared::config::Config config;
        config.LoadFromFile("config.json");
        const auto& engineConfig = config.getEngineConfig();

        dashboard::UiData uiData{ 0, 0, 0, 0, true };
        dashboard::DashboardDataSource dataSource{ uiData };
        dataSource.setMaxRpms(engineConfig.max_rpm);

        dashboard::Process process;
        std::thread dashboardProcessThread([&process]() { process.run(); });

        std::mutex inboxMutex;
        std::condition_variable inboxCv;
        std::queue<std::vector<std::uint8_t>> inbox;

        shared::can::Bus canBus(0, 15000);
        (void)canBus.addPeer("ecm", 15000);
        (void)canBus.addPeer("tcm", 15000);

        // The Receiver now manages the inbox directly
        shared::can::Receiver canRx(running, 15000, inbox, inboxMutex, inboxCv);

        std::thread canRxThread([&]() {
            canRx.run();
            });

        // CAN Consumer Thread 
        std::thread canProcessThread([&]()
            {
                while (running)
                {
                    std::vector<std::uint8_t> rawData;
                    {
                        std::unique_lock<std::mutex> lk(inboxMutex);
                        inboxCv.wait(lk, [&] { return !running || !inbox.empty(); });

                        if (!running && inbox.empty()) break;

                        rawData = std::move(inbox.front());
                        inbox.pop();
                    }

                    if (rawData.empty()) continue;

                    shared::bit_parser::BitReader r(
                        shared::bit_parser::Span<const std::uint8_t>(rawData.data(), rawData.size())
                    );

                    const auto category = static_cast<shared::can::MessageCategory>(r.readU8());
                    const auto typeHeader = r.readU8();

                    switch (category)
                    {
                    case shared::can::MessageCategory::Status:
                    {
                        shared::can::message::StatusMessage msg(std::move(rawData));

                        switch (static_cast<shared::can::headers::Status>(typeHeader))
                        {
                        case shared::can::headers::Status::CurrentGear:
                            dataSource.setGear(msg.getValue());
                            break;
                        case shared::can::headers::Status::Speed:
                            dataSource.setSpeed(msg.getValue());
                            break;
                        case shared::can::headers::Status::RPM:
                            dataSource.setRpm(msg.getValue());
                            break;
                        case shared::can::headers::Status::EngineTemperature:
                            dataSource.setEngineTemp(static_cast<std::uint32_t>(
                                std::round(static_cast<double>(msg.getValue()) / 10.0)));
                            break;
                        case shared::can::headers::Status::Fuel:
                            dataSource.setEngineFuel(msg.getValue());
                            break;
                        default:
                            break;
                        } 
                        break;
                    }
                    case shared::can::MessageCategory::Error:
                    {
                        shared::can::message::ErrorMessage msg(std::move(rawData));
                        dataSource.addError(msg.getErrorType(), msg.getErrorMessage());
                        LogFile::warn("Error received: " + msg.getErrorMessage());
                        break;
                    }
                    case shared::can::MessageCategory::Control:
	                    break; // Dashboard has no control functionality 
                    default:
                        break;
                    }
                }
            });

        // Command & WebSocket Servers 
        net::io_context cmdIoc; 
        dashboard::CommandHttpServer commandServer{ cmdIoc, running, 8081, canBus };
        std::thread commandThread([&]() { commandServer.run(); });

        net::io_context wsIoc;
        tcp::acceptor acceptor(wsIoc, tcp::endpoint(tcp::v4(), 8080));
        g_acceptor = &acceptor;

        while (running)
        {
            try 
            {
                tcp::socket socket(wsIoc);
                boost::system::error_code ec;
                acceptor.accept(socket, ec);

                if (!running) break;
                if (ec) continue;

                beast::flat_buffer buffer;
                websocket::stream<tcp::socket> ws(std::move(socket));
                http::request<http::string_body> req;
                http::read(ws.next_layer(), buffer, req);

                const auto target = std::string(req.target());
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
                    ws.close(websocket::close_code::protocol_error);
                }
            }
            catch (const std::exception& e)
            {
                if (running)
                {
                    LogFile::error("Socket error: " + std::string(e.what()));
                }
            }
        }

        // === Shutdown ===
        LogFile::info("Shutting down...");
        canRx.stop(); // Ensure the receiver socket closes
        inboxCv.notify_all();

        if (commandThread.joinable()) commandThread.join();
        if (canRxThread.joinable()) canRxThread.join();
        if (canProcessThread.joinable()) canProcessThread.join();
        if (dashboardProcessThread.joinable()) dashboardProcessThread.join();

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }
}
