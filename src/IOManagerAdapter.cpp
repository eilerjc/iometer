// IOManagerAdapter.cpp - MFC ↔ core library bridge
#include "IOManagerAdapter.h"
#include <cstring>

IOManagerAdapter::IOManagerAdapter()
    : m_workerPool(std::make_unique<WorkerPool>()),
      m_testRunner(std::make_unique<TestRunner>())
{
}

IOManagerAdapter::~IOManagerAdapter()
{
}

bool IOManagerAdapter::loadConfig(const char *filepath)
{
    if (!filepath) return false;

    std::string path = mfcStringToStd(filepath);
    std::vector<AccessSpec> specs;
    std::vector<IcfFile::BatchWorker> workers;

    if (!IcfFile::load(path, m_config, specs, workers)) {
        return false;
    }

    // Populate WorkerPool from loaded configuration
    ManagerInfo mgr;
    mgr.name = "Primary Manager";
    m_workerPool->addManager(mgr);

    // Add batch workers to the pool
    for (const auto &bw : workers) {
        WorkerInfo w;
        w.name = bw.name;
        w.id = bw.name;
        w.managerName = mgr.name;
        w.targets = bw.targets;
        w.assignedSpecs = bw.assignedSpecs;
        m_workerPool->addWorker(mgr.name, w);
    }

    return true;
}

bool IOManagerAdapter::saveConfig(const char *filepath)
{
    if (!filepath) return false;

    std::string path = mfcStringToStd(filepath);
    std::vector<IcfFile::BatchWorker> batchWorkers;

    // Convert WorkerPool state back to batch format
    auto managers = m_workerPool->managerInfos();
    for (const auto &mgr : managers) {
        auto workers = m_workerPool->workers(mgr.name);
        for (const auto &w : workers) {
            IcfFile::BatchWorker bw;
            bw.name = w.name;
            bw.assignedSpecs = w.assignedSpecs;
            bw.targets = w.targets;
            batchWorkers.push_back(bw);
        }
    }

    std::vector<AccessSpec> specs;  // Would load from m_config
    return IcfFile::save(path, m_config, specs, batchWorkers);
}

bool IOManagerAdapter::addWorkerFromSpec(const char *managerName, const Target_Spec *spec)
{
    if (!managerName || !spec) return false;

    std::string mgrName = mfcStringToStd(managerName);
    WorkerInfo w = specToWorkerInfo(spec, managerName);

    return m_workerPool->addWorker(mgrName, w);
}

bool IOManagerAdapter::startTestFromSpec(const Test_Spec *spec)
{
    if (!spec) return false;

    // Get current workers and specs
    auto managers = m_workerPool->managerInfos();
    if (managers.empty()) return false;

    std::vector<WorkerInfo> workers;
    for (const auto &mgr : managers) {
        auto mgrWorkers = m_workerPool->workers(mgr.name);
        workers.insert(workers.end(), mgrWorkers.begin(), mgrWorkers.end());
    }

    std::vector<AccessSpec> specs;
    // Convert Test_Spec to AccessSpec (would need proper mapping)

    return m_testRunner->startTest(m_config, workers, specs);
}

bool IOManagerAdapter::stopTest()
{
    return m_testRunner->stopTest();
}

bool IOManagerAdapter::writeResults(const char *filepath)
{
    if (!filepath) return false;

    std::string path = mfcStringToStd(filepath);
    return ResultsWriter::writeBatchResultsCsv(path, m_results, m_config);
}

std::string IOManagerAdapter::mfcStringToStd(const char *mfcStr)
{
    if (!mfcStr) return "";
    return std::string(mfcStr);
}

WorkerInfo IOManagerAdapter::specToWorkerInfo(const Target_Spec *spec, const char *managerName)
{
    WorkerInfo w;
    w.managerName = mfcStringToStd(managerName);
    // Would populate from spec fields (spec->name, spec->blockDevName, etc.)
    return w;
}

AccessSpec IOManagerAdapter::testSpecToAccessSpec(const Test_Spec *spec)
{
    AccessSpec as;
    // Would convert from Test_Spec to AccessSpec
    return as;
}
