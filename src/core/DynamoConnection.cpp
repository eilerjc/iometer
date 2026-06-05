#include "DynamoConnection.h"
#include <cstring>

DynamoConnection::DynamoConnection()
    : m_socket(nullptr), m_state(State::Disconnected)
{
}

DynamoConnection::~DynamoConnection()
{
    disconnect();
}

bool DynamoConnection::startLogin(const std::string &managerName, int timeoutMs)
{
    // Platform-specific implementation would go here
    // For now, this is a stub for MFC integration
    m_managerName = managerName;
    m_state = State::Connecting;
    return true;
}

bool DynamoConnection::sendMessage(const DyMsg &msg)
{
    // Platform-specific socket send
    return m_state == State::Connected;
}

DyDataMessage* DynamoConnection::receiveDataMessage(int timeoutMs)
{
    // Platform-specific socket receive
    return nullptr;
}

std::string DynamoConnection::remoteAddress() const
{
    // Platform-specific socket query
    return "";
}

int DynamoConnection::remotePort() const
{
    // Platform-specific socket query
    return DYNAMO_PORT;
}

void DynamoConnection::disconnect()
{
    m_state = State::Disconnected;
    m_socket = nullptr;
    if (onDisconnected) {
        onDisconnected();
    }
}
