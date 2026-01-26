#include "Receiver.h"
#include "LogFile.h"

#include <array>

namespace shared::can
{
    static inline int32_t read_i32_be(const uint8_t* p)
    {
        // p[0] is MSB
        uint32_t u =
            (static_cast<uint32_t>(p[0]) << 24) |
            (static_cast<uint32_t>(p[1]) << 16) |
            (static_cast<uint32_t>(p[2]) << 8) |
            (static_cast<uint32_t>(p[3]) << 0);

        return static_cast<int32_t>(u);
    }

    Receiver::Receiver(std::atomic<bool>& runningFlag, uint16_t listenPort)
        : running_(runningFlag)
        , socket_(io_)
    {
        boost::system::error_code ec;

        // Open + bind UDP socket to all interfaces (inside container)
        socket_.open(udp::v4(), ec);
        if (ec)
        {
            LogFile::Error("CAN Receiver: socket.open failed: " + ec.message());
            return;
        }

        socket_.bind(udp::endpoint(udp::v4(), listenPort), ec);
        if (ec)
        {
            LogFile::Error("CAN Receiver: socket.bind failed: " + ec.message());
            return;
        }

        LogFile::Info("CAN Receiver listening on UDP port " + std::to_string(listenPort));
    }

    void Receiver::Stop()
    {
        boost::system::error_code ec;
        socket_.close(ec); // closing unblocks receive_from on most platforms
    }

    bool Receiver::TryDecodeMessage(const uint8_t* data, std::size_t len, Message& out)
    {
        // Your current TX format:
        // [u8 can_id][i32 value BE] => 5 bytes
        if (len != 5)
            return false;

        const uint8_t canIdRaw = data[0];
        const int32_t value = read_i32_be(data + 1);

        const auto type = static_cast<MessageType>(canIdRaw);

        // Construct using your Message class API (public ctor)
        out = Message{ type, static_cast<std::uint32_t>(value) }; // if your Message value is unsigned
        // If your Message stores signed ints, change to: out = Message{ type, value };

        return true;
    }

    void Receiver::Run()
    {
        LogFile::Info("CAN Receiver thread started");

        if (!socket_.is_open())
        {
            LogFile::Error("CAN Receiver: socket not open, exiting Run()");
            return;
        }

        std::array<uint8_t, 256> buf{};
        udp::endpoint sender;

        while (running_)
        {
            LogFile::Info("Running the receiverrrrr");
            boost::system::error_code ec;
            
            LogFile::Info("Receive is the next line.");
            const std::size_t n = socket_.receive_from(
                boost::asio::buffer(buf),
                sender,
                0,
                ec
            );
            LogFile::Info("Receive from ran.");
            
            LogFile::Info(
                "CAN RX raw datagram: bytes=" + std::to_string(n) +
                " from " + sender.address().to_string() + ":" +
                std::to_string(sender.port())
            );
            Message msg{ MessageType{0}, 0u };
            if (!TryDecodeMessage(buf.data(), n, msg))
            {
                LogFile::Error("CAN Receiver: invalid datagram size=" + std::to_string(n));
                continue;
            }

            if (!running_)
                break;

            if (ec)
            {
                // If socket closed during shutdown, receive_from will error.
                LogFile::Error("CAN Receiver: receive_from failed: " + ec.message());
                continue;
            }

            // Log received values via accessors
            LogFile::Info(
                "CAN RX from " + sender.address().to_string() + ":" + std::to_string(sender.port()) +
                " type=" + toString(static_cast<MessageType>(msg.getMessageType())) +
                " value=" + std::to_string(msg.getValue())
            );

        }

        LogFile::Info("CAN Receiver stopped.");
    }
}
