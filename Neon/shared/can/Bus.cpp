#include "Bus.h"
#include "LogFile.h"
#include <iostream>

namespace shared::can
{

    Bus::Bus(unsigned short bindPort,
        unsigned short defaultPeerPort)
        : ioc_()
        , socket_(ioc_)
        , resolver_(ioc_)
        , bindPort_(bindPort)
        , defaultPeerPort_(defaultPeerPort)
    {
        boost::system::error_code ec;

        udp::endpoint local(udp::v4(), bindPort_);

        socket_.open(local.protocol(), ec);
        if (ec)
        {
            std::cerr << "Bus: socket open failed: "
                << ec.message() << "\n";
            return;
        }

        socket_.set_option(boost::asio::socket_base::reuse_address(true), ec);

        socket_.set_option(boost::asio::socket_base::broadcast(true), ec);

        socket_.bind(local, ec);
        if (ec)
        {
            std::cerr << "Bus: bind failed: "
                << ec.message() << "\n";
            return;
        }

        std::cout << "Bus: bound to local port "
            << GetLocalPort() << "\n";
    }

    bool Bus::AddPeer(const std::string& host,
        unsigned short port)
    {
        auto allOk = true;
        if (port == 0)
            port = defaultPeerPort_;

        boost::system::error_code ec;

        auto results =
            resolver_.resolve(udp::v4(),
                host,
                std::to_string(port),
                ec);

        if (ec)
        {
            LogFile::Error("CAN BUS: Failed to AddPeer: host");
            allOk = false;
        }

        udp::endpoint endpoint = *results.begin();

        {
            std::lock_guard<std::mutex> lock(peersMutex_);
            peers_.push_back(endpoint);
        }

        LogFile::Info("CAN BUS: Added peer: host, " + endpoint.address().to_string());
        return allOk;
    }

    bool Bus::AddPeer(const std::string& host)
    {
        return AddPeer(host, defaultPeerPort_);
    }

    bool Bus::Send(const Message& msg)
    {
        std::lock_guard<std::mutex> sendLock(sendMutex_);

        if (!socket_.is_open())
            return false;

        std::vector<std::uint8_t> datagram;
        PackMessage(msg, datagram);

        std::vector<udp::endpoint> peersCopy;

        {
            std::lock_guard<std::mutex> lock(peersMutex_);
            peersCopy = peers_;
        }

        bool allOk = true;

        for (const auto& ep : peersCopy)
        {
            LogFile::Info("Sending datagram to: "
                + ep.address().to_string());
            boost::system::error_code ec;

            const std::size_t sent =
                socket_.send_to(
                    boost::asio::buffer(datagram),
                    ep,
                    0,
                    ec
                );

            if (ec || sent != datagram.size())
            {
                allOk = false;

                LogFile::Info("Bus: send_to failed to "
                    + ep.address().to_string()
                    + " : "
                    + (ec ? ec.message() : "short send")
                    + "\n");
            }
        }
        return allOk;
    }

    unsigned short Bus::GetLocalPort() const
    {
        boost::system::error_code ec;

        auto endpoint = socket_.local_endpoint(ec);

        if (ec)
            return 0;

        return endpoint.port();
    }

    void Bus::PackMessage(const Message& msg,
        std::vector<std::uint8_t>& datagram)
    {
        datagram.resize(5);

        const std::uint8_t id =
            static_cast<std::uint8_t>(msg.getMessageType());

        const std::uint32_t value =
            static_cast<std::uint32_t>(msg.getValue());

        datagram[0] = id;
        datagram[1] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
        datagram[2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
        datagram[3] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
        datagram[4] = static_cast<std::uint8_t>((value >> 0) & 0xFF);
    }

} // namespace shared::can
