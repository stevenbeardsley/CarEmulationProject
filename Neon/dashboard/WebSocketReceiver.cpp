#include "WebSocketReceiver.h"
#include "LogFile.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>

namespace beast = boost::beast;
using namespace std;

WebSocketReceiver::WebSocketReceiver(tcp::acceptor& acceptor, std::atomic<bool>& running)
    : acceptor_(acceptor), running_(running)
{
}

WebSocketReceiver::~WebSocketReceiver() {
    stop();
}

void WebSocketReceiver::start() {
    listenerThread_ = std::thread([this]() { listen(); });
}

void WebSocketReceiver::stop() {
    if (listenerThread_.joinable())
        listenerThread_.join();
}

void WebSocketReceiver::listen() {
    LogFile::Info("WebSocketReceiver listening for /carCommands connections...");

    while (running_) {
        try {
            tcp::socket socket(acceptor_.get_executor());
            acceptor_.accept(socket);

            // Spawn a thread for each session
            std::thread(&WebSocketReceiver::handleSession, this, std::move(socket)).detach();
        }
        catch (const std::exception& e) {
            LogFile::Error("Receiver accept error: " + std::string(e.what()));
        }
    }
}

void WebSocketReceiver::handleSession(tcp::socket socket) {
    try {
        websocket::stream<tcp::socket> ws(std::move(socket));
        ws.accept();

        LogFile::Info("Receiver client connected to /carCommands");

        while (running_) {
            beast::flat_buffer buffer;
            ws.read(buffer);
            std::string msg = beast::buffers_to_string(buffer.data());
            processMessage(msg);
        }
    }
    catch (const std::exception& e) {
        LogFile::Error("Receiver session error: " + std::string(e.what()));
    }
}

void WebSocketReceiver::processMessage(const std::string& msg) {
    LogFile::Info("Received command: " + msg);
    if (msg == "STOP") {
        LogFile::Info("Command: STOP");
    }
    else if (msg == "START") {
        LogFile::Info("Command: START");
    }
    else {
        LogFile::Info("Unknown command: " + msg);
    }
}
