#include "Receiver.h"
#include "LogFile.h" // Assuming this is your custom logging class

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
        if (n == 0) return; // Ignore empty packets

        // 1. Copy the raw network data into a vector
        std::vector<std::uint8_t> rawData(data, data + n);

        // 2. Lock the mutex and push to the inbox
        {
            std::lock_guard<std::mutex> lock(inboxMutex_);
            inbox_.push(std::move(rawData));
        }

        // 3. Wake up your consumer thread!
        inboxCv_.notify_one();
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

            // Check if we were told to stop while waiting for a packet
            if (!running_)
                break;

            if (ec)
            {
                LogFile::Error("CAN Receiver: receive_from failed: " + ec.message());
                continue;
            }

            handleDatagram(buf.data(), n, sender);
        }

        LogFile::Info("CAN Receiver stopped.");
    }
}