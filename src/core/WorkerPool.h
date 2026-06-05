#pragma once

#include <QString>
#include <QList>
#include <QObject>

// Forward declarations
struct WorkerInfo;
struct ManagerInfo;

// Manages multiple manager/worker configurations across Dynamo instances
// Handles:
//   - Add/remove managers (Dynamo connections)
//   - Add/remove workers per manager
//   - Worker validation
//   - Worker lifecycle queries
// Used by both MFC (ManagerList) and Qt (DynamoEngine) implementations

class WorkerPool : public QObject {
    Q_OBJECT

public:
    explicit WorkerPool(QObject *parent = nullptr);
    ~WorkerPool() override;

    // Manager operations
    bool addManager(const ManagerInfo &info);
    bool removeManager(const QString &managerName);
    bool hasManager(const QString &managerName) const;
    int managerCount() const;

    // Worker operations
    bool addWorker(const QString &managerName, const WorkerInfo &info);
    bool removeWorker(const QString &managerName, const QString &workerId);
    bool updateWorker(const WorkerInfo &info);
    bool hasWorker(const QString &managerName, const QString &workerId) const;
    int workerCount(const QString &managerName = QString()) const;
    int totalWorkerCount() const;

    // Query operations
    const QList<ManagerInfo>& managers() const { return m_managers; }
    const QList<WorkerInfo>& workers(const QString &managerName) const;

    // Validation
    bool isValid() const;
    QString lastError() const;

    // Clear all
    void clear();

signals:
    void managerAdded(const QString &managerName);
    void managerRemoved(const QString &managerName);
    void workerAdded(const QString &managerName, const QString &workerId);
    void workerRemoved(const QString &managerName, const QString &workerId);
    void error(const QString &message);

private:
    struct ManagerEntry {
        ManagerInfo info;
        QList<WorkerInfo> workers;
    };

    QList<ManagerEntry> m_managers;
    QString m_lastError;

    ManagerEntry* findManager(const QString &managerName);
    const ManagerEntry* findManagerConst(const QString &managerName) const;
};
