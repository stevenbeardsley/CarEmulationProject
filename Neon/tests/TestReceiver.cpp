#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdint>

#include "Receiver.h"

// ─────────────────────────────────────────────────────────────────────────────
// Type aliases matching the production code
// ─────────────────────────────────────────────────────────────────────────────

using InboxQueue = std::queue<std::vector<std::uint8_t>>;
using udp = boost::asio::ip::udp;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Sends a UDP datagram to 127.0.0.1:port from an ephemeral source socket.
static void sendTo(unsigned short port, const std::vector<uint8_t>& payload)
{
    boost::asio::io_context ioc;
    udp::socket sock(ioc, udp::endpoint(udp::v4(), 0));
    udp::endpoint dest(boost::asio::ip::address_v4::loopback(), port);
    sock.send_to(boost::asio::buffer(payload), dest);
}

/// Blocks until the inbox holds at least `count` items or the timeout elapses.
static bool waitForItems(InboxQueue& inbox,
    std::mutex& mtx,
    std::condition_variable& cv,
    std::size_t              count = 1,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(500))
{
    std::unique_lock lock(mtx);
    return cv.wait_for(lock, timeout, [&] { return inbox.size() >= count; });
}

/// Joins a thread with a hard deadline; returns false if it did not finish in time.
static bool joinWithTimeout(std::thread& t,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(2000))
{
    std::atomic<bool> done{ false };
    std::thread watcher([&] { t.join(); done = true; });
    watcher.detach();

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!done && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    return done.load();
}

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class ReceiverTest : public ::testing::Test
{
protected:
    std::atomic<bool>       running_{ true };
    InboxQueue              inbox_;
    std::mutex              inboxMutex_;
    std::condition_variable inboxCv_;

    shared::can::Receiver makeReceiver(uint16_t port)
    {
        return shared::can::Receiver(running_, port, inbox_, inboxMutex_, inboxCv_);
    }

    std::vector<uint8_t> popFront()
    {
        std::lock_guard lock(inboxMutex_);
        auto item = inbox_.front();
        inbox_.pop();
        return item;
    }

    /// Signals the receiver to stop and asserts that run() exits within the timeout.
    void stopAndJoin(shared::can::Receiver& receiver, std::thread& runThread)
    {
        running_ = false;
        receiver.stop();
        EXPECT_TRUE(joinWithTimeout(runThread)) << "run() did not exit in time";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// stop
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ReceiverTest, Stop_DoesNotThrow)
{
    auto receiver = makeReceiver(46100);
    EXPECT_NO_THROW(receiver.stop());
}

TEST_F(ReceiverTest, Stop_IsIdempotent)
{
    auto receiver = makeReceiver(46101);
    EXPECT_NO_THROW(receiver.stop());
    EXPECT_NO_THROW(receiver.stop());
}

// ─────────────────────────────────────────────────────────────────────────────
// run — happy-path delivery
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ReceiverTest, Run_ReceivesSingleDatagram)
{
    constexpr uint16_t port = 46200;
    auto receiver = makeReceiver(port);
    std::thread runThread([&] { receiver.run(); });

    const std::vector<uint8_t> payload = { 0xCA, 0xFE };
    sendTo(port, payload);

    ASSERT_TRUE(waitForItems(inbox_, inboxMutex_, inboxCv_));
    EXPECT_EQ(popFront(), payload);

    stopAndJoin(receiver, runThread);
}

TEST_F(ReceiverTest, Run_ReceivesMultipleDatagramsInOrder)
{
    constexpr uint16_t port = 46201;
    auto receiver = makeReceiver(port);
    std::thread runThread([&] { receiver.run(); });

    const std::vector<std::vector<uint8_t>> packets = {
        {0x01}, {0x02, 0x03}, {0x04, 0x05, 0x06}
    };
    for (const auto& pkt : packets)
        sendTo(port, pkt);

    ASSERT_TRUE(waitForItems(inbox_, inboxMutex_, inboxCv_, packets.size()));

    stopAndJoin(receiver, runThread);

    std::lock_guard lock(inboxMutex_);
    ASSERT_EQ(inbox_.size(), packets.size());
    for (const auto& expected : packets)
    {
        EXPECT_EQ(inbox_.front(), expected);
        inbox_.pop();
    }
}

TEST_F(ReceiverTest, Run_PreservesAllByteValues)
{
    constexpr uint16_t port = 46202;
    auto receiver = makeReceiver(port);
    std::thread runThread([&] { receiver.run(); });

    std::vector<uint8_t> payload(256);
    for (int i = 0; i < 256; ++i)
        payload[i] = static_cast<uint8_t>(i);

    sendTo(port, payload);

    ASSERT_TRUE(waitForItems(inbox_, inboxMutex_, inboxCv_));
    EXPECT_EQ(popFront(), payload);

    stopAndJoin(receiver, runThread);
}

