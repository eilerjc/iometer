#include "DynamoConnection.h"
#include "../DyProto.h"

#include <QTcpSocket>
#include <QHostAddress>
#include <QElapsedTimer>

DynamoConnection::DynamoConnection(QTcpSocket *socket, QObject *parent)
    : QObject(parent), m_socket(socket), m_state(State::Disconnected)
{
    Q_ASSERT(socket);

    connect(m_socket, &QTcpSocket::connected, this, &DynamoConnection::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &DynamoConnection::onSocketDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &DynamoConnection::onSocketError);
    connect(m_socket, &QTcpSocket::readyRead, this, &DynamoConnection::onSocketReadyRead);
}

DynamoConnection::~DynamoConnection()
{
    if (m_socket && m_socket->state() == QTcpSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool DynamoConnection::startLogin(const QString &managerName, int timeoutMs)
{
    m_managerName = managerName;
    m_state = State::WaitingLoginData;

    // Send DY_LOGIN message (8 bytes)
    DyMsg msg;
    msg.purpose = DY_LOGIN;
    msg.data = DY_VERSION;

    if (!sendMessage(msg)) {
        m_state = State::Error;
        emit error("Failed to send DY_LOGIN");
        return false;
    }

    // Wait for Manager_Info data message (first 8-byte header, then payload)
    DyDataMessage *data = receiveDataMessage(timeoutMs);
    if (!data) {
        m_state = State::Error;
        emit error("Timeout waiting for Manager_Info");
        return false;
    }

    // TODO: Parse Manager_Info to extract manager details
    // For now, just verify we got something
    delete data;

    m_state = State::Connected;
    emit connected(managerName);
    return true;
}

bool DynamoConnection::sendMessage(const DyMsg &msg)
{
    if (!m_socket || m_socket->state() != QTcpSocket::ConnectedState) {
        return false;
    }

    // Write 8 bytes: purpose (4) + data (4)
    char buf[8];
    uint32_t *p = reinterpret_cast<uint32_t *>(buf);
    p[0] = msg.purpose;
    p[1] = msg.data;

    qint64 written = m_socket->write(buf, 8);
    return written == 8;
}

DyDataMessage* DynamoConnection::receiveDataMessage(int timeoutMs)
{
    // First, receive 8-byte message header
    if (!waitForBytes(8, timeoutMs)) {
        return nullptr;
    }
    if (!receiveExactly(8)) {
        return nullptr;
    }

    // Parse header
    const DyMsg *hdr = reinterpret_cast<const DyMsg *>(m_buffer.constData());
    int32_t size = hdr->data;  // Data message size is in the 'data' field

    if (size <= 0 || size > 1000000) {
        return nullptr;  // Sanity check
    }

    // Now wait for the full data payload
    if (!waitForBytes(size, timeoutMs)) {
        return nullptr;
    }
    if (!receiveExactly(size)) {
        return nullptr;
    }

    // Allocate and copy result
    DyDataMessage *result = new DyDataMessage;
    memcpy(result, m_buffer.constData(), qMin((int)sizeof(DyDataMessage), size));
    m_buffer.clear();

    return result;
}

QString DynamoConnection::remoteAddress() const
{
    return m_socket ? m_socket->peerAddress().toString() : QString();
}

int DynamoConnection::remotePort() const
{
    return m_socket ? m_socket->peerPort() : 0;
}

void DynamoConnection::disconnect()
{
    m_state = State::Disconnected;
    if (m_socket) {
        m_socket->disconnectFromHost();
    }
}

void DynamoConnection::onSocketConnected()
{
    // Connection established at TCP level, but not yet at Dynamo protocol level
    // startLogin() will handle the protocol handshake
}

void DynamoConnection::onSocketDisconnected()
{
    m_state = State::Disconnected;
    m_buffer.clear();
    emit disconnected();
}

void DynamoConnection::onSocketError(QAbstractSocket::SocketError error)
{
    m_state = State::Error;
    emit this->error(QString("Socket error: %1").arg((int)error));
}

void DynamoConnection::onSocketReadyRead()
{
    // Data is available; call receiveDataMessage() to get it
}

bool DynamoConnection::waitForBytes(int count, int timeoutMs)
{
    if (m_buffer.size() >= count) {
        return true;
    }

    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs) {
        if (!m_socket->isReadable()) {
            m_socket->waitForReadyRead(100);
            continue;
        }

        QByteArray chunk = m_socket->read(count - m_buffer.size());
        m_buffer.append(chunk);

        if (m_buffer.size() >= count) {
            return true;
        }
    }

    return false;
}

bool DynamoConnection::receiveExactly(int count)
{
    if (m_buffer.size() < count) {
        return false;
    }
    // Data already in buffer from waitForBytes()
    return true;
}
