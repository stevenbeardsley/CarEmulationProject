#ifndef SHARED_CAN_RECEIVER_H
#define SHARED_CAN_RECEIVER_H

#include <atomic>
#include <cstdint>
#include <string>

#include <boost/asio.hpp>

#include "Message.h"
#include "MessageType.h"

namespace shared::can
{
    class Receiver
    {
    public:
        // Bind to listenPort on 0.0.0.0 inside the container.
        Receiver(std::atomic<bool>& runningFlag, uint16_t listenPort);

        Receiver(const Receiver&) = delete;
        Receiver& operator=(const Receiver&) = delete;

        // Blocking receive loop. Run this on a dedicated thread.
        void Run();

        // Optional: force the socket to unblock and exit sooner.
        void Stop();

    private:
        static bool TryDecodeMessage(const uint8_t* data, std::size_t len, Message& out);

    private:
        using udp = boost::asio::ip::udp;

        std::atomic<bool>& running_;
        boost::asio::io_context io_;
        udp::socket socket_;
    };
}
#endif