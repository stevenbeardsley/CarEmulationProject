#include "Receiver.h"
#include "LogFile.h"

#include <array>

namespace shared::can
{
    using boost::asio::ip::udp;

    Receiver::Receiver(std::atomic<bool>& runningFlag,
        uint16_t listenPort,
        InboxQueue& inbox,
        std::mutex& inboxMutex,
        std::condition_variable& inboxCv)
        : running_(runningFlag)
        , io_()
        , socket_(io_)
        , inbox_(inbox)
        , inboxMutex_(inboxMutex)
        , inboxCv_(inboxCv)
    {
        boost::system::error_code ec;

        socket_.open(udp::v4(), ec);
        if (ec)
        {
            LogFile::error("CAN Receiver: socket.open failed: " + ec.message());
            return;
        }

        socket_.bind(udp::endpoint(udp::v4(), listenPort), ec);
        if (ec)
        {
            LogFile::error("CAN Receiver: socket.bind failed: " + ec.message());
            return;
        }

        LogFile::info("CAN Receiver listening on UDP port " + std::to_string(listenPort));
    }

    void Receiver::stop()
    {
        boost::system::error_code ec;
        socket_.close(ec);
    }

    void Receiver::handleDatagram(const uint8_t* data, std::size_t n, const udp::endpoint& sender) const
    {
        if (n == 0) return; // Ignore empty packets
        {
			std::vector<std::uint8_t> rawData(data, data + n);
            std::lock_guard<std::mutex> lock(inboxMutex_);
            inbox_.push(std::move(rawData));
        }

        inboxCv_.notify_one();
    }

    void Receiver::run()
    {
        LogFile::info("CAN Receiver thread started");

        if (!socket_.is_open())
        {
            LogFile::error("CAN Receiver: socket not open, exiting Run()");
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
                LogFile::error("CAN Receiver: receive_from failed: " + ec.message());
                continue;
            }

            handleDatagram(buf.data(), n, sender);
        }

        LogFile::info("CAN Receiver stopped.");
    }
}