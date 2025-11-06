#include "DashboardDataSource.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <chrono>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;


int main() {
    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));
        std::cout << "WebSocket server listening on port 8080...\n";

        dashboard::DashboardDataSource dataSource;

        // Thread: periodically updates data with JSON from toJson()
        std::thread updater([&dataSource]() {
            for (int i = 0; i < 20; ++i) {
                int speed = 50 + i * 5;     // example data
                bool status = (i % 2 == 0); // alternating true/false

                std::string newJson = dataSource.toJson(speed, status);
                dataSource.updateData(newJson);

                std::cout << "Updated JSON: " << newJson << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            });

        // Thread: handles WebSocket connections
        for (;;) {
            try {
                tcp::socket socket(ioc);
                acceptor.accept(socket);

                websocket::stream<tcp::socket> ws(std::move(socket));
                ws.accept();

                std::cout << "Client connected.\n";

                for (int i = 0; i < 20; ++i) {
                    std::string currentData = dataSource.getData();
                    ws.write(net::buffer(currentData)); // send current JSON
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }

                ws.close(websocket::close_code::normal);
                std::cout << "Connection closed normally.\n";
            }
            catch (std::exception const& e) {
                std::cerr << "Client connection error: " << e.what() << std::endl;
            }
        }

        updater.join();
    }
    catch (std::exception const& e) {
        std::cerr << "Fatal server error: " << e.what() << std::endl;
    }
    return 1;
}
