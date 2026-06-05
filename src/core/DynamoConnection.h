#pragma once

#include <string>
#include <cstdint>
#include <functional>

// Forward declarations
struct DyMsg;
struct DyDataMessage;

// Encapsulates TCP connection and protocol handshake (platform-agnostic)
// Handles:
//   - TCP socket on port 1066
//   - DY_LOGIN message exchange
//   - Manager_Info data reception
//   - Message framing
// Used by both Qt (DySession wrapper) and MFC implementations

class DynamoConnection {
public:
    enum class State {
        Disconnected,
        Connecting,
        WaitingLoginData,
        Connected,
        Error
    };

    DynamoConnection();
    ~DynamoConnection();

    // Initiate login handshake with Dynamo
    bool startLogin(const std::string &managerName, int timeoutMs = 30000);

    // Send raw 8-byte message (e.g., DY_LOGIN, START, STOP, RECORD_ON/OFF)
    bool sendMessage(const DyMsg &msg);

    // Receive data message (Manager_Info, Target data, Worker results, etc.)
    // Returns nullptr if timeout or error
    DyDataMessage* receiveDataMessage(int timeoutMs = 5000);

    // Check if connection is active
    State state() const { return m_state; }
    bool isConnected() const { return m_state == State::Connected; }

    // Get connection details
    std::string remoteAddress() const;
    int remotePort() const;

    // Close connection gracefully
    void disconnect();

    // Optional callbacks (for Qt/MFC integration)
    std::function<void(const std::string&)> onConnected;
    std::function<void()> onDisconnected;
    std::function<void(DyDataMessage*)> onDataReceived;
    std::function<void(const std::string&)> onError;

private:
    bool waitForBytes(int count, int timeoutMs);
    bool receiveExactly(int count);

    void *m_socket;  // Opaque pointer to platform-specific socket (QTcpSocket* on Qt)
    State m_state;
    std::string m_managerName;
    std::string m_buffer;  // Input buffer for partial messages

    static constexpr int DYNAMO_PORT = 1066;
    static constexpr int MESSAGE_SIZE = 8;
};
