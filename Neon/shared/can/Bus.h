#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <boost/asio.hpp>

namespace shared::can
{
    using boost::asio::ip::udp;

    class Bus
    {
    public:
        Bus(unsigned short bindPort, unsigned short defaultPeerPort);

        bool AddPeer(const std::string& host, unsigned short port);
        bool AddPeer(const std::string& host);

        // Send now just takes a raw byte buffer!
        bool Send(const std::vector<std::uint8_t>& datagram);

        unsigned short GetLocalPort() const;

    private:
        boost::asio::io_context ioc_;
        udp::socket socket_;
        udp::resolver resolver_;

        unsigned short bindPort_;
        unsigned short defaultPeerPort_;

        std::mutex peersMutex_;
        std::vector<udp::endpoint> peers_;

        std::mutex sendMutex_;
    };
}