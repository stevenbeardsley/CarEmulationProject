#include "Bus.h"

#include <iostream>

namespace shared::can
{
    static inline void append_u16_be(std::vector<uint8_t>& out, uint16_t v)
    {
        out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    }

    static inline void append_i32_be(std::vector<uint8_t>& out, int32_t v)
    {
        uint32_t u = static_cast<uint32_t>(v);
        out.push_back(static_cast<uint8_t>((u >> 24) & 0xFF));
        out.push_back(static_cast<uint8_t>((u >> 16) & 0xFF));
        out.push_back(static_cast<uint8_t>((u >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>((u >> 0) & 0xFF));
    }

    Bus::Bus(uint16_t defaultPort)
        : resolver_(io_)
        , socket_(io_)
        , defaultPort_(defaultPort)
    {
        boost::system::error_code ec;

        socket_.open(udp::v4(), ec);
        if (ec)
        {
            std::cerr << "Bus: socket.open failed: " << ec.message() << "\n";
        }

        // TX-only; no bind required.
        // If/when you add RX, you’ll bind to 0.0.0.0:<port> in the receiver.
    }

    bool Bus::AddPeer(const std::string& host, uint16_t port)
    {
        boost::system::error_code ec;

        // Resolve host:port to UDP endpoints.
        // Docker user-defined networks provide DNS so host like "ecm" resolves.
        auto results = resolver_.resolve(udp::v4(), host, std::to_string(port), ec);
        if (ec)
        {
            std::cerr << "Bus: resolve failed for " << host << ":" << port
                << " : " << ec.message() << "\n";
            return false;
        }

        // Store the first resolved endpoint.
        peers_.push_back(*results.begin());
        return true;
    }

    bool Bus::AddPeer(const std::string& host)
    {
        return AddPeer(host, defaultPort_);
    }

    void Bus::PackMessage(const Message& msg, std::vector<uint8_t>& out)
    {
        // Minimal binary envelope (v1):
        // [u16 can_id] [i32 value]
        //
        // For now MessageType == CAN ID.

        out.clear();
        out.reserve(2 + 4);

        const uint8_t canId = static_cast<uint8_t>(msg.getMessageType());
        const int32_t  value = static_cast<int32_t>(msg.getValue());  

        append_u16_be(out, canId);
        append_i32_be(out, value);
    }

    bool Bus::Send(const Message& msg)
    {
        if (!socket_.is_open())
            return false;

        std::vector<uint8_t> datagram;
        PackMessage(msg, datagram);

        bool allOk = true;

        for (const auto& ep : peers_)
        {
            boost::system::error_code ec;
            const std::size_t sent = socket_.send_to(
                boost::asio::buffer(datagram),
                ep,
                0,
                ec
            );

            if (ec || sent != datagram.size())
            {
                allOk = false;
                std::cerr << "Bus: send_to failed to " << ep.address().to_string()
                    << ":" << ep.port()
                    << " : " << (ec ? ec.message() : "short send") << "\n";
            }
        }

        return allOk;
    }
}
