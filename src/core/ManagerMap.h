#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstddef>

// Platform-neutral manager-restore map: the decision logic for mapping the
// managers named in an ICF file to the running manager objects when a config is
// loaded. Ported verbatim from the MFC ManagerMap (which is the canonical spec);
// both GUIs wrap this so they share one restore/spawn/wait implementation.
//
// This holds ONLY the decisions (match an incoming manager to a saved entry,
// decide which unassigned managers are local and should be spawned, decide if we
// are still waiting). It performs NO I/O and touches no GUI: the running manager
// object is an opaque Handle the front-end owns, spawning is returned as a list
// for the front-end to act on, and the waiting-list dialog / local machine name
// are the front-end's concern.

namespace iocore {

class ManagerMap {
public:
    // Opaque handle to the front-end's running manager object (e.g. the MFC
    // Manager* or a Qt object). nullptr means "unassigned / still waiting".
    using Handle = void *;

    // Tests whether a network address is local to this machine.
    using IsLocalFn = std::function<bool(const std::string &address)>;

    // The special "this is the local machine" manager name (matches the MFC
    // LocalHostName define, "(Local)").
    static const char *const LocalHostName;

    struct Entry {
        std::string name;
        int         id = 0;
        std::string address;
        Handle      handle = nullptr;   // running manager, or null if waiting
    };

    // How the front-end should launch a local Dynamo for an unassigned entry.
    // nameArg empty  -> launch with default local name (no -n);
    // nameArg set    -> launch passing " -n <nameArg>".
    struct SpawnRequest {
        std::string nameArg;
    };

    // Prepare for reuse.
    void reset();

    // Append an entry (handle may be null = waiting on that manager).
    void store(const std::string &name, int id, const std::string &address,
               Handle handle = nullptr);

    // First entry matching name+id, OR any LocalHostName entry; null if none.
    Handle retrieve(const std::string &name, int id) const;

    // On login, assign the first matching unassigned entry the given handle.
    // Returns true if a matching entry was found and assigned.
    bool managerLoggedIn(const std::string &name, const std::string &address,
                         Handle handle);

    // If there is exactly one entry and it is unassigned, assign it; else false.
    bool setIfOneManager(Handle handle);

    // Mark the entry holding this handle as unassigned again (e.g. the manager
    // disconnected); the slot returns to the waiting state. Returns true if a
    // matching entry was cleared.
    bool managerLoggedOut(Handle handle);

    // True while any entry is still unassigned (waiting for a login).
    bool isWaitingList() const;

    // True if this handle is one of the mapped managers (i.e. needed by the
    // config being restored).
    bool isManagerNeeded(Handle handle) const;

    // Decide which unassigned, LOCAL managers to spawn. `localName` is this
    // machine's NetBIOS name; `isLocal` tests an address. Mirrors the MFC
    // SpawnLocalManagers decision exactly.
    std::vector<SpawnRequest> localManagersToSpawn(const IsLocalFn &isLocal,
                                                   const std::string &localName) const;

    std::size_t size() const { return m_entries.size(); }
    const std::vector<Entry> &entries() const { return m_entries; }

private:
    std::vector<Entry> m_entries;
};

} // namespace iocore
