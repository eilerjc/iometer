#include "NetworkSetup.h"
#include <algorithm>
#include <cctype>

bool NetworkSetup::isNetworkWorker(const std::string &workerType)
{
    // Carries the ICF 'Worker type: "Disk", "Network", or the raw ICF strings
    // "NETWORK,TCP" / "NETWORK,VI" / "TCP". Anything that isn't plainly a disk
    // worker and mentions network/TCP/VI is treated as a network worker.
    std::string t = workerType;
    std::transform(t.begin(), t.end(), t.begin(),
                   [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    if (t == "DISK" || t.empty())
        return false;
    return t.find("NETWORK") != std::string::npos
        || t.find("TCP")     != std::string::npos
        || t.find("VI")      != std::string::npos;
}

NetworkSetup::NetworkTarget NetworkSetup::serverTarget(const std::string &nic, int port)
{
    NetworkTarget t;
    t.name           = "TCP server " + nic;
    t.role           = Role::Server;
    t.localInterface = nic;
    t.remoteAddress.clear();
    t.port           = port;
    return t;
}

NetworkSetup::NetworkTarget NetworkSetup::clientTarget(const std::string &serverAddress, int port)
{
    NetworkTarget t;
    t.name          = "TCP client -> " + serverAddress;
    t.role          = Role::Client;
    t.localInterface.clear();
    t.remoteAddress = serverAddress;
    t.port          = port;
    return t;
}

std::vector<NetworkSetup::NetworkTarget> NetworkSetup::planLoopback(
    std::size_t networkWorkerCount,
    const std::string &loopback,
    int port)
{
    std::vector<NetworkTarget> plan;
    plan.reserve(networkWorkerCount);
    for (std::size_t i = 0; i < networkWorkerCount; ++i) {
        if (i % 2 == 0)
            plan.push_back(serverTarget(loopback, port));   // server binds loopback
        else
            plan.push_back(clientTarget(loopback, port));   // client connects to it
    }
    return plan;
}
