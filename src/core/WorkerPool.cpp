#include "WorkerPool.h"

WorkerPool::WorkerPool()
{
}

WorkerPool::~WorkerPool()
{
    clear();
}

bool WorkerPool::addManager(const ManagerInfo &info)
{
    if (hasManager(info.name)) {
        m_lastError = "Manager '" + info.name + "' already exists";
        return false;
    }

    ManagerEntry entry;
    entry.info = info;
    m_managers.push_back(entry);
    return true;
}

bool WorkerPool::removeManager(const std::string &managerName)
{
    for (size_t i = 0; i < m_managers.size(); ++i) {
        if (m_managers[i].info.name == managerName) {
            m_managers.erase(m_managers.begin() + i);
            return true;
        }
    }

    m_lastError = "Manager '" + managerName + "' not found";
    return false;
}

bool WorkerPool::hasManager(const std::string &managerName) const
{
    return findManagerConst(managerName) != nullptr;
}

int WorkerPool::managerCount() const
{
    return m_managers.size();
}

bool WorkerPool::addWorker(const std::string &managerName, const WorkerInfo &info)
{
    ManagerEntry *mgr = findManager(managerName);
    if (!mgr) {
        m_lastError = "Manager '" + managerName + "' not found";
        return false;
    }

    // Check for duplicate worker ID
    for (const auto &w : mgr->workers) {
        if (w.id == info.id) {
            m_lastError = "Worker '" + info.id + "' already exists in manager '" + managerName + "'";
            return false;
        }
    }

    mgr->workers.push_back(info);
    return true;
}

bool WorkerPool::removeWorker(const std::string &managerName, const std::string &workerId)
{
    ManagerEntry *mgr = findManager(managerName);
    if (!mgr) {
        m_lastError = "Manager '" + managerName + "' not found";
        return false;
    }

    for (size_t i = 0; i < mgr->workers.size(); ++i) {
        if (mgr->workers[i].id == workerId) {
            mgr->workers.erase(mgr->workers.begin() + i);
            return true;
        }
    }

    m_lastError = "Worker '" + workerId + "' not found in manager '" + managerName + "'";
    return false;
}

bool WorkerPool::updateWorker(const WorkerInfo &info)
{
    ManagerEntry *mgr = findManager(info.managerName);
    if (!mgr) {
        m_lastError = "Manager '" + info.managerName + "' not found";
        return false;
    }

    for (auto &w : mgr->workers) {
        if (w.id == info.id) {
            w = info;
            return true;
        }
    }

    m_lastError = "Worker '" + info.id + "' not found";
    return false;
}

bool WorkerPool::hasWorker(const std::string &managerName, const std::string &workerId) const
{
    const ManagerEntry *mgr = findManagerConst(managerName);
    if (!mgr) return false;

    for (const auto &w : mgr->workers) {
        if (w.id == workerId) return true;
    }
    return false;
}

int WorkerPool::workerCount(const std::string &managerName) const
{
    if (managerName.empty()) {
        return totalWorkerCount();
    }

    const ManagerEntry *mgr = findManagerConst(managerName);
    return mgr ? mgr->workers.size() : 0;
}

int WorkerPool::totalWorkerCount() const
{
    int count = 0;
    for (const auto &mgr : m_managers) {
        count += mgr.workers.size();
    }
    return count;
}

const std::vector<WorkerInfo>& WorkerPool::workers(const std::string &managerName) const
{
    static const std::vector<WorkerInfo> emptyList;
    const ManagerEntry *mgr = findManagerConst(managerName);
    return mgr ? mgr->workers : emptyList;
}

std::vector<ManagerInfo> WorkerPool::managerInfos() const
{
    std::vector<ManagerInfo> infos;
    for (const auto &mgr : m_managers) {
        infos.push_back(mgr.info);
    }
    return infos;
}

bool WorkerPool::isValid() const
{
    if (m_managers.empty()) {
        return false;  // Need at least one manager
    }

    for (const auto &mgr : m_managers) {
        if (mgr.workers.empty()) {
            return false;  // Each manager needs at least one worker
        }
    }

    return true;
}

std::string WorkerPool::lastError() const
{
    return m_lastError;
}

void WorkerPool::clear()
{
    m_managers.clear();
    m_lastError.clear();
}

WorkerPool::ManagerEntry* WorkerPool::findManager(const std::string &managerName)
{
    for (auto &mgr : m_managers) {
        if (mgr.info.name == managerName) {
            return &mgr;
        }
    }
    return nullptr;
}

const WorkerPool::ManagerEntry* WorkerPool::findManagerConst(const std::string &managerName) const
{
    for (const auto &mgr : m_managers) {
        if (mgr.info.name == managerName) {
            return &mgr;
        }
    }
    return nullptr;
}
