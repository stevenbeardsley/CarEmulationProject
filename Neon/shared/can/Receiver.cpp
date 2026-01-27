#include "Receiver.h"
#include "LogFile.h"

#include <array>

namespace shared::can
{
    static inline std::uint32_t read_u32_be(const std::uint8_t* p)
    {
        return (static_cast<std::uint32_t>(p[0]) << 24) |
            (static_cast<std::uint32_t>(p[1]) << 16) |
            (static_cast<std::uint32_t>(p[2]) << 8) |
            (static_cast<std::uint32_t>(p[3]) << 0);
    }

    Receiver::Receiver(std::atomic<bool>& runningFlag, uint16_t listenPort)
        : running_(runningFlag)
        , socket_(io_)
    {
        boost::system::error_code ec;

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
        socket_.close(ec);
    }

    void Receiver::handleDatagram(const uint8_t* data, std::size_t n, const udp::endpoint& sender)
    {
        // Expect exactly 5 bytes: [u8 id][u32 value BE]
        if (n < 5)
        {
            LogFile::Warn("CAN RX: datagram too small (n=" + std::to_string(n) + ")");
            return;
        }

        const std::uint8_t canIdRaw = data[0];
        const std::uint32_t value = read_u32_be(data + 1);
        const auto type = static_cast<MessageType>(canIdRaw);

        LogFile::Debug("RX raw: id=" + std::to_string(canIdRaw) +
            " value=" + std::to_string(value) +
            " from " + sender.address().to_string() + ":" + std::to_string(sender.port()));

        Message msg{ type, value };

        if (handler_)
            handler_(msg, sender);
    }

    void Receiver::Run()
    {
        LogFile::Info("CAN Receiver thread started");

        if (!socket_.is_open())
        {
            LogFile::Error("CAN Receiver: socket not open, exiting Run()");
            return;
        }

        std::array<std::uint8_t, 256> buf{};
        udp::endpoint sender;

        while (running_)
        {
            boost::system::error_code ec;
            const std::size_t n = socket_.receive_from(
                boost::asio::buffer(buf),
                sender,
                0,
                ec
            );

            if (!running_)
                break;

            if (ec)
            {
                LogFile::Error("CAN Receiver: receive_from failed: " + ec.message());
                continue;
            }

            LogFile::Debug(
                "CAN RX raw datagram: bytes=" + std::to_string(n) +
                " from " + sender.address().to_string() + ":" + std::to_string(sender.port())
            );

            //  ACTUALLY PARSE THE BUFFER YOU JUST RECEIVED
            handleDatagram(buf.data(), n, sender);
        }

        LogFile::Info("CAN Receiver stopped.");
    }
}
