#include "ManagerMap.h"
#include <cctype>

namespace iocore {

const char *const ManagerMap::LocalHostName = "(Local)";

// Case-insensitive string compare (the MFC original used CString::CompareNoCase).
static bool iequals(const std::string &a, const std::string &b)
{
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

void ManagerMap::reset()
{
    m_entries.clear();
}

void ManagerMap::store(const std::string &name, int id, const std::string &address,
                       Handle handle)
{
    Entry e;
    e.name = name;
    e.id = id;
    e.address = address;
    e.handle = handle;
    m_entries.push_back(e);
    // NOTE: the MFC original also adds an unassigned entry to its waiting-list
    // dialog here; that GUI side-effect is the front-end's responsibility.
}

ManagerMap::Handle ManagerMap::retrieve(const std::string &name, int id) const
{
    for (const auto &e : m_entries) {
        if ((iequals(name, e.name) && id == e.id)
            || e.name == LocalHostName)   // "special local host" case
            return e.handle;
    }
    return nullptr;
}

bool ManagerMap::managerLoggedIn(const std::string &name, const std::string &address,
                                 Handle handle)
{
    for (auto &e : m_entries) {
        // Verbatim port of the MFC predicate, including its operator precedence:
        //   (handle==null && name&&address match) || (address=="" && name==LOCAL)
        // (&& binds tighter than ||, so the local-host clause is independent.)
        if ((e.handle == nullptr
             && (iequals(name, e.name) && iequals(address, e.address)))
            || (e.address.empty() && e.name == LocalHostName)) {
            e.handle = handle;
            // NOTE: front-end removes (name,address) from its waiting-list dialog.
            return true;
        }
    }
    return false;
}

bool ManagerMap::setIfOneManager(Handle handle)
{
    if (m_entries.size() != 1)
        return false;
    if (m_entries[0].handle != nullptr)
        return false;
    m_entries[0].handle = handle;
    return true;
}

bool ManagerMap::managerLoggedOut(Handle handle)
{
    for (auto &e : m_entries) {
        if (e.handle == handle) {
            e.handle = nullptr;
            return true;
        }
    }
    return false;
}

bool ManagerMap::isWaitingList() const
{
    for (const auto &e : m_entries) {
        if (e.handle == nullptr)
            return true;
    }
    return false;
}

bool ManagerMap::isManagerNeeded(Handle handle) const
{
    for (const auto &e : m_entries) {
        if (e.handle == handle)
            return true;
    }
    return false;
}

std::vector<ManagerMap::SpawnRequest> ManagerMap::localManagersToSpawn(
    const IsLocalFn &isLocal, const std::string &localName) const
{
    std::vector<SpawnRequest> requests;
    for (const auto &e : m_entries) {
        if (e.handle != nullptr)
            continue;   // already assigned
        if ((e.address.empty() && e.name == LocalHostName)
            || iequals(e.name, localName)) {
            // Spawn a Dynamo with local defaults (no -n).
            requests.push_back(SpawnRequest{ std::string() });
        } else if (isLocal && isLocal(e.address)) {
            // Spawn a local Dynamo with the appropriate -n name.
            requests.push_back(SpawnRequest{ e.name });
        }
    }
    return requests;
}

} // namespace iocore
