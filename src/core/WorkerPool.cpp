#include "WorkerPool.h"
#include "../qt/IometerTypes.h"

WorkerPool::WorkerPool(QObject *parent)
    : QObject(parent)
{
}

WorkerPool::~WorkerPool()
{
    clear();
}

bool WorkerPool::addManager(const ManagerInfo &info)
{
    if (hasManager(info.name)) {
        m_lastError = QString("Manager '%1' already exists").arg(info.name);
        emit error(m_lastError);
        return false;
    }

    ManagerEntry entry;
    entry.info = info;
    m_managers.append(entry);

    emit managerAdded(info.name);
    return true;
}

bool WorkerPool::removeManager(const QString &managerName)
{
    for (int i = 0; i < m_managers.size(); ++i) {
        if (m_managers[i].info.name == managerName) {
            m_managers.removeAt(i);
            emit managerRemoved(managerName);
            return true;
        }
    }

    m_lastError = QString("Manager '%1' not found").arg(managerName);
    emit error(m_lastError);
    return false;
}

bool WorkerPool::hasManager(const QString &managerName) const
{
    return findManagerConst(managerName) != nullptr;
}

int WorkerPool::managerCount() const
{
    return m_managers.size();
}

bool WorkerPool::addWorker(const QString &managerName, const WorkerInfo &info)
{
    ManagerEntry *mgr = findManager(managerName);
    if (!mgr) {
        m_lastError = QString("Manager '%1' not found").arg(managerName);
        emit error(m_lastError);
        return false;
    }

    // Check for duplicate worker ID
    for (const auto &w : mgr->workers) {
        if (w.id == info.id) {
            m_lastError = QString("Worker '%1' already exists in manager '%2'")
                .arg(info.id, managerName);
            emit error(m_lastError);
            return false;
        }
    }

    mgr->workers.append(info);
    emit workerAdded(managerName, info.id);
    return true;
}

bool WorkerPool::removeWorker(const QString &managerName, const QString &workerId)
{
    ManagerEntry *mgr = findManager(managerName);
    if (!mgr) {
        m_lastError = QString("Manager '%1' not found").arg(managerName);
        return false;
    }

    for (int i = 0; i < mgr->workers.size(); ++i) {
        if (mgr->workers[i].id == workerId) {
            mgr->workers.removeAt(i);
            emit workerRemoved(managerName, workerId);
            return true;
        }
    }

    m_lastError = QString("Worker '%1' not found in manager '%2'")
        .arg(workerId, managerName);
    emit error(m_lastError);
    return false;
}

bool WorkerPool::updateWorker(const WorkerInfo &info)
{
    ManagerEntry *mgr = findManager(info.managerName);
    if (!mgr) {
        m_lastError = QString("Manager '%1' not found").arg(info.managerName);
        return false;
    }

    for (auto &w : mgr->workers) {
        if (w.id == info.id) {
            w = info;
            return true;
        }
    }

    m_lastError = QString("Worker '%1' not found").arg(info.id);
    return false;
}

bool WorkerPool::hasWorker(const QString &managerName, const QString &workerId) const
{
    const ManagerEntry *mgr = findManagerConst(managerName);
    if (!mgr) return false;

    for (const auto &w : mgr->workers) {
        if (w.id == workerId) return true;
    }
    return false;
}

int WorkerPool::workerCount(const QString &managerName) const
{
    if (managerName.isEmpty()) {
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

const QList<WorkerInfo>& WorkerPool::workers(const QString &managerName) const
{
    static const QList<WorkerInfo> emptyList;
    const ManagerEntry *mgr = findManagerConst(managerName);
    return mgr ? mgr->workers : emptyList;
}

QList<ManagerInfo> WorkerPool::managerInfos() const
{
    QList<ManagerInfo> infos;
    for (const auto &mgr : m_managers) {
        infos.append(mgr.info);
    }
    return infos;
}

bool WorkerPool::isValid() const
{
    if (m_managers.isEmpty()) {
        return false;  // Need at least one manager
    }

    for (const auto &mgr : m_managers) {
        if (mgr.workers.isEmpty()) {
            return false;  // Each manager needs at least one worker
        }
    }

    return true;
}

QString WorkerPool::lastError() const
{
    return m_lastError;
}

void WorkerPool::clear()
{
    m_managers.clear();
    m_lastError.clear();
}

WorkerPool::ManagerEntry* WorkerPool::findManager(const QString &managerName)
{
    for (auto &mgr : m_managers) {
        if (mgr.info.name == managerName) {
            return &mgr;
        }
    }
    return nullptr;
}

const WorkerPool::ManagerEntry* WorkerPool::findManagerConst(const QString &managerName) const
{
    for (const auto &mgr : m_managers) {
        if (mgr.info.name == managerName) {
            return &mgr;
        }
    }
    return nullptr;
}
