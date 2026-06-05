// QtWorkerPoolAdapter.h - Qt wrapper around pure C++ WorkerPool
#pragma once

#include <QObject>
#include "../core/WorkerPool.h"

// Qt adapter: provides Qt-friendly access to WorkerPool
class QtWorkerPoolAdapter : public QObject {
    Q_OBJECT

public:
    explicit QtWorkerPoolAdapter(QObject *parent = nullptr);
    ~QtWorkerPoolAdapter();

    // Manager operations
    void addManager(const ManagerInfo &mgr);
    void removeManager(const std::string &managerName);
    bool hasManager(const std::string &managerName) const;

    // Worker operations
    void addWorker(const std::string &managerName, const WorkerInfo &worker);
    void removeWorker(const std::string &managerName, const std::string &workerId);
    bool hasWorker(const std::string &managerName, const std::string &workerId) const;
    void updateWorker(const std::string &managerName, const WorkerInfo &worker);

    // Queries
    std::vector<WorkerInfo> workers(const std::string &managerName) const;
    std::vector<ManagerInfo> managerInfos() const;
    int workerCount(const std::string &managerName) const;
    int totalWorkerCount() const;
    bool isValid() const;

    // Clear all
    void clear();

    // Access underlying WorkerPool
    WorkerPool *getWorkerPool() { return m_workerPool.get(); }

signals:
    // Emitted when manager is added
    void managerAdded(const QString &managerName);

    // Emitted when manager is removed
    void managerRemoved(const QString &managerName);

    // Emitted when worker is added
    void workerAdded(const QString &managerName, const QString &workerId);

    // Emitted when worker is removed
    void workerRemoved(const QString &managerName, const QString &workerId);

private:
    std::unique_ptr<WorkerPool> m_workerPool;
};
