// QtWorkerPoolAdapter.cpp
#include "QtWorkerPoolAdapter.h"

QtWorkerPoolAdapter::QtWorkerPoolAdapter(QObject *parent)
    : QObject(parent), m_workerPool(std::make_unique<WorkerPool>())
{
}

QtWorkerPoolAdapter::~QtWorkerPoolAdapter()
{
}

void QtWorkerPoolAdapter::addManager(const ManagerInfo &mgr)
{
    m_workerPool->addManager(mgr);
    emit managerAdded(QString::fromStdString(mgr.name));
}

void QtWorkerPoolAdapter::removeManager(const std::string &managerName)
{
    m_workerPool->removeManager(managerName);
    emit managerRemoved(QString::fromStdString(managerName));
}

bool QtWorkerPoolAdapter::hasManager(const std::string &managerName) const
{
    return m_workerPool->hasManager(managerName);
}

void QtWorkerPoolAdapter::addWorker(const std::string &managerName, const WorkerInfo &worker)
{
    m_workerPool->addWorker(managerName, worker);
    emit workerAdded(QString::fromStdString(managerName), QString::fromStdString(worker.id));
}

void QtWorkerPoolAdapter::removeWorker(const std::string &managerName, const std::string &workerId)
{
    m_workerPool->removeWorker(managerName, workerId);
    emit workerRemoved(QString::fromStdString(managerName), QString::fromStdString(workerId));
}

bool QtWorkerPoolAdapter::hasWorker(const std::string &managerName, const std::string &workerId) const
{
    return m_workerPool->hasWorker(managerName, workerId);
}

void QtWorkerPoolAdapter::updateWorker(const std::string &managerName, const WorkerInfo &worker)
{
    m_workerPool->updateWorker(managerName, worker);
}

std::vector<WorkerInfo> QtWorkerPoolAdapter::workers(const std::string &managerName) const
{
    return m_workerPool->workers(managerName);
}

std::vector<ManagerInfo> QtWorkerPoolAdapter::managerInfos() const
{
    return m_workerPool->managerInfos();
}

int QtWorkerPoolAdapter::workerCount(const std::string &managerName) const
{
    return m_workerPool->workerCount(managerName);
}

int QtWorkerPoolAdapter::totalWorkerCount() const
{
    return m_workerPool->totalWorkerCount();
}

bool QtWorkerPoolAdapter::isValid() const
{
    return m_workerPool->isValid();
}

void QtWorkerPoolAdapter::clear()
{
    m_workerPool->clear();
}
