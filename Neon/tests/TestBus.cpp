#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdint>
#include <atomic>
#include <string>

#include "shared/can/Bus.h"


using udp = boost::asio::ip::udp;

/// Opens a bound UDP socket on an OS-assigned port and returns both the socket
/// and the actual port, so individual tests can use it as a "receiver".
static std::pair<udp::socket, unsigned short>
makeReceiverSocket(boost::asio::io_context& ioc)
{
    udp::socket sock(ioc);
    sock.open(udp::v4());
    sock.set_option(boost::asio::socket_base::reuse_address(true));
    sock.bind(udp::endpoint(udp::v4(), 0));   // OS assigns a free port
    const unsigned short port = sock.local_endpoint().port();
    return { std::move(sock), port };
}

/// Blocking receive with a hard timeout.  Returns the payload bytes or an empty
/// vector when the deadline elapses.
static std::vector<uint8_t>
timedReceive(udp::socket& sock,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(500))
{
    // Put the socket into non-blocking mode for the duration of this helper.
    sock.non_blocking(true);

    std::vector<uint8_t> buf(4096);
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
        boost::system::error_code ec;
        udp::endpoint sender;
        const std::size_t n = sock.receive_from(boost::asio::buffer(buf), sender, 0, ec);

        if (!ec)
        {
            buf.resize(n);
            sock.non_blocking(false);
            return buf;
        }

        if (ec == boost::asio::error::would_block)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        break; 
    }

    sock.non_blocking(false);
    return {};
}


class BusTest : public ::testing::Test
{
protected:
    // Use ports in the ephemeral range that are unlikely to collide in CI.
    // Each test creates its own Bus on a distinct port.
    static constexpr unsigned short kDefaultPeerPort = 45100;

    boost::asio::io_context receiverIoc_;
};


TEST_F(BusTest, ConstructorBindsToRequestedPort)
{
    constexpr unsigned short port = 45200;
    shared::can::Bus bus(port, kDefaultPeerPort);

    EXPECT_EQ(bus.getLocalPort(), port);
}

TEST_F(BusTest, ConstructorWithPortZeroReceivesOsAssignedPort)
{
    // Binding to port 0 should let the OS pick a free port.
    shared::can::Bus bus(0, kDefaultPeerPort);
    const unsigned short assigned = bus.getLocalPort();

    EXPECT_GT(assigned, 0u);
}

TEST_F(BusTest, TwoBusesOnDifferentPortsBothBind)
{
    shared::can::Bus bus1(45201, kDefaultPeerPort);
    shared::can::Bus bus2(45202, kDefaultPeerPort);

    EXPECT_EQ(bus1.getLocalPort(), 45201u);
    EXPECT_EQ(bus2.getLocalPort(), 45202u);
}


TEST_F(BusTest, AddPeerLocalhostReturnsTrue)
{
    shared::can::Bus bus(45210, kDefaultPeerPort);

    // Any resolvable host/port combination should succeed.
    EXPECT_TRUE(bus.addPeer("127.0.0.1", 45210));
}

TEST_F(BusTest, AddPeerUsesDefaultPortWhenPortIsZero)
{
    // Construct with a known defaultPeerPort; call addPeer with port == 0.
    shared::can::Bus bus(45211, 45211 /*defaultPeerPort == bindPort*/);

    // Port 0 triggers the default-port path.
    EXPECT_TRUE(bus.addPeer("127.0.0.1", 0));
}

TEST_F(BusTest, AddPeerOneArgOverloadUsesDefaultPort)
{
    shared::can::Bus bus(45212, 45212);
    EXPECT_TRUE(bus.addPeer("127.0.0.1"));
}

TEST_F(BusTest, AddPeerDeduplicatesSameEndpoint)
{
    auto [recvSock, recvPort] = makeReceiverSocket(receiverIoc_);

    shared::can::Bus bus(45213, kDefaultPeerPort);
    bus.addPeer("127.0.0.1", recvPort);
    bus.addPeer("127.0.0.1", recvPort); // duplicate — should be ignored

    const std::vector<uint8_t> datagram = { 0xDE, 0xAD };
    bus.send(datagram);

    // Receive the first datagram.
    auto firstRecv = timedReceive(recvSock);
    ASSERT_EQ(firstRecv, datagram);

    // A second datagram should NOT arrive because the peer was added only once.
    auto secondRecv = timedReceive(recvSock, std::chrono::milliseconds(200));
    EXPECT_TRUE(secondRecv.empty())
        << "Duplicate peer caused the datagram to be sent twice";
}

