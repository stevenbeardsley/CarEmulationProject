#include "Bus.h"
#include "LogFile.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

namespace shared::can
{
    Bus::Bus(const unsigned short bindPort, const unsigned short defaultPeerPort)
        : ioc_()
        , socket_(ioc_)
        , resolver_(ioc_)
        , bindPort_(bindPort)
        , m_defaultPeerPort(defaultPeerPort)
    {
        boost::system::error_code ec;
        const udp::endpoint local(udp::v4(), bindPort_);

        socket_.open(local.protocol(), ec);
        if (ec)
        {
            std::cerr << "Bus: socket open failed: " << ec.message() << "\n";
            return;
        }

        socket_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        socket_.set_option(boost::asio::socket_base::broadcast(true), ec);

        socket_.bind(local, ec);
        if (ec)
        {
            std::cerr << "Bus: bind failed: " << ec.message() << "\n";
            return;
        }

        std::cout << "Bus: bound to local port " << getLocalPort() << "\n";
    }

    bool Bus::addPeer(const std::string& host, unsigned short port)
    {
        if (port == 0)
            port = m_defaultPeerPort;

        constexpr auto maxAttempts = 25;                    
        const auto delay = std::chrono::milliseconds(200);  

        for (auto attempt = 1; attempt <= maxAttempts; ++attempt)
        {
            boost::system::error_code ec;

            auto results = resolver_.resolve(
                udp::v4(),
                host,
                std::to_string(port),
                ec
            );

            if (!ec && !results.empty())
            {
                const udp::endpoint endpoint = *results.begin();

                {
	                std::scoped_lock lock(peersMutex_);

                    // Optional: de-dupe
                    if (std::find(peers_.begin(), peers_.end(), endpoint) == peers_.end())
                        peers_.push_back(endpoint);
                }

                LogFile::Info("CAN BUS: Added peer: " + host + " -> "
                    + endpoint.address().to_string() + ":" + std::to_string(endpoint.port()));
                return true;
            }

            if (attempt == 1 || attempt == maxAttempts)
            {
                LogFile::Warn("CAN BUS: AddPeer resolve failed for " + host + ":"
                    + std::to_string(port) + " (attempt "
                    + std::to_string(attempt) + "/" + std::to_string(maxAttempts)
                    + ") : " + ec.message());
            }

            std::this_thread::sleep_for(delay);
        }

        return false;
    }

    bool Bus::addPeer(const std::string& host)
    {
        return addPeer(host, m_defaultPeerPort);
    }

    void Bus::send(const std::vector<std::uint8_t>& datagram)
    {
	    std::scoped_lock sendLock(sendMutex_);

        if (!socket_.is_open() || datagram.empty())
        {
            LogFile::Warn("Bus: Failed to send datagram, datagram is empty or socket was not open");
        }

        std::vector<udp::endpoint> peersCopy;
        {
	        std::scoped_lock lock(peersMutex_);
            peersCopy = peers_;
        }

        for (const auto& ep : peersCopy)
        {
            LogFile::Info("Sending datagram to: " + ep.address().to_string());
            boost::system::error_code ec;

            const std::size_t sent = socket_.send_to(
                boost::asio::buffer(datagram),
                ep,
                0,
                ec
            );

            if (ec || sent != datagram.size())
            {
                LogFile::Info("Bus: send_to failed to "
                    + ep.address().to_string() + " : "
                    + (ec ? ec.message() : "short send") + "\n");
            }
        }
    }

    unsigned short Bus::getLocalPort() const
    {
        boost::system::error_code ec;
        const auto endpoint = socket_.local_endpoint(ec);
        if (ec) 
        {
            return 0;
        };
        return endpoint.port();
    }

} // namespace shared::can