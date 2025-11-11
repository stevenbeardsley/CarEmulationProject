#include "DashboardDataSource.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include "LogFile.h"
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <chrono>
#include <atomic>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

int main() {
    try {
        // Set up log file
        LogFile::Instance().setLogFile("dashboard.log");
        LogFile::Instance().setLevel(LogLevel::DEBUG);
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));
        LogFile::Info("WebSocket server listening on port 8080");
        std::cout << "WebSocket server listening on port 8080...\n";

        dashboard::DashboardDataSource dataSource;
        std::atomic<bool> running(true);

        // Thread: periodically updates data with new JSON
        std::thread updater([&dataSource, &running]() 
            {
            int iteration = 0;
            while (running) {
                int speed = 50 + (iteration % 10) * 10;
                bool status = (iteration % 3 != 0);

                // Just update with values, not JSON string
                dataSource.updateData(speed, status);

                std::cout << "Updated data [iteration " << iteration << "]: speed="
                    << speed << ", status=" << status << "\n";
                LogFile::Info("Updated data[iteration " + std::to_string(iteration) + "]: speed = "
                    + std::to_string(speed) + ", status=" + std::to_string(status)  + "\n");

                iteration++;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            });
        // Thread: handles WebSocket connections
        std::thread server([&acceptor, &ioc, &dataSource, &running]()
            {
            while (running) 
            {
                try 
                {
                    tcp::socket socket(ioc);
                    acceptor.accept(socket);
                    websocket::stream<tcp::socket> ws(std::move(socket));
                    ws.accept();
                    std::cout << "Client connected.\n";

                    // Send data as long as connection is alive
                    while (running) 
                    {
                        try
                        {
                            std::string currentData = dataSource.getData();
                            ws.write(net::buffer(currentData));
                            std::this_thread::sleep_for(std::chrono::seconds(2));
                        }
                        catch (std::exception const& e)
                        {
                            std::cerr << "Send error: " << e.what() << std::endl;
                            break;
                        }
                    }

                    ws.close(websocket::close_code::normal);
                    std::cout << "Connection closed.\n";
                }
                catch (std::exception const& e)
                {
                    std::cerr << "Client connection error: " << e.what() << std::endl;
                }
            }
            });

        // Let it run for demonstration (or use signal handling for graceful shutdown)
        std::this_thread::sleep_for(std::chrono::seconds(60));

        running = false;
        updater.join();
        server.join();
    }
    catch (std::exception const& e) 
    {
        std::cerr << "Fatal server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}