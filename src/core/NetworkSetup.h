#pragma once

#include <string>
#include <vector>
#include <cstddef>

// Platform-agnostic network test setup, shared by BOTH GUI front-ends.
// Dynamo's network code is proven and does the actual sockets; this only decides
// server/client ROLES and the TARGET each network worker needs, so a controller
// can build its SET_TARGETS messages. Deliberately self-contained - it depends
// on neither GUI toolkit nor the core domain model (no WorkerInfo / TargetKind),
// just std:: types - so either GUI can use it and map the result onto its own
// wire format (each GUI maps it onto its own target spec tcp_info).
//
// Iometer network model: a CLIENT worker connects to a SERVER worker's network
// interface. A single-manager "loopback" test pairs workers on 127.0.0.1 so the
// whole exchange runs in one Dynamo with no second machine and no elevation.

class NetworkSetup {
public:
    // Default Iometer network data port (0 lets Dynamo pick its default).
    static constexpr int DEFAULT_PORT = 0;

    enum class Role { Server, Client };

    // What a controller must tell Dynamo about one network worker's target.
    // Each GUI maps Role onto its own TargetType (TCPServerType/TCPClientType).
    struct NetworkTarget {
        std::string name;                 // display name
        Role        role = Role::Client;
        std::string localInterface;       // NIC the SERVER binds   (server role)
        std::string remoteAddress;        // server address to reach (client role)
        int         port = DEFAULT_PORT;
    };

    // True if an ICF/worker type string denotes a network (TCP/VI) worker.
    // Accepts "Network", "NETWORK,TCP", "TCP", "NETWORK,VI", etc.; "Disk"/"" -> false.
    static bool isNetworkWorker(const std::string &workerType);

    // Target for a server worker that binds the local interface `nic`.
    static NetworkTarget serverTarget(const std::string &nic, int port = DEFAULT_PORT);

    // Target for a client worker that connects to a server at `serverAddress`.
    static NetworkTarget clientTarget(const std::string &serverAddress, int port = DEFAULT_PORT);

    // Single-manager loopback plan for `networkWorkerCount` network workers.
    // Pairs them in order into (server, client) couples on `loopback`
    // (default 127.0.0.1): even-indexed workers are servers, the following
    // odd-indexed worker is its client. A trailing unpaired worker is left as a
    // server. Returns one NetworkTarget per network worker, in order; the caller
    // maps them back to its own network workers positionally.
    static std::vector<NetworkTarget> planLoopback(
        std::size_t networkWorkerCount,
        const std::string &loopback = "127.0.0.1",
        int port = DEFAULT_PORT);
};