TEST_F(ReceiverTest, Run_SingleBytePayloadIsQueued)
{
    constexpr uint16_t port = 46203;
    auto receiver = makeReceiver(port);
    std::thread runThread([&] { receiver.run(); });

    sendTo(port, { 0xFF });

    ASSERT_TRUE(waitForItems(inbox_, inboxMutex_, inboxCv_));
    const auto received = popFront();
    ASSERT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0], 0xFF);

    stopAndJoin(receiver, runThread);
}

// ─────────────────────────────────────────────────────────────────────────────
// run — filtering behaviour
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ReceiverTest, Run_EmptyUdpPacketIsNotQueued)
{
    constexpr uint16_t port = 46210;
    auto receiver = makeReceiver(port);
    std::thread runThread([&] { receiver.run(); });

    // Send empty packet then a real one; only the real one should be queued.
    sendTo(port, {});
    sendTo(port, { 0xBE, 0xEF });

    ASSERT_TRUE(waitForItems(inbox_, inboxMutex_, inboxCv_));

    // Short wait to allow any spurious second item to arrive before we check.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    stopAndJoin(receiver, runThread);

    std::lock_guard lock(inboxMutex_);
    ASSERT_EQ(inbox_.size(), 1u) << "Empty packet should have been dropped";
    EXPECT_EQ(inbox_.front(), (std::vector<uint8_t>{0xBE, 0xEF}));
}

TEST_F(ReceiverTest, Run_NotifiesConditionVariableForEachDatagram)
{
    constexpr uint16_t port = 46211;
    auto receiver = makeReceiver(port);
    std::thread runThread([&] { receiver.run(); });

    // Each datagram should wake a waiter independently.
    for (std::size_t i = 1; i <= 3; ++i)
    {
        sendTo(port, { static_cast<uint8_t>(i) });
        EXPECT_TRUE(waitForItems(inbox_, inboxMutex_, inboxCv_, i))
            << "CV not notified for datagram " << i;
    }

    stopAndJoin(receiver, runThread);
}

// ─────────────────────────────────────────────────────────────────────────────
// run — lifecycle / shutdown
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ReceiverTest, Run_ExitsAfterStop)
{
    constexpr uint16_t port = 46220;
    auto receiver = makeReceiver(port);
    std::thread runThread([&] { receiver.run(); });

    running_ = false;
    receiver.stop(); // closing the socket unblocks receive_from

    EXPECT_TRUE(joinWithTimeout(runThread)) << "run() did not exit after stop()";
}

TEST_F(ReceiverTest, Run_ExitsImmediatelyIfSocketAlreadyClosed)
{
    // Exercises the !socket_.is_open() guard at the top of run().
    constexpr uint16_t port = 46221;
    auto receiver = makeReceiver(port);
    receiver.stop(); // close before run() is entered

    std::thread runThread([&] { receiver.run(); });

    EXPECT_TRUE(joinWithTimeout(runThread)) << "run() blocked on a closed socket";

    running_ = false;
}

TEST_F(ReceiverTest, Run_TwoIndependentReceiversBothDeliverData)
{
    // Verifies that each Receiver holds its own socket state with no crosstalk.
    constexpr uint16_t portA = 46230;
    constexpr uint16_t portB = 46231;

    // Both receivers share the same inbox/mutex/cv for simplicity.
    auto receiverA = makeReceiver(portA);
    auto receiverB = makeReceiver(portB);

    std::thread threadA([&] { receiverA.run(); });
    std::thread threadB([&] { receiverB.run(); });

    sendTo(portA, { 0xAA });
    sendTo(portB, { 0xBB });

    ASSERT_TRUE(waitForItems(inbox_, inboxMutex_, inboxCv_, 2));

    running_ = false;
    receiverA.stop();
    receiverB.stop();
    EXPECT_TRUE(joinWithTimeout(threadA));
    EXPECT_TRUE(joinWithTimeout(threadB));

    // Both payloads arrived; order may vary so compare as a set.
    std::lock_guard lock(inboxMutex_);
    ASSERT_EQ(inbox_.size(), 2u);
    std::vector<std::vector<uint8_t>> received;
    while (!inbox_.empty()) { received.push_back(inbox_.front()); inbox_.pop(); }

    EXPECT_NE(std::find(received.begin(), received.end(),
        std::vector<uint8_t>{0xAA}), received.end());
    EXPECT_NE(std::find(received.begin(), received.end(),
        std::vector<uint8_t>{0xBB}), received.end());
}