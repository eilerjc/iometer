#include "ResultsAggregator.h"
#include <algorithm>
#include <set>

namespace iocore {

AggregateResult aggregateResults(const std::vector<WorkerResult> &results)
{
    AggregateResult out;
    WorkerResult &agg = out.all;
    agg.managerName = "ALL";
    agg.workerName  = "All";
    agg.isAggregate = true;

    std::set<std::string> managers;
    for (const auto &r : results) {
        if (r.isAggregate)
            continue;
        agg.iops         += r.iops;
        agg.readIops     += r.readIops;
        agg.writeIops    += r.writeIops;
        agg.mbpsDec      += r.mbpsDec;
        agg.readMbpsDec  += r.readMbpsDec;
        agg.writeMbpsDec += r.writeMbpsDec;
        agg.mbpsBin      += r.mbpsBin;
        agg.errors       += r.errors;
        agg.avgLatencyMs += r.avgLatencyMs;
        agg.maxLatencyMs  = std::max(agg.maxLatencyMs, r.maxLatencyMs);
        agg.cpuUtil      += r.cpuUtil;
        agg.cpuUser      += r.cpuUser;
        agg.cpuKernel    += r.cpuKernel;
        managers.insert(r.managerName);
        ++out.workerCount;
    }

    if (out.workerCount > 0) {
        agg.avgLatencyMs /= out.workerCount;
        agg.cpuUtil      /= out.workerCount;
        agg.cpuUser      /= out.workerCount;
        agg.cpuKernel    /= out.workerCount;
    }
    out.managerCount = static_cast<int>(managers.size());
    return out;
}

} // namespace iocore
