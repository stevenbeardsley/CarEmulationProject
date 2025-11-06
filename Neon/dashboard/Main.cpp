#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <string>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;


// Task: Split up the datasource into a string parser and create another 
// thread for networking
// DashboardDataSource just stores a string
class DashboardDataSource {
public:
    void updateData(const std::string& newData) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_ = newData; // replaces previous data
    }

    std::string getData() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_;
    }

private:
    mutable std::mutex mutex_;
    std::string data_;
};

int main() {
    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));
        std::cout << "WebSocket server listening on port 8080...\n";

        DashboardDataSource dataSource;

        // Simulate updating data in a separate thread
        std::thread updater([&dataSource]() {
            for (int i = 0; i < 20; ++i) {
                std::string newData = "Current pid: " + std::to_string(i + 1);
                dataSource.updateData(newData); // replaces old string
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            });

        for (;;) {
            try {
                tcp::socket socket(ioc);
                acceptor.accept(socket);

                websocket::stream<tcp::socket> ws(std::move(socket));
                ws.accept();

                std::cout << "Client connected.\n";

                for (int i = 0; i < 20; ++i) {
                    ws.write(net::buffer(dataSource.getData())); // send current string
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
}
