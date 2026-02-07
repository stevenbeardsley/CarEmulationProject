#ifndef SHARED_CAN_BUS_H
#define SHARED_CAN_BUS_H

#include "Message.h"

#include <boost/asio.hpp>
#include <vector>
#include <mutex>
#include <cstdint>
#include <string>

namespace shared::can
{

    class Bus
    {
    public:

        using udp = boost::asio::ip::udp;

        // bindPort:
        //    Local port to bind socket to (0 = ephemeral, recommended)
        //
        // defaultPeerPort:
        //    Port used when AddPeer(host) is called without specifying port.
        //    This should be the Receiver listen port (e.g. 15000).
        //
        Bus(unsigned short bindPort,
            unsigned short defaultPeerPort);

        // Add peer with explicit port
        bool AddPeer(const std::string& host,
            unsigned short port);

        // Add peer using defaultPeerPort_
        bool AddPeer(const std::string& host);

        // Thread-safe send
        bool Send(const Message& msg);

        // Optional debug helper
        unsigned short GetLocalPort() const;

    private:

        void PackMessage(const Message& msg,
            std::vector<std::uint8_t>& datagram);

    private:

        boost::asio::io_context ioc_;
        udp::socket socket_;
        udp::resolver resolver_;

        unsigned short bindPort_;
        unsigned short defaultPeerPort_;

        std::vector<udp::endpoint> peers_;
        mutable std::mutex peersMutex_;

        std::mutex sendMutex_;
    };

} // namespace shared::can

#endif
