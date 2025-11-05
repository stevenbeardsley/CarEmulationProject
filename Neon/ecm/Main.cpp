#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;

int main() {
    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));
        std::cout << "WebSocket server listening on port 8080...\n";

        for (;;) {
            try {
                tcp::socket socket(ioc);
                acceptor.accept(socket);

                websocket::stream<tcp::socket> ws(std::move(socket));
                ws.accept();

                std::cout << "Client connected.\n";

                json::object status;
                status["status"] = "running";
                status["pid"] = 1;

                for (int i = 0; i < 20; ++i) {
                    status["pid"] = i + 1;
                    ws.write(net::buffer(json::serialize(status)));
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }

                ws.close(websocket::close_code::normal);
                std::cout << "Connection closed normally.\n";
            }
            catch (std::exception const& e) {
                // Handle per-client errors here, so one failure doesn't stop the server
                std::cerr << "Client connection error: " << e.what() << std::endl;
            }
        }
    }
    catch (std::exception const& e) {
        std::cerr << "Fatal server error: " << e.what() << std::endl;
    }
}
