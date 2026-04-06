#ifndef SHARED_CAN_RECEIVER_H
#define SHARED_CAN_RECEIVER_H

#include <atomic>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <boost/asio.hpp>

namespace shared::can
{
    class Receiver
    {
    public:
        using InboxQueue = std::queue<std::vector<std::uint8_t>>;

        Receiver(std::atomic<bool>& runningFlag,
            uint16_t listenPort,
            InboxQueue& inbox,
            std::mutex& inboxMutex,
            std::condition_variable& inboxCv);

        void run();
        void stop();

    private:
        void handleDatagram(const uint8_t* data, std::size_t n, const boost::asio::ip::udp::endpoint& sender) const;

        std::atomic<bool>& running_;
        boost::asio::io_context io_;
        boost::asio::ip::udp::socket socket_;

        // Shared state with the consumer thread
        InboxQueue& inbox_;
        std::mutex& inboxMutex_;
        std::condition_variable& inboxCv_;
    };
}

#endif