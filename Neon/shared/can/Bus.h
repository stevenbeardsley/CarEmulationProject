#ifndef SHARED_CAN_BUS_H
#define SHARED_CAN_BUS_H

#include <cstdint>
#include <string>
#include <vector>

#include <boost/asio.hpp>

#include "Message.h"  // your Message class (MessageType == CAN ID for now)

namespace shared::can
{
    class Bus
    {
    public:
        explicit Bus(uint16_t defaultPort);
        ~Bus() = default;

        Bus(const Bus&) = delete;
        Bus& operator=(const Bus&) = delete;

        // Add a peer by hostname (docker service/container name) or IPv4 address string.
        // Example: AddPeer("ecm", 15000);
        bool AddPeer(const std::string& host, uint16_t port);

        // Convenience: uses defaultPort passed in ctor.
        bool AddPeer(const std::string& host);

        // Logical broadcast: sends the datagram to every configured peer.
        // Returns true if ALL sends succeeded; false if any failed.
        bool Send(const Message& msg);

    private:
        static void PackMessage(const Message& msg, std::vector<uint8_t>& out);

    private:
        using udp = boost::asio::ip::udp;

        boost::asio::io_context io_;
        udp::resolver resolver_;
        udp::socket socket_;

        uint16_t defaultPort_{ 0 };
        std::vector<udp::endpoint> peers_;
    };
}

#endif