#ifndef SHARED_CAN_RECEIVER_H
#define SHARED_CAN_RECEIVER_H

#include <functional>
#include <atomic>
#include <cstdint>
#include <boost/asio.hpp>

#include "Message.h" // your CANMessage type

namespace shared::can {

    class Receiver {
    public:
        using Handler = std::function<void(const Message& msg, const boost::asio::ip::udp::endpoint& sender)>;

        Receiver(std::atomic<bool>& runningFlag, uint16_t listenPort);

        void SetHandler(Handler h) { handler_ = std::move(h); }

        void Run();   // blocking loop
        void Stop();


    private:
        void handleDatagram(const uint8_t* data, std::size_t n,
            const boost::asio::ip::udp::endpoint& sender);
        using udp = boost::asio::ip::udp;
        std::atomic<bool>& running_;
        boost::asio::io_context io_;
        udp::socket socket_;
        Handler handler_;
    };

} // namespace shared::can

#endif