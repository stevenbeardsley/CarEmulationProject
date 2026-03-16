#ifndef SHARED_CAN_BUS_h
#define SHARED_CAN_BUS_h

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

        void send(const std::vector<std::uint8_t>& datagram);

        [[nodiscard]]
        bool addPeer(const std::string& host, unsigned short port);
        
        [[nodiscard]]
        bool addPeer(const std::string& host);


        [[nodiscard]] unsigned short getLocalPort() const;

    private:
        boost::asio::io_context ioc_;
        udp::socket socket_;
        udp::resolver resolver_;

        unsigned short bindPort_;
        unsigned short m_defaultPeerPort;

        std::mutex peersMutex_;
        std::vector<udp::endpoint> peers_;

        std::mutex sendMutex_;
    };
}

#endif