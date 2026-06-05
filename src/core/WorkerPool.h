#pragma once

#include <string>
#include <vector>
#include "IometerTypes.h"

// Manages multiple manager/worker configurations (platform-agnostic)
// Handles:
//   - Add/remove managers (Dynamo connections)
//   - Add/remove workers per manager
//   - Worker validation
//   - Worker lifecycle queries
// Used by both MFC and Qt implementations

class WorkerPool {
public:
    WorkerPool();
    ~WorkerPool();

    // Manager operations
    bool addManager(const ManagerInfo &info);
    bool removeManager(const std::string &managerName);
    bool hasManager(const std::string &managerName) const;
    int managerCount() const;

    // Worker operations
    bool addWorker(const std::string &managerName, const WorkerInfo &info);
    bool removeWorker(const std::string &managerName, const std::string &workerId);
    bool updateWorker(const WorkerInfo &info);
    bool hasWorker(const std::string &managerName, const std::string &workerId) const;
    int workerCount(const std::string &managerName = "") const;
    int totalWorkerCount() const;

    // Query operations
    const std::vector<WorkerInfo>& workers(const std::string &managerName) const;

    // Get list of all manager infos (for UI/display)
    std::vector<ManagerInfo> managerInfos() const;

    // Validation
    bool isValid() const;
    std::string lastError() const;

    // Clear all
    void clear();

private:
    struct ManagerEntry {
        ManagerInfo info;
        std::vector<WorkerInfo> workers;
    };

    std::vector<ManagerEntry> m_managers;
    std::string m_lastError;

    ManagerEntry* findManager(const std::string &managerName);
    const ManagerEntry* findManagerConst(const std::string &managerName) const;
};
