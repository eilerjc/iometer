#pragma once

#include <QString>
#include <QObject>
#include <QAbstractSocket>
#include <cstdint>

// Forward declarations
class QTcpSocket;
struct DyMsg;
struct DyDataMessage;

// Encapsulates TCP connection and protocol handshake with a single Dynamo instance
// Handles:
//   - TCP socket on port 1066
//   - DY_LOGIN message exchange
//   - Manager_Info data reception
//   - Message framing and dispatch
// Used by both Qt (DySession wrapper) and MFC (IOTest equivalent)

class DynamoConnection : public QObject {
    Q_OBJECT

public:
    enum class State {
        Disconnected,
        Connecting,
        WaitingLoginData,
        Connected,
        Error
    };

    explicit DynamoConnection(QTcpSocket *socket, QObject *parent = nullptr);
    ~DynamoConnection() override;

    // Initiate login handshake with Dynamo
    // Sends: DY_LOGIN message with version + manager info
    // Waits for: Manager_Info data message
    bool startLogin(const QString &managerName, int timeoutMs = 30000);

    // Send raw 8-byte message (e.g., DY_LOGIN, START, STOP, RECORD_ON/OFF)
    bool sendMessage(const DyMsg &msg);

    // Receive data message (Manager_Info, Target data, Worker results, etc.)
    // Returns nullptr if timeout or error
    DyDataMessage* receiveDataMessage(int timeoutMs = 5000);

    // Check if connection is active
    State state() const { return m_state; }
    bool isConnected() const { return m_state == State::Connected; }

    // Get connection details
    QString remoteAddress() const;
    int remotePort() const;

    // Close connection gracefully
    void disconnect();

signals:
    void connected(const QString &managerName);
    void disconnected();
    void dataReceived(DyDataMessage *data);
    void error(const QString &message);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketReadyRead();

private:
    bool waitForBytes(int count, int timeoutMs);
    bool receiveExactly(int count);

    QTcpSocket *m_socket;
    State m_state;
    QString m_managerName;
    QByteArray m_buffer;  // Input buffer for partial messages

    static constexpr int DYNAMO_PORT = 1066;
    static constexpr int MESSAGE_SIZE = 8;
};