TEST_F(BusTest, AddPeerWithUnresolvableHostReturnsFalse)
{
    shared::can::Bus bus(45214, kDefaultPeerPort);

    // This hostname is guaranteed not to resolve per RFC 2606.
    EXPECT_FALSE(bus.addPeer("this.host.does.not.exist.invalid", 9999));
}

TEST_F(BusTest, SendDeliversDatgramToSinglePeer)
{
    auto [recvSock, recvPort] = makeReceiverSocket(receiverIoc_);

    shared::can::Bus bus(45220, kDefaultPeerPort);
    bus.addPeer("127.0.0.1", recvPort);

    const std::vector<uint8_t> payload = { 0x01, 0x02, 0x03, 0x04 };
    bus.send(payload);

    const auto received = timedReceive(recvSock);
    EXPECT_EQ(received, payload);
}

TEST_F(BusTest, SendDeliversSameDatagramToAllPeers)
{
    auto [recvSock1, recvPort1] = makeReceiverSocket(receiverIoc_);
    auto [recvSock2, recvPort2] = makeReceiverSocket(receiverIoc_);

    shared::can::Bus bus(45221, kDefaultPeerPort);
    bus.addPeer("127.0.0.1", recvPort1);
    bus.addPeer("127.0.0.1", recvPort2);

    const std::vector<uint8_t> payload = { 0xCA, 0xFE };
    bus.send(payload);

    const auto recv1 = timedReceive(recvSock1);
    const auto recv2 = timedReceive(recvSock2);

    EXPECT_EQ(recv1, payload);
    EXPECT_EQ(recv2, payload);
}

TEST_F(BusTest, SendWithNoPeersDoesNotThrow)
{
    shared::can::Bus bus(45222, kDefaultPeerPort);
    const std::vector<uint8_t> payload = { 0xFF };

    EXPECT_NO_THROW(bus.send(payload));
}

TEST_F(BusTest, SendEmptyDatagramDoesNotThrow)
{
    auto [recvSock, recvPort] = makeReceiverSocket(receiverIoc_);

    shared::can::Bus bus(45223, kDefaultPeerPort);
    bus.addPeer("127.0.0.1", recvPort);

    EXPECT_NO_THROW(bus.send({}));

    // Nothing meaningful should arrive.
    const auto received = timedReceive(recvSock, std::chrono::milliseconds(200));
    EXPECT_TRUE(received.empty());
}

TEST_F(BusTest, SendPreservesPayloadBytesExactly)
{
    auto [recvSock, recvPort] = makeReceiverSocket(receiverIoc_);

    shared::can::Bus bus(45224, kDefaultPeerPort);
    bus.addPeer("127.0.0.1", recvPort);

    // Larger, more varied payload.
    std::vector<uint8_t> payload(256);
    for (int i = 0; i < 256; ++i)
        payload[i] = static_cast<uint8_t>(i);

    bus.send(payload);

    const auto received = timedReceive(recvSock);
    EXPECT_EQ(received, payload);
}

TEST_F(BusTest, SendIsIdempotentAcrossMultipleCalls)
{
    auto [recvSock, recvPort] = makeReceiverSocket(receiverIoc_);

    shared::can::Bus bus(45225, kDefaultPeerPort);
    bus.addPeer("127.0.0.1", recvPort);

    const std::vector<uint8_t> payload = { 0xAB, 0xCD };

    bus.send(payload);
    const auto recv1 = timedReceive(recvSock);

    bus.send(payload);
    const auto recv2 = timedReceive(recvSock);

    EXPECT_EQ(recv1, payload);
    EXPECT_EQ(recv2, payload);
}

TEST_F(BusTest, ConcurrentSendAndAddPeerDoNotRaceOrCrash)
{
    shared::can::Bus bus(45230, kDefaultPeerPort);

    constexpr int kThreads = 4;
    constexpr int kIterations = 50;

    std::atomic<int> failures{ 0 };

    auto senderTask = [&]() {
        const std::vector<uint8_t> payload = { 0x01 };
        for (int i = 0; i < kIterations; ++i)
        {
            try { bus.send(payload); }
            catch (...) { ++failures; }
        }
        };

    auto addPeerTask = [&]() {
        for (int i = 0; i < kIterations; ++i)
        {
            try { bus.addPeer("127.0.0.1", 45231); }
            catch (...) { ++failures; }
        }
        };

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads / 2; ++i)
    {
        threads.emplace_back(senderTask);
        threads.emplace_back(addPeerTask);
    }

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(failures.load(), 0);
}